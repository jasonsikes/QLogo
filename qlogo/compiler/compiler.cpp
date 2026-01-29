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
#include "datum_types.h"
#include "workspace/exports.h"
#include "workspace/kernel.h"
#include "sharedconstants.h"
#include "treeifyer.h"
#include "workspace/callframe.h"
#include "workspace/procedures.h"
#include <iostream>

QHash<Datum *, std::shared_ptr<CompiledText>> Compiler::compiledTextTable;

using namespace llvm;
using namespace llvm::orc;

CompilerContext::CompilerContext(std::unique_ptr<llvm::orc::ExecutionSession> es,
                                 llvm::orc::JITTargetMachineBuilder jtmb,
                                 const llvm::DataLayout &dl)
    : exeSession(std::move(es)), dataLayout(dl), mangler(*this->exeSession, this->dataLayout),
      objectLayer(*this->exeSession, []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
      compileLayer(*this->exeSession,
                   objectLayer,
                   std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(jtmb))),
      jitLib(this->exeSession->createBareJITDylib("<main>"))
{
    jitLib.addGenerator(cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(dl.getGlobalPrefix())));
    // Note: jtmb was moved in the initializer list, so we need to check the target triple
    // from the ExecutionSession instead
    if (exeSession->getExecutorProcessControl().getTargetTriple().isOSBinFormatCOFF())
    {
        objectLayer.setOverrideObjectFlagsWithResponsibilityFlags(true);
        objectLayer.setAutoClaimResponsibilityForObjectSymbols(true);
    }
}

CompilerContext::~CompilerContext() // NOLINT(modernize-use-equals-default) - not trivial, needs to call endSession()
{
    if (auto err = exeSession->endSession())
        exeSession->reportError(std::move(err));
}

CompilerContext *CompilerContext::Create()
{
    auto epc = llvm::orc::SelfExecutorProcessControl::Create();
    Q_ASSERT(epc);

    auto es = std::make_unique<llvm::orc::ExecutionSession>(std::move(*epc));

    llvm::orc::JITTargetMachineBuilder jtmb(es->getExecutorProcessControl().getTargetTriple());

    auto dl = jtmb.getDefaultDataLayoutForTarget();
    Q_ASSERT(dl);

    return new CompilerContext(std::move(es), std::move(jtmb), *dl);
}

const llvm::DataLayout &CompilerContext::getDataLayout() const
{
    return dataLayout;
}

llvm::orc::JITDylib &CompilerContext::getMainJITDylib()
{
    return jitLib;
}

llvm::Error CompilerContext::addModule(llvm::orc::ThreadSafeModule tsm, llvm::orc::ResourceTrackerSP rt)
{
    if (!rt)
        rt = jitLib.getDefaultResourceTracker();
    return compileLayer.add(rt, std::move(tsm));
}

llvm::Expected<llvm::orc::ExecutorSymbolDef> CompilerContext::lookup(llvm::StringRef name)
{
    return exeSession->lookup({&jitLib}, mangler(name.str()));
}

Scaffold::Scaffold(void *parent, const llvm::DataLayout &dataLayout)
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

    // The name of the function is the address of the parent node.
    name = std::to_string(reinterpret_cast<uint64_t>(parent));
}

CompiledText::~CompiledText()
{
    // Only remove resource tracker if context is still valid
    // (it may have been destroyed if CompiledText outlives the Compiler singleton)
    if (context != nullptr && rt)
    {
        context->exitOnErr(rt->remove());
    }
}

Compiler::Compiler()
{
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    context_ = std::unique_ptr<CompilerContext>(CompilerContext::Create());
}

Compiler::~Compiler()
{
    // Clear compiledTextTable first to ensure all CompiledText objects are destroyed
    // while context_ is still valid
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

    // The Tag-to-Block Table of Contents.
    QList<BasicBlock *> tocEntries;

    // Since the TOC is prepended, insert them at the start of the function in reverse order.
    BasicBlock *previousBlock = blocks[0];
    for (int i = 1; i < blocks.size(); ++i)
    {
        BasicBlock *tocBlock = BasicBlock::Create(*scaff->theContext, "Toc", theFunction, previousBlock);
        tocEntries.prepend(tocBlock);
        previousBlock = tocBlock;
    }

    for (int i = 0; i < tocEntries.size(); ++i)
    {
        BasicBlock *tocEntry = tocEntries[i];
        uint32_t tagId = i + 1;
        BasicBlock *destBlock = blocks[tagId];
        BasicBlock *nextBlock = (tagId == blocks.size() - 1) ? blocks[0] : tocEntries[tagId];

        scaff->builder.SetInsertPoint(tocEntry);
        Value *tagIdValue = CoInt32(tagId);

        Value *cond = scaff->builder.CreateICmpEQ(blockId, tagIdValue, "tocCond");
        scaff->builder.CreateCondBr(cond, destBlock, nextBlock);
    }
    return tocEntries[0];
}

CompiledFunctionPtr Compiler::generateFunctionPtrFromASTList(QList<QList<DatumPtr>> parsedList, Datum *key)
{
    Scaffold compilerScaffolding(reinterpret_cast<void *>(key), context_->getDataLayout());
    scaff = &compilerScaffolding;

    auto *compiledText = new CompiledText();
    compiledText->astList = parsedList;
    compiledText->context = context_.get();
    compiledTextTable[key] = std::shared_ptr<CompiledText>(compiledText);

    // Generate the prototype and add it to the module.
    // Param1: pointer to the Evaluator object.
    // Param2: ID of the block to begin execution at.
    std::vector<Type *> paramAry = {TyAddr, TyInt32};

    // Returning an int64* type, indicates pointer to a Datum.
    FunctionType *ft = FunctionType::get(TyAddr, paramAry, false);
    Function *theFunction = Function::Create(ft, Function::ExternalLinkage, scaff->name, *scaff->theModule);

    // Set the name for the evaluator argument
    evaluator = theFunction->getArg(0);
    evaluator->setName("evaluator");

    // Set the name for the block ID argument
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

    BasicBlock *currentBlock = BasicBlock::Create(*scaff->theContext, "Block", theFunction);
    QList<BasicBlock *> blocks = {currentBlock};
    scaff->builder.SetInsertPoint(currentBlock);

    Value *nodeResult;
    RequestReturnType returnTypeRequest = RequestReturnNothing;
    for (auto &srcBlock : parsedList)
    {
        if (isTag(srcBlock.first()))
        {
            ++localBlockId;
            BasicBlock *newBlock = BasicBlock::Create(*scaff->theContext, "Block", theFunction);
            blocks.append(newBlock);
            scaff->builder.SetInsertPoint(currentBlock);
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

    // Run the optimizer on the function.
    scaff->theFPM.run(*theFunction, scaff->theFAM);

    std::string str;
    llvm::raw_string_ostream output(str);
    // Validate the generated code, checking for consistency.
    if (Config::get().verifyIR && verifyFunction(*theFunction, &output))
    {
        std::cout << str << "\n";
        Q_ASSERT(false);
    }

    if (Config::get().showIR)
    {
        theFunction->print(errs());
        fprintf(stderr, "\n");
    }

    if (Config::get().showCFG)
    {
        theFunction->viewCFG();
    }

    // Create a ResourceTracker to track JIT'd memory allocated to our
    // anonymous expression -- that way we can free it with the source list.
    compiledText->rt = context_->getMainJITDylib().createResourceTracker();
    auto tsm = ThreadSafeModule(std::move(scaff->theModule), std::move(scaff->theContext));
    context_->exitOnErr(context_->addModule(std::move(tsm), compiledText->rt));

    // Search the JIT for the expression by name.
    ExecutorSymbolDef exprSymbol = context_->exitOnErr(context_->lookup(scaff->name));

    // Get the symbol's address and cast it to the right type so we can
    // call it as a native function.
    compiledText->functionPtr = reinterpret_cast<CompiledFunctionPtr>(exprSymbol.getAddress().getValue());
    return compiledText->functionPtr;
}

CompiledFunctionPtr Compiler::functionPtrFromList(List *aList)
{
    if (aList->compileTimeStamp <= Procedures::get().timeOfLastProcedureCreation())
    {
        QList<QList<DatumPtr>> parsedList = Treeifier::astFromList(aList);
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

    // If the source and destination are the same type, no casting necessary.
    if (srcReturnType == destReturnType)
        return src;

    // If src is a number(REAL)
    if (src->getType()->isDoubleTy())
    {
        if (destReturnType & RequestReturnReal)
            return src;
        Value *srcAsWord = generateWordFromDouble(src);
        if (destReturnType & RequestReturnDatum)
            return srcAsWord;
        if (destReturnType & (RequestReturnBool))
            return generateImmediateReturn(generateErrorNoLike(parent, srcAsWord));
        if (destReturnType & RequestReturnNothing)
            return generateImmediateReturn(generateErrorNoSay(srcAsWord));
        Q_ASSERT(false);
    }

    // If src is a boolean
    if (src->getType()->isIntegerTy(1))
    {
        if (destReturnType & RequestReturnBool)
            return src;
        Value *srcAsWord = generateWordFromBool(src);
        if (destReturnType & RequestReturnDatum)
            return srcAsWord;
        if (destReturnType & (RequestReturnReal))
            return generateImmediateReturn(generateErrorNoLike(parent, srcAsWord));
        if (destReturnType & RequestReturnNothing)
            return generateImmediateReturn(generateErrorNoSay(srcAsWord));
        Q_ASSERT(false);
    }

    // If src is a Datum
    if (srcReturnType == RequestReturnDatum)
    {
        if (destReturnType & RequestReturnDatum)
            return src;
        if (destReturnType & RequestReturnBool)
            return generateBoolFromDatum(parent, src);
        if (destReturnType & RequestReturnReal)
            return generateDoubleFromDatum(parent, src);
        if (destReturnType & RequestReturnNothing)
            return generateImmediateReturn(generateErrorNoSay(src));
        Q_ASSERT(false);
    }

    if (srcReturnType == RequestReturnNothing)
    {
        if (destReturnType & RequestReturnNothing)
            return src;

        return generateImmediateReturn(generateErrorNoOutput(src, parent));
    }

    if (srcReturnType == RequestReturnDN)
    {
        if (destReturnType == RequestReturnNothing)
            return generateNothingFromDatum(parent, src);
        if (destReturnType == RequestReturnDatum)
            return generateNotNothingFromDatum(parent, src);
        if (destReturnType == RequestReturnReal)
            return generateDoubleFromDatum(parent, src);
        if (destReturnType == RequestReturnBool)
            return generateBoolFromDatum(parent, src);
        Q_ASSERT(false);
    }

    Q_ASSERT(false);
    return nullptr;
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
    if (returnType == RequestReturnReal)
    {
        if (literalPtr.isWord())
        {
            Word *wVal = literalPtr.wordValue();
            double val = wVal->numberValue();
            if (wVal->numberIsValid)
                return CoDouble(val);
        }
        return CoAddr(literalPtr.datumValue());
    }
    if (returnType == RequestReturnBool)
    {
        if (literalPtr.isWord())
        {
            Word *wVal = literalPtr.wordValue();
            bool val = wVal->boolValue();
            if (wVal->boolIsValid)
                return CoBool(val);
        }
        return CoAddr(literalPtr.datumValue());
    }

    // If it's not Bool or Real then it must be a Datum or Nothing.
    Q_ASSERT ((returnType & RequestReturnDN) != 0);
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

    // We are generating an immediate return value. Even though the code afterward will never be executed,
    // we need to allow the compiler to finish generating the code.
    BasicBlock *bailoutBB = BasicBlock::Create(*scaff->theContext, "bailout", theFunction);
    BasicBlock *throwAwayBB = BasicBlock::Create(*scaff->theContext, "throw_away", theFunction);
    Value *cond = scaff->builder.CreateICmpEQ(CoInt64(1), CoInt64(0), "fakeTest");
    scaff->builder.CreateCondBr(cond, throwAwayBB, bailoutBB);

    scaff->builder.SetInsertPoint(bailoutBB);
    scaff->builder.CreateRet(retval);

    // This is never reached.
    scaff->builder.SetInsertPoint(throwAwayBB);
    return retval;
}

std::vector<Value *> Compiler::generateChildren(ASTNode *node, RequestReturnType returnType)
{
    std::vector<Value *> retval;
    retval.reserve(node->countOfChildren());
    for (int i = 0; i < node->countOfChildren(); ++i)
    {
        retval.push_back(generateChildOfNode(node, node->childAtIndex(i), returnType));
    }
    for (int i = 0; i < node->countOfChildren(); ++i)
    {
        retval[i] = generateCast(retval[i], node, node->childAtIndex(i), returnType);
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
    Value *offset = CoInt64(sizeof(addr_t));
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
    BasicBlock *doubleCheckBB = BasicBlock::Create(*scaff->theContext, "doubleCheck", theFunction);
    BasicBlock *notDatumBB = BasicBlock::Create(*scaff->theContext, "notDatum", theFunction);
    BasicBlock *bailoutBB = BasicBlock::Create(*scaff->theContext, "bailout", theFunction);
    BasicBlock *acceptBB = BasicBlock::Create(*scaff->theContext, "accept", theFunction);

    scaff->builder.CreateBr(validateBB);

    // Validate the number.
    scaff->builder.SetInsertPoint(validateBB);
    PHINode *candidate = scaff->builder.CreatePHI(TyDouble, 2, "candidate");
    candidate->addIncoming(src, srcBB);
    Value *isValidCond = validator(candidate);
    scaff->builder.CreateCondBr(isValidCond, acceptBB, convertBB);

    // The number is bad. Convert it to a word, and then decide what to do with it.
    scaff->builder.SetInsertPoint(convertBB);
    Value *badWord = generateWordFromDouble(candidate);
    Value *varErroract = generateCallExtern(TyBool, getvarErroract);
    Value *isTrueCond = scaff->builder.CreateICmpEQ(varErroract, CoBool(true), "isTrueCond");
    scaff->builder.CreateCondBr(isTrueCond, erractBB, bailoutBB);

    // Perform ERRACT (PAUSE)
    scaff->builder.SetInsertPoint(erractBB);
    PHINode *badValue = scaff->builder.CreatePHI(TyAddr, 3, "badValue");
    badValue->addIncoming(badWord, convertBB);
    Value *errmsg = generateErrorNoLike(parent, badValue);
    generateCallExtern(TyAddr, stdWriteDatum, PaAddr(errmsg), PaBool(CoBool(true)));
    Value *newCandidate = generateCallExtern(TyAddr, callPause, PaAddr(evaluator));
    Value *datamIsa = generateGetDatumIsa(newCandidate);
    Value *isDatumMasked = scaff->builder.CreateAnd(datamIsa, CoInt32(Datum::typeDataMask), "isDatumMasked");
    Value *isDatumCond = scaff->builder.CreateICmpNE(isDatumMasked, CoInt32(0), "isDatumCond");
    scaff->builder.CreateCondBr(isDatumCond, doubleCheckBB, notDatumBB);

    // Check if the new candidate is a double.
    scaff->builder.SetInsertPoint(doubleCheckBB);
    Value *dVal = generateCallExtern(TyDouble, getDoubleForDatum, PaAddr(evaluator), PaAddr(newCandidate));
    badValue->addIncoming(newCandidate, doubleCheckBB);
    candidate->addIncoming(dVal, doubleCheckBB);
    Value *dType = generateCallExtern(TyBool, getValidityOfDoubleForDatum, PaAddr(evaluator), PaAddr(newCandidate));
    Value *isDoubleCond = scaff->builder.CreateICmpEQ(dType, CoBool(true), "isDoubleCond");
    scaff->builder.CreateCondBr(isDoubleCond, validateBB, erractBB);

    // The new candidate is not a datum. Return it.
    scaff->builder.SetInsertPoint(notDatumBB);
    Value *toplevelErrorObj = generateErrorToplevel();
    scaff->builder.CreateRet(toplevelErrorObj);

    // The number is bad, and ERRACT is not set. Return an error.
    scaff->builder.SetInsertPoint(bailoutBB);
    Value *errObj = generateErrorNoLike(parent, badWord);
    scaff->builder.CreateRet(errObj);

    // The number is good. Run with it.
    scaff->builder.SetInsertPoint(acceptBB);
    return candidate;
}

Value *Compiler::generateValidationDatum(ASTNode *parent, Value *src, const validatorFunction &validator)
{
    BasicBlock *srcBB = scaff->builder.GetInsertBlock();
    Function *theFunction = srcBB->getParent();

    BasicBlock *tryBB = BasicBlock::Create(*scaff->theContext, "try", theFunction);
    BasicBlock *isNotValidBB = BasicBlock::Create(*scaff->theContext, "notValid", theFunction);
    BasicBlock *errorActionBB = BasicBlock::Create(*scaff->theContext, "errorAction", theFunction);
    BasicBlock *notDatumBB = BasicBlock::Create(*scaff->theContext, "notDatum", theFunction);
    BasicBlock *bailoutBB = BasicBlock::Create(*scaff->theContext, "bailout", theFunction);
    BasicBlock *acceptBB = BasicBlock::Create(*scaff->theContext, "accept", theFunction);

    scaff->builder.CreateBr(tryBB);

    scaff->builder.SetInsertPoint(tryBB);
    PHINode *candidate = scaff->builder.CreatePHI(TyAddr, 2, "candidate");
    candidate->addIncoming(src, srcBB);
    Value *cond = validator(candidate);
    scaff->builder.CreateCondBr(cond, acceptBB, isNotValidBB);

    scaff->builder.SetInsertPoint(isNotValidBB);
    Value *varErroract = generateCallExtern(TyBool, getvarErroract);
    Value *isTrue = scaff->builder.CreateICmpEQ(varErroract, CoBool(true), "isTrue");
    scaff->builder.CreateCondBr(isTrue, errorActionBB, bailoutBB);

    scaff->builder.SetInsertPoint(errorActionBB);
    Value *errmsg = generateErrorNoLike(parent, src);
    generateCallExtern(TyAddr, stdWriteDatum, PaAddr(errmsg), PaBool(CoBool(true)));
    Value *newCandidate = generateCallExtern(TyAddr, callPause, PaAddr(evaluator));
    Value *datamIsa = generateGetDatumIsa(newCandidate);
    Value *isDatumMasked = scaff->builder.CreateAnd(datamIsa, CoInt32(Datum::typeDataMask), "isDatumMasked");
    Value *isDatum = scaff->builder.CreateICmpNE(isDatumMasked, CoInt32(0), "isDatum");
    candidate->addIncoming(newCandidate, errorActionBB);
    scaff->builder.CreateCondBr(isDatum, tryBB, notDatumBB);

    scaff->builder.SetInsertPoint(notDatumBB);
    scaff->builder.CreateRet(newCandidate);

    scaff->builder.SetInsertPoint(bailoutBB);
    Value *errObj = generateErrorNoLike(parent, src);
    scaff->builder.CreateRet(errObj);

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
