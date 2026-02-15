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

#include "llvm/IR/Constants.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

#include <memory>

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
    llvm::Function *theFunction;

    Scaffold(const llvm::DataLayout &dataLayout);

    // a pointer to the Evaluator object that supports execution of the compiled function.
    llvm::Value *evaluator;

    // a pointer to the return value address argument of the compiled function.
    // Instead of returning a value, the compiled function returns a coroutine handle
    // So we store the return value here at this address.
    llvm::Value *returnValueAddress;

    // a pointer to the block ID argument of the compiled function.
    // Since the function can be called immediately after a GOTO and the TAG can appear anywhere,
    // we separate any code that occurs before and after a TAG into different blocks.
    llvm::Value *blockId;

    // The fourth argument: resume handle (null = initial entry, non-null = resume from that state).
    llvm::Value *resumeHandle;

    // The suspend and cleanup blocks for the coroutine.
    llvm::BasicBlock *suspendBB;
    llvm::BasicBlock *cleanupBB;

    // The coroutine handle for the compiled function, created at the beginning and returned at the end.
    llvm::Value *coroutineHandle = nullptr;

    // The coroutine id token from llvm.coro.id, needed in cleanup for llvm.coro.free.
    llvm::Value *coroutineToken = nullptr;
};

// Some defines to reduce boilerplate

// Data types
#define TyVoid   (Type::getVoidTy(*scaff->theContext))
#define TyInt8   (Type::getInt8Ty(*scaff->theContext))
#define TyInt16  (Type::getInt16Ty(*scaff->theContext))
#define TyInt32  (Type::getInt32Ty(*scaff->theContext))
#define TyInt64  (Type::getInt64Ty(*scaff->theContext))
#define TyDouble (Type::getDoubleTy(*scaff->theContext))
#define TyAddr   (PointerType::get(*scaff->theContext, 0))
#define TyBool   (Type::getInt1Ty(*scaff->theContext))

// Data value constants
#define CoInt8(VAL)  (ConstantInt::get(*scaff->theContext, APInt(8, (uint8_t)(VAL))))
#define CoInt16(VAL)  (ConstantInt::get(*scaff->theContext, APInt(16, (uint16_t)(VAL))))
#define CoInt32(VAL)  (ConstantInt::get(*scaff->theContext, APInt(32, (uint32_t)(VAL))))
#define CoInt64(VAL)  (ConstantInt::get(*scaff->theContext, APInt(64, (uint64_t)(VAL))))
#define CoDouble(VAL) (ConstantFP::get(*scaff->theContext, APFloat((VAL))))
#define CoAddr(VAL)   (ConstantExpr::getIntToPtr(CoInt64(VAL), TyAddr))
#define CoBool(VAL)   (ConstantInt::get(*scaff->theContext, APInt(1, VAL)))

// Parameter combinations
#define PaInt8(VAL)  {TyInt8, (VAL)}
#define PaInt16(VAL)  {TyInt16, (VAL)}
#define PaInt32(VAL)  {TyInt32, (VAL)}
#define PaInt64(VAL)  {TyInt64, (VAL)}
#define PaDouble(VAL) {TyDouble, (VAL)}
#define PaAddr(VAL)   {TyAddr, (VAL)}
#define PaBool(VAL)   {TyBool, (VAL)}

// Debug name mangler: prefix with enclosing C++ function name (e.g. "tocCond" -> "generateTOC tocCond")
const char *dbgName(const char *enclosing, const char *name);
#define DBG_NAME(name) (::dbgName(__func__, (name)))

#endif // COMPILER_INTERNAL_H
