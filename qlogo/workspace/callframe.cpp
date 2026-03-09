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
/// This file contains the implementation of the CallFrameStack, and CallFrame classes, which
///  hold the state of the execution of a procedure.
///
//===----------------------------------------------------------------------===//

#include "datum_types.h"
#include "workspace/callframe.h"
#include "astnode.h"
#include "workspace/kernel.h"
#include "workspace/procedures.h"

#include <QObject>

CallFrame::CallFrame(ASTNode *node, Datum **paramAry, uint32_t paramCount)
{
    if (node == nullptr)
    {
        // nullptr source node means this frame is REPL.
        return;
    }
    DatumPtr body = node->procedure_;
    sourceNode_ = DatumPtr(node);
    runningSourceList_ = body.procedureValue()->instructionList_.listValue()->tail;

    parameters_ = body.procedureValue()->instructionList_.listValue()->head;
    arguments_ = emptyList();
    for (int i = paramCount - 1; i >= 0; i--)
    {
        arguments_ = DatumPtr(new List(paramAry[i], arguments_.listValue()));
    }
    isReadingArgs_ = true;
}

CallFrame::~CallFrame()
{
    for (auto iter = localVars_.begin(); iter != localVars_.end(); ++iter)
    {
        if (iter.value().isNothing())
        {
            Kernel::get().eraseVar(iter.key());
        }
        else
        {
            Kernel::get().setDatumForName(iter.value(), iter.key());
        }
    }
}

// TODO: create a common initialization function for this and the constructor.
void CallFrame::applyContinuation(ASTNode *node, const DatumPtr &arguments)
{
    DatumPtr body = node->procedure_;
    sourceNode_ = DatumPtr(node);
    retvalToParent_ = nothing();
    runningSourceList_ = body.procedureValue()->instructionList_.listValue()->tail;

    parameters_ = body.procedureValue()->instructionList_.listValue()->head;
    arguments_ = arguments;
    isReadingArgs_ = true;
}

void CallFrame::pushEvaluator(const DatumPtr &aList)
{
    evaluationStack_.push_back(std::make_unique<Evaluator>(this, aList));
}

void CallFrame::popEvaluator()
{
    evaluationStack_.pop_back();
}

size_t CallFrame::evaluationStackSize() const
{
    return evaluationStack_.size();
}

void CallFrame::setVarAsLocal(const QString &name)
{
    if ( ! localVars_.contains(name))
    {
        DatumPtr originalValue = Kernel::get().datumForName(name);
        localVars_.insert(name, originalValue);
        Kernel::get().setDatumForName(nothing(), name);
    }
}




/*
void CallFrameStack::setExplicitSlotList(const DatumPtr &aList)
{
    Q_ASSERT(stack.size() > 0);
    stack.last()->explicitSlotList = aList;
}

DatumPtr CallFrameStack::explicitSlotList() const
{
    Q_ASSERT(stack.size() > 0);
    return stack.last()->explicitSlotList;
}

*/
