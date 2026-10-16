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

#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/EarlyCSE.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Scalar/SROA.h"

#include <memory>
#include <string>
#include <type_traits>

namespace llvm
{
class FunctionType;
class LLVMContext;
class StringRef;
class TargetMachine;
} // namespace llvm

/// LLVM FunctionType for an EXPORTC host function interned by name.
llvm::FunctionType *hostExportFunctionType(llvm::LLVMContext &ctx, llvm::StringRef name);

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
    llvm::PassBuilder thePB_;
    llvm::Function *theFunction_;

    // A list of basic blocks that make up the "cold path" of execution.
    QList<llvm::BasicBlock *> coldPathBlocks_;

    Scaffold(const llvm::DataLayout &dataLayout, llvm::TargetMachine *tm);

    /// @brief True when IR value names should be prefixed with the generator function.
    bool mangleNames_ = false;

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

    // Itanium prefix of Datum: vptr, isa_, retainCount_, alertOnDelete_.
    llvm::StructType *datumHeaderTy_ = nullptr;
    llvm::MDNode *isaLoadTbaa_ = nullptr;

    // Create a new basic block. Inserted before MainBailout when that block
    // exists so the exit stays at the end of the function; otherwise appended.
    llvm::BasicBlock *createBasicBlock(const std::string &name);

    // Alloca in the current entry block so the allocation is static (not a
    // dynamic stack bump each time the emitting block runs).
    llvm::AllocaInst *createEntryBlockAlloca(llvm::Type *type, llvm::Value *arraySize,
                                             const std::string &name = "");

    // After TOC / coro prologue prepend a new entry, move every alloca there
    // so they stay static and dominate all uses (including after suspend).
    void hoistAllocasToEntry();

    // MD_prof on edges into coldPathBlocks_ (cold successor unlikely), cold on
    // non-intrinsic calls in those blocks, then move the blocks to the end.
    void finalizeColdPaths();

    // add one or more basic blocks to the cold path.
    template <typename... Blocks>
    std::enable_if_t<(std::is_same_v<Blocks, llvm::BasicBlock> && ...)>
    addColdPathBlocks(Blocks *...blocks) {
        (coldPathBlocks_.append(blocks), ...);
    }
};

// Some defines to reduce boilerplate

// Data types
#define TyVoid   (Type::getVoidTy(*scaff_->theContext_))
#define TyInt8   (Type::getInt8Ty(*scaff_->theContext_))
#define TyInt16  (Type::getInt16Ty(*scaff_->theContext_))
#define TyInt32  (Type::getInt32Ty(*scaff_->theContext_))
#define TyInt64  (Type::getInt64Ty(*scaff_->theContext_))
#define TyDouble (Type::getDoubleTy(*scaff_->theContext_))
#define TyAddr   (PointerType::get(*scaff_->theContext_, 0))
#define TyBool   (Type::getInt1Ty(*scaff_->theContext_))

// Data value constants
#define CoInt8(VAL)  (ConstantInt::get(*scaff_->theContext_, APInt(8, (uint8_t)(VAL))))
#define CoInt16(VAL)  (ConstantInt::get(*scaff_->theContext_, APInt(16, (uint16_t)(VAL))))
#define CoInt32(VAL)  (ConstantInt::get(*scaff_->theContext_, APInt(32, (uint32_t)(VAL))))
#define CoInt64(VAL)  (ConstantInt::get(*scaff_->theContext_, APInt(64, (uint64_t)(VAL))))
#define CoDouble(VAL) (ConstantFP::get(*scaff_->theContext_, APFloat((VAL))))
#define CoAddr(VAL)   (ConstantExpr::getIntToPtr(CoInt64(VAL), TyAddr))
#define CoBool(VAL)   (ConstantInt::get(*scaff_->theContext_, APInt(1, VAL)))

// Parameter combinations
#define PaInt8(VAL)  {TyInt8, (VAL)}
#define PaInt16(VAL)  {TyInt16, (VAL)}
#define PaInt32(VAL)  {TyInt32, (VAL)}
#define PaInt64(VAL)  {TyInt64, (VAL)}
#define PaDouble(VAL) {TyDouble, (VAL)}
#define PaAddr(VAL)   {TyAddr, (VAL)}
#define PaBool(VAL)   {TyBool, (VAL)}

// Debug name mangler: prefix with enclosing C++ function name (e.g. "tocCond" -> "generateTOC_tocCond")
std::string dbgName(const char *enclosing, const char *name);
#define DBG_NAME(name) (::dbgName(__func__, (name)))

// Some operations only work in the GUI mode. For the CLI mode, we return an error.
#define CHECK_GRAPHICS_MODE \
    if (Config::get().hasGUI_ == false) \
    { \
        return generateImmediateReturn(generateErrorNoGraphics()); \
    }


#endif // COMPILER_INTERNAL_H
