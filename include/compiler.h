#ifndef COMPILER_H
#define COMPILER_H

//===-- qlogo/compiler.h - Compiler class definition -------*- C++ -*-===//
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
/// This file contains the declarations of the Compiler class, which
/// compiles an ASTList into LLVM JIT code.
///
//===----------------------------------------------------------------------===//

#include "llvm/IR/DerivedTypes.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "datum.h"

class Scaffold;

namespace llvm
{
    class Value;
    class AllocaInst;
};

/// @brief A function that validates a value.
/// @param parent The parent ASTNode of the value to validate.
/// @param value The value to validate.
/// @return The result of a comparison operation, e.g. CreateICmpEQ().
/// @note The retval of this function will immediately be passed to CreateCondBr().
typedef std::function<llvm::Value *(llvm::Value *)> validatorFunction;


class Compiler
{
    // Scaffold is a collection of objects needed to compile a JIT module.
    Scaffold *scaff;

    // a pointer to the Evaluator object that supports execution of the compiled function.
    llvm::Value *evaluator;

    // a pointer to the block ID argument of the compiled function.
    llvm::Value *blockId;

    // The hash table of compiled texts referenced by lists or ASTNodes.
    static QHash<Datum *, std::shared_ptr<CompiledText> > compiledTextTable;

    // Child node generation.

    // Generate code for all children of the given node and cast them to the requested data type.
    std::vector<llvm::Value *> generateChildren(ASTNode *node, RequestReturnType);

    // Generate code for all children of the given node and cast them to the requested data types.
    std::vector<llvm::Value *> generateChildren(ASTNode *node, std::vector<RequestReturnType>);

    // Generate code for all children of the given node and cast them to the requested data type.
    llvm::AllocaInst *generateChildrenAlloca(ASTNode *node, RequestReturnType, const std::string &name = "");

    // Generate code to save a vector of values to an alloca array.
    llvm::AllocaInst *generateAllocaAry(const std::vector<llvm::Value *> &values, const std::string &name = "");

    // Glue to ensure requested data type matches the type returned from child
    llvm::Value *generateCast(llvm::Value *child, ASTNode *parent, DatumPtr, RequestReturnType);

    // Generate code for a child node and cast it to the requested data type.
    llvm::Value *generateChild(ASTNode *parent, DatumPtr, RequestReturnType);

    // Generate code for the child node at the given index and cast it to the requested data type.
    llvm::Value *generateChild(ASTNode *parent, unsigned int index, RequestReturnType);


    // Generate a call to an external function
    llvm::Value *generateCallExtern(llvm::Type *returnType,
                               std::string name,
                               const std::vector<std::pair<llvm::Type *, llvm::Value *>> &args);

    // Generate a query to return a datum type (isa) of a given object.
    llvm::Value *generateGetDatumIsa(llvm::Value *objAddr);

    // Generate a call to a child node
    llvm::Value *generateChildOfNode(ASTNode *parent, DatumPtr, RequestReturnType);

    // Generate a void return value using the ASTNode to represent the source (for blame).
    llvm::Value *generateVoidRetval(DatumPtr node);

    // Generate a call to execute a list.
    llvm::Value *generateCallList(llvm::Value *list, RequestReturnType returnType);

    // Generate a call to return a value immediately.
    llvm::Value *generateImmediateReturn(llvm::Value *retval);

    // Generate a call to construct error: SYSTEM
    llvm::Value *generateErrorSystem();

    // Generate a call to construct error: "X doesn't like Y as input"
    llvm::Value *generateErrorNoLike(ASTNode *who, llvm::Value *what);

    // Generate a call to construct error: "You don't say what to do with X"
    llvm::Value *generateErrorNoSay(llvm::Value *what);

    // Generate a call to construct error: "X without TEST"
    llvm::Value *generateErrorNoTest(llvm::Value *who);

    // Generate a call to construct error: "X didn't output to Y"
    llvm::Value *generateErrorNoOutput(llvm::Value *x, ASTNode *y);

    // Generate a call to construct error: "X has no value"
    llvm::Value *generateErrorNoValue(llvm::Value *what);

    // Generate a call to construct error: "Not enough inputs to X"
    llvm::Value *generateErrorNotEnoughInputs(ASTNode *x);

    // Validated conversion generators

    llvm::Value *generateValidationDouble(ASTNode *parent, llvm::Value *val, validatorFunction validator);

    llvm::Value *generateValidationDatum(ASTNode *parent, llvm::Value *val, validatorFunction validator);

    // Generate a call to convert a double to a Word object
    llvm::Value *generateWordFromDouble(llvm::Value *val);

    // Generate a call to convert a boolean to a Word object
    llvm::Value *generateWordFromBool(llvm::Value *val);

    // Convert a Datum pointer to a number.
    // Emit return "doesn't like" error if conversion not possible.
    llvm::Value *generateDoubleFromDatum(ASTNode *parent, llvm::Value *src);

    // Convert a double to an int32.
    // Emit return "doesn't like" error if conversion not possible.
    llvm::Value *generateInt32FromDouble(ASTNode *parent, llvm::Value *src, bool isSigned);

    // Convert a Datum pointer to a boolean.
    // Emit return "doesn't like" error if conversion not possible.
    llvm::Value *generateBoolFromDatum(ASTNode *parent, llvm::Value *src);

    // Validate that given double is not zero.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateNotZeroFromDouble(ASTNode *parent, llvm::Value *src);

    // Validate that given double is greater than or equal to zero.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateNotNegativeFromDouble(ASTNode *parent, llvm::Value *src);

    // Convert a double to an int32 and validate that it is greater than or equal to zero.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateNotNegativeInt32FromDouble(ASTNode *parent, llvm::Value *src);

    // Convert a double to an int32 and validate that it is not equal to zero.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateNotZeroInt32FromDouble(ASTNode *parent, llvm::Value *src);

    // Validate that given double is greater than zero.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateGTZeroFromDouble(ASTNode *parent, llvm::Value *src);

    // Validate that Datum is Word.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateWordFromDatum(ASTNode *parent, llvm::Value *src);

    // Validate that Datum is List.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateListFromDatum(ASTNode *parent, llvm::Value *src);

    // Validate that Datum is Array.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateArrayFromDatum(ASTNode *parent, llvm::Value *src);

    // Common methodology for the generate*FromDatum() methods.
    llvm::Value *generateFromDatum(Datum::DatumType t, ASTNode *parent, llvm::Value *src);

    // Ensure that Datum is a Word or List, AND
    // it is not empty.
    llvm::Value *generateNotEmptyWordOrListFromDatum(ASTNode *parent, llvm::Value *src);

    // Ensure that Datum is a List, AND
    // it is not empty.
    llvm::Value *generateNotEmptyListFromDatum(ASTNode *parent, llvm::Value *src);

    // Ensure that the Datum is NOT nothing (NOT ASTNode).
    // Emit return "didn't output" error if not.
    llvm::Value *generateNotNothingFromDatum(ASTNode *parent, llvm::Value *src);

    // Ensure that the Datum is nothing (ASTNode).
    // Emit return "don't say" error if not.
    llvm::Value *generateNothingFromDatum(ASTNode *parent, llvm::Value *src);

    // Common methodology for the genAnd()/genOr() methods.
    // @param isAnd true if AND, false if OR
    llvm::Value *generateAndOr(DatumPtr node, RequestReturnType returnType, bool isAnd);

    // Get the compiled function pointer for a list of ASTNodes.
    CompiledFunctionPtr generateFunctionPtrFromASTList(QList<QList<DatumPtr>> parsedList, Datum *key);

    // Generate a number array from a datum.
    llvm::AllocaInst *generateNumberAryFromDatum(ASTNode *parent, llvm::Value *src);

    // Generate a number array from a datum with specified size.
    llvm::AllocaInst *generateNumberAryFromDatum(ASTNode *parent, DatumPtr srcPtr, int32_t size);

    // Get the tag name from a tag node.
    QString getTagNameFromNode(DatumPtr node);

    // Process the tags in a block of ASTNodes.
    void setTagToBlockIdInProcedure(QList<DatumPtr> tagList, int32_t blockId);

    // Generate the TagId-to-Block Table of Contents.
    llvm::BasicBlock *generateTOC(QList<llvm::BasicBlock *> blocks, llvm::Function *theFunction);

  public:
    Compiler();
    ~Compiler();

    /// Get the compiled function pointer for a list.
    CompiledFunctionPtr functionPtrFromList(List *aList);

    /// Get the compiled function pointer for an ASTNode.
    CompiledFunctionPtr functionPtrFromASTNode(ASTNode *aNode);

    /// Destroy the compiled text for a datum (either a List or an ASTNode).
    static void destroyCompiledTextForDatum(Datum *aDatum);

    // The generators for the different ASTNodes. Since this list changes often during development
    // and it needs to be consistent across several files, we keep the master list in the compiler
    // implementation files. Here, primitives.h is one of the generated files.
#include <primitives.h>

    // Generate a noop expression.
    llvm::Value *genNoop(DatumPtr node, RequestReturnType returnType);


    llvm::Value *genValueOf(DatumPtr node, RequestReturnType returnType);
    llvm::Value *genLiteral(DatumPtr node, RequestReturnType returnType);

    llvm::Value *genExecProcedure(DatumPtr node, RequestReturnType returnType);
};

#endif // COMPILER_H
