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
#include <iostream>

QHash<Datum *, std::shared_ptr<CompiledText>> Compiler::compiledTextTable;

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
} // namespace

Scaffold::Scaffold(const llvm::DataLayout &dataLayout)
    : theContext(std::make_unique<LLVMContext>()), theModule(std::make_unique<Module>("QLogoJIT", *theContext)),
      builder(IRBuilder<>(*theContext)), theFPM(FunctionPassManager()), theLAM(LoopAnalysisManager()),
      theFAM(FunctionAnalysisManager()), theCGAM(CGSCCAnalysisManager()), theMAM(ModuleAnalysisManager()),
      thePIC(PassInstrumentationCallbacks()), theSI(StandardInstrumentations(*theContext,
                                                                             /*DebugLogging*/ true))

{
    // Open a new context and module.
    theModule->setDataLayout(dataLayout);

    // Create new pass and analysis managers.
    theSI.registerCallbacks(thePIC, &theMAM);

    // Add transform passes.
    // Do simple "peephole" optimizations and bit-twiddling optimizations.
    theFPM.addPass(InstCombinePass());
    // Reassociate expressions.
    theFPM.addPass(ReassociatePass());
    // Eliminate Common SubExpressions.
    theFPM.addPass(GVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    theFPM.addPass(SimplifyCFGPass());

    // Register analysis passes used in these transform passes.
    PassBuilder pb;
    pb.registerModuleAnalyses(theMAM);
    pb.registerFunctionAnalyses(theFAM);
    pb.crossRegisterProxies(theLAM, theFAM, theCGAM, theMAM);

    // The name of the function is sequentially numbered.
    static uint64_t functionCount = 1;
    name = "function_" + std::to_string(functionCount++);
}

CompiledText::~CompiledText()
{
    // Only remove resource tracker if compiler is still valid
    // (it may have been destroyed if CompiledText outlives the Compiler singleton)
    if (compiler != nullptr && rt)
    {
        cantFail(rt->remove());
    }
}

Compiler::Compiler()
{
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    lljit = createLLJIT();
}

Compiler::~Compiler()
{
    // Clear compiledTextTable first to ensure all CompiledText objects are destroyed
    // while lljit is still valid
    compiledTextTable.clear();
}

QString Compiler::getTagNameFromNode(const DatumPtr &node) const
{
    Q_ASSERT(isTag(node));
    ASTNode *tagNode = node.astnodeValue()->childAtIndex(0).astnodeValue();
    if (tagNode->genExpression == &Compiler::genLiteral)
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
    // Get the currently-executing procedure.
    CallFrame *currentFrame = Kernel::get().callStack.localFrame();

    // If the current frame is not a procedure, there is no need to save the tag names
    // because we can't jump to them.
    if (!currentFrame->sourceNode.isASTNode())
    {
        return;
    }

    Procedure *currentProcedure = currentFrame->sourceNode.astnodeValue()->procedure.procedureValue();
    DatumPtr currentRunningLine = currentFrame->runningSourceList;

    for (auto &node : tagList)
    {
        QString tagName = getTagNameFromNode(node);
        if (!tagName.isEmpty())
        {
            currentProcedure->tagToBlockId[tagName] = blockId;
            currentProcedure->tagToLine[tagName] = currentRunningLine;
        }
    }
}

BasicBlock *Compiler::generateTOC(QList<BasicBlock *> blocks, Function *theFunction)
{
    Q_ASSERT(blocks.size() > 1);

    BasicBlock *tocBlock = BasicBlock::Create(*scaff->theContext, "Toc", theFunction, blocks[0]);
    scaff->builder.SetInsertPoint(tocBlock);

    llvm::SwitchInst *switchInst = scaff->builder.CreateSwitch(blockId, blocks[0], static_cast<unsigned>(blocks.size() - 1));
    for (unsigned i = 1; i < blocks.size(); ++i)
    {
        switchInst->addCase(llvm::cast<llvm::ConstantInt>(
                                ConstantInt::get(blockId->getType(), i)),
                            blocks[i]);
    }
    return tocBlock;
}

CompiledFunctionPtr Compiler::generateFunctionPtrFromASTList(QList<QList<DatumPtr>> parsedList, Datum *key)
{
    Scaffold compilerScaffolding(lljit->getDataLayout());
    scaff = &compilerScaffolding;

    auto *compiledText = new CompiledText();
    compiledText->astList = parsedList;
    compiledText->compiler = this;
    compiledTextTable[key] = std::shared_ptr<CompiledText>(compiledText);

    // Generate the prototype and add it to the module.
    // Param1: pointer to the Evaluator object.
    // Param2: ID of the block to begin execution at.
    std::vector<Type *> paramAry = {TyAddr, TyInt32};

    // Returning an int64* type, indicates pointer to a Datum.
    FunctionType *ft = FunctionType::get(TyAddr, paramAry, false);
    Function *theFunction = Function::Create(ft, Function::ExternalLinkage, scaff->name, *scaff->theModule);

    // The first argument is the evaluator pointer.
    evaluator = theFunction->getArg(0);
    evaluator->setName("evaluator");

    // The second argument is the block ID for the block to begin execution at.
    blockId = theFunction->getArg(1);
    blockId->setName("blockId");

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

    BasicBlock *currentBlock = BasicBlock::Create(*scaff->theContext, "First Block", theFunction);
    QList<BasicBlock *> blocks = {currentBlock};
    scaff->builder.SetInsertPoint(currentBlock);

    Value *nodeResult;
    RequestReturnType returnTypeRequest = RequestReturnNothing;
    for (auto &srcBlock : parsedList)
    {
        if (isTag(srcBlock.first()))
        {
            ++localBlockId;
            BasicBlock *newBlock = BasicBlock::Create(*scaff->theContext, "Next Block", theFunction);
            blocks.append(newBlock);
            scaff->builder.SetInsertPoint(scaff->builder.GetInsertBlock());
            scaff->builder.CreateBr(newBlock);
            currentBlock = newBlock;
            scaff->builder.SetInsertPoint(newBlock);
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
    scaff->builder.CreateRet(nodeResult);

    if (blocks.size() > 1)
    {
        generateTOC(blocks, theFunction);
    }

    std::string str;
    llvm::raw_string_ostream output(str);
    // Validate the generated code, checking for consistency.
    if (Config::get().verifyIR && verifyFunction(*theFunction, &output))
    {
        qCritical() << "IR verification failed: " << str << "\n";
        throw FCError::fatalInternal();
    }

    // Run the optimizer on the function.
    scaff->theFPM.run(*theFunction, scaff->theFAM);

    if (Config::get().showIR)
    {
        theFunction->print(errs());
        fprintf(stderr, "\n");
    }

    if (Config::get().showCFG)
    {
        theFunction->viewCFG();
    }

    auto tsm = ThreadSafeModule(std::move(scaff->theModule), std::move(scaff->theContext));
    auto [addr, rt] = addModuleAndLookup(*lljit, std::move(tsm), scaff->name);
    compiledText->rt = std::move(rt);
    compiledText->functionPtr = reinterpret_cast<CompiledFunctionPtr>(addr);
    return compiledText->functionPtr;
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
        noopNode->genExpression = &Compiler::genNoop;
        noopNode->returnType = RequestReturnNothing;

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

    return compiledTextTable[static_cast<Datum *>(aList)]->functionPtr;
}

void Compiler::destroyCompiledTextForDatum(Datum *aDatum)
{
    compiledTextTable.remove(aDatum);
}

Value *Compiler::generateChildOfNode(ASTNode *parent, const DatumPtr &node, RequestReturnType returnType)
{
    Generator method = node.astnodeValue()->genExpression;
    Value *retval = ((this->*method)(node, returnType));
    return retval;
}

Value *Compiler::generateCast(Value *src, ASTNode *parent, const DatumPtr &node, RequestReturnType destReturnType)
{
    Q_ASSERT(!src->getType()->isVoidTy());
    RequestReturnType srcReturnType = node.astnodeValue()->returnType;

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
        retval = generateCallExtern(TyDouble, getDoubleForDatum, PaAddr(evaluator), PaAddr(src));
        Value *dType = generateCallExtern(TyBool, getValidityOfDoubleForDatum, PaAddr(evaluator), PaAddr(src));
        return scaff->builder.CreateICmpEQ(dType, CoBool(true), "isValidTest");
    };
    generateValidationDatum(parent, src, realTest);
    return retval;
}

Value *Compiler::generateBoolFromDatum(ASTNode *parent, Value *src)
{
    Value *retval = nullptr;
    auto boolTest = [this, &retval](Value *src) {
        retval = generateCallExtern(TyBool, getBoolForDatum, PaAddr(evaluator), PaAddr(src));
        Value *dType = generateCallExtern(TyBool, getValidityOfBoolForDatum, PaAddr(evaluator), PaAddr(src));
        return scaff->builder.CreateICmpEQ(dType, CoBool(true), "isValidTest");
    };
    generateValidationDatum(parent, src, boolTest);
    return retval;
}

Value *Compiler::generateFromDatum(Datum::DatumType t, ASTNode *parent, Value *src)
{
    auto typeTest = [this, t](Value *src) {
        Value *dType = generateGetDatumIsa(src);
        Value *mask = scaff->builder.CreateAnd(dType, CoInt32(t), "dataTypeMask");
        Value *cond = scaff->builder.CreateICmpNE(mask, CoInt32(0), "typeTest");
        return cond;
    };
    return generateValidationDatum(parent, src, typeTest);
}

Value *Compiler::generateNotNothingFromDatum(ASTNode *parent, Value *src)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();

    BasicBlock *isNothingBB = BasicBlock::Create(*scaff->theContext, "isNothing", theFunction);
    BasicBlock *notNothingBB = BasicBlock::Create(*scaff->theContext, "notNothing", theFunction);

    // isNothing?
    Value *dType = generateGetDatumIsa(src);
    Value *mask = scaff->builder.CreateAnd(dType, CoInt32(Datum::typeDataMask), "dataTypeMask");
    Value *cond = scaff->builder.CreateICmpEQ(mask, CoInt32(0), "dataTypeMaskTest");
    scaff->builder.CreateCondBr(cond, isNothingBB, notNothingBB);

    // Bad
    scaff->builder.SetInsertPoint(isNothingBB);
    Value *errWhat = src;
    Value *errObj = generateErrorNoOutput(errWhat, parent);
    scaff->builder.CreateRet(errObj);

    // Good
    scaff->builder.SetInsertPoint(notNothingBB);

    return src;
}

Value *Compiler::generateNothingFromDatum(ASTNode *parent, Value *src)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();

    BasicBlock *notNothingBB = BasicBlock::Create(*scaff->theContext, "notNothing", theFunction);
    BasicBlock *isNothingBB = BasicBlock::Create(*scaff->theContext, "isNothing", theFunction);

    // isNothing?
    Value *dType = generateGetDatumIsa(src);
    Value *mask = scaff->builder.CreateAnd(dType, CoInt32(Datum::typeDataMask), "dataTypeMask");
    Value *cond = scaff->builder.CreateICmpEQ(mask, CoInt32(0), "dataTypeMaskTest");
    scaff->builder.CreateCondBr(cond, isNothingBB, notNothingBB);

    // Bad
    scaff->builder.SetInsertPoint(notNothingBB);
    Value *errObj = generateErrorNoSay(src);
    scaff->builder.CreateRet(errObj);

    // Good
    scaff->builder.SetInsertPoint(isNothingBB);

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
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    BasicBlock *noValueBB = BasicBlock::Create(*scaff->theContext, "NoValue", theFunction);
    BasicBlock *hasValueBB = BasicBlock::Create(*scaff->theContext, "hasValue", theFunction);

    Word *varName = node.astnodeValue()->childAtIndex(0).wordValue();
    Value *nameAddr = CoAddr(varName);
    Value *retval = generateCallExtern(TyAddr, getDatumForVarname, PaAddr(nameAddr));

    Value *dType = generateGetDatumIsa(retval);
    Value *mask = scaff->builder.CreateAnd(dType, CoInt32(Datum::typeDataMask), "dataMask");
    Value *cond = scaff->builder.CreateICmpEQ(mask, CoInt32(0), "dataMaskTest");
    scaff->builder.CreateCondBr(cond, noValueBB, hasValueBB);

    scaff->builder.SetInsertPoint(noValueBB);
    Value *errObj = generateErrorNoValue(nameAddr);
    scaff->builder.CreateRet(errObj);

    scaff->builder.SetInsertPoint(hasValueBB);
    return retval;
}

Value *Compiler::genExecProcedure(const DatumPtr &node, RequestReturnType returnType)
{
    AllocaInst *paramAry = generateChildrenAlloca(node.astnodeValue(), RequestReturnDatum, "paramAry");
    Value *vAstnodeValue = CoAddr(node.astnodeValue());
    Value *vParamArySize = CoInt32(node.astnodeValue()->countOfChildren());
    return generateCallExtern(
        TyAddr, runProcedure, PaAddr(evaluator), PaAddr(vAstnodeValue), PaAddr(paramAry), PaInt32(vParamArySize));
}

Value *Compiler::generateCallList(Value *list, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, runList, PaAddr(evaluator), PaAddr(list));
}

Value *Compiler::generateWordFromDouble(Value *val)
{
    return generateCallExtern(TyAddr, getWordForDouble, PaAddr(evaluator), PaDouble(val));
}

Value *Compiler::generateWordFromBool(Value *val)
{
    return generateCallExtern(TyAddr, getWordForBool, PaAddr(evaluator), PaBool(val));
}

Value *Compiler::generateErrorSystem()
{
    Value *errObj = generateCallExtern(TyAddr, getErrorSystem, PaAddr(evaluator));
    return errObj;
}

Value *Compiler::generateErrorToplevel()
{
    Value *errObj = generateCallExtern(TyAddr, getErrorToplevel, PaAddr(evaluator));
    return errObj;
}

Value *Compiler::generateErrorNoLike(ASTNode *who, Value *what)
{
    Value *errWho = CoAddr(who->nodeName.datumValue());
    Value *errObj = generateCallExtern(TyAddr, getErrorNoLike, PaAddr(evaluator), PaAddr(errWho), PaAddr(what));
    return errObj;
}

Value *Compiler::generateErrorNoSay(Value *what)
{
    Value *errObj = generateCallExtern(TyAddr, getErrorNoSay, PaAddr(evaluator), PaAddr(what));
    return errObj;
}

Value *Compiler::generateErrorNoTest(Value *who)
{
    Value *errObj = generateCallExtern(TyAddr, getErrorNoTest, PaAddr(evaluator), PaAddr(who));
    return errObj;
}

Value *Compiler::generateErrorNoValue(Value *what)
{
    Value *errObj = generateCallExtern(TyAddr, getErrorNoValue, PaAddr(evaluator), PaAddr(what));
    return errObj;
}

Value *Compiler::generateErrorNoOutput(Value *x, ASTNode *y)
{
    Value *vY = CoAddr(y->nodeName.datumValue());
    Value *errObj = generateCallExtern(TyAddr, getErrorNoOutput, PaAddr(evaluator), PaAddr(x), PaAddr(vY));
    return errObj;
}

Value *Compiler::generateErrorNotEnoughInputs(ASTNode *x)
{
    Value *vX = CoAddr(x->nodeName.datumValue());
    Value *errObj = generateCallExtern(TyAddr, getErrorNotEnoughInputs, PaAddr(evaluator), PaAddr(vX));
    return errObj;
}

Value *Compiler::generateImmediateReturn(llvm::Value *retval)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();

    BasicBlock *bailoutBB = BasicBlock::Create(*scaff->theContext, "bailout", theFunction);
    BasicBlock *throwAwayBB = BasicBlock::Create(*scaff->theContext, "throwAway", theFunction);

    // We are going to return something at this point.
    // However, there may be code after this point that we have to compile and then ignore.
    // So we need to:
    // 1. Allow the code before this point to execute.
    // 2. Return the control operation.
    // 3. Ignore any code after the return operation.
    // To do this we allow the compiler to finish generating the code after the return operation,
    // and insert it after a test that will always fail, so the code will never be executed.
    Value *cond = scaff->builder.CreateICmpEQ(CoBool(1), CoBool(0), "fakeTest");
    scaff->builder.CreateCondBr(cond, throwAwayBB, bailoutBB);

    scaff->builder.SetInsertPoint(bailoutBB);
    scaff->builder.CreateRet(retval);

    // Any code that the compiler has remaining to generate after the return operation will
    // be placed here, and then ignored.
    scaff->builder.SetInsertPoint(throwAwayBB);
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
    Value *offset = CoInt64(lljit->getDataLayout().getPointerSize());
    AllocaInst *retval = scaff->builder.CreateAlloca(TyAddr, childCount, name);
    Value *aryPtr = retval;
    for (int i = 0; i < values.size(); ++i)
    {
        scaff->builder.CreateStore(values[i], aryPtr);
        if (i < values.size() - 1)
            aryPtr = scaff->builder.CreatePtrAdd(aryPtr, offset, name + "Incr");
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
    FunctionCallee calleeF = scaff->theModule->getOrInsertFunction(name, fType);

    Q_ASSERT(calleeF.getFunctionType()->getNumParams() == argsV.size());

    if (returnType->isVoidTy())
        return scaff->builder.CreateCall(calleeF, argsV);
    else
        return scaff->builder.CreateCall(calleeF, argsV, name + "_result");
}

AllocaInst *Compiler::generateNumberAryFromDatum(ASTNode *parent, const DatumPtr &srcPtr, int32_t size)
{
    Value *vSize = CoInt32(size);
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    Value *list = generateChild(parent, srcPtr, RequestReturnDatum);
    Value *count = generateCallExtern(TyInt32, getCountOfList, PaAddr(list));
    // There should be two doubles in the list
    BasicBlock *bailoutBB = BasicBlock::Create(*scaff->theContext, "notGood", theFunction);
    BasicBlock *continueBB = BasicBlock::Create(*scaff->theContext, "good", theFunction);

    Value *countGood = scaff->builder.CreateICmpEQ(count, vSize, "countTest");
    scaff->builder.CreateCondBr(countGood, continueBB, bailoutBB);

    scaff->builder.SetInsertPoint(bailoutBB);
    Value *errWho = CoAddr(parent->nodeName.datumValue());
    Value *errObj = generateCallExtern(TyAddr, getErrorNoLike, PaAddr(evaluator), PaAddr(errWho), PaAddr(list));
    scaff->builder.CreateRet(errObj);

    scaff->builder.SetInsertPoint(continueBB);
    AllocaInst *ary = scaff->builder.CreateAlloca(TyDouble, vSize, "ary");

    Value *isGood = generateCallExtern(TyInt32, getNumberAryFromList, PaAddr(list), PaAddr(ary));
    BasicBlock *gotPosBB = BasicBlock::Create(*scaff->theContext, "gotPos", theFunction);

    Value *countCond = scaff->builder.CreateICmpEQ(isGood, CoInt32(1), "countTest");
    scaff->builder.CreateCondBr(countCond, gotPosBB, bailoutBB);

    scaff->builder.SetInsertPoint(gotPosBB);
    return ary;
}

Value *Compiler::generateValidationDouble(ASTNode *parent, Value *src, const validatorFunction &validator)
{
    BasicBlock *srcBB = scaff->builder.GetInsertBlock();
    Function *theFunction = srcBB->getParent();

    BasicBlock *validateBB = BasicBlock::Create(*scaff->theContext, "validate", theFunction);
    BasicBlock *convertBB = BasicBlock::Create(*scaff->theContext, "convert", theFunction);
    BasicBlock *erractBB = BasicBlock::Create(*scaff->theContext, "errorAction", theFunction);
    BasicBlock *bailoutBB = BasicBlock::Create(*scaff->theContext, "bailout", theFunction);
    BasicBlock *acceptBB = BasicBlock::Create(*scaff->theContext, "accept", theFunction);

    scaff->builder.CreateBr(validateBB);

    // Validate the number.
    scaff->builder.SetInsertPoint(validateBB);
    PHINode *candidate = scaff->builder.CreatePHI(TyDouble, 2, "candidate");
    candidate->addIncoming(src, srcBB);
    Value *isValidCond = validator(candidate);
    scaff->builder.CreateCondBr(isValidCond, acceptBB, erractBB);

    // The number is bad. Call handleBadDouble and maybe retry with the result.
    scaff->builder.SetInsertPoint(erractBB);
    Value *handlerResult = generateCallExtern(TyAddr, handleBadDouble, PaAddr(evaluator), PaAddr(CoAddr(parent)), PaDouble(candidate));
    Value *datamIsa = generateGetDatumIsa(handlerResult); // See if result is a datum.
    Value *isDatumMasked = scaff->builder.CreateAnd(datamIsa, CoInt32(Datum::typeDataMask), "isDatumMasked");
    Value *isDatumCond = scaff->builder.CreateICmpNE(isDatumMasked, CoInt32(0), "isDatumCond");
    scaff->builder.CreateCondBr(isDatumCond, convertBB, bailoutBB);

    // A Word was returned. Convert it to a double and try validating it again.
    scaff->builder.SetInsertPoint(convertBB);
    Value *dVal = generateCallExtern(TyDouble, getDoubleForDatum, PaAddr(evaluator), PaAddr(handlerResult));
    candidate->addIncoming(dVal, convertBB);
    scaff->builder.CreateBr(validateBB);

    // The number is bad, and ERRACT is not set. Return a DOESN'T LIKE error.
    scaff->builder.SetInsertPoint(bailoutBB);
    scaff->builder.CreateRet(handlerResult);

    // The number is good. Continue.
    scaff->builder.SetInsertPoint(acceptBB);
    return candidate;
}

Value *Compiler::generateValidationDatum(ASTNode *parent, Value *src, const validatorFunction &validator)
{
    BasicBlock *srcBB = scaff->builder.GetInsertBlock();
    Function *theFunction = srcBB->getParent();

    BasicBlock *validateBB = BasicBlock::Create(*scaff->theContext, "validate", theFunction);
    BasicBlock *erractBB = BasicBlock::Create(*scaff->theContext, "errorAction", theFunction);
    BasicBlock *bailoutBB = BasicBlock::Create(*scaff->theContext, "bailout", theFunction);
    BasicBlock *acceptBB = BasicBlock::Create(*scaff->theContext, "accept", theFunction);

    scaff->builder.CreateBr(validateBB);

    // Validate the datum.
    scaff->builder.SetInsertPoint(validateBB);
    PHINode *candidate = scaff->builder.CreatePHI(TyAddr, 2, "candidate");
    candidate->addIncoming(src, srcBB);
    Value *cond = validator(candidate);
    scaff->builder.CreateCondBr(cond, acceptBB, erractBB);

    // The datum is bad. Call handleBadDatum and maybe retry with the result.
    scaff->builder.SetInsertPoint(erractBB);
    Value *handlerResult = generateCallExtern(TyAddr, handleBadDatum, PaAddr(evaluator), PaAddr(CoAddr(parent)), PaAddr(candidate));
    Value *datamIsa = generateGetDatumIsa(handlerResult); // See if result is a datum.
    Value *isDatumMasked = scaff->builder.CreateAnd(datamIsa, CoInt32(Datum::typeDataMask), "isDatumMasked");
    Value *isDatumCond = scaff->builder.CreateICmpNE(isDatumMasked, CoInt32(0), "isDatumCond");
    candidate->addIncoming(handlerResult, erractBB);
    scaff->builder.CreateCondBr(isDatumCond, validateBB, bailoutBB);

    // The datum is bad, and ERRACT is not set. Return a DOESN'T LIKE error.
    scaff->builder.SetInsertPoint(bailoutBB);
    scaff->builder.CreateRet(handlerResult);

    // The datum is good. Continue.
    scaff->builder.SetInsertPoint(acceptBB);
    return candidate;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"

// Generate code to return a datum type (isa) of a given object.
Value *Compiler::generateGetDatumIsa(Value *objAddr)
{
    const unsigned int isaOffset = offsetof(Datum, isa);
    Value *isaAddr = scaff->builder.CreatePtrAdd(objAddr, CoInt64(isaOffset), "isaAddr");

    Value *dType = scaff->builder.CreateLoad(TyInt32, isaAddr, "isaLoad");
    return dType;
}

#pragma GCC diagnostic pop
