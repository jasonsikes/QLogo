#ifndef CALLFRAME_H
#define CALLFRAME_H

//===-- qlogo/callframe.h - CallFrame class definition -------*- C++ -*-===//
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
/// This file contains the declarations of the CallFrame class, which
/// handles the execution state of a procedure in qlogo. An execution state
/// includes the local variables, the evaluation stack, and the call frame
/// stack.
///
//===----------------------------------------------------------------------===//

#include "datum_ptr.h"

#include <QList>
#include <QHash>

struct Evaluator;
struct FCGoto;
struct ASTNode;


/// @brief The CallFrame object holds the state of execution of a procedure (or REPL).
/// @note The state includes named variables, anonymous variables (explicit slot, or
/// "?"), and the test state (for TEST, IFTRUE, IFFALSE).
struct CallFrame
{
    /// @brief The index of the first evaluator in the evaluation stack.
    std::size_t evaluationStackStartIndex_ = 0;

    /// @brief The list of parameters being processed.
    DatumPtr parameters_;

    /// @brief The list of arguments being processed.
    DatumPtr arguments_;

    /// @brief The ASTNode source of this running procedure.
    DatumPtr sourceNode_;

    /// @brief The return value to be passed to the parent procedure.
    DatumPtr retvalToParent_;

    /// @brief Set to true iff we are reading arguments for a procedure.
    bool isReadingArgs_ = false;

    /// @brief The current argument being processed (currentParameterName_ and optional default value).
    DatumPtr currentArgument_;

    /// @brief The name of the current parameter being processed.
    QString currentParameterName_;

    /// @brief The current source list being executed.
    /// The head of this list is removed when its execution begins.
    /// This list then contains the lines remaining to be executed.
    DatumPtr runningSourceList_;

    /// @brief Set this value to set a jump location within a line.
    int32_t jumpLocation_ = 0;

    /// @brief The test state for TEST/IFTRUE/IFFALSE.
    /// @note 0 = not tested, 3 = tested true, 2 = tested false.
    int8_t testState_ = 0;

    /// @brief The explicit slot list, placeholders for "?".
    /// @note This is for the "explicit slot" APPLY command.
    DatumPtr explicitSlotList_;

    /// @brief Variable names held in this scope and the values held prior to the invocation of this scope.
    QHash<QString, DatumPtr> localVars_;

    /// @brief Add an evaluator to the evaluation stack.
    /// @param aList The list to evaluate.
    void pushEvaluator(const DatumPtr &aList);

    /// @brief Pop the topmost evaluator from the evaluation stack.
    void popEvaluator();

    /// @brief Get the size of the evaluation stack.
    /// @return The size of the evaluation stack.
    size_t evaluationStackSize() const;

    /// @brief Return the topmost Evaluator object.
    /// @return The topmost Evaluator object.
    Evaluator *topEvaluator();

    /// @brief Insert an entry for 'name' in the variables hash. Save the previous value
    /// of the variable in the localVars hash. Store 'nothing' for the entry if name wasn't
    /// already present.
    /// @param name The name of the variable to insert.
    void setVarAsLocal(const QString &name);

    /// @brief End the current procedure by continuing with the given node and parameters.
    /// @param node The ASTNode of the new procedure to continue with.
    /// @param arguments The linked list of arguments to apply to the new node.
    void applyContinuation(ASTNode *node, const DatumPtr &arguments);

    /// @brief return true if the evaluation stack is empty.
    /// @return True if the evaluation stack is empty, false otherwise.
    bool isEvaluationStackViewEmpty() const;

    /// @brief Get the evaluator at the given index.
    /// @param index The index of the evaluator to get.
    /// @return The evaluator at the given index.
    Evaluator *evaluatorAtIndex(std::size_t index);

    /// @brief Constructor.
    /// @param aSourceNode The ASTNode source of this running procedure. 'nothing'
    /// is reserved for the global frame or PAUSE.
    explicit CallFrame(ASTNode *node, Datum **paramAry, uint32_t paramCount);

    /// @brief Destructor. Removes local variables from the frame stack variables hash,
    /// and restores the original values of the variables.
    /// @note This frame will be removed from the stack.
    ~CallFrame();

    CallFrame() = delete;
    CallFrame(const CallFrame &) = delete;
    CallFrame(CallFrame *) = delete;
    CallFrame(CallFrame &&) = delete;
    CallFrame &operator=(const CallFrame &) = delete;
    CallFrame &operator=(CallFrame &&) = delete;
};


#endif // CALLFRAME_H
