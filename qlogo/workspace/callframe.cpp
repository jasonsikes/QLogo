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

#include "workspace/callframe.h"
#include "astnode.h"
#include "datum_types.h"
#include "workspace/kernel.h"
#include "workspace/procedures.h"

#include <QObject>

CallFrame::CallFrame(ASTNode *node, Datum **paramAry, uint32_t paramCount)
{
    evaluationStackStartIndex_ = Kernel::get().evaluationStack_.size();
    if (node == nullptr)
    {
        // nullptr source node means this frame is REPL. Keep any evaluators
        // belonging to a paused procedure below this frame's stack view.
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
    isMacroCall_ = body.procedureValue()->isMacro_;
    if (isMacroCall_)
    {
        // Captured from the call site. applyContinuation must not overwrite these:
        // a tail call inside the macro still expands in the original caller's context.
        macroExpectsSingleExpression_ = node->macroExpectsSingleExpression_;
        macroTailSplice_ = node->inSequenceTail_ && !macroExpectsSingleExpression_;
    }
    evaluationStackStartIndex_ = Kernel::get().evaluationStack_.size();
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
void CallFrame::applyContinuation(ASTNode *node, const DatumPtr &arguments, bool demandNothing)
{
    DatumPtr body = node->procedure_;
    sourceNode_ = DatumPtr(node);
    retvalToParent_ = nothing();
    runningSourceList_ = body.procedureValue()->instructionList_.listValue()->tail;

    parameters_ = body.procedureValue()->instructionList_.listValue()->head;
    arguments_ = arguments;
    isReadingArgs_ = true;
    // The continued procedure has not run a body line yet.
    lastBodyLine_ = nothing();
    // isMacroCall_ and the splice flags stay as they were for the original call.
    // demandNothing_ only turns on: STOP TCO after OUTPUT TCO still forbids a value.
    // STOP also drops outputRequired_: the continued procedure must not output.
    // nestedContinuation_, allowNothing_, outputRequired_ (unless cleared here),
    // and outputSource_ stay as well: a tail call inside a nested OUTPUT/STOP
    // still reports to the waiting evaluator.
    if (demandNothing)
    {
        demandNothing_ = true;
        outputRequired_ = false;
    }
}

Evaluator *CallFrame::topEvaluator()
{
    return &(Kernel::get().evaluationStack_.back());
}

Evaluator *CallFrame::topOwnEvaluator()
{
    Kernel &kernel = Kernel::get();
    std::size_t end = kernel.evaluationStack_.size();
    for (std::size_t i = 0; i + 1 < kernel.callFrameStack_.size(); ++i)
    {
        if (&kernel.callFrameStack_[i] == this)
        {
            end = kernel.callFrameStack_[i + 1].evaluationStackStartIndex_;
            break;
        }
    }
    if (end <= evaluationStackStartIndex_ || end > kernel.evaluationStack_.size())
        return nullptr;
    return &kernel.evaluationStack_[end - 1];
}

bool CallFrame::isEvaluationStackViewEmpty() const
{
    return evaluationStackStartIndex_ == Kernel::get().evaluationStack_.size();
}

Evaluator *CallFrame::evaluatorAtIndex(std::size_t index)
{
    return &(Kernel::get().evaluationStack_[evaluationStackStartIndex_ + index]);
}

void CallFrame::pushEvaluator(const DatumPtr &aList)
{
    Kernel::get().evaluationStack_.emplace_back(this, aList);
}

void CallFrame::popEvaluator()
{
    Kernel::get().evaluationStack_.pop_back();
}

size_t CallFrame::evaluationStackSize() const
{
    return Kernel::get().evaluationStack_.size() - evaluationStackStartIndex_;
}

void CallFrame::setVarAsLocal(Word *name)
{
    Word *atom = name->interned();
    if (!localVars_.contains(atom))
        bindLocal(atom, nothing());
    else
        Kernel::get().warnSteppedShadow(atom);
}

void CallFrame::bindLocal(Word *name, const DatumPtr &value)
{
    Word *atom = name->interned();
    DatumPtr previous = Kernel::get().setDatumForName(value, atom);
    if (!localVars_.contains(atom))
        localVars_.insert(atom, previous);
    Kernel::get().warnSteppedShadow(atom);
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
