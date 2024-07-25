#ifndef CALLFRAME_H
#define CALLFRAME_H

//===-- qlogo/callframe.h - CallFrame class definition -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// QLogo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with QLogo.  If not, see <http://www.gnu.org/licenses/>.
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

#include "datum.h"
#include "workspace/workspace.h"

struct CallFrame;
struct Evaluator;

/// @brief  The call frame stack.
///
/// The call frame stack is the stack of call frames, each representing the state
/// of a procedure. The first of the list is the 'top' call frame. That is,
/// new frames are pushed to the front of the stack, and the oldest, 'global'
/// frame is the last element of the list.
struct CallFrameStack : public Workspace
{
    /// @brief The call frame stack.
    QList<CallFrame *> stack;

    /// @brief Search downward through the variable stack for the first occurrence of
    /// 'name'.
    /// @param name The name of the variable to search for.
    /// @return The stored value associated with 'name'.
    DatumPtr datumForName(QString name);

    /// @brief Search downward through the variable stack for the first occurrence of
    /// 'name'. Replace the stored value with aDatum. If 'name' is not found,
    /// store value at the bottom of the stack.
    /// @param aDatum The value to store.
    /// @param name The name of the variable to search for.
    void setDatumForName(DatumPtr &aDatum, const QString &name);

    /// @brief Insert an entry for 'name' at the top of the variable stack. Store
    /// 'nothing' for the entry if name wasn't already present.
    /// @param name The name of the variable to insert.
    void setVarAsLocal(QString name);

    /// @brief Insert an entry for 'name' at the bottom of the variable stack. Store
    /// 'nothing' for the entry if name wasn't already present.
    /// @param name The name of the variable to insert.
    void setVarAsGlobal(QString name);

    /// @brief Return true if value keyed by name exists somewhere in the stack.
    /// @param name The name of the variable to search for.
    /// @return True if the variable exists, false otherwise.
    bool doesExist(QString name);

    /// @brief Erase every occurrence of name from the variable stack.
    /// @param name The name of the variable to erase.
    void eraseVar(QString name);

    /// @brief Erase every unburied variable from the variable stack.
    void eraseAll();

    /// @brief Returns the size of the stack, i.e. the number of stack frames.
    /// @return The size of the stack.
    int size()
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
    bool isTested();

    /// @brief Return true if the highest registered test state is true.
    /// @return True if the highest registered test state is true, false otherwise.
    /// @note This is for the commands TEST, IFTRUE, and IFFALSE.
    bool testedState();

    /// @brief Returns true if the named variable exists in the lowest stack frame.
    /// @param name The name of the variable to search for.
    /// @return True if the variable exists, false otherwise.
    bool isVarGlobal(QString name);

    /// @brief Return a list of all variables defined and buried/unburied (determined by
    /// 'showWhat'.)
    /// @param showWhat The type of variables to show (buried or unburied).
    /// @return A list of all variables defined and buried/unburied.
    DatumPtr allVariables(showContents_t showWhat);

    /// @brief In "explicit slot" APPLY command, sets the list of values of the explicit
    /// slot variables ("?1", "?2", etc.)
    /// @param aList The list of values to set the explicit slot variables to.
    void setExplicitSlotList(DatumPtr aList);

    /// @brief In "explicit slot" APPLY command, retrieves the list of values of the
    /// explicit slot variables ("?1", "?2", etc.)
    /// @return The list of values of the explicit slot variables.
    DatumPtr explicitSlotList();

    /// @brief Return the global frame, i.e. the last element of the stack list.
    /// @return The global frame.
    CallFrame *globalFrame()
    {
        return stack.last();
    }

    /// @brief Return the local frame, i.e. the first element of the stack list.
    /// @return The local frame.
    CallFrame *localFrame()
    {
        return stack.first();
    }

    /// @brief Return the parent of the local frame, i.e. the second element of the stack list.
    /// @return The parent frame.
    CallFrame *parentFrame()
    {
        return stack[1];
    }
};

/// @brief The CallFrame object holds the state of execution of a procedure or
/// shell-like procedure [e.g. getLineAndRunIt()].
/// @note The state includes named variables, anonymous variables (explicit slot, or
/// "?"), and the test state (for TEST, IFTRUE, IFFALSE).
struct CallFrame
{

    /// @brief A pointer to the call frame stack.
    /// @note The constructor and destructor will add and remove this frame to and
    /// from the stack.
    CallFrameStack *frameStack;

    /// @brief The ASTNode source of this running procedure. 'nothing' indicates global.
    DatumPtr sourceNode;

    /// @brief Set to true iff a TEST command has occurred.
    /// @note This is for the commands TEST, IFTRUE, and IFFALSE.
    bool isTested = false;

    /// @brief This holds the result of the most recent TEST.
    /// @note This is for the commands TEST, IFTRUE, and IFFALSE.
    bool testResult;

    /// @brief The explicit slot list, placeholders for "?".
    /// @note This is for the "explicit slot" APPLY command.
    DatumPtr explicitSlotList;

    /// @brief Variables held in this scope.
    QHash<QString, DatumPtr> localVars;

    /// @brief The evaluation stack, maintains the stack of currently-executing lists and
    /// sublists.
    /// @note When a list is executed, a new Evaluator is created and pushed onto the stack.
    /// It will stay on the stack as long as it is executing. When it is done, it is popped
    /// from the stack. A list may call a sublist for execution (e.g. IF or REPEAT), which will
    /// also create an Evaluator and push it onto the stack.
    QList<Evaluator *> evalStack;

    /// @brief Return the topmost Evaluator object.
    /// @return The topmost Evaluator object.
    Evaluator *localEvaluator()
    {
        return evalStack.first();
    }

    /// @brief Constructor.
    /// @param aFrameStack A pointer to the call frame stack.
    /// @param aSourceNode The ASTNode source of this running procedure. 'nothing'
    /// is reserved for the global frame.
    CallFrame(CallFrameStack *aFrameStack, DatumPtr aSourceNode = nothing)
        : frameStack(aFrameStack), sourceNode(aSourceNode)
    {
        frameStack->stack.push_front(this);
    }

    /// @brief Destructor.
    /// @note This will remove this frame from the stack.
    ~CallFrame()
    {
        Q_ASSERT(frameStack->stack.first() == this);
        frameStack->stack.pop_front();
    }
};

/// @brief  The evaluator.
///
/// The evaluator will handle the evaluation of a list. It doesn't do anything
/// right now except add and remove itself from the evaluation stack.
/// @note The constructor and destructor will add and remove this evaluator to and
/// from the evaluation stack.
struct Evaluator
{
    /// @brief A reference to the evaluation stack.
    QList<Evaluator *> &evalStack;

    /// @brief The list to evaluate.
    DatumPtr list;

    /// @brief Constructor.
    /// @param aList The list to evaluate.
    /// @param anEvalStack A reference to the evaluation stack.
    Evaluator(DatumPtr aList, QList<Evaluator *> &anEvalStack) : evalStack(anEvalStack), list(aList)
    {
        evalStack.push_front(this);
    }

    /// @brief Destructor.
    /// @note This will remove this evaluator from the evaluation stack.
    ~Evaluator()
    {
        Q_ASSERT(evalStack.first() == this);
        evalStack.removeFirst();
    }
};

#endif // CALLFRAME_H
