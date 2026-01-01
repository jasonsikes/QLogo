#ifndef COMPILER_PRIVATE_H
#define COMPILER_PRIVATE_H

//===-- qlogo/compiler_private.h - Compiler class definition -------*- C++ -*-===//
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
/// This file contains the declarations of structures to support the Compiler
/// class and are only needed by the Compiler class.
///
//===----------------------------------------------------------------------===//

#include <QDebug>

// Qt #defines "emit". llvm uses "emit" as a function name.
#ifdef emit
#undef emit
#endif

#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

#include <memory>

// The CompilerContext class holds information that is necessary for the compiler
// but is not necessary for the objects using the compiler.
class CompilerContext
{
    std::unique_ptr<llvm::orc::ExecutionSession> exeSession;
    llvm::DataLayout dataLayout;
    llvm::orc::MangleAndInterner mangler;
    llvm::orc::RTDyldObjectLinkingLayer objectLayer;
    llvm::orc::IRCompileLayer compileLayer;
    llvm::orc::JITDylib &jitLib;

  public:
    llvm::ExitOnError exitOnErr;

    CompilerContext(std::unique_ptr<llvm::orc::ExecutionSession> es,
                    llvm::orc::JITTargetMachineBuilder jtmb,
                    llvm::DataLayout dl)
        : exeSession(std::move(es)), dataLayout(std::move(dl)), mangler(*this->exeSession, this->dataLayout),
          objectLayer(*this->exeSession, []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
          compileLayer(
              *this->exeSession, objectLayer, std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(jtmb))),
          jitLib(this->exeSession->createBareJITDylib("<main>"))
    {
        jitLib.addGenerator(
            cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(dl.getGlobalPrefix())));
        // Note: jtmb was moved in the initializer list, so we need to check the target triple
        // from the ExecutionSession instead
        if (exeSession->getExecutorProcessControl().getTargetTriple().isOSBinFormatCOFF())
        {
            objectLayer.setOverrideObjectFlagsWithResponsibilityFlags(true);
            objectLayer.setAutoClaimResponsibilityForObjectSymbols(true);
        }
    }

    ~CompilerContext()
    {
        if (auto err = exeSession->endSession())
            exeSession->reportError(std::move(err));
    }

    static CompilerContext *Create()
    {
        auto epc = llvm::orc::SelfExecutorProcessControl::Create();
        Q_ASSERT(epc);

        auto es = std::make_unique<llvm::orc::ExecutionSession>(std::move(*epc));

        llvm::orc::JITTargetMachineBuilder jtmb(es->getExecutorProcessControl().getTargetTriple());

        auto dl = jtmb.getDefaultDataLayoutForTarget();
        Q_ASSERT(dl);

        return new CompilerContext(std::move(es), std::move(jtmb), std::move(*dl));
    }

    const llvm::DataLayout &getDataLayout() const
    {
        return dataLayout;
    }

    llvm::orc::JITDylib &getMainJITDylib()
    {
        return jitLib;
    }

    llvm::Error addModule(llvm::orc::ThreadSafeModule tsm, llvm::orc::ResourceTrackerSP rt = nullptr)
    {
        if (!rt)
            rt = jitLib.getDefaultResourceTracker();
        return compileLayer.add(rt, std::move(tsm));
    }

    llvm::Expected<llvm::orc::ExecutorSymbolDef> lookup(llvm::StringRef name)
    {
        return exeSession->lookup({&jitLib}, mangler(name.str()));
    }
};

struct Scaffold
{
    std::string name;
    std::unique_ptr<llvm::LLVMContext> theContext;
    std::unique_ptr<llvm::Module> theModule;
    llvm::IRBuilder<> builder;
    llvm::FunctionPassManager theFPM;
    llvm::LoopAnalysisManager theLAM;
    llvm::FunctionAnalysisManager theFAM;
    llvm::CGSCCAnalysisManager theCGAM;
    llvm::ModuleAnalysisManager theMAM;
    llvm::PassInstrumentationCallbacks thePIC;
    llvm::StandardInstrumentations theSI;

    Scaffold(void *parent);
};

// Some defines to reduce boilerplate

// Data types
#define TyVoid   (Type::getVoidTy(*scaff->theContext))
#define TyInt16  (Type::getInt16Ty(*scaff->theContext))
#define TyInt32  (Type::getInt32Ty(*scaff->theContext))
#define TyInt64  (Type::getInt64Ty(*scaff->theContext))
#define TyDouble (Type::getDoubleTy(*scaff->theContext))
#define TyAddr   (PointerType::get(*scaff->theContext, 0))
#define TyBool   (Type::getInt1Ty(*scaff->theContext))

// Data value constants
#define CoInt16(VAL)  (ConstantInt::get(*scaff->theContext, APInt(16, (uint16_t)(VAL))))
#define CoInt32(VAL)  (ConstantInt::get(*scaff->theContext, APInt(32, (uint32_t)(VAL))))
#define CoInt64(VAL)  (ConstantInt::get(*scaff->theContext, APInt(64, (uint64_t)(VAL))))
#define CoDouble(VAL) (ConstantFP::get(*scaff->theContext, APFloat((VAL))))
#define CoAddr(VAL)   (scaff->builder.CreateIntToPtr(CoInt64(VAL), TyAddr))
#define CoBool(VAL)   (ConstantInt::get(*scaff->theContext, APInt(1, VAL)))

// Parameter combinations
#define PaInt16(VAL)  {TyInt16, (VAL)}
#define PaInt32(VAL)  {TyInt32, (VAL)}
#define PaInt64(VAL)  {TyInt64, (VAL)}
#define PaDouble(VAL) {TyDouble, (VAL)}
#define PaAddr(VAL)   {TyAddr, (VAL)}
#define PaBool(VAL)   {TyBool, (VAL)}

#endif // COMPILER_PRIVATE_H
