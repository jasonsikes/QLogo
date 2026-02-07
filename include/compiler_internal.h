#ifndef COMPILER_INTERNAL_H
#define COMPILER_INTERNAL_H

//===-- qlogo/compiler_internal.h - Compiler class definition -------*- C++ -*-===//
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

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

#include <memory>

/// Thin wrapper around LLJIT: owns the JIT and adapts lookup to return ExecutorSymbolDef.
class CompilerContext
{
    std::unique_ptr<llvm::orc::LLJIT> jit;

  public:
    llvm::ExitOnError exitOnErr;

    CompilerContext();

    const llvm::DataLayout &getDataLayout() const;
    llvm::orc::JITDylib &getMainJITDylib();
    llvm::Error addModule(llvm::orc::ThreadSafeModule tsm, llvm::orc::ResourceTrackerSP rt = nullptr);
    llvm::Expected<llvm::orc::ExecutorSymbolDef> lookup(llvm::StringRef name);
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

    Scaffold(const llvm::DataLayout &dataLayout);
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
#define CoAddr(VAL)   (ConstantExpr::getIntToPtr(CoInt64(VAL), TyAddr))
#define CoBool(VAL)   (ConstantInt::get(*scaff->theContext, APInt(1, VAL)))

// Parameter combinations
#define PaInt16(VAL)  {TyInt16, (VAL)}
#define PaInt32(VAL)  {TyInt32, (VAL)}
#define PaInt64(VAL)  {TyInt64, (VAL)}
#define PaDouble(VAL) {TyDouble, (VAL)}
#define PaAddr(VAL)   {TyAddr, (VAL)}
#define PaBool(VAL)   {TyBool, (VAL)}

#endif // COMPILER_INTERNAL_H
