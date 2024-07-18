//===-- qlogo/callframe.cpp - Vars class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the CallFrameStack,  class, which
///  holds the state of the execution of a procedure.
///
//===----------------------------------------------------------------------===//

#include "workspace/callframe.h"


void CallFrameStack::setDatumForName(DatumPtr &aDatum, const QString &name) {
    for (auto frame : stack) {
        if (frame->localVars.contains(name)) {
            frame->localVars[name] = aDatum;
            return;
        }
    }
    stack.last()->localVars.insert(name, aDatum);
}


DatumPtr CallFrameStack::datumForName(QString name) {
    for (auto frame : stack) {
        auto result = frame->localVars.find(name);
        if (result != frame->localVars.end()) {
            return *result;
        }
    }
    return nothing;
}



void CallFrameStack::setVarAsLocal(QString name) {
    if ( ! stack.first()->localVars.contains(name))
        stack.first()->localVars.insert(name, nothing);
}


void CallFrameStack::setVarAsGlobal(QString name) {
    if ( ! stack.last()->localVars.contains(name))
        stack.last()->localVars.insert(name, nothing);
}




bool CallFrameStack::isVarGlobal(QString name)
{
    return stack.last()->localVars.contains(name);
}




bool CallFrameStack::doesExist(QString name) {
    for (auto frame : stack) {
        if (frame->localVars.contains(name))
            return true;
    }
    return false;
}



DatumPtr CallFrameStack::allVariables(showContents_t showWhat) {
    List *retval = new List();
    QSet<QString> seenVars;

    for (auto frame : stack) {
        QStringList varnames = frame->localVars.keys();
        for (auto &varname : varnames) {
            if (!seenVars.contains(varname)) {
                seenVars.insert(varname);

                if (shouldInclude(showWhat, varname))
                    retval->append(DatumPtr(varname));
            }
        }
    }

    return DatumPtr(retval);
}



void CallFrameStack::eraseAll() {
    for (auto frame : stack) {
        QStringList varnames = frame->localVars.keys();
        for (auto &varname : varnames) {
            if (!isBuried(varname))
                frame->localVars.remove(varname);
        }
    }
}



void CallFrameStack::eraseVar(QString name) {
    for (auto frame : stack) {
        if (frame->localVars.remove(name) > 0)
            return;
    }
}



void CallFrameStack::setTest(bool isTrue) {
    stack.first()->isTested = true;
    stack.first()->testResult = isTrue;
}



bool CallFrameStack::isTested() {
    for (auto frame : stack) {
        if (frame->isTested)
            return true;
    }
    return false;
}




void CallFrameStack::setExplicitSlotList(DatumPtr aList)
{
    stack.last()->explicitSlotList = aList;
}



DatumPtr CallFrameStack::explicitSlotList()
{
    return stack.last()->explicitSlotList;
}



bool CallFrameStack::testedState() {
    for (auto frame : stack) {
        if (frame->isTested)
            return frame->testResult;
    }
    Q_ASSERT(false);
    return false;
}

