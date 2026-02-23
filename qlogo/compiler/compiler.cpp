//===-- qlogo/compiler.cpp - Compiler implementation -------*- C++ -*-===//
//
// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the supporting methods of the
/// Compiler class, along with the supporting C functions.
///
//===----------------------------------------------------------------------===//

#include "compiler.h"
#include "astnode.h"
#include "compiler_internal.h"
#include "llvm/IR/CFG.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/Support/Error.h"
#include "flowcontrol.h"
#include "datum_types.h"
#include "workspace/exports.h"
#include "workspace/kernel.h"
#include "op_strings.h"
#include "sharedconstants.h"
#include "treeifyer.h"
#include "workspace/callframe.h"
#include "workspace/procedures.h"
#include <string>
#include <iostream>

QHash<Datum *, std::shared_ptr<CompiledText>> Compiler::compiledTextTable_;

const char *dbgName(const char *enclosing, const char *name)
{
    static bool shouldMangle = Config::get().showIR_ || Config::get().showCFG_ || Config::get().showModuleIR_;
    if (shouldMangle)
    {
        static std::string storage;
        storage = std::string(enclosing) + "_" + name;
        return storage.c_str();
    }
    return name;
}

using namespace llvm;
using namespace llvm::orc;

namespace
{
std::unique_ptr<LLJIT> createLLJIT()
{
    auto jitOrErr = llvm::orc::LLJITBuilder()
                        .setNotifyCreatedCallback([](llvm::orc::LLJIT &J) -> llvm::Error {
                            if (!J.getTargetTriple().isOSBinFormatCOFF())
                                return llvm::Error::success();
                            auto *rtdyld =
                                llvm::dyn_cast<llvm::orc::RTDyldObjectLinkingLayer>(&J.getObjLinkingLayer());
                            if (rtdyld)
                            {
                                rtdyld->setOverrideObjectFlagsWithResponsibilityFlags(true);
                                rtdyld->setAutoClaimResponsibilityForObjectSymbols(true);
                            }
                            return llvm::Error::success();
                        })
                        .create();
    return cantFail(std::move(jitOrErr));
}

/// Creates a resource tracker, adds the module to the JIT, and looks up the symbol.
/// Returns (symbol address, resource tracker for later removal).
std::pair<uint64_t, ResourceTrackerSP> addModuleAndLookup(LLJIT &jit, ThreadSafeModule tsm, StringRef name)
{
    auto rt = jit.getMainJITDylib().createResourceTracker();
    cantFail(jit.addIRModule(rt, std::move(tsm)));
    uint64_t addr = cantFail(jit.lookup(name)).getValue();
    return {addr, std::move(rt)};
}

void addDefaultPasses(FunctionPassManager &fpm)
{
    fpm.addPass(InstCombinePass());
    fpm.addPass(ReassociatePass());
    fpm.addPass(GVNPass());
    fpm.addPass(SimplifyCFGPass());
}

void setupPassManager(PassInstrumentationCallbacks &pic, ModuleAnalysisManager &mam,
                      FunctionAnalysisManager &fam, LoopAnalysisManager &lam,
                      CGSCCAnalysisManager &cgam, StandardInstrumentations &si)
{
    si.registerCallbacks(pic, &mam);
    PassBuilder pb;
    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);
}

/// Run the coroutine lowering pipeline so that llvm.coro.* intrinsics are
/// lowered before the module is sent to the backend.
void runCoroutinePasses(Module &M, ModuleAnalysisManager &mam,
                        FunctionAnalysisManager &fam, LoopAnalysisManager &lam,
                        CGSCCAnalysisManager &cgam)
{
    PassBuilder pb;
    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);
    ModulePassManager coroMPM;
    cantFail(pb.parsePassPipeline(coroMPM, "coro-early,cgscc(coro-split),function(coro-elide),coro-cleanup"));
    coroMPM.run(M, mam);
}
} // namespace

Scaffold::Scaffold(const llvm::DataLayout &dataLayout)
    : theContext_(std::make_unique<LLVMContext>()), theModule_(std::make_unique<Module>("QLogoJIT", *theContext_)),
      builder_(IRBuilder<>(*theContext_)), theFPM_(FunctionPassManager()), theLAM_(LoopAnalysisManager()),
      theFAM_(FunctionAnalysisManager()), theCGAM_(CGSCCAnalysisManager()), theMAM_(ModuleAnalysisManager()),
      thePIC_(PassInstrumentationCallbacks()), theSI_(StandardInstrumentations(*theContext_,
                                                                               /*DebugLogging*/ true))
{
    theModule_->setDataLayout(dataLayout);
    addDefaultPasses(theFPM_);
    setupPassManager(thePIC_, theMAM_, theFAM_, theLAM_, theCGAM_, theSI_);

    static uint64_t functionCount = 1;
    name_ = "function_" + std::to_string(functionCount++);

    auto addr_type = PointerType::get(*theContext_, 0);
    auto int32_type = Type::getInt32Ty(*theContext_);

    // Generate the prototype and add it to the module.
    // Param1: pointer to the Evaluator object.
    // Param2: address of the return value.
    // Param3: ID of the block to begin execution at (only when param4 is nullptr).
    // Param4: resume handle; nullptr = initial entry (use blockId), non-null = resume from that state.
    std::vector<Type *> paramAry = {addr_type, addr_type, int32_type, addr_type};

    // Return type: coroutine handle (addr_type).
    FunctionType *ft = FunctionType::get(addr_type, paramAry, false);
    theFunction_ = Function::Create(ft, Function::ExternalLinkage, name_, *theModule_);
    theFunction_->setPresplitCoroutine();

    // The first argument is the evaluator pointer.
    evaluator_ = theFunction_->getArg(0);
    evaluator_->setName("evaluator");

    // The second argument is the address of the return value.
    returnValueAddress_ = theFunction_->getArg(1);
    returnValueAddress_->setName("returnValueAddress");

    // The third argument is the block ID for the block to begin execution at.
    // Needed when we build the Table of Contents for this function.
    blockId_ = theFunction_->getArg(2);
    blockId_->setName("blockId");

    // The fourth argument is the resume handle (null = start, non-null = resume).
    resumeHandle_ = theFunction_->getArg(3);
    resumeHandle_->setName("resumeHandle");

}

BasicBlock *Scaffold::createBasicBlock(const std::string &name)
{
    return BasicBlock::Create(*theContext_, name, theFunction_);
}


CompiledText::~CompiledText()
{
    // Only remove resource tracker if compiler is still valid
    // (it may have been destroyed if CompiledText outlives the Compiler singleton)
    if (compiler_ != nullptr && rt_)
    {
        cantFail(rt_->remove());
    }
}

Compiler::Compiler()
{
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    lljit_ = createLLJIT();
}

Compiler::~Compiler()
{
    // Clear compiledTextTable first to ensure all CompiledText objects are destroyed
    // while lljit is still valid
    compiledTextTable_.clear();
}

QString Compiler::getTagNameFromNode(const DatumPtr &node) const
{
    Q_ASSERT(isTag(node));
    ASTNode *tagNode = node.astnodeValue()->childAtIndex(0).astnodeValue();
    if (tagNode->genExpression_ == &Compiler::genLiteral)
    {
        DatumPtr tagNameNode = tagNode->childAtIndex(0);
        if (tagNameNode.isWord())
        {
            return tagNameNode.toString(Datum::ToStringFlags_Key);
        }
    }
    return {};
}

void Compiler::setTagToBlockIdInProcedure(const QList<DatumPtr> &tagList, int32_t blockId)
{
    // // Get the currently-executing procedure.
    // NewCallFrame *currentFrame = Kernel::get().callFrameStack.top();

    // // If the current frame is not a procedure, there is no need to save the tag names
    // // because we can't jump to them.
    // if (!currentFrame->sourceNode.isASTNode())
    // {
    //     return;
    // }

    // Procedure *currentProcedure = currentFrame->sourceNode.astnodeValue()->procedure.procedureValue();
    // DatumPtr currentRunningLine = currentFrame->runningSourceList;

    // for (auto &node : tagList)
    // {
    //     QString tagName = getTagNameFromNode(node);
    //     if (!tagName.isEmpty())
    //     {
    //         currentProcedure->tagToBlockId[tagName] = blockId;
    //         currentProcedure->tagToLine[tagName] = currentRunningLine;
    //     }
    // }
}

BasicBlock *Compiler::generateTOC(QList<BasicBlock *> blocks, Function *theFunction)
{
    Q_ASSERT(blocks.size() > 1);

    BasicBlock *tocBlock = BasicBlock::Create(*scaff_->theContext_, "Toc", theFunction, blocks[0]);
    scaff_->builder_.SetInsertPoint(tocBlock);

    llvm::SwitchInst *switchInst = scaff_->builder_.CreateSwitch(scaff_->blockId_, blocks[0], static_cast<unsigned>(blocks.size() - 1));
    for (unsigned i = 1; i < blocks.size(); ++i)
    {
        switchInst->addCase(llvm::cast<llvm::ConstantInt>(
                                ConstantInt::get(scaff_->blockId_->getType(), i)),
                            blocks[i]);
    }

    // Coroutine prologue was inserted before the body entry; redirect it to Toc so execution
    // goes CoroPrologue -> Toc -> body (or suspend).
    if (scaff_->coroPrologueBB_)
    {
        llvm::BranchInst *prologueBr = llvm::cast<llvm::BranchInst>(scaff_->coroPrologueBB_->getTerminator());
        prologueBr->setSuccessor(0, tocBlock);
    }
    return tocBlock;
}

CompiledFunctionPtr Compiler::generateFunctionPtrFromASTList(QList<QList<DatumPtr>> parsedList, Datum *key)
{
    Scaffold compilerScaffolding(lljit_->getDataLayout());
    scaff_ = &compilerScaffolding;

    auto *compiledText = new CompiledText();
    compiledText->astList_ = parsedList;
    compiledText->compiler_ = this;
    compiledTextTable_[key] = std::shared_ptr<CompiledText>(compiledText);

    // The first block is number zero.
    int localBlockId = 0;

    // If the first block is a tag, save the tag names
    if (isTag(parsedList.first().first()))
    {
        setTagToBlockIdInProcedure(parsedList.first(), localBlockId);
        // Remove the first tag block from the list.
        parsedList.removeFirst();
    }

    // At this point we know that the first block and last block are not tags.

    BasicBlock *currentBlock = BasicBlock::Create(*scaff_->theContext_, "FirstBlock", scaff_->theFunction_);

    // TODO: move this to the end of the function.
    scaff_->mainBailoutBB_ = BasicBlock::Create(*scaff_->theContext_, "MainBailout", scaff_->theFunction_);

    QList<BasicBlock *> blocks = {currentBlock};
    scaff_->builder_.SetInsertPoint(currentBlock);

    Value *nodeResult;
    RequestReturnType returnTypeRequest = RequestReturnNothing;
    for (auto &srcBlock : parsedList)
    {
        if (isTag(srcBlock.first()))
        {
            ++localBlockId;
            BasicBlock *newBlock = BasicBlock::Create(*scaff_->theContext_, "Next Block", scaff_->theFunction_);
            blocks.append(newBlock);
            scaff_->builder_.SetInsertPoint(scaff_->builder_.GetInsertBlock());
            scaff_->builder_.CreateBr(newBlock);
            currentBlock = newBlock;
            scaff_->builder_.SetInsertPoint(newBlock);
            setTagToBlockIdInProcedure(srcBlock, localBlockId);
        }
        else
        {
            for (auto &node : srcBlock)
            {
                // If this is the last node, accept any Datum return type.
                // Otherwise, emit error if output is not nothing.
                if (node == parsedList.last().last())
                    returnTypeRequest = RequestReturnDN;
                nodeResult = generateChild(nullptr, node, returnTypeRequest);
            }
        }
    }

    // Finish off the function
    generateReturn(nodeResult);
    generateWrapup();

    // Move the cold path blocks to the end of the function.
    for (auto *block : scaff_->coldPathBlocks_) {
        block->moveAfter(&scaff_->theFunction_->back());
    }

    if (blocks.size() > 1)
    {
        generateTOC(blocks, scaff_->theFunction_);
    }

    if (Config::get().showIR_)
    {
        // Print the whole module so we see all functions after coroutine lowering (ramp, resume, destroy).
        scaff_->theFunction_->print(errs());
        fprintf(stderr, "\n");
    }

    if (Config::get().verifyIR_)
    {
        std::string str;
        llvm::raw_string_ostream output(str);
        // Validate the generated code, checking for consistency.
        if (verifyFunction(*(scaff_->theFunction_), &output))
        {
            std::cerr << "IR verification failed:\n" << str << "\n";
            throw FCError::fatalInternal();
        }
    }

    if (Config::get().showCFG_)
    {
        scaff_->theFunction_->viewCFG();
    }

    // Lower coroutine intrinsics before backend.
    runCoroutinePasses(*scaff_->theModule_, scaff_->theMAM_, scaff_->theFAM_, scaff_->theLAM_, scaff_->theCGAM_);

    // Run the optimizer on the function.
    scaff_->theFPM_.run(*(scaff_->theFunction_), scaff_->theFAM_);

    if (Config::get().showModuleIR_)
    {
        // Print the whole module so we see all functions after coroutine lowering (ramp, resume, destroy).
        scaff_->theModule_->print(errs(), nullptr);
        fprintf(stderr, "\n");
    }

    auto tsm = ThreadSafeModule(std::move(scaff_->theModule_), std::move(scaff_->theContext_));
    auto [addr, rt] = addModuleAndLookup(*lljit_, std::move(tsm), scaff_->name_);
    compiledText->rt_ = std::move(rt);
    compiledText->functionPtr_ = reinterpret_cast<CompiledFunctionPtr>(addr);
    return compiledText->functionPtr_;
}

QList<QList<DatumPtr>> Compiler::groupConsecutiveExpressions(const QList<DatumPtr> &expressions)
{
    QList<QList<DatumPtr>> retval;
    QList<DatumPtr> currentBlock;
    for (auto &node : expressions)
    {
        if (currentBlock.isEmpty())
            // Start a new block with this expression
            currentBlock.append(node);
        else
        {
            if (isTag(node) == isTag(currentBlock.last()))
                // Same type as previous expression: add to current block
                currentBlock.append(node);
            else
            {
                // Type changed: save current block and start a new one with this expression
                retval.append(currentBlock);
                currentBlock = {node};
            }
        }
    }
    // Append the final block of expressions
    retval.append(currentBlock);

    // If the last block is a tag, append a NOOP expression to ensure that there is an instruction to jump to.
    if (!currentBlock.isEmpty() && isTag(currentBlock.last()))
    {
        auto *noopNode = new ASTNode(DatumPtr(StringConstants::keywordNoop()));
        noopNode->genExpression_ = &Compiler::genNoop;
        noopNode->returnType_ = RequestReturnNothing;

        currentBlock = {DatumPtr(noopNode)};
        retval.append(currentBlock);
    }

    return retval;
}

CompiledFunctionPtr Compiler::functionPtrFromList(List *aList)
{
    if (aList->compileTimeStamp <= Procedures::get().timeOfLastProcedureCreation())
    {
        QList<DatumPtr> astFlatList = Treeifier::astFromList(aList);
        QList<QList<DatumPtr>> parsedList = groupConsecutiveExpressions(astFlatList);
        return generateFunctionPtrFromASTList(parsedList, static_cast<Datum *>(aList));
    }

    return compiledTextTable_[static_cast<Datum *>(aList)]->functionPtr_;
}

void Compiler::destroyCompiledTextForDatum(Datum *aDatum)
{
    compiledTextTable_.remove(aDatum);
}

Value *Compiler::generateChildOfNode(ASTNode *parent, const DatumPtr &node, RequestReturnType returnType)
{
    Generator method = node.astnodeValue()->genExpression_;
    Value *retval = ((this->*method)(node, returnType));
    return retval;
}

Value *Compiler::generateCast(Value *src, ASTNode *parent, const DatumPtr &node, RequestReturnType destReturnType)
{
    Q_ASSERT(!src->getType()->isVoidTy());
    RequestReturnType srcReturnType = node.astnodeValue()->returnType_;

    if (srcReturnType == destReturnType)
        return src;

    if (src->getType()->isDoubleTy())
        return generateCastFromDouble(src, parent, destReturnType);
    if (src->getType()->isIntegerTy(1))
        return generateCastFromBool(src, parent, destReturnType);
    if (srcReturnType == RequestReturnDatum)
        return generateCastFromDatum(src, parent, destReturnType);
    if (srcReturnType == RequestReturnNothing)
        return generateCastFromNothing(src, parent, destReturnType);

    Q_ASSERT(srcReturnType == RequestReturnDN);
    return generateCastFromDN(src, parent, destReturnType);
}

Value *Compiler::generateCastFromDouble(Value *src, ASTNode *parent, RequestReturnType destReturnType)
{
    if (destReturnType & RequestReturnReal)
        return src;
    Value *srcAsWord = generateWordFromDouble(src);
    if (destReturnType & RequestReturnDatum)
        return srcAsWord;
    if (destReturnType & RequestReturnBool)
        return generateImmediateReturn(generateErrorNoLike(parent, srcAsWord));
    Q_ASSERT(destReturnType & RequestReturnNothing);
    return generateImmediateReturn(generateErrorNoSay(srcAsWord));
}

Value *Compiler::generateCastFromBool(Value *src, ASTNode *parent, RequestReturnType destReturnType)
{
    if (destReturnType & RequestReturnBool)
        return src;
    Value *srcAsWord = generateWordFromBool(src);
    if (destReturnType & RequestReturnDatum)
        return srcAsWord;
    if (destReturnType & RequestReturnReal)
        return generateImmediateReturn(generateErrorNoLike(parent, srcAsWord));
    Q_ASSERT(destReturnType & RequestReturnNothing);
    return generateImmediateReturn(generateErrorNoSay(srcAsWord));
}

Value *Compiler::generateCastFromDatum(Value *src, ASTNode *parent, RequestReturnType destReturnType)
{
    if (destReturnType & RequestReturnDatum)
        return src;
    if (destReturnType & RequestReturnBool)
        return generateBoolFromDatum(parent, src);
    if (destReturnType & RequestReturnReal)
        return generateDoubleFromDatum(parent, src);
    Q_ASSERT(destReturnType & RequestReturnNothing);
    return generateImmediateReturn(generateErrorNoSay(src));
}

Value *Compiler::generateCastFromNothing(Value *src, ASTNode *parent, RequestReturnType destReturnType)
{
    if (destReturnType & RequestReturnNothing)
        return src;
    return generateImmediateReturn(generateErrorNoOutput(src, parent));
}

Value *Compiler::generateCastFromDN(Value *src, ASTNode *parent, RequestReturnType destReturnType)
{
    if (destReturnType == RequestReturnNothing)
        return generateNothingFromDatum(parent, src);
    if (destReturnType == RequestReturnDatum)
        return generateNotNothingFromDatum(parent, src);
    if (destReturnType == RequestReturnReal)
        return generateDoubleFromDatum(parent, src);
    Q_ASSERT(destReturnType == RequestReturnBool);
    return generateBoolFromDatum(parent, src);
}

Value *Compiler::generateChild(ASTNode *parent, const DatumPtr &node, RequestReturnType returnType)
{
    Value *child = generateChildOfNode(parent, node, returnType);
    return generateCast(child, parent, node, returnType);
}

Value *Compiler::generateChild(ASTNode *parent, unsigned int index, RequestReturnType returnType)
{
    DatumPtr node = parent->childAtIndex(index);
    return generateChild(parent, node, returnType);
}

Value *Compiler::generateDoubleFromDatum(ASTNode *parent, Value *src)
{
    Value *retval = nullptr;
    auto realTest = [this, &retval](Value *src) {
        retval = generateCallExtern(TyDouble, getDoubleForDatum, PaAddr(scaff_->evaluator_), PaAddr(src));
        Value *dType = generateCallExtern(TyBool, getValidityOfDoubleForDatum, PaAddr(scaff_->evaluator_), PaAddr(src));
        return scaff_->builder_.CreateICmpEQ(dType, CoBool(true), DBG_NAME("isValidTest"));
    };
    generateValidationDatum(parent, src, realTest);
    return retval;
}

Value *Compiler::generateBoolFromDatum(ASTNode *parent, Value *src)
{
    Value *retval = nullptr;
    auto boolTest = [this, &retval](Value *src) {
        retval = generateCallExtern(TyBool, getBoolForDatum, PaAddr(scaff_->evaluator_), PaAddr(src));
        Value *dType = generateCallExtern(TyBool, getValidityOfBoolForDatum, PaAddr(scaff_->evaluator_), PaAddr(src));
        return scaff_->builder_.CreateICmpEQ(dType, CoBool(true), DBG_NAME("isValidTest"));
    };
    generateValidationDatum(parent, src, boolTest);
    return retval;
}

Value *Compiler::generateFromDatum(Datum::DatumType t, ASTNode *parent, Value *src)
{
    auto typeTest = [this, t](Value *src) {
        Value *dType = generateGetDatumIsa(src);
        Value *mask = scaff_->builder_.CreateAnd(dType, CoInt32(t), DBG_NAME("dataTypeMask"));
        Value *cond = scaff_->builder_.CreateICmpNE(mask, CoInt32(0), DBG_NAME("typeTest"));
        return cond;
    };
    return generateValidationDatum(parent, src, typeTest);
}

Value *Compiler::generateNotNothingFromDatum(ASTNode *parent, Value *src)
{
    BasicBlock *isNothingBB = scaff_->createBasicBlock(DBG_NAME("isNothing"));
    BasicBlock *notNothingBB = scaff_->createBasicBlock(DBG_NAME("notNothing"));

    scaff_->addColdPathBlocks(isNothingBB);

    // isNothing?
    Value *dType = generateGetDatumIsa(src);
    Value *mask = scaff_->builder_.CreateAnd(dType, CoInt32(Datum::typeDataMask), DBG_NAME("dataTypeMask"));
    Value *cond = scaff_->builder_.CreateICmpEQ(mask, CoInt32(0), DBG_NAME("dataTypeMaskTest"));
    scaff_->builder_.CreateCondBr(cond, isNothingBB, notNothingBB);

    // Bad
    scaff_->builder_.SetInsertPoint(isNothingBB);
    Value *errWhat = src;
    Value *errObj = generateErrorNoOutput(errWhat, parent);
    generateReturn(errObj);

    // Good
    scaff_->builder_.SetInsertPoint(notNothingBB);

    return src;
}

Value *Compiler::generateNothingFromDatum(ASTNode *parent, Value *src)
{
    BasicBlock *notNothingBB = scaff_->createBasicBlock(DBG_NAME("notNothing"));
    BasicBlock *isNothingBB = scaff_->createBasicBlock(DBG_NAME("isNothing"));

    scaff_->addColdPathBlocks(notNothingBB);

    // isNothing?
    Value *dType = generateGetDatumIsa(src);
    Value *mask = scaff_->builder_.CreateAnd(dType, CoInt32(Datum::typeDataMask), DBG_NAME("dataTypeMask"));
    Value *cond = scaff_->builder_.CreateICmpEQ(mask, CoInt32(0), DBG_NAME("dataTypeMaskTest"));
    scaff_->builder_.CreateCondBr(cond, isNothingBB, notNothingBB);

    // Bad
    scaff_->builder_.SetInsertPoint(notNothingBB);
    Value *errObj = generateErrorNoSay(src);
    generateReturn(errObj);

    // Good
    scaff_->builder_.SetInsertPoint(isNothingBB);

    return src;
}

Value *Compiler::generateWordFromDatum(ASTNode *parent, Value *src)
{
    return generateFromDatum(Datum::typeWord, parent, src);
}

Value *Compiler::generateListFromDatum(ASTNode *parent, Value *src)
{
    return generateFromDatum(Datum::typeList, parent, src);
}

Value *Compiler::generateArrayFromDatum(ASTNode *parent, Value *src)
{
    return generateFromDatum(Datum::typeArray, parent, src);
}

Value *Compiler::genLiteral(const DatumPtr &node, RequestReturnType returnType)
{
    DatumPtr literalPtr = node.astnodeValue()->childAtIndex(0);

    // A literal is a Word, List, or Array.
    // However, the caller may want a Bool or Real.
    // We can cast, but only if the literal is a Word and can be cast to the requested type.
    if (literalPtr.isWord())
    {
        Word *wVal = literalPtr.wordValue();
        if (returnType == RequestReturnReal)
        {
            double val = wVal->numberValue();
            if (wVal->numberIsValid)
                return CoDouble(val);
        }
        if (returnType == RequestReturnBool)
        {
            bool val = wVal->boolValue();
            if (wVal->boolIsValid)
                return CoBool(val);
        }
    }

    // Casting has failed, or the caller simply requested a Datum.
    // In any case, return the literal as a Datum.
    Datum *val = literalPtr.datumValue();
    return CoAddr(val);
}

Value *Compiler::generateVoidRetval(const DatumPtr &node)
{
    return CoAddr(node.astnodeValue());
}

Value *Compiler::genValueOf(const DatumPtr &node, RequestReturnType returnType)
{
    BasicBlock *noValueBB = scaff_->createBasicBlock(DBG_NAME("NoValue"));
    BasicBlock *hasValueBB = scaff_->createBasicBlock(DBG_NAME("hasValue"));

    scaff_->addColdPathBlocks(noValueBB);

    Word *varName = node.astnodeValue()->childAtIndex(0).wordValue();
    Value *nameAddr = CoAddr(varName);
    Value *retval = generateCallExtern(TyAddr, getDatumForVarname, PaAddr(nameAddr));

    Value *dType = generateGetDatumIsa(retval);
    Value *mask = scaff_->builder_.CreateAnd(dType, CoInt32(Datum::typeDataMask), DBG_NAME("dataMask"));
    Value *cond = scaff_->builder_.CreateICmpEQ(mask, CoInt32(0), DBG_NAME("dataMaskTest"));
    scaff_->builder_.CreateCondBr(cond, noValueBB, hasValueBB);

    scaff_->builder_.SetInsertPoint(noValueBB);
    Value *errObj = generateErrorNoValue(nameAddr);
    generateReturn(errObj);

    scaff_->builder_.SetInsertPoint(hasValueBB);
    return retval;
}

Value *Compiler::genExecProcedure(const DatumPtr &node, RequestReturnType returnType)
{
    AllocaInst *paramAry = generateChildrenAlloca(node.astnodeValue(), RequestReturnDatum, DBG_NAME("paramAry"));
    Value *vAstnodeValue = CoAddr(node.astnodeValue());
    Value *vParamArySize = CoInt32(node.astnodeValue()->countOfChildren());
    return generateCallExtern(
        TyAddr, runProcedure, PaAddr(scaff_->evaluator_), PaAddr(vAstnodeValue), PaAddr(paramAry), PaInt32(vParamArySize));
}

Value *Compiler::ensureCoroutineFrame()
{
    if (scaff_->suspendBB_ != nullptr)
        return scaff_->coroutineHandle_;

    // Emit the coroutine frame at the beginning of the function so it runs on every path (including
    // paths that never suspend). Place it before the current entry block; if generateTOC runs later,
    // it will redirect this prologue's branch to Toc.
    BasicBlock *bodyEntry = &scaff_->theFunction_->getEntryBlock();
    scaff_->coroPrologueBB_ =
        BasicBlock::Create(*scaff_->theContext_, "CoroPrologue", scaff_->theFunction_, bodyEntry);
    BasicBlock *savedBlock = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.SetInsertPoint(scaff_->coroPrologueBB_);

    scaff_->suspendBB_ = BasicBlock::Create(*scaff_->theContext_, "suspend", scaff_->theFunction_);
    scaff_->cleanupBB_ = BasicBlock::Create(*scaff_->theContext_, "cleanup", scaff_->theFunction_);
    // Coroutine frame: id -> size -> alloc -> begin
    // llvm.coro.id(i32 align, ptr promise, ptr coroutine, ptr info) -> token
    Function *coroIdFn = Intrinsic::getOrInsertDeclaration(scaff_->theModule_.get(), Intrinsic::coro_id);
    scaff_->coroutineToken_ = scaff_->builder_.CreateCall(
        coroIdFn, {CoInt32(0), CoAddr(0), CoAddr(0), CoAddr(0)}, DBG_NAME("id"));
    Value *coroutineCallToken = scaff_->coroutineToken_;

    Function *coroSizeFn = Intrinsic::getOrInsertDeclaration(scaff_->theModule_.get(), Intrinsic::coro_size, {TyInt32});
    Value *coroutineSize = scaff_->builder_.CreateCall(coroSizeFn, {}, DBG_NAME("size"));
    Value *coroutineAlloc = generateCallExtern(TyAddr, q_malloc, PaAddr(scaff_->evaluator_), PaInt32(coroutineSize));

    Function *coroBeginFn = Intrinsic::getOrInsertDeclaration(scaff_->theModule_.get(), Intrinsic::coro_begin);
    CallInst *coroBeginCall = cast<CallInst>(
        scaff_->builder_.CreateCall(coroBeginFn, {coroutineCallToken, coroutineAlloc}, DBG_NAME("handle")));
    coroBeginCall->addRetAttr(Attribute::NoAlias);
    scaff_->coroutineHandle_ = coroBeginCall;

    scaff_->builder_.CreateBr(bodyEntry);
    scaff_->builder_.SetInsertPoint(savedBlock);
    return scaff_->coroutineHandle_;
}

Value *Compiler::generateCallList(Value *list, RequestReturnType returnType)
{
    // Explicit control: push list onto evaluation stack, suspend so driver can run it, then pop and return result.
    generateCallExtern(TyVoid, pushListOntoEvaluationStack, PaAddr(scaff_->evaluator_), PaAddr(list));

    ensureCoroutineFrame();

    // Suspend point: llvm.coro.suspend(token none, i1 false) -> i8 (0=resume, 1=destroy, default=suspend)
    Function *coroSuspendFn = Intrinsic::getOrInsertDeclaration(scaff_->theModule_.get(), Intrinsic::coro_suspend);
    Value *coroutineSuspend = scaff_->builder_.CreateCall(
        coroSuspendFn, {ConstantTokenNone::get(*scaff_->theContext_), CoBool(false)}, DBG_NAME("suspend"));

    BasicBlock *continueBB = BasicBlock::Create(*scaff_->theContext_, "continue", scaff_->theFunction_);
    SwitchInst *sw = scaff_->builder_.CreateSwitch(coroutineSuspend, scaff_->suspendBB_, 2);
    sw->addCase(CoInt8(0), continueBB);
    sw->addCase(CoInt8(1), scaff_->cleanupBB_);

    scaff_->builder_.SetInsertPoint(continueBB);
    return generateCallExtern(TyAddr, popEvaluationStackAndGetResult, PaAddr(scaff_->evaluator_));
}

Value *Compiler::generateWordFromDouble(Value *val)
{
    return generateCallExtern(TyAddr, getWordForDouble, PaAddr(scaff_->evaluator_), PaDouble(val));
}

Value *Compiler::generateWordFromBool(Value *val)
{
    return generateCallExtern(TyAddr, getWordForBool, PaAddr(scaff_->evaluator_), PaBool(val));
}

Value *Compiler::generateErrorSystem()
{
    Value *errObj = generateCallExtern(TyAddr, getErrorSystem, PaAddr(scaff_->evaluator_));
    return errObj;
}

Value *Compiler::generateErrorToplevel()
{
    Value *errObj = generateCallExtern(TyAddr, getErrorToplevel, PaAddr(scaff_->evaluator_));
    return errObj;
}

Value *Compiler::generateErrorNoLike(ASTNode *who, Value *what)
{
    Value *errWho = CoAddr(who->nodeName_.datumValue());
    Value *errObj = generateCallExtern(TyAddr, getErrorNoLike, PaAddr(scaff_->evaluator_), PaAddr(errWho), PaAddr(what));
    return errObj;
}

Value *Compiler::generateErrorNoSay(Value *what)
{
    Value *errObj = generateCallExtern(TyAddr, getErrorNoSay, PaAddr(scaff_->evaluator_), PaAddr(what));
    return errObj;
}

Value *Compiler::generateErrorNoTest(Value *who)
{
    Value *errObj = generateCallExtern(TyAddr, getErrorNoTest, PaAddr(scaff_->evaluator_), PaAddr(who));
    return errObj;
}

Value *Compiler::generateErrorNoValue(Value *what)
{
    Value *errObj = generateCallExtern(TyAddr, getErrorNoValue, PaAddr(scaff_->evaluator_), PaAddr(what));
    return errObj;
}

Value *Compiler::generateErrorNoOutput(Value *x, ASTNode *y)
{
    Value *vY = CoAddr(y->nodeName_.datumValue());
    Value *errObj = generateCallExtern(TyAddr, getErrorNoOutput, PaAddr(scaff_->evaluator_), PaAddr(x), PaAddr(vY));
    return errObj;
}

Value *Compiler::generateErrorNotEnoughInputs(ASTNode *x)
{
    Value *vX = CoAddr(x->nodeName_.datumValue());
    Value *errObj = generateCallExtern(TyAddr, getErrorNotEnoughInputs, PaAddr(scaff_->evaluator_), PaAddr(vX));
    return errObj;
}

Value *Compiler::generateImmediateReturn(llvm::Value *retval)
{
    BasicBlock *bailoutBB = scaff_->createBasicBlock(DBG_NAME("bailout"));
    BasicBlock *throwAwayBB = scaff_->createBasicBlock(DBG_NAME("throwAway"));

    // We are going to return something at this point.
    // However, there may be code after this point that we have to compile and then ignore.
    // So we need to:
    // 1. Allow the code before this point to execute.
    // 2. Return the control operation.
    // 3. Ignore any code after the return operation.
    // To do this we allow the compiler to finish generating the code after the return operation,
    // and insert it after a test that will always fail, so the code will never be executed.
    Value *cond = scaff_->builder_.CreateICmpEQ(CoBool(1), CoBool(0), DBG_NAME("fakeTest"));
    scaff_->builder_.CreateCondBr(cond, throwAwayBB, bailoutBB);

    scaff_->builder_.SetInsertPoint(bailoutBB);
    generateReturn(retval);

    // Any code that the compiler has remaining to generate after the return operation will
    // be placed here, and then ignored.
    scaff_->builder_.SetInsertPoint(throwAwayBB);
    return retval;
}

std::vector<Value *> Compiler::generateChildren(ASTNode *node, RequestReturnType returnType)
{
    std::vector<Value *> retval;
    retval.reserve(node->countOfChildren());
    for (int i = 0; i < node->countOfChildren(); ++i)
    {
        Value *v = generateChildOfNode(node, node->childAtIndex(i), returnType);
        Value *casted = generateCast(v, node, node->childAtIndex(i), returnType);
        retval.push_back(casted);
    }

    return retval;
}

AllocaInst *Compiler::generateChildrenAlloca(ASTNode *node, RequestReturnType returnType, const std::string &name)
{
    std::vector<Value *> children = generateChildren(node, returnType);
    return generateAllocaAry(children, name);
}

AllocaInst *Compiler::generateAllocaAry(const std::vector<llvm::Value *> &values, const std::string &name)
{
    Value *childCount = CoInt32(values.size());
    Value *offset = CoInt64(lljit_->getDataLayout().getPointerSize());
    AllocaInst *retval = scaff_->builder_.CreateAlloca(TyAddr, childCount, name);
    Value *aryPtr = retval;
    for (int i = 0; i < values.size(); ++i)
    {
        scaff_->builder_.CreateStore(values[i], aryPtr);
        if (i < values.size() - 1)
            aryPtr = scaff_->builder_.CreatePtrAdd(aryPtr, offset, name + "Incr");
    }
    return retval;
}

std::vector<Value *> Compiler::generateChildren(ASTNode *node, std::vector<RequestReturnType> returnTypeAry)
{
    Q_ASSERT(node->countOfChildren() == returnTypeAry.size());
    std::vector<Value *> retval;
    retval.reserve(node->countOfChildren());
    for (int i = 0; i < node->countOfChildren(); ++i)
    {
        retval.push_back(generateChildOfNode(node, node->childAtIndex(i), returnTypeAry[i]));
    }
    for (int i = 0; i < node->countOfChildren(); ++i)
    {
        retval[i] = generateCast(retval[i], node, node->childAtIndex(i), returnTypeAry[i]);
    }
    return retval;
}

// Generate a call to an external function
Value *Compiler::generateExternFunctionCall(Type *returnType,
                                            const std::string &name,
                                            const std::vector<std::pair<Type *, Value *>> &args)
{
    std::vector<Type *> paramTypes;
    std::vector<Value *> argsV;
    paramTypes.reserve(args.size());
    argsV.reserve(args.size());
    for (const auto &arg : args)
    {
        paramTypes.push_back(arg.first);
        argsV.push_back(arg.second);
    }

    FunctionType *fType = FunctionType::get(returnType, paramTypes, false);
    FunctionCallee calleeF = scaff_->theModule_->getOrInsertFunction(name, fType);

    Q_ASSERT(calleeF.getFunctionType()->getNumParams() == argsV.size());

    if (returnType->isVoidTy())
        return scaff_->builder_.CreateCall(calleeF, argsV);
    else
        return scaff_->builder_.CreateCall(calleeF, argsV, name + "_result");
}

AllocaInst *Compiler::generateNumberAryFromDatum(ASTNode *parent, const DatumPtr &srcPtr, int32_t size)
{
    Value *vSize = CoInt32(size);
    Value *list = generateChild(parent, srcPtr, RequestReturnDatum);
    Value *count = generateCallExtern(TyInt32, getCountOfList, PaAddr(list));
    // There should be two doubles in the list
    BasicBlock *noLikeBB = scaff_->createBasicBlock(DBG_NAME("noLike"));
    BasicBlock *continueBB = scaff_->createBasicBlock(DBG_NAME("good"));
    BasicBlock *gotPosBB = scaff_->createBasicBlock(DBG_NAME("gotPos"));

    scaff_->addColdPathBlocks(noLikeBB);

    Value *countGood = scaff_->builder_.CreateICmpEQ(count, vSize, DBG_NAME("countTest"));
    scaff_->builder_.CreateCondBr(countGood, continueBB, noLikeBB);

    scaff_->builder_.SetInsertPoint(noLikeBB);
    Value *errWho = CoAddr(parent->nodeName_.datumValue());
    Value *errObj = generateCallExtern(TyAddr, getErrorNoLike, PaAddr(scaff_->evaluator_), PaAddr(errWho), PaAddr(list));
    generateReturn(errObj);

    scaff_->builder_.SetInsertPoint(continueBB);
    AllocaInst *ary = scaff_->builder_.CreateAlloca(TyDouble, vSize, DBG_NAME("ary"));
    Value *isGood = generateCallExtern(TyInt32, getNumberAryFromList, PaAddr(list), PaAddr(ary));
    Value *countCond = scaff_->builder_.CreateICmpEQ(isGood, CoInt32(1), DBG_NAME("countTest"));
    scaff_->builder_.CreateCondBr(countCond, gotPosBB, noLikeBB);

    scaff_->builder_.SetInsertPoint(gotPosBB);
    return ary;
}

Value *Compiler::generateValidationDouble(ASTNode *parent, Value *src, const validatorFunction &validator)
{
    BasicBlock *srcBB = scaff_->builder_.GetInsertBlock();

    BasicBlock *validateBB = scaff_->createBasicBlock(DBG_NAME("validate"));
    BasicBlock *convertBB = scaff_->createBasicBlock(DBG_NAME("convert"));
    BasicBlock *erractBB = scaff_->createBasicBlock(DBG_NAME("errorAction"));
    BasicBlock *bailoutBB = scaff_->createBasicBlock(DBG_NAME("bailout"));
    BasicBlock *acceptBB = scaff_->createBasicBlock(DBG_NAME("accept"));

    scaff_->addColdPathBlocks(erractBB, convertBB, bailoutBB);

    scaff_->builder_.CreateBr(validateBB);

    // Validate the number.
    scaff_->builder_.SetInsertPoint(validateBB);
    PHINode *candidate = scaff_->builder_.CreatePHI(TyDouble, 2, DBG_NAME("candidate"));
    candidate->addIncoming(src, srcBB);
    Value *isValidCond = validator(candidate);
    scaff_->builder_.CreateCondBr(isValidCond, acceptBB, erractBB);

    // The number is bad. Call handleBadDouble and maybe retry with the result.
    scaff_->builder_.SetInsertPoint(erractBB);
    Value *handlerResult = generateCallExtern(TyAddr, handleBadDouble, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(parent)), PaDouble(candidate));
    Value *datamIsa = generateGetDatumIsa(handlerResult); // See if result is a datum.
    Value *isDatumMasked = scaff_->builder_.CreateAnd(datamIsa, CoInt32(Datum::typeDataMask), DBG_NAME("isDatumMasked"));
    Value *isDatumCond = scaff_->builder_.CreateICmpNE(isDatumMasked, CoInt32(0), DBG_NAME("isDatumCond"));
    scaff_->builder_.CreateCondBr(isDatumCond, convertBB, bailoutBB);

    // A Word was returned. Convert it to a double and try validating it again.
    scaff_->builder_.SetInsertPoint(convertBB);
    Value *dVal = generateCallExtern(TyDouble, getDoubleForDatum, PaAddr(scaff_->evaluator_), PaAddr(handlerResult));
    candidate->addIncoming(dVal, convertBB);
    scaff_->builder_.CreateBr(validateBB);

    // The number is bad, and ERRACT is not set. Return a DOESN'T LIKE error.
    scaff_->builder_.SetInsertPoint(bailoutBB);
    generateReturn(handlerResult);

    // The number is good. Continue.
    scaff_->builder_.SetInsertPoint(acceptBB);
    return candidate;
}

Value *Compiler::generateValidationDatum(ASTNode *parent, Value *src, const validatorFunction &validator)
{
    BasicBlock *srcBB = scaff_->builder_.GetInsertBlock();

    BasicBlock *validateBB = scaff_->createBasicBlock(DBG_NAME("validate"));
    BasicBlock *erractBB = scaff_->createBasicBlock(DBG_NAME("errorAction"));
    BasicBlock *bailoutBB = scaff_->createBasicBlock(DBG_NAME("bailout"));
    BasicBlock *acceptBB = scaff_->createBasicBlock(DBG_NAME("accept"));

    scaff_->addColdPathBlocks(erractBB, bailoutBB);

    scaff_->builder_.CreateBr(validateBB);

    // Validate the datum.
    scaff_->builder_.SetInsertPoint(validateBB);
    PHINode *candidate = scaff_->builder_.CreatePHI(TyAddr, 2, DBG_NAME("candidate"));
    candidate->addIncoming(src, srcBB);
    Value *cond = validator(candidate);
    scaff_->builder_.CreateCondBr(cond, acceptBB, erractBB);

    // The datum is bad. Call handleBadDatum and maybe retry with the result.
    scaff_->builder_.SetInsertPoint(erractBB);
    Value *handlerResult = generateCallExtern(TyAddr, handleBadDatum, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(parent)), PaAddr(candidate));
    Value *datamIsa = generateGetDatumIsa(handlerResult); // See if result is a datum.
    Value *isDatumMasked = scaff_->builder_.CreateAnd(datamIsa, CoInt32(Datum::typeDataMask), DBG_NAME("isDatumMasked"));
    Value *isDatumCond = scaff_->builder_.CreateICmpNE(isDatumMasked, CoInt32(0), DBG_NAME("isDatumCond"));
    candidate->addIncoming(handlerResult, erractBB);
    scaff_->builder_.CreateCondBr(isDatumCond, validateBB, bailoutBB);

    // The datum is bad, and ERRACT is not set. Return a DOESN'T LIKE error.
    scaff_->builder_.SetInsertPoint(bailoutBB);
    generateReturn(handlerResult);

    // The datum is good. Continue.
    scaff_->builder_.SetInsertPoint(acceptBB);
    return candidate;
}

void Compiler::generateReturn(Value *retval)
{
    scaff_->builder_.CreateStore(retval, scaff_->returnValueAddress_);
    scaff_->builder_.CreateBr(scaff_->mainBailoutBB_);
}

void Compiler::generateWrapup()
{
    scaff_->builder_.SetInsertPoint(scaff_->mainBailoutBB_);
 
    if (scaff_->coroutineHandle_)
    {
        scaff_->builder_.CreateStore(ConstantPointerNull::get(TyAddr), scaff_->coroutineHandle_);
        scaff_->builder_.CreateBr(scaff_->suspendBB_);
        // cleanup:
        scaff_->builder_.SetInsertPoint(scaff_->cleanupBB_);
        //   %mem = call ptr @llvm.coro.free(token %id, ptr %hdl)
        Function *coroFreeFn = Intrinsic::getOrInsertDeclaration(scaff_->theModule_.get(), Intrinsic::coro_free);
        Value *memToFree = scaff_->builder_.CreateCall(coroFreeFn, {scaff_->coroutineToken_, scaff_->coroutineHandle_}, DBG_NAME("mem"));
        //   Only free when allocation was dynamic (mem non-null); skip when allocation was elided.
        Value *needFree = scaff_->builder_.CreateICmpNE(memToFree, CoAddr(0), DBG_NAME("needFree"));
        BasicBlock *dynFreeBB = BasicBlock::Create(*scaff_->theContext_, "cleanup.free", scaff_->theFunction_);
        BasicBlock *cleanupEndBB = BasicBlock::Create(*scaff_->theContext_, "cleanup.end", scaff_->theFunction_);
        scaff_->builder_.CreateCondBr(needFree, dynFreeBB, cleanupEndBB);
        scaff_->builder_.SetInsertPoint(dynFreeBB);
        generateCallExtern(TyAddr, q_free, PaAddr(scaff_->evaluator_), PaAddr(memToFree));
        scaff_->builder_.CreateBr(cleanupEndBB);
        scaff_->builder_.SetInsertPoint(cleanupEndBB);
        //   br label %suspend
        scaff_->builder_.CreateBr(scaff_->suspendBB_);

        scaff_->builder_.SetInsertPoint(scaff_->suspendBB_);
        //   call i1 @llvm.coro.end(ptr %hdl, i1 false, token none)  -- result unused; intrinsic returns i1 in current LLVM
        Function *coroEndFn = Intrinsic::getOrInsertDeclaration(scaff_->theModule_.get(), Intrinsic::coro_end);
        scaff_->builder_.CreateCall(coroEndFn, {scaff_->coroutineHandle_, CoBool(false), ConstantTokenNone::get(*scaff_->theContext_)}, DBG_NAME("end"));
        //   ret ptr %hdl
        scaff_->builder_.CreateRet(scaff_->coroutineHandle_);
    }
    else
    {
        scaff_->builder_.CreateRet(ConstantPointerNull::get(TyAddr));
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"

// Generate code to return a datum type (isa) of a given object.
Value *Compiler::generateGetDatumIsa(Value *objAddr)
{
    const unsigned int isaOffset = offsetof(Datum, isa_);
    Value *isaAddr = scaff_->builder_.CreatePtrAdd(objAddr, CoInt64(isaOffset), DBG_NAME("isaAddr"));

    Value *dType = scaff_->builder_.CreateLoad(TyInt32, isaAddr, DBG_NAME("isaLoad"));
    return dType;
}

#pragma GCC diagnostic pop
