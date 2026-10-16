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
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/ExecutionEngine/Orc/AbsoluteSymbols.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/Mangling.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/Error.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Coroutines/ABI.h"
#include "llvm/Transforms/Coroutines/CoroCleanup.h"
#include "llvm/Transforms/Coroutines/CoroEarly.h"
#include "llvm/Transforms/Coroutines/CoroElide.h"
#include "llvm/Transforms/Coroutines/CoroSplit.h"
#include "llvm/Transforms/Coroutines/MaterializationUtils.h"
#include "flowcontrol.h"
#include "datum_types.h"
#include "workspace/exports.inc"
#include "op_strings.h"
#include "sharedconstants.h"
#include "treeifyer.h"
#include "workspace/callframe.h"
#include "workspace/kernel.h"
#include <QDateTime>
#include "workspace/procedures.h"
#include <algorithm>
#include <string>
#include <iostream>
#include <vector>

std::vector<std::weak_ptr<CompiledText>> Compiler::liveCompiledTexts_;

std::string dbgName(const char *enclosing, const char *name)
{
    const Scaffold *scaff = Compiler::get().scaff_;
    if (scaff != nullptr && scaff->mangleNames_)
        return std::string(enclosing) + "_" + name;
    return name;
}

using namespace llvm;
using namespace llvm::orc;

static_assert(sizeof(Datum::DatumType) == 4);
static_assert(sizeof(int) == 4);
static_assert(sizeof(bool) == 1);

namespace
{
[[noreturn]] void throwFatalJitError(Error err, const char *what)
{
    std::string detail = toString(std::move(err));
    std::cerr << "JIT error (" << what << "): " << detail << '\n';
    throw FCError::fatalInternal();
}

void checkJitError(Error err, const char *what)
{
    if (err)
        throwFatalJitError(std::move(err), what);
}

template <typename T>
T unwrapOrThrow(Expected<T> value, const char *what)
{
    if (!value)
        throwFatalJitError(value.takeError(), what);
    return std::move(*value);
}

template <typename Ret, typename... Args>
FunctionType *hostExportType(Ret (*)(Args...), LLVMContext &ctx)
{
    return FunctionType::get(CTypeToLLVM<Ret>::get(ctx), {CTypeToLLVM<Args>::get(ctx)...}, false);
}

struct HostExport
{
    const char *name;
    FunctionType *(*getType)(LLVMContext &);
};

const HostExport hostExports[] = {
#define JIT_EXPORT(ret, name, params) {#name, [](LLVMContext &c) { return hostExportType(name, c); }},
#include "workspace/exports.inc"
#undef JIT_EXPORT
};

Error defineHostExports(LLJIT &jit)
{
    MangleAndInterner mangle(jit.getExecutionSession(), jit.getDataLayout());
    SymbolMap symbols;
#define JIT_EXPORT(ret, name, params) \
    symbols[mangle(#name)] = ExecutorSymbolDef::fromPtr(name, JITSymbolFlags::Exported);
#include "workspace/exports.inc"
#undef JIT_EXPORT
    return jit.getMainJITDylib().define(absoluteSymbols(std::move(symbols)));
}

std::unique_ptr<LLJIT> createLLJIT(JITTargetMachineBuilder jtmb)
{
    auto jitOrErr = llvm::orc::LLJITBuilder()
                        .setJITTargetMachineBuilder(std::move(jtmb))
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
    auto jit = unwrapOrThrow(std::move(jitOrErr), "LLJITBuilder::create");
    checkJitError(defineHostExports(*jit), "defineHostExports");
    return jit;
}

/// Creates a resource tracker, adds the module to the JIT, and looks up the symbol.
/// Returns (symbol address, resource tracker for later removal).
std::pair<uint64_t, ResourceTrackerSP> addModuleAndLookup(LLJIT &jit, ThreadSafeModule tsm, StringRef name)
{
    auto rt = jit.getMainJITDylib().createResourceTracker();
    checkJitError(jit.addIRModule(rt, std::move(tsm)), "addIRModule");
    auto symOrErr = jit.lookup(name);
    if (!symOrErr)
    {
        consumeError(rt->remove());
        throwFatalJitError(symOrErr.takeError(), "lookup");
    }
    return {symOrErr->getValue(), std::move(rt)};
}

void addDefaultPasses(FunctionPassManager &fpm)
{
    fpm.addPass(SROAPass(SROAOptions::ModifyCFG));
    fpm.addPass(EarlyCSEPass());
    fpm.addPass(InstCombinePass());
    fpm.addPass(SimplifyCFGPass());
}

void runFunctionPasses(Module &M, FunctionPassManager &fpm, FunctionAnalysisManager &fam)
{
    for (Function &F : M)
    {
        if (F.isDeclaration())
            continue;
        fpm.run(F, fam);
    }
}

void registerPassAnalyses(PassBuilder &pb, PassInstrumentationCallbacks &pic,
                          ModuleAnalysisManager &mam, FunctionAnalysisManager &fam,
                          LoopAnalysisManager &lam, CGSCCAnalysisManager &cgam,
                          StandardInstrumentations &si)
{
    si.registerCallbacks(pic, &mam);
    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);
}

/// Clones are invoked through C function pointers (LLVMCoroFrameHeader).
/// Match that ABI, and copy uwtable / nounwind from the host EXPORTC decls.
void applyCAbiToCoroClone(Module &M, Function &F)
{
    F.setCallingConv(CallingConv::C);

    bool sawExport = false;
    bool allNoUnwind = true;
    UWTableKind exportUW = UWTableKind::None;
    for (Function &E : M)
    {
        if (!E.isDeclaration() || E.isIntrinsic())
            continue;
        sawExport = true;
        if (E.hasUWTable())
            exportUW = E.getUWTableKind();
        if (!E.doesNotThrow())
            allNoUnwind = false;
    }
    F.setUWTableKind(exportUW != UWTableKind::None ? exportUW : UWTableKind::Default);
    if (sawExport && allNoUnwind)
        F.setDoesNotThrow();

    for (User *U : F.users())
    {
        auto *CB = dyn_cast<CallBase>(U);
        if (CB && CB->getCalledFunction() == &F)
            CB->setCallingConv(CallingConv::C);
    }
}

/// coro-split emits switch clones as fastcc. Patch the official Clones list.
struct CCallingConvSwitchABI : coro::SwitchABI
{
    using SwitchABI::SwitchABI;

    void splitCoroutine(Function &F, coro::Shape &Shape, SmallVectorImpl<Function *> &Clones,
                        TargetTransformInfo &TTI) override
    {
        SwitchABI::splitCoroutine(F, Shape, Clones, TTI);
        for (Function *Clone : Clones)
            applyCAbiToCoroClone(*F.getParent(), *Clone);
    }
};

/// Run the coroutine lowering pipeline so that llvm.coro.* intrinsics are
/// lowered before the module is sent to the backend.
void runCoroutinePasses(Module &M, ModuleAnalysisManager &mam)
{
    CoroSplitPass splitPass(/*OptimizeFrame=*/false);
    splitPass.CreateAndInitABI = [](Function &F, coro::Shape &S) {
        auto ABI = std::make_unique<CCallingConvSwitchABI>(F, S, coro::isTriviallyMaterializable);
        ABI->init();
        return ABI;
    };

    CGSCCPassManager cgscc;
    cgscc.addPass(std::move(splitPass));

    FunctionPassManager elideFPM;
    elideFPM.addPass(CoroElidePass());

    ModulePassManager coroMPM;
    coroMPM.addPass(CoroEarlyPass());
    coroMPM.addPass(createModuleToPostOrderCGSCCPassAdaptor(std::move(cgscc)));
    coroMPM.addPass(createModuleToFunctionPassAdaptor(std::move(elideFPM)));
    coroMPM.addPass(CoroCleanupPass());
    coroMPM.run(M, mam);
}

void verifyModuleOrThrow(Module &M)
{
    std::string str;
    raw_string_ostream output(str);
    if (verifyModule(M, &output))
    {
        std::cerr << "IR verification failed:\n" << str << "\n";
        throw FCError::fatalInternal();
    }
}
} // namespace

FunctionType *hostExportFunctionType(LLVMContext &ctx, StringRef name)
{
    for (const auto &exp : hostExports)
    {
        if (name == exp.name)
            return exp.getType(ctx);
    }
    std::cerr << "JIT error (host export): unknown function '" << name.str() << "'\n";
    throw FCError::fatalInternal();
}

Scaffold::Scaffold(const llvm::DataLayout &dataLayout, TargetMachine *tm)
    : theContext_(std::make_unique<LLVMContext>()), theModule_(std::make_unique<Module>("QLogoJIT", *theContext_)),
      builder_(IRBuilder<>(*theContext_)), theFPM_(FunctionPassManager()), theLAM_(LoopAnalysisManager()),
      theFAM_(FunctionAnalysisManager()), theCGAM_(CGSCCAnalysisManager()), theMAM_(ModuleAnalysisManager()),
      thePIC_(PassInstrumentationCallbacks()),
      theSI_(StandardInstrumentations(*theContext_, Compiler::get().dumpRequest().any())),
      thePB_(tm, PipelineTuningOptions(), std::nullopt, &thePIC_)
{
    theModule_->setDataLayout(dataLayout);
    if (tm)
        theModule_->setTargetTriple(tm->getTargetTriple());

    // Itanium layout of Datum: vptr, isa_ (u32), retainCount_ (int), alertOnDelete_ (bool).
    datumHeaderTy_ = StructType::create(
        *theContext_,
        {PointerType::get(*theContext_, 0), Type::getInt32Ty(*theContext_), Type::getInt32Ty(*theContext_),
         Type::getInt8Ty(*theContext_)},
        "DatumHeader");
    {
        const StructLayout *sl = theModule_->getDataLayout().getStructLayout(datumHeaderTy_);
        const Datum *probe = Datum::notADatum();
        const char *base = reinterpret_cast<const char *>(probe);
        Q_ASSERT(sl->getElementOffset(1) ==
                 static_cast<uint64_t>(reinterpret_cast<const char *>(&probe->isa_) - base));
        Q_ASSERT(sl->getElementOffset(2) ==
                 static_cast<uint64_t>(reinterpret_cast<const char *>(&probe->retainCount_) - base));
        Q_ASSERT(sl->getElementOffset(3) ==
                 static_cast<uint64_t>(reinterpret_cast<const char *>(&probe->alertOnDelete_) - base));
    }
    MDBuilder tbaaMdb(*theContext_);
    MDNode *tbaaRoot = tbaaMdb.createTBAARoot("qlogo");
    MDNode *isaTbaaTy =
        tbaaMdb.createTBAATypeNode(tbaaRoot, 4, MDString::get(*theContext_, "datum.isa"));
    isaLoadTbaa_ = tbaaMdb.createTBAAAccessTag(isaTbaaTy, isaTbaaTy, 0, 4, /*IsImmutable=*/true);

    addDefaultPasses(theFPM_);
    registerPassAnalyses(thePB_, thePIC_, theMAM_, theFAM_, theLAM_, theCGAM_, theSI_);

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
    return BasicBlock::Create(*theContext_, name, theFunction_, mainBailoutBB_);
}

AllocaInst *Scaffold::createEntryBlockAlloca(Type *type, Value *arraySize, const std::string &name)
{
    BasicBlock &entry = theFunction_->getEntryBlock();
    IRBuilder<> tmp(&entry, entry.begin());
    return tmp.CreateAlloca(type, arraySize, name);
}

void Scaffold::hoistAllocasToEntry()
{
    BasicBlock &entry = theFunction_->getEntryBlock();
    std::vector<AllocaInst *> allocas;
    for (BasicBlock &bb : *theFunction_)
    {
        for (Instruction &inst : bb)
        {
            if (auto *ai = dyn_cast<AllocaInst>(&inst))
                allocas.push_back(ai);
        }
    }

    Instruction *insertBefore = &*entry.getFirstInsertionPt();
    while (insertBefore && isa<AllocaInst>(insertBefore))
        insertBefore = insertBefore->getNextNode();
    Q_ASSERT(insertBefore != nullptr);

    for (AllocaInst *ai : allocas)
    {
        if (ai->getParent() == &entry)
            continue;
        ai->moveBefore(insertBefore->getIterator());
    }
}

void Scaffold::finalizeColdPaths()
{
    SmallPtrSet<BasicBlock *, 16> cold;
    for (BasicBlock *bb : coldPathBlocks_)
        cold.insert(bb);

    MDBuilder mdb(*theContext_);
    SmallPtrSet<Instruction *, 16> seen;
    constexpr uint32_t hotWeight = 1u << 20;
    constexpr uint32_t coldWeight = 1u;

    for (BasicBlock *coldBB : coldPathBlocks_)
    {
        for (BasicBlock *pred : predecessors(coldBB))
        {
            Instruction *term = pred->getTerminator();
            if (!seen.insert(term).second)
                continue;

            if (auto *br = dyn_cast<BranchInst>(term))
            {
                if (!br->isConditional())
                    continue;
                const bool trueCold = cold.contains(br->getSuccessor(0));
                const bool falseCold = cold.contains(br->getSuccessor(1));
                if (trueCold == falseCold)
                    continue;
                br->setMetadata(LLVMContext::MD_prof,
                                trueCold ? mdb.createUnlikelyBranchWeights()
                                         : mdb.createLikelyBranchWeights());
            }
            else if (auto *sw = dyn_cast<SwitchInst>(term))
            {
                SmallVector<uint32_t, 8> weights;
                weights.reserve(sw->getNumSuccessors());
                bool anyCold = false;
                bool anyHot = false;
                for (unsigned i = 0, n = sw->getNumSuccessors(); i < n; ++i)
                {
                    const bool isCold = cold.contains(sw->getSuccessor(i));
                    anyCold |= isCold;
                    anyHot |= !isCold;
                    weights.push_back(isCold ? coldWeight : hotWeight);
                }
                if (anyCold && anyHot)
                    sw->setMetadata(LLVMContext::MD_prof,
                                    mdb.createBranchWeights(weights, /*IsExpected=*/true));
            }
        }

        for (Instruction &inst : *coldBB)
        {
            auto *cb = dyn_cast<CallBase>(&inst);
            if (cb && cb->getIntrinsicID() == Intrinsic::not_intrinsic)
                cb->addFnAttr(Attribute::Cold);
        }
    }

    for (BasicBlock *block : coldPathBlocks_)
        block->moveAfter(&theFunction_->back());
}


CompiledText::~CompiledText()
{
    // Only remove resource tracker if compiler is still valid
    // (it may have been destroyed if CompiledText outlives the Compiler singleton)
    if (compiler_ != nullptr && rt_)
    {
        consumeError(rt_->remove());
    }
}

Compiler::Compiler()
{
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    auto jtmb = unwrapOrThrow(JITTargetMachineBuilder::detectHost(), "detectHost");
    targetMachine_ = unwrapOrThrow(jtmb.createTargetMachine(), "createTargetMachine");
    lljit_ = createLLJIT(std::move(jtmb));
}

Compiler::~Compiler()
{
    // Release ResourceTrackers while lljit is still valid.
    releaseAllCompiledText();
}

void Compiler::noteCodeChange()
{
    lastCodeChangeTimestamp_ = QDateTime::currentMSecsSinceEpoch();
}

void Compiler::trackCompiledText(const std::shared_ptr<CompiledText> &compiledText)
{
    liveCompiledTexts_.erase(std::remove_if(liveCompiledTexts_.begin(), liveCompiledTexts_.end(),
                                            [](const std::weak_ptr<CompiledText> &w) { return w.expired(); }),
                             liveCompiledTexts_.end());
    liveCompiledTexts_.push_back(compiledText);
}

void Compiler::releaseAllCompiledText()
{
    for (auto &w : liveCompiledTexts_)
    {
        if (auto compiledText = w.lock())
        {
            if (compiledText->rt_)
            {
                consumeError(compiledText->rt_->remove());
                compiledText->rt_.reset();
            }
            compiledText->functionPtr_ = nullptr;
            compiledText->compiler_ = nullptr;
        }
    }
    liveCompiledTexts_.clear();
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
    // Get the currently-executing procedure.
    CallFrame *currentFrame = Kernel::get().currentCallFrame();

    // If the current frame is not a procedure, there is no need to save the tag names.
    if ( ! currentFrame->sourceNode_.isASTNode())
    {
        return;
    }

    // tagLineLocation_ must be the procedure-body suffix whose head is the line that
    // contains these tags (set by getCtrlGoto or Evaluator before compiling).
    if (tagLineLocation_.isNothing())
    {
        return;
    }

    Procedure *currentProcedure = currentFrame->sourceNode_.astnodeValue()->procedure_.procedureValue();

    for (auto &node : tagList)
    {
        QString tagName = getTagNameFromNode(node);
        if (!tagName.isEmpty())
        {
            currentProcedure->tagToLineAndBlockId_[tagName] = {tagLineLocation_, blockId};
        }
    }
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

std::shared_ptr<CompiledText> Compiler::generateFunctionPtrFromASTList(QList<QList<DatumPtr>> parsedList, List *aList)
{
    const DumpRequest dump = dumpRequest_;
    Scaffold compilerScaffolding(lljit_->getDataLayout(), targetMachine_.get());
    compilerScaffolding.mangleNames_ = dump.any();

    struct RestoreScaff
    {
        Compiler &compiler;
        Scaffold *saved;
        ~RestoreScaff() { compiler.scaff_ = saved; }
    } restore{*this, scaff_};
    scaff_ = &compilerScaffolding;

    auto compiledText = std::make_shared<CompiledText>();
    compiledText->astList_ = parsedList;
    compiledText->compiler_ = this;
    aList->compiledText_ = compiledText;
    trackCompiledText(compiledText);

    // The first block is number zero.
    int localBlockId = 0;

    // If the first block is a tag, save the tag names
    if (!parsedList.isEmpty() && !parsedList.first().isEmpty() && isTag(parsedList.first().first()))
    {
        setTagToBlockIdInProcedure(parsedList.first(), localBlockId);
        // Remove the first tag block from the list.
        parsedList.removeFirst();
    }

    BasicBlock *currentBlock = BasicBlock::Create(*scaff_->theContext_, "FirstBlock", scaff_->theFunction_);
    // After FirstBlock so it is last until body blocks insert before it.
    scaff_->mainBailoutBB_ = scaff_->createBasicBlock("MainBailout");

    QList<BasicBlock *> blocks = {currentBlock};
    scaff_->builder_.SetInsertPoint(currentBlock);

    Value *nodeResult = nullptr;
    TypeMask returnTypeRequest = TypeNothing;
    for (auto &srcBlock : parsedList)
    {
        if (srcBlock.isEmpty())
            continue;
        if (isTag(srcBlock.first()))
        {
            ++localBlockId;
            BasicBlock *newBlock = scaff_->createBasicBlock("Next Block");
            blocks.append(newBlock);
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
                // Otherwise, expect that nothing is returned.
                if (!parsedList.last().isEmpty() && node == parsedList.last().last()) {
                    returnTypeRequest = TypeDN;
                    // Also, set the inSequenceTail_ flag so that the child will know it can generate a tail call.
                    node.astnodeValue()->inSequenceTail_ = true;
                }
                nodeResult = generateChild(nullptr, node, returnTypeRequest);
            }
        }
    }

    // Empty list, or only TAGs after the leading tag block was stripped.
    if (nodeResult == nullptr)
        nodeResult = CoAddr(Datum::notADatum());
    generateReturn(nodeResult);
    generateWrapup();
    scaff_->finalizeColdPaths();

    if (blocks.size() > 1)
    {
        generateTOC(blocks, scaff_->theFunction_);
    }

    scaff_->hoistAllocasToEntry();

    if (dump.showCFG)
    {
        scaff_->theFunction_->viewCFG();
    }

    if (Config::get().verifyIR_)
    {
        std::string str;
        raw_string_ostream output(str);
        if (verifyFunction(*(scaff_->theFunction_), &output))
        {
            std::cerr << "IR verification failed:\n" << str << "\n";
            throw FCError::fatalInternal();
        }
    }

    // Lower coroutine intrinsics before backend.
    runCoroutinePasses(*scaff_->theModule_, scaff_->theMAM_);

    if (Config::get().verifyIR_)
        verifyModuleOrThrow(*scaff_->theModule_);

    if (dump.showIR)
    {
        scaff_->theModule_->print(errs(), nullptr);
        fprintf(stderr, "\n");
    }

    // Optimize the ramp and every coro-split clone.
    runFunctionPasses(*scaff_->theModule_, scaff_->theFPM_, scaff_->theFAM_);

    if (Config::get().verifyIR_)
        verifyModuleOrThrow(*scaff_->theModule_);

    if (dump.showModuleIR)
    {
        scaff_->theModule_->print(errs(), nullptr);
        fprintf(stderr, "\n");
    }

    auto tsm = ThreadSafeModule(std::move(scaff_->theModule_), std::move(scaff_->theContext_));
    auto [addr, rt] = addModuleAndLookup(*lljit_, std::move(tsm), scaff_->name_);
    compiledText->rt_ = std::move(rt);
    compiledText->functionPtr_ = reinterpret_cast<CompiledFunctionPtr>(addr);
    return compiledText;
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
    if (!currentBlock.isEmpty())
        retval.append(currentBlock);

    // If the last block is a tag, append a NOOP expression to ensure that there is an instruction to jump to.
    if (!currentBlock.isEmpty() && isTag(currentBlock.last()))
    {
        auto *noopNode = new ASTNode(DatumPtr(StringConstants::keywordNoop()));
        noopNode->genExpression_ = &Compiler::genNoop;
        noopNode->produces_ = TypeNothing;

        currentBlock = {DatumPtr(noopNode)};
        retval.append(currentBlock);
    }

    return retval;
}

std::shared_ptr<CompiledText> Compiler::compiledTextFromList(List *aList)
{
    if (aList->compileTimeStamp <= lastCodeChangeTimestamp_ || !aList->compiledText_ ||
        !aList->compiledText_->functionPtr_)
    {
        QList<DatumPtr> astFlatList = Treeifier::astFromList(aList);
        QList<QList<DatumPtr>> parsedList = groupConsecutiveExpressions(astFlatList);
        return generateFunctionPtrFromASTList(parsedList, aList);
    }

    return aList->compiledText_;
}

Value *Compiler::generateChildOfNode(ASTNode *parent, const DatumPtr &node, TypeMask expected)
{
    Generator method = node.astnodeValue()->genExpression_;
    Value *retval = ((this->*method)(node, expected));
    return retval;
}

Value *Compiler::generateCast(Value *src, ASTNode *parent, const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(!src->getType()->isVoidTy());
    TypeMask produces = node.astnodeValue()->produces_;

    // produces_ is the native LLVM kind; expected is a mask of what the parent
    // can consume. Keep the native kind when that bit is set. NFC every datum
    // pointer the parent will treat as data (including TypeDB).
    if (src->getType()->isDoubleTy())
        return generateCastFromDouble(src, parent, expected);
    if (src->getType()->isIntegerTy(1))
        return generateCastFromBool(src, parent, expected);
    if (produces == TypeDatum)
        return generateCastFromDatum(src, parent, expected);
    if (produces == TypeNothing)
        return generateCastFromNothing(src, parent, expected);

    Q_ASSERT(produces == TypeDN);
    return generateCastFromDN(src, parent, expected);
}

Value *Compiler::generateCastFromDouble(Value *src, ASTNode *parent, TypeMask expected)
{
    if (expected & TypeReal)
        return src;
    Value *srcAsWord = generateWordFromDouble(src);
    if (expected & TypeDatum)
        return generateNotFlowControl(srcAsWord);
    if (expected & TypeBool)
        return generateBoolFromDatum(parent, srcAsWord);
    return generateImmediateReturn(generateErrorNoSay(srcAsWord));
}

Value *Compiler::generateCastFromBool(Value *src, ASTNode *parent, TypeMask expected)
{
    if (expected & TypeBool)
        return src;
    Value *srcAsWord = generateWordFromBool(src);
    if (expected & TypeDatum)
        return generateNotFlowControl(srcAsWord);
    if (expected & TypeReal)
        return generateDoubleFromDatum(parent, srcAsWord);
    return generateImmediateReturn(generateErrorNoSay(srcAsWord));
}

Value *Compiler::generateCastFromDatum(Value *src, ASTNode *parent, TypeMask expected)
{
    if (expected & TypeDatum)
        return generateNotFlowControl(src);
    if (expected & TypeBool)
        return generateBoolFromDatum(parent, src);
    if (expected & TypeReal)
        return generateDoubleFromDatum(parent, src);
    return generateImmediateReturn(generateErrorNoSay(src));
}

Value *Compiler::generateCastFromNothing(Value *src, ASTNode *parent, TypeMask expected)
{
    if (expected & TypeNothing)
        return src->getType()->isPointerTy() ? generateNotFlowControl(src) : src;

    Value *recovered = generateRecoverDidntOutput(src, parent);
    if (expected & TypeReal)
        return generateDoubleFromDatum(parent, recovered);
    if ((expected & TypeBool) && !(expected & TypeDatum))
        return generateBoolFromDatum(parent, recovered);
    return generateNotFlowControl(recovered);
}

Value *Compiler::generateCastFromDN(Value *src, ASTNode *parent, TypeMask expected)
{
    if (expected == TypeNothing)
        return generateNothingFromDatum(parent, src);

    // Recover "did not output" only when Nothing is not allowed. Keep a datum
    // pointer when the parent accepts Datum (IF's TypeDB, RUN / user procs).
    if (!(expected & TypeNothing))
        src = generateNotNothingFromDatum(parent, src);

    if (expected & TypeDatum)
        return generateNotFlowControl(src);
    if (expected & TypeBool)
        return generateBoolFromDatum(parent, src);
    return generateDoubleFromDatum(parent, src);
}

Value *Compiler::generateChild(ASTNode *parent, const DatumPtr &node, TypeMask expected)
{
    Value *child = generateChildOfNode(parent, node, expected);
    return generateCast(child, parent, node, expected);
}

Value *Compiler::generateChild(ASTNode *parent, unsigned int index, TypeMask expected)
{
    DatumPtr node = parent->childAtIndex(index);
    return generateChild(parent, node, expected);
}

Value *Compiler::generateDoubleFromDatum(ASTNode *parent, Value *src)
{
    AllocaInst *out = scaff_->createEntryBlockAlloca(TyDouble, nullptr, DBG_NAME("doubleOut"));
    Value *retval = nullptr;
    auto realTest = [this, &retval, out](Value *src) {
        Value *ok = generateCallExtern(parseDoubleForDatum, PaAddr(scaff_->evaluator_), PaAddr(src),
                                       PaAddr(out));
        retval = scaff_->builder_.CreateLoad(TyDouble, out, DBG_NAME("doubleVal"));
        return ok;
    };
    generateValidationDatum(parent, src, realTest);
    return retval;
}

Value *Compiler::generateBoolFromDatum(ASTNode *parent, Value *src)
{
    AllocaInst *out = scaff_->createEntryBlockAlloca(TyInt8, nullptr, DBG_NAME("boolOut"));
    Value *retval = nullptr;
    auto boolTest = [this, &retval, out](Value *src) {
        Value *ok = generateCallExtern(parseBoolForDatum, PaAddr(scaff_->evaluator_), PaAddr(src),
                                       PaAddr(out));
        retval = scaff_->builder_.CreateTrunc(scaff_->builder_.CreateLoad(TyInt8, out, DBG_NAME("boolI8")),
                                              TyBool, DBG_NAME("boolVal"));
        return ok;
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
    src = generateNotFlowControl(src);

    BasicBlock *srcBB = scaff_->builder_.GetInsertBlock();
    BasicBlock *isNothingBB = scaff_->createBasicBlock(DBG_NAME("isNothing"));
    BasicBlock *notNothingBB = scaff_->createBasicBlock(DBG_NAME("notNothing"));

    scaff_->addColdPathBlocks(isNothingBB);

    Value *dType = generateGetDatumIsa(src);
    Value *dataMask = scaff_->builder_.CreateAnd(dType, CoInt32(Datum::typeDataMask), DBG_NAME("dataTypeMask"));
    Value *isData = scaff_->builder_.CreateICmpNE(dataMask, CoInt32(0), DBG_NAME("isDataTest"));
    scaff_->builder_.CreateCondBr(isData, notNothingBB, isNothingBB);

    scaff_->builder_.SetInsertPoint(isNothingBB);
    Value *recovered = generateRecoverDidntOutput(src, parent);
    BasicBlock *recoveredBB = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.CreateBr(notNothingBB);

    scaff_->builder_.SetInsertPoint(notNothingBB);
    PHINode *retval = scaff_->builder_.CreatePHI(TyAddr, 2, DBG_NAME("notNothing"));
    retval->addIncoming(src, srcBB);
    retval->addIncoming(recovered, recoveredBB);
    return retval;
}

Value *Compiler::generateNotFlowControl(Value *src)
{
    BasicBlock *isFlowBB = scaff_->createBasicBlock(DBG_NAME("isFlow"));
    BasicBlock *notFlowBB = scaff_->createBasicBlock(DBG_NAME("notFlow"));

    scaff_->addColdPathBlocks(isFlowBB);

    Value *dType = generateGetDatumIsa(src);
    Value *flowMask = scaff_->builder_.CreateAnd(dType, CoInt32(Datum::typeFlowControlMask), DBG_NAME("flowControlMask"));
    Value *isFlow = scaff_->builder_.CreateICmpNE(flowMask, CoInt32(0), DBG_NAME("isFlowTest"));
    scaff_->builder_.CreateCondBr(isFlow, isFlowBB, notFlowBB);

    scaff_->builder_.SetInsertPoint(isFlowBB);
    generateReturn(src);

    scaff_->builder_.SetInsertPoint(notFlowBB);
    return src;
}

Value *Compiler::generateNothingFromDatum(ASTNode *parent, Value *src)
{
    src = generateNotFlowControl(src);

    BasicBlock *notNothingBB = scaff_->createBasicBlock(DBG_NAME("notNothing"));
    BasicBlock *isNothingBB = scaff_->createBasicBlock(DBG_NAME("isNothing"));

    scaff_->addColdPathBlocks(notNothingBB);

    Value *dType = generateGetDatumIsa(src);
    Value *dataMask = scaff_->builder_.CreateAnd(dType, CoInt32(Datum::typeDataMask), DBG_NAME("dataTypeMask"));
    Value *isData = scaff_->builder_.CreateICmpNE(dataMask, CoInt32(0), DBG_NAME("isDataTest"));
    scaff_->builder_.CreateCondBr(isData, notNothingBB, isNothingBB);

    scaff_->builder_.SetInsertPoint(notNothingBB);
    Value *errObj = generateErrorNoSay(src);
    generateReturn(errObj);

    scaff_->builder_.SetInsertPoint(isNothingBB);
    return src;
}

Value *Compiler::generateIsWord(Value *src)
{
    Value *isa = generateGetDatumIsa(src);
    Value *wordMask = scaff_->builder_.CreateAnd(isa, CoInt32(Datum::typeWord), DBG_NAME("wordMask"));
    return scaff_->builder_.CreateICmpNE(wordMask, CoInt32(0), DBG_NAME("isWord"));
}

Value *Compiler::generateIsEmptyList(Value *src)
{
    return scaff_->builder_.CreateICmpEQ(src, CoAddr(EmptyList::instance()), DBG_NAME("isEmptyList"));
}

Value *Compiler::generateIsWordOrEmptyList(Value *src)
{
    Value *isWord = generateIsWord(src);
    Value *isEmptyList = generateIsEmptyList(src);
    return scaff_->builder_.CreateOr(isWord, isEmptyList, DBG_NAME("isWordOrEmptyList"));
}

Value *Compiler::generateWordOrEmptyListFromDatum(ASTNode *parent, Value *src)
{
    return generateValidationDatum(parent, src, [this](Value *candidate) {
        return generateIsWordOrEmptyList(candidate);
    });
}

Value *Compiler::generateContentsListFromDatum(ASTNode *parent, Value *src)
{
    return generateValidationDatum(parent, src, [this](Value *candidate) {
        return generateCallExtern(isContentsList, PaAddr(candidate));
    });
}

Value *Compiler::generateNamedContentsListFromDatum(ASTNode *parent, Value *src)
{
    return generateValidationDatum(parent, src, [this](Value *candidate) {
        return generateCallExtern(isNamedContentsList, PaAddr(candidate));
    });
}

Value *Compiler::generateDefinableProcNameFromDatum(ASTNode *parent, Value *src)
{
    return generateValidationDatum(parent, src, [this](Value *candidate) {
        return generateCallExtern(isDefinableProcName, PaAddr(candidate));
    });
}

Value *Compiler::generateDefineTextFromDatum(ASTNode *parent, Value *src)
{
    return generateValidationDatum(parent, src, [this](Value *candidate) {
        return generateCallExtern(isDefineText, PaAddr(candidate));
    });
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

Value *Compiler::genLiteral(const DatumPtr &node, TypeMask expected)
{
    DatumPtr literalPtr = node.astnodeValue()->childAtIndex(0);

    // A literal is a Word, List, or Array.
    // However, the caller may want a Bool or Real.
    // We can cast, but only if the literal is a Word and can be cast to the requested type.
    if (literalPtr.isWord())
    {
        Word *wVal = literalPtr.wordValue();
        if (expected & TypeReal)
        {
            double val = wVal->numberValue();
            if (wVal->numberIsValid)
                return CoDouble(val);
        }
        if (expected & TypeBool)
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

Value *Compiler::generateLookupVarname(Value *nameAddr)
{
    Value *lookedUp = generateCallExtern(getDatumForVarname, PaAddr(nameAddr));
    BasicBlock *lookupBB = scaff_->builder_.GetInsertBlock();
    BasicBlock *noValueBB = scaff_->createBasicBlock(DBG_NAME("NoValue"));
    BasicBlock *hasValueBB = scaff_->createBasicBlock(DBG_NAME("hasValue"));

    scaff_->addColdPathBlocks(noValueBB);

    Value *dType = generateGetDatumIsa(lookedUp);
    Value *mask = scaff_->builder_.CreateAnd(dType, CoInt32(Datum::typeDataMask), DBG_NAME("dataMask"));
    Value *cond = scaff_->builder_.CreateICmpEQ(mask, CoInt32(0), DBG_NAME("dataMaskTest"));
    scaff_->builder_.CreateCondBr(cond, noValueBB, hasValueBB);

    // Unbound. Run ERRACT if it has a value; a datum from it stands in for the lookup.
    scaff_->builder_.SetInsertPoint(noValueBB);
    Value *handlerResult = generateCallExtern(handleNoValue, PaAddr(scaff_->evaluator_), PaAddr(nameAddr));
    handlerResult = generateNotFlowControl(handlerResult);
    BasicBlock *recoveredBB = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.CreateBr(hasValueBB);

    scaff_->builder_.SetInsertPoint(hasValueBB);
    PHINode *retval = scaff_->builder_.CreatePHI(TyAddr, 2, DBG_NAME("lookup"));
    retval->addIncoming(lookedUp, lookupBB);
    retval->addIncoming(handlerResult, recoveredBB);
    return retval;
}

Value *Compiler::genValueOf(const DatumPtr &node, TypeMask expected)
{
    Word *varName = node.astnodeValue()->childAtIndex(0).wordValue();
    return generateLookupVarname(CoAddr(varName));
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
        BasicBlock::Create(*scaff_->theContext_, DBG_NAME("CoroPrologue"), scaff_->theFunction_, bodyEntry);
    BasicBlock *savedBlock = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.SetInsertPoint(scaff_->coroPrologueBB_);

    scaff_->suspendBB_ = BasicBlock::Create(*scaff_->theContext_, DBG_NAME("suspend"), scaff_->theFunction_);
    scaff_->cleanupBB_ = BasicBlock::Create(*scaff_->theContext_, DBG_NAME("cleanup"), scaff_->theFunction_);
    // Coroutine frame: id -> size -> alloc -> begin
    // llvm.coro.id(i32 align, ptr promise, ptr coroutine, ptr info) -> token
    Function *coroIdFn = Intrinsic::getOrInsertDeclaration(scaff_->theModule_.get(), Intrinsic::coro_id);
    scaff_->coroutineToken_ = scaff_->builder_.CreateCall(
        coroIdFn, {CoInt32(0), CoAddr(0), CoAddr(0), CoAddr(0)}, DBG_NAME("id"));
    Value *coroutineCallToken = scaff_->coroutineToken_;

    Function *coroSizeFn = Intrinsic::getOrInsertDeclaration(scaff_->theModule_.get(), Intrinsic::coro_size, {TyInt32});
    Value *coroutineSize = scaff_->builder_.CreateCall(coroSizeFn, {}, DBG_NAME("size"));

    Function *coroAlignFn = Intrinsic::getOrInsertDeclaration(scaff_->theModule_.get(), Intrinsic::coro_align, {TyInt32});
    Value *coroutineAlign = scaff_->builder_.CreateCall(coroAlignFn, {}, DBG_NAME("align"));

    Value *coroutineAlloc = generateCallExtern(
        q_malloc, PaAddr(scaff_->evaluator_), PaInt32(coroutineSize), PaInt32(coroutineAlign));

    Function *coroBeginFn = Intrinsic::getOrInsertDeclaration(scaff_->theModule_.get(), Intrinsic::coro_begin);
    auto *coroBeginCall = cast<CallInst>(
        scaff_->builder_.CreateCall(coroBeginFn, {coroutineCallToken, coroutineAlloc}, DBG_NAME("handle")));
    coroBeginCall->addRetAttr(Attribute::NoAlias);
    scaff_->coroutineHandle_ = coroBeginCall;

    scaff_->builder_.CreateBr(bodyEntry);
    scaff_->builder_.SetInsertPoint(savedBlock);
    return scaff_->coroutineHandle_;
}

void Compiler::generateCoroSuspend()
{
    ensureCoroutineFrame();

    Function *coroSuspendFn = Intrinsic::getOrInsertDeclaration(scaff_->theModule_.get(), Intrinsic::coro_suspend);
    Value *coroutineSuspend = scaff_->builder_.CreateCall(
        coroSuspendFn, {ConstantTokenNone::get(*scaff_->theContext_), CoBool(false)}, DBG_NAME("suspend"));

    BasicBlock *continueBB = scaff_->createBasicBlock(DBG_NAME("continue"));
    SwitchInst *sw = scaff_->builder_.CreateSwitch(coroutineSuspend, scaff_->suspendBB_, 2);
    sw->addCase(CoInt8(0), continueBB);
    sw->addCase(CoInt8(1), scaff_->cleanupBB_);

    scaff_->builder_.SetInsertPoint(continueBB);
}

Value *Compiler::generateCallList(Value *list)
{
    generateCallExtern(pushListOntoEvaluationStack, PaAddr(scaff_->evaluator_), PaAddr(list));
    generateCoroSuspend();
    return generateCallExtern(getLastEvaluationResult, PaAddr(scaff_->evaluator_));
}

Value *Compiler::generateCallListContinuation(Value *list)
{
    return generateCallExtern(runListAfterCurrentList, PaAddr(scaff_->evaluator_), PaAddr(list));
}

Value *Compiler::genExecProcedure(const DatumPtr &node, TypeMask expected)
{
    ASTNode *ast = node.astnodeValue();
    DatumPtr proc = ast->procedure_;
    if (proc.datumValue()->isa_ == Datum::typeProcedure && proc.procedureValue()->isMacro_)
    {
        // Command position (Nothing is acceptable) splices the whole output list.
        // An input must be exactly one expression.
        ast->macroExpectsSingleExpression_ = (expected & TypeNothing) == 0;
    }

    CallSite cs(this, node, TypeDatum);
    AllocaInst *paramAry = cs.allocaAry(DBG_NAME("paramAry"));
    Value *vAstnodeValue = CoAddr(node.astnodeValue());
    Value *vParamArySize = CoInt32(node.astnodeValue()->countOfChildren());
    generateCallExtern(beginProcedure, PaAddr(scaff_->evaluator_), PaAddr(vAstnodeValue), PaAddr(paramAry), PaInt32(vParamArySize));

    generateCoroSuspend();
    return cs.ret(generateCallExtern(getLastEvaluationResult, PaAddr(scaff_->evaluator_)));
}

Value *Compiler::generateWordFromDouble(Value *val)
{
    return generateCallExtern(getWordForDouble, PaAddr(scaff_->evaluator_), PaDouble(val));
}

Value *Compiler::generateWordFromBool(Value *val)
{
    return generateCallExtern(getWordForBool, PaAddr(scaff_->evaluator_), PaBool(val));
}

Value *Compiler::generateErrorSystem()
{
    Value *errObj = generateCallExtern(getErrorSystem, PaAddr(scaff_->evaluator_));
    return errObj;
}

Value *Compiler::generateErrorToplevel()
{
    Value *errObj = generateCallExtern(getErrorToplevel, PaAddr(scaff_->evaluator_));
    return errObj;
}

Value *Compiler::generateErrorNoLike(ASTNode *who, Value *what)
{
    Value *errWho = CoAddr(who->nodeName_.datumValue());
    Value *errObj = generateCallExtern(getErrorNoLike, PaAddr(scaff_->evaluator_), PaAddr(errWho), PaAddr(what));
    return errObj;
}

Value *Compiler::generateErrorNoSay(Value *what)
{
    Value *errObj = generateCallExtern(getErrorNoSay, PaAddr(scaff_->evaluator_), PaAddr(what));
    return errObj;
}

Value *Compiler::generateErrorNoTest(Value *who)
{
    Value *errObj = generateCallExtern(getErrorNoTest, PaAddr(scaff_->evaluator_), PaAddr(who));
    return errObj;
}

Value *Compiler::generateErrorNoValue(Value *what)
{
    Value *errObj = generateCallExtern(getErrorNoValue, PaAddr(scaff_->evaluator_), PaAddr(what));
    return errObj;
}

Value *Compiler::generateErrorNoOutput(Value *x, ASTNode *y)
{
    Value *vY = CoAddr(y->nodeName_.datumValue());
    Value *errObj = generateCallExtern(getErrorNoOutput, PaAddr(scaff_->evaluator_), PaAddr(x), PaAddr(vY));
    return errObj;
}

Value *Compiler::generateRecoverDidntOutput(Value *who, ASTNode *parent)
{
    Value *vY = CoAddr(parent->nodeName_.datumValue());
    Value *handlerResult =
        generateCallExtern(handleDidntOutput, PaAddr(scaff_->evaluator_), PaAddr(who), PaAddr(vY));
    return generateNotFlowControl(handlerResult);
}

Value *Compiler::generateErrorNotEnoughInputs(ASTNode *x)
{
    Value *vX = CoAddr(x->nodeName_.datumValue());
    Value *errObj = generateCallExtern(getErrorNotEnoughInputs, PaAddr(scaff_->evaluator_), PaAddr(vX));
    return errObj;
}

Value *Compiler::generateErrorNoGraphics()
{
    Value *errObj = generateCallExtern(getErrorNoGraphics, PaAddr(scaff_->evaluator_));
    return errObj;
}

Value *Compiler::generateImmediateReturn(llvm::Value *retval)
{
    BasicBlock *bailoutBB = scaff_->createBasicBlock(DBG_NAME("bailout"));
    BasicBlock *throwAwayBB = scaff_->createBasicBlock(DBG_NAME("throwAway"));
    scaff_->addColdPathBlocks(bailoutBB);

    // Leave this path with the flow-control / error result. Remaining nodes on
    // the line are still generated into throwAwayBB which has no predecessor.
    scaff_->builder_.CreateBr(bailoutBB);

    scaff_->builder_.SetInsertPoint(bailoutBB);
    generateReturn(retval);

    scaff_->builder_.SetInsertPoint(throwAwayBB);
    return retval;
}

std::vector<Value *> Compiler::generateChildren(ASTNode *node, TypeMask expected)
{
    std::vector<Value *> retval;
    retval.reserve(node->countOfChildren());
    for (int i = 0; i < node->countOfChildren(); ++i)
    {
        Value *v = generateChildOfNode(node, node->childAtIndex(i), expected);
        Value *casted = generateCast(v, node, node->childAtIndex(i), expected);
        retval.push_back(casted);
    }

    return retval;
}

AllocaInst *Compiler::generateChildrenAlloca(ASTNode *node, TypeMask expected, const std::string &name)
{
    std::vector<Value *> children = generateChildren(node, expected);
    return generateAllocaAry(children, name);
}

AllocaInst *Compiler::generateAllocaAry(const std::vector<llvm::Value *> &values, const std::string &name)
{
    Value *childCount = CoInt32(values.size());
    AllocaInst *retval = scaff_->createEntryBlockAlloca(TyAddr, childCount, name);
    for (size_t i = 0; i < values.size(); ++i)
    {
        Value *ptr = scaff_->builder_.CreateConstInBoundsGEP1_32(
            TyAddr, retval, static_cast<unsigned>(i), name);
        scaff_->builder_.CreateStore(values[i], ptr);
    }
    return retval;
}

Value *Compiler::generateDatumForTrace(Value *value)
{
    if (value->getType()->isDoubleTy())
        return generateWordFromDouble(value);
    if (value->getType()->isIntegerTy(1))
        return generateWordFromBool(value);
    Q_ASSERT(value->getType()->isPointerTy());
    return value;
}

void Compiler::generateTraceBegin(ASTNode *node, const std::vector<Value *> &children)
{
    const QString key = node->nodeName_.toString(Datum::ToStringFlags_Key);
    if (!Kernel::get().containsName(Kernel::NameMark::Traced, Kernel::NameKind::Procedures, key))
        return;

    std::vector<Value *> boxed;
    boxed.reserve(children.size());
    for (Value *child : children)
        boxed.push_back(generateDatumForTrace(child));

    Value *ary = boxed.empty() ? static_cast<Value *>(CoAddr(0))
                               : static_cast<Value *>(generateAllocaAry(boxed, DBG_NAME("traceAry")));
    generateCallExtern(stdTraceBegin,
                       PaAddr(scaff_->evaluator_),
                       PaAddr(CoAddr(node)),
                       PaAddr(ary),
                       PaInt32(CoInt32(static_cast<int32_t>(boxed.size()))));
}

void Compiler::generateTraceEnd(ASTNode *node, Value *result)
{
    const QString key = node->nodeName_.toString(Datum::ToStringFlags_Key);
    if (!Kernel::get().containsName(Kernel::NameMark::Traced, Kernel::NameKind::Procedures, key))
        return;

    generateCallExtern(stdTraceEnd,
                       PaAddr(scaff_->evaluator_),
                       PaAddr(CoAddr(node)),
                       PaAddr(generateDatumForTrace(result)));
}

std::vector<Value *> Compiler::generateChildren(ASTNode *node, std::vector<TypeMask> expectedAry)
{
    Q_ASSERT(node->countOfChildren() == expectedAry.size());
    std::vector<Value *> retval;
    retval.reserve(node->countOfChildren());
    for (int i = 0; i < node->countOfChildren(); ++i)
    {
        retval.push_back(generateChildOfNode(node, node->childAtIndex(i), expectedAry[i]));
    }
    for (int i = 0; i < node->countOfChildren(); ++i)
    {
        retval[i] = generateCast(retval[i], node, node->childAtIndex(i), expectedAry[i]);
    }
    return retval;
}

AllocaInst *Compiler::generateNumberAryFromDatum(ASTNode *parent, Value *src, int32_t size,
                                                 const validatorFunction &elementOk)
{
    Value *vSize = CoInt32(size);
    AllocaInst *ary = scaff_->createEntryBlockAlloca(TyDouble, vSize, DBG_NAME("ary"));

    auto isNumberList = [this, vSize, ary, size, &elementOk](Value *candidate) {
        BasicBlock *notListBB = scaff_->createBasicBlock(DBG_NAME("notList"));
        BasicBlock *isListBB = scaff_->createBasicBlock(DBG_NAME("isList"));
        BasicBlock *tryFillBB = scaff_->createBasicBlock(DBG_NAME("tryFill"));
        BasicBlock *isListDoneBB = scaff_->createBasicBlock(DBG_NAME("isListDone"));
        BasicBlock *checkedBB = scaff_->createBasicBlock(DBG_NAME("checked"));

        scaff_->addColdPathBlocks(notListBB);

        Value *dType = generateGetDatumIsa(candidate);
        Value *mask = scaff_->builder_.CreateAnd(dType, CoInt32(Datum::typeList), DBG_NAME("dataTypeMask"));
        Value *isList = scaff_->builder_.CreateICmpNE(mask, CoInt32(0), DBG_NAME("isListTest"));
        scaff_->builder_.CreateCondBr(isList, isListBB, notListBB);

        scaff_->builder_.SetInsertPoint(notListBB);
        scaff_->builder_.CreateBr(checkedBB);

        scaff_->builder_.SetInsertPoint(isListBB);
        Value *count = generateCallExtern(getCountOfList, PaAddr(candidate));
        Value *countGood = scaff_->builder_.CreateICmpEQ(count, vSize, DBG_NAME("countTest"));
        scaff_->builder_.CreateCondBr(countGood, tryFillBB, isListDoneBB);

        scaff_->builder_.SetInsertPoint(tryFillBB);
        Value *isGood = generateCallExtern(getNumberAryFromList, PaAddr(candidate), PaAddr(ary));
        Value *fillGood = scaff_->builder_.CreateICmpEQ(isGood, CoInt32(1), DBG_NAME("fillTest"));

        // Optional per-element predicate. Runs only after a complete numeric fill,
        // and a failure still rejects the list (one ERRACT trip).
        BasicBlock *elementsBB = nullptr;
        BasicBlock *fillFailBB = nullptr;
        Value *elementsOk = nullptr;
        if (elementOk)
        {
            elementsBB = scaff_->createBasicBlock(DBG_NAME("elements"));
            fillFailBB = scaff_->createBasicBlock(DBG_NAME("fillFail"));
            scaff_->builder_.CreateCondBr(fillGood, elementsBB, fillFailBB);

            scaff_->builder_.SetInsertPoint(elementsBB);
            elementsOk = CoBool(true);
            for (int32_t i = 0; i < size; ++i)
            {
                Value *ptr = scaff_->builder_.CreateConstInBoundsGEP1_32(
                    TyDouble, ary, static_cast<unsigned>(i), DBG_NAME("elemPtr"));
                Value *elem = scaff_->builder_.CreateLoad(TyDouble, ptr, DBG_NAME("elem"));
                elementsOk = scaff_->builder_.CreateAnd(elementsOk, elementOk(elem), DBG_NAME("elemOk"));
            }
            scaff_->builder_.CreateBr(isListDoneBB);

            scaff_->builder_.SetInsertPoint(fillFailBB);
            scaff_->builder_.CreateBr(isListDoneBB);
        }
        else
        {
            scaff_->builder_.CreateBr(isListDoneBB);
        }

        scaff_->builder_.SetInsertPoint(isListDoneBB);
        PHINode *listOk = scaff_->builder_.CreatePHI(TyBool, elementOk ? 3 : 2, DBG_NAME("listOk"));
        if (elementOk)
        {
            listOk->addIncoming(elementsOk, elementsBB);
            listOk->addIncoming(CoBool(false), fillFailBB);
        }
        else
        {
            listOk->addIncoming(fillGood, tryFillBB);
        }
        listOk->addIncoming(CoBool(false), isListBB);
        scaff_->builder_.CreateBr(checkedBB);

        scaff_->builder_.SetInsertPoint(checkedBB);
        PHINode *ok = scaff_->builder_.CreatePHI(TyBool, 2, DBG_NAME("isNumberList"));
        ok->addIncoming(CoBool(false), notListBB);
        ok->addIncoming(listOk, isListDoneBB);
        return ok;
    };

    generateValidationDatum(parent, src, isNumberList);
    return ary;
}

Value *Compiler::generateValidationDouble(ASTNode *parent, Value *src, const validatorFunction &validator)
{
    BasicBlock *srcBB = scaff_->builder_.GetInsertBlock();

    BasicBlock *validateBB = scaff_->createBasicBlock(DBG_NAME("validate"));
    BasicBlock *erractBB = scaff_->createBasicBlock(DBG_NAME("errorAction"));
    BasicBlock *acceptBB = scaff_->createBasicBlock(DBG_NAME("accept"));

    scaff_->addColdPathBlocks(erractBB);

    scaff_->builder_.CreateBr(validateBB);

    // Validate the number.
    scaff_->builder_.SetInsertPoint(validateBB);
    PHINode *candidate = scaff_->builder_.CreatePHI(TyDouble, 2, DBG_NAME("candidate"));
    candidate->addIncoming(src, srcBB);
    Value *isValidCond = validator(candidate);
    scaff_->builder_.CreateCondBr(isValidCond, acceptBB, erractBB);

    // The number is bad. Call handleBadDouble and maybe retry with the result.
    scaff_->builder_.SetInsertPoint(erractBB);
    Value *handlerResult = generateCallExtern(handleBadDouble, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(parent)), PaDouble(candidate));
    handlerResult = generateNotFlowControl(handlerResult);

    AllocaInst *out = scaff_->createEntryBlockAlloca(TyDouble, nullptr, DBG_NAME("retryDoubleOut"));
    Value *numberIsValid = generateCallExtern(parseDoubleForDatum, PaAddr(scaff_->evaluator_),
                                              PaAddr(handlerResult), PaAddr(out));
    Value *dVal = scaff_->builder_.CreateLoad(TyDouble, out, DBG_NAME("retryDouble"));
    dVal = scaff_->builder_.CreateSelect(numberIsValid, dVal, ConstantFP::getNaN(TyDouble),
                                         DBG_NAME("retryOrNaN"));
    candidate->addIncoming(dVal, scaff_->builder_.GetInsertBlock());
    scaff_->builder_.CreateBr(validateBB);

    // The number is good. Continue.
    scaff_->builder_.SetInsertPoint(acceptBB);
    return candidate;
}

Value *Compiler::generateValidationDatum(ASTNode *parent, Value *src, const validatorFunction &validator)
{
    BasicBlock *srcBB = scaff_->builder_.GetInsertBlock();

    BasicBlock *validateBB = scaff_->createBasicBlock(DBG_NAME("validate"));
    BasicBlock *erractBB = scaff_->createBasicBlock(DBG_NAME("errorAction"));
    BasicBlock *acceptBB = scaff_->createBasicBlock(DBG_NAME("accept"));

    scaff_->addColdPathBlocks(erractBB);

    scaff_->builder_.CreateBr(validateBB);

    // Validate the datum.
    scaff_->builder_.SetInsertPoint(validateBB);
    PHINode *candidate = scaff_->builder_.CreatePHI(TyAddr, 2, DBG_NAME("candidate"));
    candidate->addIncoming(src, srcBB);
    Value *cond = validator(candidate);
    scaff_->builder_.CreateCondBr(cond, acceptBB, erractBB);

    // The datum is bad. Call handleBadDatum and maybe retry with the result.
    scaff_->builder_.SetInsertPoint(erractBB);
    Value *handlerResult = generateCallExtern(handleBadDatum, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(parent)), PaAddr(candidate));
    handlerResult = generateNotFlowControl(handlerResult);
    candidate->addIncoming(handlerResult, scaff_->builder_.GetInsertBlock());
    scaff_->builder_.CreateBr(validateBB);

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
        generateCallExtern(q_free, PaAddr(scaff_->evaluator_), PaAddr(memToFree));
        scaff_->builder_.CreateBr(cleanupEndBB);
        scaff_->builder_.SetInsertPoint(cleanupEndBB);
        //   br label %suspend
        scaff_->builder_.CreateBr(scaff_->suspendBB_);

        scaff_->builder_.SetInsertPoint(scaff_->suspendBB_);
        //   call void @llvm.coro.end(ptr %hdl, i1 false, token none)
        Function *coroEndFn = Intrinsic::getOrInsertDeclaration(scaff_->theModule_.get(), Intrinsic::coro_end);
        scaff_->builder_.CreateCall(coroEndFn, {scaff_->coroutineHandle_, CoBool(false), ConstantTokenNone::get(*scaff_->theContext_)});
        //   ret ptr %hdl
        scaff_->builder_.CreateRet(scaff_->coroutineHandle_);
    }
    else
    {
        scaff_->builder_.CreateRet(ConstantPointerNull::get(TyAddr));
    }
}

Value *Compiler::generateGetDatumIsa(Value *objAddr)
{
    Value *isaAddr = scaff_->builder_.CreateStructGEP(scaff_->datumHeaderTy_, objAddr, 1, DBG_NAME("isaAddr"));
    LoadInst *load = scaff_->builder_.CreateAlignedLoad(TyInt32, isaAddr, Align(4), DBG_NAME("isaLoad"));
    load->setMetadata(LLVMContext::MD_invariant_load, MDNode::get(*scaff_->theContext_, {}));
    load->setMetadata(LLVMContext::MD_tbaa, scaff_->isaLoadTbaa_);
    return load;
}


/***DOC ALLOWGETSET
ALLOWGETSET						(variable)

	if TRUE, indicates that an attempt to use a procedure that doesn't
	exist should be taken as an implicit getter or setter procedure
	(setter if the first three letters of the name are SET) for a variable
	of the same name (without the SET if appropriate).

COD***/

/***DOC BUTTONACT
BUTTONACT						(variable)

	if nonempty, should be an instruction list that will be evaluated
	whenever a mouse button is pressed.  Note that the user may have
	released the button before the instructions are evaluated.  BUTTON
	will still output which button was most recently pressed.  CLICKPOS
	will output the position of the mouse cursor at the moment the
	button was pressed; this may be different from MOUSEPOS if the
	user moves the mouse after clicking.

	Note that it's possible for the user to press a button during the
	evaluation of the instruction list.  If this would confuse your
	program, prevent it by temporarily setting BUTTONACT to the empty
	list.  One easy way to do that is the following:

		make "buttonact [button.action]

		to button.action [:buttonact []]
		... ; whatever you want the button to do
		end

COD***/

/***DOC CASEIGNOREDP
CASEIGNOREDP						(variable)

	if TRUE, indicates that lower case and upper case letters should be
	considered equal by EQUALP, BEFOREP, MEMBERP, etc.  Logo initially
	makes this variable TRUE, and buries it.

COD***/

/***DOC ARGV
ARGV							(variable)

	a list of every word on the command line used to start QLogo,
	including the program name. This is a QLogo extension.
	See also COMMANDLINE.

COD***/

/***DOC COMMANDLINE
COMMANDLINE						(variable)

	contains any text appearing after a hyphen on the command line
	used to start Logo. The list is empty if there is no hyphen.

COD***/

/***DOC ERRACT
ERRACT							(variable)

	an instructionlist that will be run in the event of an error.
	Typically has the value [PAUSE] to allow interactive debugging.

COD***/

/***DOC FULLPRINTP
FULLPRINTP						(variable)

	if TRUE, then words that were created using backslash or vertical bar
	(to include characters that would otherwise not be treated as part of
	a word) are printed with the backslashes or vertical bars shown, so
	that the printed result could be re-read by Logo to produce the same
	value.  If FULLPRINTP is TRUE then the empty word (however it was
	created) prints as ||.  (Otherwise it prints as nothing at all.)

COD***/

/***DOC KEYACT
KEYACT							(variable)

	if nonempty, should be an instruction list that will be evaluated
	whenever a key is pressed on the keyboard.  The instruction list
	can use READCHAR to find out what key was pressed.  Note that only
	keys that produce characters qualify; pressing SHIFT or CONTROL
	alone will not cause KEYACT to be evaluated.

	Note that it's possible for the user to press a key during the
	evaluation of the instruction list.  If this would confuse your
	program, prevent it by temporarily setting KEYACT to the empty
	list.  One easy way to do that is the following:

		make "keyact [key.action]

		to key.action [:keyact []]
		... ; whatever you want the key to do
		end

COD***/

/***DOC LOADNOISILY
LOADNOISILY						(variable)

	if TRUE, prints the names of procedures defined when loading
	from a file (including the temporary file made by EDIT).

COD***/

/***DOC LOGOPLATFORM
LOGOPLATFORM						(variable)

	one of the following words: wxWidgets, X11, Windows, or
	Unix-Nographics.



COD***/

/***DOC LOGOVERSION
LOGOVERSION						(variable)

	a real number indicating the Logo version number, e.g., 5.5

COD***/

/***DOC PRINTDEPTHLIMIT
PRINTDEPTHLIMIT						(variable)

	if a nonnegative integer, indicates the maximum depth of sublist
	structure that will be printed by PRINT, etc.

COD***/

/***DOC PRINTWIDTHLIMIT
PRINTWIDTHLIMIT						(variable)

	if a nonnegative integer, indicates the maximum number of members
	in any one list that will be printed by PRINT, etc.

COD***/

/***DOC REDEFP
REDEFP							(variable)

	if TRUE, allows primitives to be erased (ERASE) or redefined (COPYDEF).

COD***/

/***DOC STARTUP
STARTUP							(variable)

	if assigned a list value in a file loaded by LOAD, that value is
	run as an instructionlist after the loading.

COD***/

/***DOC UNBURYONEDIT
UNBURYONEDIT						(variable)

	if TRUE, causes any procedure defined during EDIT or LOAD to be
	unburied, so that it will be saved by a later SAVE.  Files that
	want to define and bury procedures must do it in that order.

COD***/

/***DOC USEALTERNATENAMES
USEALTERNATENAMES					(variable)

	if TRUE, causes Logo to generate non-English words (from the
	Messages file) instead of TRUE, FALSE, END, etc.


COD***/
