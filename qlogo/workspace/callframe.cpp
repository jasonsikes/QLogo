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
    DatumPtr body = node->procedure_.procedureValue();
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
    DatumPtr body = node->procedure_.procedureValue();
    sourceNode_ = DatumPtr(node);
    runningSourceList_ = body.procedureValue()->instructionList_.listValue()->tail;

    parameters_ = body.procedureValue()->instructionList_.listValue()->head;
    arguments_ = arguments;
    isReadingArgs_ = true;
}

void CallFrame::pushEvaluator(const DatumPtr &aList)
{
    evaluationStack_.push(std::make_unique<Evaluator>(this, aList));
}

void CallFrame::popEvaluator()
{
    evaluationStack_.pop();
}

size_t CallFrame::evaluationStackSize() const
{
    return evaluationStack_.size();
}

void CallFrame::setVarAsLocal(const QString &name)
{
    DatumPtr originalValue = Kernel::get().datumForName(name);
    localVars_.insert(name, originalValue);
    Kernel::get().setDatumForName(nothing(), name);
}


// Datum *NewCallFrame::applyGoto(FCGoto *node)
// {
//     DatumPtr tag = node->tag();
//     Datum *procedure = sourceNode.astnodeValue()->procedure.datumValue();
//     DatumPtr runningSourceListSnapshot;

//     // Have we seen this tag already?
//     auto *proc = static_cast<Procedure *>(procedure);
//     auto blockIdIterator = proc->tagToBlockId.find(tag.toString(Datum::ToStringFlags_Key));
//     if (blockIdIterator != proc->tagToBlockId.end())
//     {
//         goto foundTag;
//     }

//     // If not, then search through the remaining lines in the procedure.

//     // Save our running state in case we need to restore it later.
//     runningSourceListSnapshot = runningSourceList;

//     while (runningSourceList.isList() && runningSourceList.listValue()->isEmpty() == false)
//     {
//         List *list = runningSourceList.listValue()->head.listValue();
//         try
//         {
//             Compiler::get().functionPtrFromList(list);
//         }
//         catch (FCError *e)
//         {
//             return e;
//         }
//         blockIdIterator = proc->tagToBlockId.find(tag.toString(Datum::ToStringFlags_Key));
//         if (blockIdIterator != proc->tagToBlockId.end())
//         {
//             goto foundTag;
//         }
//         runningSourceList = runningSourceList.listValue()->tail;
//     }

//     // If we still didn't find the tag, return an error.
//     runningSourceList = runningSourceListSnapshot;
//     return FCError::doesntLike(node->sourceNode.astnodeValue()->nodeName, tag);

// foundTag:
//     // Now, we need to jump to the block that contains the tag.
//     runningSourceList = proc->tagToLine[tag.toString(Datum::ToStringFlags_Key)];
//     jumpLocation = blockIdIterator.value();
//     return nullptr;
//     return nullptr;
// }





/*
void CallFrameStack::setTest(bool isTrue)
{
    Q_ASSERT(stack.size() > 0);
    stack.first()->isTested = true;
    stack.first()->testResult = isTrue;
}

bool CallFrameStack::isTested() const
{
    return std::any_of(stack.begin(), stack.end(), [](const auto &frame) { return frame->isTested; });
}

bool CallFrameStack::testedState() const
{
    auto it = std::find_if(stack.begin(), stack.end(), [](const auto &frame) { return frame->isTested; });
    Q_ASSERT(it != stack.end());
    return (*it)->testResult;
}

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
