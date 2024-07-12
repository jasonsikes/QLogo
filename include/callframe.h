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
/// handles the execution state of a procedure in qlogo.
///
//===----------------------------------------------------------------------===//

#include "datum.h"



/// The CallFrame object holds the state of execution of a procedure or
/// shell-like procedure [e.g. getLineAndRunIt()].
/// The state includes named variables, anonymous variables (explicit slot, or
/// "?"), and the test state (for TEST, IFTRUE, IFFALSE).
struct CallFrame {

    /// The ASTNode source of this running procedure. 'nothing' indicates global.
    DatumPtr sourceNode;

    /// Set to true iff a TEST command has occurred.
    bool isTested = false;

    /// This holds the result of the most recent TEST.
    bool testResult;

    /// The explicit slot array, placeholders for "?".
    QList<DatumPtr> explicitSlotAry;

    /// Variables held in this scope.
    QHash<QString, DatumPtr> localVars;

    /// The parent call frame, may be NULL if this is the global scope.
    CallFrame *parent;

    /// The evaluation stack, used to handle executing lists and sublists.
    QList<DatumPtr> evalStack;

    QList<CallFrame *> &frameList;

    CallFrame(QList<CallFrame*> &list, DatumPtr aSourceNode = nothing)
        : frameList(list), sourceNode(aSourceNode)
    {
        frameList.append(this);
    }

    ~CallFrame()
    {
        Q_ASSERT(frameList.last() == this);
        frameList.pop_back();
    }

};


struct Evaluator {
    QList<DatumPtr> &evalStack;

    Evaluator(DatumPtr aList, QList<DatumPtr> &anEvalStack)
        : evalStack(anEvalStack)
    {
        evalStack.push_back(aList);
    }

    ~Evaluator()
    {
        evalStack.removeLast();
    }
};


#endif // CALLFRAME_H
