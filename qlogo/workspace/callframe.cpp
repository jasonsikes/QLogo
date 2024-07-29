//===-- qlogo/callframe.cpp - Vars class implementation -------*- C++ -*-===//
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

