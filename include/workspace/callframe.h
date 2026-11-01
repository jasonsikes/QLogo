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

#include <QHash>
#include <QList>

struct Evaluator;
struct FCGoto;
struct ASTNode;

/// @brief The CallFrame object holds the state of execution of a procedure (or REPL).
/// @note The state includes named variables and the test state (for TEST, IFTRUE, IFFALSE).
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

    /// @brief True when this frame was entered by a macro call.
    /// @details Stays true across tail continuations so the final output is still expanded
    /// in the macro's caller. A tail call to a macro is not a continuation; see
    /// Compiler::generateProcedureExit.
    bool isMacroCall_ = false;

    /// @brief True when this macro call's output must be a single expression.
    bool macroExpectsSingleExpression_ = false;

    /// @brief True when the macro output should replace the caller's suspended line.
    /// @details Used for a macro in tail position so recursive expansions do not grow the
    /// evaluation stack.
    bool macroTailSplice_ = false;

    /// @brief True when a STOP tail-call required this frame not to output.
    /// @details Sticky across later OUTPUT/.MAYBEOUTPUT continuations on the same
    /// frame. A value that would become the caller's output is "You don't say".
    bool demandNothing_ = false;

    /// @brief True when no output is acceptable from this frame.
    bool allowNothing_ = false;

    /// @brief True when the caller of this procedure requires a value.
    /// @details Set when the frame is entered, from beginProcedure or from a nested
    /// continuation. .MAYBEOUTPUT uses it so a missing input is "didn't output to
    /// .maybeoutput" inside this procedure when the caller wants a value, and acts
    /// as STOP when the caller does not. A STOP tail call clears it. Other
    /// same-frame continuations leave it unchanged.
    bool outputRequired_ = false;

    /// @brief This frame realizes a continuation that a parent evaluator is waiting on.
    /// @details A tail call inside CATCH, REPEAT, RUN, or any other nested list must
    /// not replace the waiting procedure. The frame's result is delivered back to
    /// that evaluator. Preserved across applyContinuation.
    bool nestedContinuation_ = false;

    /// @brief OUTPUT, STOP, or .MAYBEOUTPUT node of the continuation this frame is running.
    /// @details For a nested frame, this is the source of the FCReturn delivered to the
    /// waiting evaluator and is not replaced by a later tail call. For a same-frame
    /// tail call, this is the latest command. Together with demandNothing_ and
    /// allowNothing_ it is the continuation's output contract: OUTPUT leaves both
    /// flags clear, so a missing value is "didn't output".
    DatumPtr outputSource_;

    /// @brief The current argument being processed (currentParameterName_ and optional default value).
    DatumPtr currentArgument_;

    /// @brief The interned name of the current parameter being processed.
    /// @details Null when no parameter default is being evaluated.
    Word *currentParameterName_ = nullptr;

    /// @brief Remaining procedure body, including the line currently executing.
    /// @details The head is the current (or next) line. The list advances to its
    /// tail when that line completes.
    DatumPtr runningSourceList_;

    /// @brief Procedure body line that most recently finished in this frame.
    /// @details Nothing until a body line completes. May be the source of an error, if one occurs.
    DatumPtr lastBodyLine_;

    /// @brief Set this value to set a jump location within a line.
    int32_t jumpLocation_ = 0;

    /// @brief The test state for TEST/IFTRUE/IFFALSE.
    /// @note 0 = not tested, 3 = tested true, 2 = tested false.
    int8_t testState_ = 0;

    /// @brief Interned variable names held in this scope and the values held prior to this scope.
    QHash<Word *, DatumPtr> localVars_;

    /// @brief Add an evaluator to the evaluation stack.
    /// @param aList The list to evaluate.
    void pushEvaluator(const DatumPtr &aList);

    /// @brief Pop the topmost evaluator from the evaluation stack.
    void popEvaluator();

    /// @brief Get the size of the evaluation stack.
    /// @return The size of the evaluation stack.
    size_t evaluationStackSize() const;

    /// @brief Return the topmost Evaluator object on the global stack.
    /// @details This is the current frame's top evaluator. A caller frame still
    /// has its own evaluators below the callee; use topOwnEvaluator() for those.
    /// @return The topmost Evaluator object.
    Evaluator *topEvaluator();

    /// @brief Return the top evaluator owned by this frame, or nullptr if it has none.
    /// @details Evaluators for this frame run from its start index up to the next
    /// frame's start index, or to the end of the stack when this frame is current.
    Evaluator *topOwnEvaluator();

    /// @brief Make 'name' local with no value, like LOCAL.
    /// @param name Any word for the variable. Stored under Word::interned().
    /// @details No-op if this frame already localized 'name'. Otherwise the same
    /// as bindLocal(name, nothing()).
    void setVarAsLocal(Word *name);

    /// @brief Make 'name' local if it is not already, then store 'value'.
    /// @param name Any word for the variable. Stored under Word::interned().
    /// @param value The value to store.
    /// @details Saves the previous global binding the first time this frame sees
    /// 'name'. Writes 'value' once. Duplicate names in this frame keep the
    /// original saved value.
    void bindLocal(Word *name, const DatumPtr &value);

    /// @brief End the current procedure by continuing with the given node and parameters.
    /// @param node The ASTNode of the new procedure to continue with.
    /// @param arguments The linked list of arguments to apply to the new node.
    /// @param demandNothing If true, OR demandNothing_ onto the frame (STOP TCO).
    /// @param allowNothing STOP or .MAYBEOUTPUT. OUTPUT passes false: the continued
    /// procedure must produce a value.
    /// @param commandNode OUTPUT, STOP, or .MAYBEOUTPUT. Remembered when this is OUTPUT.
    void applyContinuation(
        ASTNode *node, const DatumPtr &arguments, bool demandNothing, bool allowNothing, const DatumPtr &commandNode);

    /// @brief Record the output contract of a tail call on this frame.
    /// @param demandNothing STOP. Sticks on, and clears outputRequired_.
    /// @param allowNothing STOP or .MAYBEOUTPUT. A missing value is allowed. Sticks on.
    /// @param nested This frame was just created for a continuation a parent evaluator
    /// is waiting on. Sets nestedContinuation_ and outputRequired_. outputSource_ then
    /// stays put across later tail calls.
    /// @param callerRequiresValue The value for outputRequired_ when @p nested is true
    /// and this is not STOP. Same rule as beginProcedure.
    /// @param outputSource OUTPUT, STOP, or .MAYBEOUTPUT. Stored for a new nested frame,
    /// and for a same-frame tail call that is not already nested.
    void installContinuation(
        bool demandNothing, bool allowNothing, bool nested, bool callerRequiresValue, const DatumPtr &outputSource);

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
