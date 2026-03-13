#ifndef WORKSPACE_JIT_EXPORT_TRAITS_H
#define WORKSPACE_JIT_EXPORT_TRAITS_H

//===-- workspace/jit_export_traits.h - C to LLVM type mapping -------*- C++ -*-===//
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
/// Traits to map C/C++ types used in JIT-exported functions to their
/// corresponding LLVM IR types. Used for single-source-of-truth verification
/// when calling extern functions from generated LLVM code.
///
//===----------------------------------------------------------------------===//

#include "compiler_types.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>

/// Maps a C/C++ type to its LLVM Type.
template <typename T>
struct CTypeToLLVM
{
    static llvm::Type *get(llvm::LLVMContext &ctx);
};

template <>
struct CTypeToLLVM<void>
{
    static llvm::Type *get(llvm::LLVMContext &ctx)
    {
        return llvm::Type::getVoidTy(ctx);
    }
};

template <>
struct CTypeToLLVM<bool>
{
    static llvm::Type *get(llvm::LLVMContext &ctx)
    {
        return llvm::Type::getInt1Ty(ctx);
    }
};

template <>
struct CTypeToLLVM<int8_t>
{
    static llvm::Type *get(llvm::LLVMContext &ctx)
    {
        return llvm::Type::getInt8Ty(ctx);
    }
};

template <>
struct CTypeToLLVM<int16_t>
{
    static llvm::Type *get(llvm::LLVMContext &ctx)
    {
        return llvm::Type::getInt16Ty(ctx);
    }
};

template <>
struct CTypeToLLVM<int32_t>
{
    static llvm::Type *get(llvm::LLVMContext &ctx)
    {
        return llvm::Type::getInt32Ty(ctx);
    }
};

template <>
struct CTypeToLLVM<uint32_t>
{
    static llvm::Type *get(llvm::LLVMContext &ctx)
    {
        return llvm::Type::getInt32Ty(ctx);
    }
};

template <>
struct CTypeToLLVM<int64_t>
{
    static llvm::Type *get(llvm::LLVMContext &ctx)
    {
        return llvm::Type::getInt64Ty(ctx);
    }
};

template <>
struct CTypeToLLVM<double>
{
    static llvm::Type *get(llvm::LLVMContext &ctx)
    {
        return llvm::Type::getDoubleTy(ctx);
    }
};

template <>
struct CTypeToLLVM<addr_t>
{
    static llvm::Type *get(llvm::LLVMContext &ctx)
    {
        return llvm::PointerType::get(ctx, 0);
    }
};

template <typename T>
struct CTypeToLLVM<T *>
{
    static llvm::Type *get(llvm::LLVMContext &ctx)
    {
        return llvm::PointerType::get(ctx, 0);
    }
};

#endif // WORKSPACE_JIT_EXPORT_TRAITS_H
