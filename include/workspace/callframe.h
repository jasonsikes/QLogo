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
/// The call frame stack is a stack of call frames, each representing the state
/// of a procedure. The first of the list is the 'top' call frame. That is,
/// new frames are pushed to the front of the stack, and the oldest, 'global'
/// frame is the last element of the list.
struct CallFrameStack  : public Workspace {
    QList<CallFrame*> stack;

  /// Search downward through the variable stack for the first occurrence of
  /// 'name'. Returns the stored value associated with 'name'.
  DatumPtr datumForName(QString name);

  /// Search downward through the variable stack for the first occurrence of
  /// 'name'. Replace the stored value with aDatum. If 'name' is not found,
  /// store value at the bottom of the stack.
  void setDatumForName(DatumPtr &aDatum, const QString &name);

  /// Insert an entry for 'name' at the top of the variable stack. Store
  /// 'nothing' for the entry if name wasn't already present.
  void setVarAsLocal(QString name);

  /// Insert an entry for 'name' at the bottom of the variable stack. Store
  /// 'nothing' for the entry if name wasn't already present.
  void setVarAsGlobal(QString name);

  /// Return true if value keyed by name exists somewhere in the stack.
  bool doesExist(QString name);

  /// Erase every occurrence of name from the variable stack.
  void eraseVar(QString name);

  /// Erase every unburied variable from the variable stack.
  void eraseAll();

  /// Returns the size of the stack, i.e. the number of stack frames.
  int size() { return stack.size(); }

  /// Set the test state to TRUE or FALSE
  void setTest(bool isTrue);

  /// Return true if a test state has been registered in any stack frame.
  bool isTested();

  /// Return true if the highest registered test state is true.
  bool testedState();

  /// Returns true if the named variable exists in the lowest stack frame.
  bool isVarGlobal(QString name);

  /// Return a list of all variables defined and buried/unburied (determined by
  /// 'showWhat'.)
  DatumPtr allVariables(showContents_t showWhat);

  /// In "explicit slot" APPLY command, sets the list of values of the explicit
  /// slot variables ("?1", "?2", etc.)
  void setExplicitSlotList(DatumPtr aList);

  /// In "explicit slot" APPLY command, retrieves the list of values of the
  /// explicit slot variables ("?1", "?2", etc.)
  DatumPtr explicitSlotList();

  /// Return the global frame, i.e. the last element of the stack list.
  CallFrame* globalFrame() { return stack.last(); }

  /// Return the local frame, i.e. the first element of the stack list.
  CallFrame* localFrame() { return stack.first(); }

  /// Return the parent of the local frame, i.e. the second element of the stack list.
  CallFrame* parentFrame() { return stack[1]; }
};




/// The CallFrame object holds the state of execution of a procedure or
/// shell-like procedure [e.g. getLineAndRunIt()].
/// The state includes named variables, anonymous variables (explicit slot, or
/// "?"), and the test state (for TEST, IFTRUE, IFFALSE).
struct CallFrame {

    /// A pointer to the call frame stack. The constructor and destructor
    /// will add and remove this frame to and from the stack.
    CallFrameStack *frameStack;

    /// The ASTNode source of this running procedure. 'nothing' indicates global.
    DatumPtr sourceNode;

    /// Set to true iff a TEST command has occurred.
    bool isTested = false;

    /// This holds the result of the most recent TEST.
    bool testResult;

    /// The explicit slot list, placeholders for "?".
    DatumPtr explicitSlotList;

    /// Variables held in this scope.
    QHash<QString, DatumPtr> localVars;

    /// The evaluation stack, maintains the stack of currently-executing lists and sublists.
    QList<Evaluator *> evalStack;

    /// Return the Evaluator object that is currently executing.
    Evaluator* localEvaluator() { return evalStack.first(); }

    CallFrame(CallFrameStack *aFrameStack, DatumPtr aSourceNode = nothing)
        : frameStack(aFrameStack), sourceNode(aSourceNode)
    {
        frameStack->stack.push_front(this);
    }

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
struct Evaluator {
    QList<Evaluator *> &evalStack;
    DatumPtr list;

    Evaluator(DatumPtr aList, QList<Evaluator *> &anEvalStack)
        : evalStack(anEvalStack), list(aList)
    {
        evalStack.push_front(this);
    }

    ~Evaluator()
    {
        Q_ASSERT(evalStack.first() == this);
        evalStack.removeFirst();
    }
};


#endif // CALLFRAME_H
