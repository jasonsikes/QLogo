#ifndef EVALUATOR_H
#define EVALUATOR_H

//===-- qlogo/evaluator.h - Evaluator class definition -------*- C++ -*-===//
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
/// This file contains the declarations of the Evaluator class, which
/// handles the evaluation of a list in qlogo.
///
//===----------------------------------------------------------------------===//

#include "compiler_types.h"
#include "datum_ptr.h"


/// @brief The Evaluator object handles the evaluation of a list.
///
/// The evaluator handles the evaluation of a list. It provides support functionality
/// for the list while it is executing.
struct NewEvaluator
{
    // The members

    /// @brief The list to evaluate.
    DatumPtr list;

    /// @brief The pointer to this list's compiled function.
    CompiledFunctionPtr fn;

    /// @brief The coroutine handle for this evaluation.
    /// @note Execution may resume until this value is nullptr.
    coroutine_handle_t handle = nullptr;

    /// @brief The return value of this evaluation.
    Datum *retval = nullptr;

    /// @brief A pool of objects for garbage collection.
    /// @note This is the reason the copy operators are deleted. If we enable them, the
    ///  releasePool can be copied, and both copies will contain pointers to the same Datum objects.
    ///  This will cause the Datum objects to be deleted twice.
    std::vector<Datum *> releasePool;

    // The methods

    /// @brief Constructor.
    /// @param aList The list to evaluate.
    NewEvaluator(const DatumPtr &aList);

    /// @brief Destructor.
    ~NewEvaluator();

    /// @brief Begin or resume execution of this list. Will return when execution is complete or suspended.
    /// @param jumpLocation The block number to start execution from when initiating execution.
    /// @return true if execution is complete, false if suspended.
    bool exec(int32_t jumpLocation = 0);

    /// @brief Execute the given sublist. Will return when execution is complete.
    /// @param aList The list to execute.
    /// @return the result of this execution.
    Datum *subExec(Datum *aList);

    /// @brief Execute the given procedure. Will return when execution is complete.
    /// @param node The ASTNode of the procedure to execute.
    /// @param paramAry The parameters to apply to the procedure.
    /// @param paramCount The number of parameters to apply.
    /// @return the result of this execution.
    Datum *procedureExec(ASTNode *node, Datum **paramAry, uint32_t paramCount);

    /// @brief Add a Datum to the release pool
    /// @return the given pointer (pass-through).
    /// In other areas of code, memory management is handled by using the DatumPtr class.
    /// Among other things, the DatumPtr acts like std::shared_ptr. As long as there exists at least one DatumPtr
    /// pointing to a Datum, the Datum will not be deleted. In compiled code, we use the watch function to add a Datum
    /// to the release pool. The release pool acts as an array of DatumPtrs. When the Evaluator is destroyed, it will
    /// iterate through the release pool and decrement the retain count of each Datum. If any retain count reaches 0,
    /// the Datum will be deleted.
    Datum *watch(Datum *);

    /// @brief Add a Datum to the release pool
    /// @return the given pointer (pass-through).
    Datum *watch(const DatumPtr &);

    /// @brief Returns TRUE if CASEIGNOREDP is TRUE
    bool varCASEIGNOREDP();

    NewEvaluator() = delete;
    NewEvaluator(const NewEvaluator &) = delete;
    NewEvaluator(NewEvaluator &&) = delete;
    NewEvaluator &operator=(const NewEvaluator &) = delete;
    NewEvaluator &operator=(NewEvaluator &&) = delete;
};

#endif // EVALUATOR_H
