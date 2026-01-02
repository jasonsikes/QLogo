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
#include "astnode.h"
#include "compiler.h"
#include "flowcontrol.h"
#include "kernel.h"
#include "parser.h"
#include "runparser.h"
#include "sharedconstants.h"
#include "workspace/procedures.h"

#include <QObject>
#include <algorithm>
#include <vector>

void CallFrameStack::setDatumForName(const DatumPtr &aDatum, const QString &name)
{
    variables.insert(name, aDatum);
}

DatumPtr CallFrameStack::datumForName(const QString &name) const
{
    auto result = variables.find(name);
    if (result != variables.end())
    {
        return *result;
    }
    return nothing();
}

bool CallFrameStack::doesExist(const QString &name) const
{
    return variables.contains(name);
}

DatumPtr CallFrameStack::allVariables() const
{
    ListBuilder builder;
    for (auto &varname : variables.keys())
    {
        builder.append(DatumPtr(varname));
    }
    return builder.finishedList();
}

void CallFrameStack::eraseVar(const QString &name)
{
    variables.remove(name);
}

void CallFrameStack::setTest(bool isTrue)
{
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
    stack.last()->explicitSlotList = aList;
}

DatumPtr CallFrameStack::explicitSlotList() const
{
    return stack.last()->explicitSlotList;
}

// CallFrame methods

CallFrame::~CallFrame()
{
    Q_ASSERT(frameStack->stack.first() == this);
    for (auto iter = localVars.begin(); iter != localVars.end(); ++iter)
    {
        const DatumPtr &value = iter.value();
        if (value.isNothing())
        {
            frameStack->eraseVar(iter.key());
        }
        else
        {
            frameStack->setDatumForName(value, iter.key());
        }
    }
    frameStack->stack.pop_front();
}

void CallFrame::setVarAsLocal(const QString &name)
{
    DatumPtr originalValue = frameStack->datumForName(name);
    localVars.insert(name, originalValue);
    frameStack->setDatumForName(nothing(), name);
}

void CallFrame::setValueForName(const DatumPtr &value, const QString &name)
{
    frameStack->setDatumForName(value, name);
}

Datum *CallFrame::applyProcedureParams(Datum **paramAry, uint32_t paramCount)
{
    Procedure *proc = sourceNode.astnodeValue()->procedure.procedureValue();

    QStringList &requiredInputs = proc->requiredInputs;
    QStringList &optionalInputs = proc->optionalInputs;
    QList<DatumPtr> &optionalDefaults = proc->optionalDefaults;

    // Assign the given name/value pairs to the local variables.

    size_t paramIndex = 0;
    for (auto &inputName : requiredInputs)
    {
        Q_ASSERT(paramIndex < paramCount);
        setVarAsLocal(inputName);
        DatumPtr value(*(paramAry + paramIndex));
        setValueForName(value, inputName);
        paramIndex++;
    }

    // Handle optional inputs as name/value pairs.
    // Note that optionalInputs are lists with the tail being the default expression.
    // The head is the name of the optional input, but we saved it earlier in a key form.
    // We retain the whole expression in optionalDefaults in case there is an error.
    for (int i = 0; i < optionalInputs.size(); i++)
    {
        QString name = optionalInputs[i];
        DatumPtr value;
        if (paramIndex < paramCount)
        {
            value = *(paramAry + paramIndex);
        }
        else
        {
            DatumPtr optExpression = optionalDefaults[i].listValue()->tail;
            // TODO: ensure that the generated ASTList has one root node.
            Evaluator e(optExpression, evalStack);
            value = e.exec();
            if (value.isa() == Datum::typeError)
            {
                return FCError::badDefault(optionalDefaults[i]);
            }
        }
        setVarAsLocal(name);
        setValueForName(value, name);
        paramIndex++;
    }

    // Finally, take in the remainder (if any) as a list.
    if (proc->restInput != "")
    {
        QString name = proc->restInput;
        ListBuilder builder;
        while (paramIndex < paramCount)
        {
            builder.append(*(paramAry + paramIndex));
            paramIndex++;
        }
        setVarAsLocal(name);
        DatumPtr restList = builder.finishedList();
        setValueForName(restList, name);
    }
    return nullptr;
}

Datum *CallFrame::exec(Datum **paramAry, uint32_t paramCount)
{
    Datum *retval = applyProcedureParams(paramAry, paramCount);
    if (retval != nullptr)
    {
        return retval;
    }

    retval = bodyExec();

    // If the result is "nothing", replace the result with the ASTNode of the procedure.
    // TODO: Should we test for typeUnboundMask here instead?
    if ((retval == nullptr) || (retval->isa == Datum::typeASTNode))
    {
        retval = sourceNode.astnodeValue();
    }

    return retval;
}

Datum *CallFrame::applyContinuation(const DatumPtr &newNode, const QList<DatumPtr> &paramAry)
{
    sourceNode = newNode;
    std::vector<Datum *> newParamAry;
    newParamAry.reserve(paramAry.size());
    for (const auto &param : paramAry)
    {
        newParamAry.push_back(param.datumValue());
    }

    return applyProcedureParams(newParamAry.data(), paramAry.size());
}

Datum *CallFrame::applyGoto(FCGoto *node)
{
    DatumPtr tag = node->tag();
    Datum *procedure = sourceNode.astnodeValue()->procedure.datumValue();
    DatumPtr runningSourceListSnapshot;

    // Have we seen this tag already?
    auto *proc = static_cast<Procedure *>(procedure);
    auto blockIdIterator = proc->tagToBlockId.find(tag.toString(Datum::ToStringFlags_Key));
    if (blockIdIterator != proc->tagToBlockId.end())
    {
        goto foundTag;
    }

    // If not, then search through the remaining lines in the procedure.

    // Save our running state in case we need to restore it later.
    runningSourceListSnapshot = runningSourceList;

    // TODO: What if compilation results in an error?
    while (runningSourceList.isList() && runningSourceList.listValue()->isEmpty() == false)
    {
        List *list = runningSourceList.listValue()->head.listValue();
        Config::get().mainCompiler()->functionPtrFromList(list);
        blockIdIterator = proc->tagToBlockId.find(tag.toString(Datum::ToStringFlags_Key));
        if (blockIdIterator != proc->tagToBlockId.end())
        {
            goto foundTag;
        }
        runningSourceList = runningSourceList.listValue()->tail;
    }

    // If we still didn't find the tag, return an error.
    runningSourceList = runningSourceListSnapshot;
    return FCError::doesntLike(node->sourceNode.astnodeValue()->nodeName, tag);

foundTag:
    // Now, we need to jump to the block that contains the tag.
    runningSourceList = proc->tagToLine[tag.toString(Datum::ToStringFlags_Key)];
    jumpLocation = blockIdIterator.value();
    return nullptr;
}

Datum *CallFrame::bodyExec()
{
    jumpLocation = 0;
beginBody:
    Datum *retval = nullptr;
    DatumPtr retvalPtr;
    runningSourceList = sourceNode.astnodeValue()->procedure.procedureValue()->instructionList;
continueBody:
    while (runningSourceList.listValue() != EmptyList::instance())
    {
        if (retval != nullptr)
        {
            retvalPtr = DatumPtr(retval);
        }
        DatumPtr instruction = runningSourceList.listValue()->head;

        Evaluator e = Evaluator(instruction, evalStack);
        retval = e.exec(jumpLocation);
        jumpLocation = 0;

        // Do we need to halt execution for some reason?
        if ((retval->isa & Datum::typeUnboundMask) == 0)
        {
            switch (retval->isa)
            {
            case Datum::typeError:
                // ERROR simply returns the error/return value.
                return retval;
            case Datum::typeReturn:
                // RETURN returns the data in the return value.
                return static_cast<FCReturn *>(retval)->returnValue().datumValue();
            case Datum::typeGoto:
                retval = applyGoto(static_cast<FCGoto *>(retval));
                if (retval == nullptr)
                {
                    goto continueBody;
                }
                Q_ASSERT(retval->isa == Datum::typeError);
                return retval;
            case Datum::typeContinuation:
            {
                auto *fc = static_cast<FCContinuation *>(retval);
                Datum *continuationStatus = applyContinuation(fc->procedure(), fc->params());
                if (continuationStatus != nullptr)
                {
                    return continuationStatus;
                }
                goto beginBody;
            }
            default:
                Q_ASSERT(false);
            }
        }
        runningSourceList = runningSourceList.listValue()->tail;
    }

    return retval;
}

Evaluator::Evaluator(const DatumPtr &aList, QList<Evaluator *> &anEvalStack) : evalStack(anEvalStack), list(aList)
{
    evalStack.push_front(this);
}

Evaluator::~Evaluator()
{
    Q_ASSERT(evalStack.first() == this);

    // qDebug() << "draining pool";
    for (auto &d : releasePool)
    {
        if ((d->isa & Datum::typePersistentMask) == 0)
        {
            (d->retainCount)--;
            if ((d != retval) && (d->retainCount < 1))
                delete d;
        }
    }

    // qDebug() << "drained pool";
    evalStack.removeFirst();
}

Datum *Evaluator::exec(int32_t jumpLocation)
{
    if (list.listValue() == EmptyList::instance())
    {
        return Datum::getInstance();
    }
    fn = Config::get().mainCompiler()->functionPtrFromList(list.listValue());
    retval = static_cast<Datum *>(fn((addr_t)this, jumpLocation));

    return retval;
}

Datum *Evaluator::subExec(Datum *aList)
{
    try
    {
        if (aList->isWord())
        {
            DatumPtr runparsedList = runparse(DatumPtr(aList));
            aList = runparsedList.datumValue();
            watch(aList);
        }
        if (!aList->isList())
        {
            FCError *err = FCError::noHow(DatumPtr(aList));
            watch(err);
            return err;
        }
        if (aList->listValue()->isEmpty())
        {
            return Datum::getInstance();
        }
        Evaluator e(aList, evalStack);
        retval = e.exec();
    }
    catch (FCError *err)
    {
        retval = err;
    }
    return retval;
}

Datum *Evaluator::procedureExec(ASTNode *node, Datum **paramAry, uint32_t paramCount)
{
    CallFrameStack *frameStack = &Config::get().mainKernel()->callStack;
    CallFrame frame(frameStack, DatumPtr(node));

    return frame.exec(paramAry, paramCount);
}

Datum *Evaluator::watch(const DatumPtr &d)
{
    return watch(d.datumValue());
}

Datum *Evaluator::watch(Datum *d)
{
    (d->retainCount)++;
    releasePool.push_back(d);
    return d;
}

bool Evaluator::varCASEIGNOREDP()
{
    QString name = QObject::tr("CASEIGNOREDP");
    DatumPtr val = Config::get().mainKernel()->callStack.datumForName(name);
    bool retval = false;
    if (val.isWord())
    {
        QString word = val.toString(Datum::ToStringFlags_Key);
        retval = word == QObject::tr("TRUE");
    }
    return retval;
}
