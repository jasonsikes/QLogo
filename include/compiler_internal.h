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
#include <type_traits>

struct Scaffold
{
    // The name of the function.
    std::string name_;

    std::unique_ptr<llvm::LLVMContext> theContext_;
    std::unique_ptr<llvm::Module> theModule_;
    llvm::IRBuilder<> builder_;
    llvm::FunctionPassManager theFPM_;
    llvm::LoopAnalysisManager theLAM_;
    llvm::FunctionAnalysisManager theFAM_;
    llvm::CGSCCAnalysisManager theCGAM_;
    llvm::ModuleAnalysisManager theMAM_;
    llvm::PassInstrumentationCallbacks thePIC_;
    llvm::StandardInstrumentations theSI_;
    llvm::Function *theFunction_;

    // A list of basic blocks that make up the "cold path" of execution.
    QList<llvm::BasicBlock *> coldPathBlocks_;

    Scaffold(const llvm::DataLayout &dataLayout);

    // a pointer to the Evaluator object that supports execution of the compiled function.
    llvm::Value *evaluator_;

    // a pointer to the return value address argument of the compiled function.
    // Instead of returning a value, the compiled function returns a coroutine handle
    // So we store the return value here at this address.
    llvm::Value *returnValueAddress_;

    // a pointer to the block ID argument of the compiled function.
    // Since the function can be called immediately after a GOTO and the TAG can appear anywhere,
    // we separate any code that occurs before and after a TAG into different blocks.
    llvm::Value *blockId_;

    // The fourth argument: resume handle (null = initial entry, non-null = resume from that state).
    llvm::Value *resumeHandle_;

    // The suspend and cleanup blocks for the coroutine (null when function never suspends).
    llvm::BasicBlock *suspendBB_ = nullptr;
    llvm::BasicBlock *cleanupBB_ = nullptr;

    // The main bailout block. All "bailout" operations should branch here.
    // Note: be sure to store the return value in the return value address.
    llvm::BasicBlock *mainBailoutBB_ = nullptr;

    // The coroutine handle for the compiled function, created at the beginning and returned at the end.
    llvm::Value *coroutineHandle_ = nullptr;

    // The coroutine id token from llvm.coro.id, needed in cleanup for llvm.coro.free.
    llvm::Value *coroutineToken_ = nullptr;

    // When set, the coroutine frame is emitted in this block at the start of the function (before TOC
    // if present). generateTOC redirects its branch to Toc when the table of contents is generated.
    llvm::BasicBlock *coroPrologueBB_ = nullptr;

    // create a new basic block with the given name.
    // The new block is inserted after the last block of the function.
    llvm::BasicBlock *createBasicBlock(const std::string &name);

    // add one or more basic blocks to the cold path.
    template <typename... Blocks>
    std::enable_if_t<(std::is_same_v<Blocks, llvm::BasicBlock> && ...)>
    addColdPathBlocks(Blocks *...blocks) {
        (coldPathBlocks_.append(blocks), ...);
    }
};

// Some defines to reduce boilerplate

// Data types
#define TyVoid   (Type::getVoidTy(*scaff->theContext_))
#define TyInt8   (Type::getInt8Ty(*scaff->theContext_))
#define TyInt16  (Type::getInt16Ty(*scaff->theContext_))
#define TyInt32  (Type::getInt32Ty(*scaff->theContext_))
#define TyInt64  (Type::getInt64Ty(*scaff->theContext_))
#define TyDouble (Type::getDoubleTy(*scaff->theContext_))
#define TyAddr   (PointerType::get(*scaff->theContext_, 0))
#define TyBool   (Type::getInt1Ty(*scaff->theContext_))

// Data value constants
#define CoInt8(VAL)  (ConstantInt::get(*scaff->theContext_, APInt(8, (uint8_t)(VAL))))
#define CoInt16(VAL)  (ConstantInt::get(*scaff->theContext_, APInt(16, (uint16_t)(VAL))))
#define CoInt32(VAL)  (ConstantInt::get(*scaff->theContext_, APInt(32, (uint32_t)(VAL))))
#define CoInt64(VAL)  (ConstantInt::get(*scaff->theContext_, APInt(64, (uint64_t)(VAL))))
#define CoDouble(VAL) (ConstantFP::get(*scaff->theContext_, APFloat((VAL))))
#define CoAddr(VAL)   (ConstantExpr::getIntToPtr(CoInt64(VAL), TyAddr))
#define CoBool(VAL)   (ConstantInt::get(*scaff->theContext_, APInt(1, VAL)))

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
