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

#include <llvm/IR/Attributes.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>

#include <type_traits>

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
        return llvm::Type::getInt8Ty(ctx);
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

enum class CAbiExt
{
    None,
    Zero,
    Sign
};

template <typename T>
struct CTypeABI
{
    static constexpr bool isBool = false;
    static constexpr CAbiExt ext = CAbiExt::None;
};

template <>
struct CTypeABI<bool>
{
    static constexpr bool isBool = true;
    static constexpr CAbiExt ext = CAbiExt::Zero;
};

template <>
struct CTypeABI<int8_t>
{
    static constexpr bool isBool = false;
    static constexpr CAbiExt ext = CAbiExt::Sign;
};

template <>
struct CTypeABI<int16_t>
{
    static constexpr bool isBool = false;
    static constexpr CAbiExt ext = CAbiExt::Sign;
};

template <>
struct CTypeABI<int32_t>
{
    static constexpr bool isBool = false;
    static constexpr CAbiExt ext = CAbiExt::Sign;
};

template <>
struct CTypeABI<uint32_t>
{
    static constexpr bool isBool = false;
    static constexpr CAbiExt ext = CAbiExt::Zero;
};

template <typename T>
void addCAbiAttrsAtIndex(llvm::LLVMContext &ctx, llvm::AttributeList &al, unsigned index, bool extI32)
{
    if constexpr (std::is_void_v<T>)
        return;

    llvm::AttrBuilder b(ctx);
    b.addAttribute(llvm::Attribute::NoUndef);
    constexpr CAbiExt ext = CTypeABI<T>::ext;
    if constexpr (ext != CAbiExt::None)
    {
        constexpr bool isI32 = std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>;
        if (!isI32 || extI32)
            b.addAttribute(ext == CAbiExt::Zero ? llvm::Attribute::ZExt : llvm::Attribute::SExt);
    }
    al = al.addAttributesAtIndex(ctx, index, b);
}

/// noundef on every non-void; zeroext/signext for sub-register integers.
/// i32 extension is AArch64-only (extI32).
template <typename Ret, typename... Args>
llvm::AttributeList hostExportAttributeList(llvm::LLVMContext &ctx, bool extI32)
{
    llvm::AttributeList al;
    addCAbiAttrsAtIndex<Ret>(ctx, al, llvm::AttributeList::ReturnIndex, extI32);
    unsigned i = llvm::AttributeList::FirstArgIndex;
    (addCAbiAttrsAtIndex<Args>(ctx, al, i++, extI32), ...);
    return al;
}

#endif // WORKSPACE_JIT_EXPORT_TRAITS_H
