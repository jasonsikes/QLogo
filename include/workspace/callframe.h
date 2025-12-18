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
#include "compiler_types.h"

struct CallFrame;
struct Evaluator;
struct FCGoto;

/// @brief  The call frame stack.
///
/// The call frame stack is the stack of call frames, each representing the state
/// of a procedure. The first of the list is the 'top' call frame. That is,
/// new frames are pushed to the front of the stack, and the oldest, 'global'
/// frame is the last element of the list.
struct CallFrameStack
{
    /// @brief The call frame stack.
    QList<CallFrame *> stack;

    /// @brief The variables hash.
    QHash<QString, DatumPtr> variables;

    /// @brief Repcount is for use in looping functions (e.g. REPEAT)
    double repcount = -1;

    /// @brief Return the value of a variable.
    /// @param name The name of the variable to search for.
    /// @return The stored value associated with 'name' or 'nothing' if the variable is not found.
    DatumPtr datumForName(QString name) const;

    /// @brief Set a value for a variable.
    /// @param aDatum The value to store.
    /// @param name The name of the variable to set.
    void setDatumForName(DatumPtr &aDatum, const QString &name);

    /// @brief Return true if value keyed by name exists in the variables hash.
    /// @param name The name of the variable to search for.
    /// @return True if the variable exists, false otherwise.
    bool doesExist(QString name) const;

    /// @brief Erase name and its value from the variables hash.
    /// @param name The name of the variable to erase.
    void eraseVar(QString name);

    /// @brief Returns the size of the stack, i.e. the number of stack frames.
    /// @return The size of the stack.
    int size() const
    {
        return stack.size();
    }

    /// @brief Set the test state to TRUE or FALSE.
    /// @param isTrue True if the test state should be set to TRUE, false if it should be set to FALSE.
    /// @note This is for the commands TEST, IFTRUE, and IFFALSE.
    void setTest(bool isTrue);

    /// @brief Return true if a test state has been registered in any stack frame.
    /// @return True if a test state has been registered, false otherwise.
    /// @note This is for the commands TEST, IFTRUE, and IFFALSE.
    bool isTested() const;

    /// @brief Return true if the highest registered test state is true.
    /// @return True if the highest registered test state is true, false otherwise.
    /// @note This is for the commands TEST, IFTRUE, and IFFALSE.
    bool testedState() const;

    /// @brief Return a list of all variables defined.
    /// @return A list of all variables defined.
    DatumPtr allVariables() const;

    /// @brief In "explicit slot" APPLY command, sets the list of values of the explicit
    /// slot variables ("?1", "?2", etc.)
    /// @param aList The list of values to set the explicit slot variables to.
    void setExplicitSlotList(DatumPtr aList);

    /// @brief In "explicit slot" APPLY command, retrieves the list of values of the
    /// explicit slot variables ("?1", "?2", etc.)
    /// @return The list of values of the explicit slot variables.
    DatumPtr explicitSlotList() const;

    /// @brief Return the global frame, i.e. the last element of the stack list.
    /// @return The global frame.
    CallFrame *globalFrame() const
    {
        return stack.last();
    }

    /// @brief Return the local frame, i.e. the first element of the stack list.
    /// @return The local frame.
    CallFrame *localFrame() const
    {
        return stack.first();
    }

    /// @brief Return the parent of the local frame, i.e. the second element of the stack list.
    /// @return The parent frame.
    CallFrame *parentFrame() const
    {
        return stack[1];
    }
};

/// @brief The CallFrame object holds the state of execution of a procedure (or REPL).
/// @note The state includes named variables, anonymous variables (explicit slot, or
/// "?"), and the test state (for TEST, IFTRUE, IFFALSE).
struct CallFrame
{

    /// @brief A pointer to the call frame stack.
    /// @note The constructor and destructor will add and remove this frame to and
    /// from the stack.
    CallFrameStack *frameStack;

    /// @brief The ASTNode source of this running procedure.
    DatumPtr sourceNode;

    /// @brief The current source list being executed.
    /// The head of this list is the current line being executed.
    /// The tail is the lines following the current line.
    DatumPtr runningSourceList;

    /// @brief Set this value to set a jump location within a line.
    int32_t jumpLocation;

    /// @brief Set to true iff a TEST command has occurred.
    /// @note This is for the commands TEST, IFTRUE, and IFFALSE.
    bool isTested = false;

    /// @brief This holds the result of the most recent TEST.
    /// @note This is for the commands TEST, IFTRUE, and IFFALSE.
    bool testResult;

    /// @brief The explicit slot list, placeholders for "?".
    /// @note This is for the "explicit slot" APPLY command.
    DatumPtr explicitSlotList;

    /// @brief Variable names held in this scope and the values held outside of this scope.
    QHash<QString, DatumPtr> localVars;

    /// @brief The evaluation stack, maintains the stack of currently-executing lists and
    /// sublists.
    /// @note When a list is executed, a new Evaluator is created and pushed onto the stack.
    /// It will stay on the stack as long as it is executing. When it is done, it is popped
    /// from the stack. A list may call a sublist for execution (e.g. RUN, IF, or REPEAT), which
    /// will also create an Evaluator and push it onto the stack.
    QList<Evaluator *> evalStack;

    /// @brief Return the topmost Evaluator object.
    /// @return The topmost Evaluator object.
    Evaluator *localEvaluator() const
    {
        return evalStack.first();
    }

    /// @brief Insert an entry for 'name' in the variables hash. Save the previous value
    /// of the variable in the localVars hash. Store 'nothing' for the entry if name wasn't
    /// already present.
    /// @param name The name of the variable to insert.
    void setVarAsLocal(QString name);

    /// @brief Set the value of a variable.
    /// @param value The value to set the variable to.
    /// @param name The name of the variable to set.
    void setValueForName(DatumPtr value, QString name);

    /// @brief Apply the given parameters to the procedure.
    /// @param paramAry The parameters to apply.
    /// @param paramCount The number of parameters to apply.
    /// @returns nothing if successful, or an error if not.
    Datum* applyProcedureParams(Datum **paramAry, uint32_t paramCount);


    /// @brief End the current procedure by continuing with the given node and parameters.
    /// @param newNode The ASTNode of the new procedure to continue with.
    /// @param paramAry The parameters to apply to the new node.
    /// @returns nothing if successful, or an error if not.
    Datum* applyContinuation(DatumPtr newNode, QList<DatumPtr> paramAry);

    /// @brief Jump to the line in the procedure containing the given tag.
    /// @param node The FCGoto node.
    /// @returns Err if the tag is not found (or nothing if the tag is found).
    Datum *applyGoto(FCGoto *node);

    /// @brief Execute procedure referenced in the source node.
    /// @param paramAry The parameters to apply.
    /// @param paramCount The number of parameters to apply.
    /// @returns the result of this execution.
    Datum* exec(Datum **paramAry, uint32_t paramCount);

    /// @brief Execute the body of the procedure referenced in the source node.
    /// @returns the result of this execution.
    Datum *bodyExec();

    /// @brief Constructor.
    /// @param aFrameStack A pointer to the call frame stack.
    /// @param aSourceNode The ASTNode source of this running procedure. 'nothing'
    /// is reserved for the global frame or PAUSE.
    CallFrame(CallFrameStack *aFrameStack, DatumPtr aSourceNode = nothing)
        : frameStack(aFrameStack), sourceNode(aSourceNode)
    {
        frameStack->stack.push_front(this);
    }

    /// @brief Destructor. Removes local variables from the frame stack variables hash,
    /// and restores the original values of the variables.
    /// @note This frame will be removed from the stack.
    ~CallFrame();
};

/// @brief  The evaluator.
///
/// The evaluator handles the evaluation of a list. It provides support functionality
/// for the list while it is executing.
/// @note The constructor and destructor will add and remove this evaluator to and
/// from the evaluation stack.
struct Evaluator
{
    /// @brief A reference to the evaluation stack.
    QList<Evaluator *> &evalStack;

    /// @brief The list to evaluate.
    DatumPtr list;

    /// @brief a pointer to this list's compiled function.
    CompiledFunctionPtr fn;

    /// @brief the return value of this evaluation
    Datum *retval = nullptr;

    /// @brief A pool of objects for garbage collection.
    std::list<Datum *> releasePool;

    /// @brief Constructor.
    /// @param aList The list to evaluate.
    /// @param anEvalStack A reference to the evaluation stack.
    Evaluator(DatumPtr aList, QList<Evaluator *> &anEvalStack);

    /// @brief Destructor.
    /// @note This will remove this evaluator from the evaluation stack and empty the releasePool.
    ~Evaluator();

    /// @brief Execute this list. Will return when execution is complete.
    /// @param jumpLocation The location within the line to jump to.
    /// @returns the result of this execution.
    Datum* exec(int32_t jumpLocation = 0);

    /// @brief Execute the given sublist. Will return when execution is complete.
    /// @returns the result of this execution.
    Datum* subExec(Datum * aList);

    /// @brief Execute the given procedure. Will return when execution is complete.
    /// @returns the result of this execution.
    Datum* procedureExec(ASTNode *node, Datum **paramAry, uint32_t paramCount);

    /// @brief Add a Datum to the release pool
    /// @returns the given pointer (pass-through).
    Datum *watch(Datum *);

    /// @brief Add a Datum to the release pool
    /// @returns the given pointer (pass-through).
    Datum *watch(DatumPtr);

    /// @brief Returns TRUE if CASEIGNOREDP is "TRUE, "true, or any combination of the two.
    bool varCASEIGNOREDP();
};

#endif // CALLFRAME_H
