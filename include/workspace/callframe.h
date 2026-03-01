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
#include <memory>
#include <stack>

struct Evaluator;
struct FCGoto;
struct ASTNode;


/// @brief The CallFrame object holds the state of execution of a procedure (or REPL).
/// @note The state includes named variables, anonymous variables (explicit slot, or
/// "?"), and the test state (for TEST, IFTRUE, IFFALSE).
class CallFrame
{
    /// @brief The evaluation stack.
    /// @note This stack is used to store the evaluation state of lists and sublists while they are executing.
    std::stack<std::unique_ptr<Evaluator>> evaluationStack_;

public:

    /// @brief The list of parameters being processed.
    DatumPtr parameters_;

    /// @brief The list of arguments being processed.
    DatumPtr arguments_;

    /// @brief The ASTNode source of this running procedure.
    DatumPtr sourceNode_;

    /// @brief Set to true iff we are reading arguments for a procedure.
    bool isReadingArgs_ = false;

    /// @brief The name of the current parameter being processed.
    QString currentParameterName_;

    /// @brief The current source list being executed.
    /// The head of this list is removed when its execution begins.
    /// This list then contains the lines remaining to be executed.
    DatumPtr runningSourceList_;

    /// @brief Set this value to set a jump location within a line.
    int32_t jumpLocation_ = 0;

    /// @brief Set to true iff a TEST command has occurred.
    /// @note This is for the commands TEST, IFTRUE, and IFFALSE.
    bool isTested_ = false;

    /// @brief This holds the result of the most recent TEST.
    /// @note This is for the commands TEST, IFTRUE, and IFFALSE.
    bool testResult_ = false;

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
    Evaluator *topEvaluator() const
    {
        Q_ASSERT(evaluationStack_.size() > 0);
        return evaluationStack_.top().get();
    }

    /// @brief Insert an entry for 'name' in the variables hash. Save the previous value
    /// of the variable in the localVars hash. Store 'nothing' for the entry if name wasn't
    /// already present.
    /// @param name The name of the variable to insert.
    void setVarAsLocal(const QString &name);

    // /// @brief End the current procedure by continuing with the given node and parameters.
    // /// @param newNode The ASTNode of the new procedure to continue with.
    // /// @param paramAry The parameters to apply to the new node.
    // /// @return nothing if successful, or an error if not.
    // Datum *applyContinuation(const DatumPtr &newNode, const QList<DatumPtr> &paramAry);

    // /// @brief Jump to the line in the procedure containing the given tag.
    // /// @param node The FCGoto node.
    // /// @return Err if the tag is not found (or nothing if the tag is found).
    // Datum *applyGoto(FCGoto *node);

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
    CallFrame(CallFrame &&) = delete;
    CallFrame &operator=(const CallFrame &) = delete;
    CallFrame &operator=(CallFrame &&) = delete;
};


#endif // CALLFRAME_H
