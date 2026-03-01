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

class CallFrame;

/// @brief The Evaluator object handles the evaluation of a list.
///
/// The evaluator handles the evaluation of a list. It provides support functionality
/// for the list while it is executing.
struct Evaluator
{
    /// @brief The call frame containing this evaluator.
    CallFrame *owningFrame_;

    /// @brief The list to evaluate.
    DatumPtr list_;

    /// @brief The pointer to this list's compiled function.
    CompiledFunctionPtr fn_;

    /// @brief The coroutine handle for this evaluation.
    /// @note Execution may resume until this value is nullptr.
    LLVMCoroFrameHeader *handle_ = nullptr;

    /// @brief The return value of this evaluation.
    Datum *retvalToParent_ = Datum::notADatum();

    /// @brief Result of the most recently run sub-list (explicit control: push list, suspend, driver runs it, resume, pop).
    /// Used by getLastEvaluationResult() after a coroutine suspend/resume for RUN/list call.
    DatumPtr retvalFromChild_;

    /// @brief A pool of objects for garbage collection.
    /// @note This is the reason the copy operators are deleted. If we enable them, the
    ///  releasePool can be copied, and both copies will contain pointers to the same Datum objects.
    ///  This will cause the Datum objects to be deleted twice.
    std::vector<Datum *> releasePool_;

    // The methods

    /// @brief Constructor.
    /// @param aList The list to evaluate.
    Evaluator(CallFrame *owningFrame, const DatumPtr &aList);

    /// @brief Destructor.
    ~Evaluator();

    /// @brief Begin or resume execution of this list. Will return when execution is complete or suspended.
    /// @param jumpLocation The block number to start execution from when initiating execution.
    /// @return true if execution is complete, false if suspended and expected to resume.
    bool exec(int32_t jumpLocation = 0);

    /// @brief Push the given sublist onto the evaluation stack and prepare it for execution.
    /// @param aList The list to push onto the evaluation stack.
    void pushSublist(Datum *aList);

    /// @brief Push the given procedure onto the evaluation stack and prepare it for execution.
    /// @param node The ASTNode of the procedure to push.
    /// @param paramAry The parameters to apply to the procedure.
    /// @param paramCount The number of parameters to apply.
    void beginProcedure(ASTNode *node, Datum **paramAry, uint32_t paramCount);

    /// @brief Add a Datum to the release pool
    /// @return the given pointer (pass-through).
    /// In other areas of code, memory management is handled by using the DatumPtr class.
    /// Among other things, the DatumPtr acts like std::shared_ptr. As long as there exists at least one DatumPtr
    /// pointing to a Datum, the Datum will not be deleted. In compiled code, we use the watch function to add a Datum
    /// to the release pool. The release pool acts as an array of DatumPtrs. When the Evaluator is destroyed, it will
    /// iterate through the release pool and decrement the retain count of each Datum that is not the return value.
    ///  If any retain count reaches 0, the Datum will be deleted.
    Datum *watch(Datum *);

    /// @brief Add a Datum to the release pool
    /// @return the given pointer (pass-through).
    Datum *watch(const DatumPtr &);

    /// @brief Returns TRUE if CASEIGNOREDP is TRUE, otherwise FALSE.
    bool varCASEIGNOREDP();

    Evaluator() = delete;
    Evaluator(const Evaluator &) = delete;
    Evaluator(Evaluator &&) = delete;
    Evaluator &operator=(const Evaluator &) = delete;
    Evaluator &operator=(Evaluator &&) = delete;
};

#endif // EVALUATOR_H
