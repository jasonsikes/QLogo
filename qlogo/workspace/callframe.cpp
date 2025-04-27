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
#include "sharedconstants.h"
#include "compiler.h"
#include "flowcontrol.h"
#include "astnode.h"
#include "workspace/procedures.h"
#include "kernel.h"
#include "parser.h"
#include "runparser.h"

void CallFrameStack::setDatumForName(DatumPtr &aDatum, const QString &name) {
    variables.insert(name, aDatum);
}


DatumPtr CallFrameStack::datumForName(QString name) {
    auto result = variables.find(name);
    if (result != variables.end()) {
        return *result;
    }
    return nothing;
}


bool CallFrameStack::doesExist(QString name) {
    return variables.contains(name);
}



DatumPtr CallFrameStack::allVariables() {
    List *retval = new List();
    ListBuilder builder(retval);
    for (auto &varname : variables.keys()) {
        builder.append(DatumPtr(varname));
    }
    return DatumPtr(retval);
}



void CallFrameStack::eraseVar(QString name) {
    variables.remove(name);
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

bool CallFrameStack::testedState() {
    for (auto frame : stack) {
        if (frame->isTested)
            return frame->testResult;
    }
    Q_ASSERT(false);
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


// CallFrame methods

CallFrame::~CallFrame() {
    Q_ASSERT(frameStack->stack.first() == this);
    for (auto iter = localVars.begin(); iter != localVars.end(); ++iter) {
        DatumPtr value = iter.value();
        if (value.isa() == Datum::typeNothing) {
            frameStack->eraseVar(iter.key());
        } else {
            frameStack->setDatumForName(value, iter.key());
        }
    }
    frameStack->stack.pop_front();
}

void CallFrame::setVarAsLocal(QString name) {
    DatumPtr originalValue = frameStack->datumForName(name);
    localVars.insert(name, originalValue);
    frameStack->setDatumForName(nothing, name);
}


void CallFrame::setValueForName(DatumPtr value, QString name) {
    frameStack->setDatumForName(value, name);
}



void CallFrame::applyProcedureParams(Datum **paramAry, uint32_t paramCount) {
    Procedure *proc = sourceNode.astnodeValue()->procedure.procedureValue();

    QStringList &requiredInputs = proc->requiredInputs;
    QStringList &optionalInputs = proc->optionalInputs;
    QList<DatumPtr> &optionalDefaults = proc->optionalDefaults;

    // Assign the given name/value pairs to the local variables.

    size_t paramIndex = 0;
    for (auto &input : requiredInputs) {
        Q_ASSERT(paramIndex < paramCount);
        setVarAsLocal(input);
        setValueForName(DatumPtr(*(paramAry + paramIndex)), input);
        paramIndex++;
    }

    // Handle optional inputs as name/value pairs.
    // Note that optionalInputs are lists with the tail being the default expression.
    // The head is the name of the optional input, but we saved it earlier in a key form.
    // We retain the whole expression in optionalDefaults in case there is an error.
    for (int i = 0; i < optionalInputs.size(); i++) {
        QString name = optionalInputs[i];
        DatumPtr value;
        if (paramIndex < paramCount) {
            value = *(paramAry + paramIndex);
        } else {
            value = optionalDefaults[i].listValue()->tail;
            // TODO: ensure that the generated ASTList has one root node.
            Evaluator e(value, evalStack);
            value = e.exec();
        }
        setVarAsLocal(name);
        setValueForName(value, name);
        paramIndex++;
    }


    // Finally, take in the remainder (if any) as a list.
    if (proc->restInput != "") {
        QString name = proc->restInput;
        List *restList = new List();
        ListBuilder builder(restList);
        while (paramIndex < paramCount) {
            builder.append(*(paramAry + paramIndex));
            paramIndex++;
        }
        setVarAsLocal(name);
        setValueForName(DatumPtr(restList), name);
    }
}


Datum *CallFrame::exec(Datum **paramAry, uint32_t paramCount) {
    applyProcedureParams(paramAry, paramCount);

    Datum *retval = bodyExec();

    // If the result is "nothing", replace the result with the ASTNode of the procedure.
    // TODO: Should we test for typeUnboundMask here instead?
    if (retval->isa == Datum::typeASTNode) {
        retval = sourceNode.astnodeValue();
    }


    return retval;
}

void CallFrame::applyContinuation(DatumPtr newNode, QList<DatumPtr> paramAry)
{
    sourceNode = newNode;
    Datum *newParamAry[paramAry.size()];
    for (int i = 0; i < paramAry.size(); i++) {
        newParamAry[i] = paramAry[i].datumValue();
    }

    applyProcedureParams(newParamAry, paramAry.size());
}

Datum *CallFrame::applyGoto(DatumPtr tag)
{
    Datum *procedure = sourceNode.astnodeValue()->procedure.datumValue();
    DatumPtr runningSourceListSnapshot;

    // Have we seen this tag already?
    Procedure *proc = static_cast<Procedure *>(procedure);
    auto blockIdIterator = proc->tagToBlockId.find(tag.wordValue()->keyValue());
    if (blockIdIterator != proc->tagToBlockId.end()) {
        goto foundTag;
    }

    // If not, then search through the remaining lines in the procedure.

    // Save our running state in case we need to restore it later.
    runningSourceListSnapshot = runningSourceList;

    // TODO: What if compilation results in an error?
    while (runningSourceList.isList() && runningSourceList.listValue()->isEmpty() == false) {
        List* list = runningSourceList.listValue()->head.listValue();
        Config::get().mainCompiler()->functionPtrFromList(list);
        blockIdIterator = proc->tagToBlockId.find(tag.wordValue()->keyValue());
        if (blockIdIterator != proc->tagToBlockId.end()) {
            goto foundTag;
        }
        runningSourceList = runningSourceList.listValue()->tail;
    }

    // If we still didn't find the tag, return an error.
    runningSourceList = runningSourceListSnapshot;
    // TODO: need the GOTO node passed in here.
    return FCError::doesntLike(tag, tag);

foundTag:
    // Now, we need to jump to the block that contains the tag.
    runningSourceList = proc->tagToLine[tag.wordValue()->keyValue()];
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
    while ((runningSourceList.isList()) && (runningSourceList.listValue()->isEmpty() == false)) {
        if (retval != nullptr) {
            retvalPtr = DatumPtr(retval);
        }
        DatumPtr instruction = runningSourceList.listValue()->head;

        Evaluator e = Evaluator(instruction, evalStack);
        retval = e.exec(jumpLocation);
        jumpLocation = 0;

        // Do we need to halt execution for some reason?
        if((retval->isa & Datum::typeUnboundMask) == 0)
        {
            switch (retval->isa)
            {
                // ERROR and RETURN simply return the error/return value.
                case Datum::typeError:
                case Datum::typeReturn:
                    return retval;
                case Datum::typeGoto:
                    retval = applyGoto(static_cast<FCGoto *>(retval)->tag());
                    if (retval == nullptr) {
                        goto continueBody;
                    }
                    Q_ASSERT(retval->isa == Datum::typeError);
                    return retval;
                case Datum::typeContinuation:
                {
                    FCContinuation *fc = static_cast<FCContinuation *>(retval);
                    applyContinuation(fc->procedure(), fc->params());
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


Evaluator::Evaluator(DatumPtr aList, QList<Evaluator *> &anEvalStack) : evalStack(anEvalStack), list(aList)
{
    evalStack.push_front(this);
}


Evaluator::~Evaluator()
{
    Q_ASSERT(evalStack.first() == this);

    //qDebug() << "draining pool";
    for (auto &d : releasePool)
    {
        (d->retainCount)--;
        if ((d != retval) && (d->retainCount < 1))
            delete d;
    }

    //qDebug() << "drained pool";
    evalStack.removeFirst();
}


Datum *Evaluator::exec(int32_t jumpLocation)
{
    if (list.listValue()->isEmpty()) {
        return &notADatum;
    }
    fn = Config::get().mainCompiler()->functionPtrFromList(list.listValue());
    retval = static_cast<Datum *>(fn((addr_t)this, jumpLocation));

    return retval;
}


Datum *Evaluator::subExec(Datum *aList)
{
    Datum *retval;

    try
    {
        if (aList->isa == Datum::typeWord) {
            DatumPtr runparsedList = runparse(DatumPtr(aList));
            aList = runparsedList.datumValue();
            watch(aList);
        }
        if (aList->isa != Datum::typeList) {
            FCError *err = FCError::noHow(DatumPtr(aList));
            watch(err);
            return err;
        }
        if (reinterpret_cast<List *>(aList)->isEmpty()) {
            return &notADatum;
        }
        Evaluator e(aList, evalStack);
            retval = e.exec();
    } catch (FCError *err)
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


Datum *Evaluator::watch(Datum *d)
{
    (d->retainCount)++;
    releasePool.push_back(d);
    return d;
}

bool Evaluator::areDatumsEqual(Datum *datum1, Datum *datum2, Qt::CaseSensitivity cs)
{
    comparedListsLHS.clear();
    comparedListsRHS.clear();
    return areDatumsEqualRecurse(datum1, datum2, cs);
}

bool Evaluator::areDatumsEqualRecurse(Datum *datum1, Datum *datum2, Qt::CaseSensitivity cs)
{
    if (datum1 == datum2)
        return true;
    if (datum1->isa != datum2->isa)
        return false;

    switch (datum1->isa)
    {
    case Datum::typeWord:
    {
        Word *word1 = static_cast<Word*>(datum1);
        Word *word2 = static_cast<Word*>(datum2);
        if (word1->isSourceNumber() || word2->isSourceNumber())
            return word1->numberValue() == word2->numberValue();

        return word1->printValue().compare(word2->printValue(), cs) == 0;
    }
    case Datum::typeList:
    {
        List *list1 = static_cast<List*>(datum1);
        List *list2 = static_cast<List*>(datum2);

        if (list1->count() != list2->count())
            return false;

               // If we have searched both of these lists before, then assume we would
               // keep searching forever.
        if (comparedListsLHS.contains(list1) && comparedListsRHS.contains(list2))
            return false;
        comparedListsLHS.insert(list1);
        comparedListsRHS.insert(list2);

        // Search the contents of each list.
        ListIterator iter1 = list1->newIterator();
        ListIterator iter2 = list2->newIterator();
        while (iter1.elementExists())
        {
            Datum *value1 = iter1.element().datumValue();
            Datum *value2 = iter2.element().datumValue();
            if (!areDatumsEqualRecurse(value1, value2, cs))
                return false;
        }
        return true;
    }
    case Datum::typeArray:
        // Arrays are equal iff they are the same array, which would have
        // passed the "datum1 == datum2" test at the beginning.
        return false;

    default:
        qDebug() << "unknown datum type in areDatumsEqualRecurse";
        Q_ASSERT(false);
    }
    return false;
}

bool Evaluator::isDatumInContainer(Datum *value, Datum *container, Qt::CaseSensitivity cs)
{
    searchedContainers.clear();
    return isDatumInContainerRecurse(value, container, cs);
}

bool Evaluator::isDatumInContainerRecurse(Datum *value, Datum *container, Qt::CaseSensitivity cs)
{
    if (searchedContainers.contains(container)) return false;
    searchedContainers.insert(container);

    switch (container->isa) {
        case Datum::typeArray:
        {
            Array *array = reinterpret_cast<Array*>(container);
            for (auto &item : array->array) {
                Datum *itemPtr = item.datumValue();
                if (areDatumsEqual(itemPtr, value, cs))
                    return true;
                if ((itemPtr->isa & (Datum::typeArray | Datum::typeList)) != 0)
                {
                    if ( ! searchedContainers.contains(itemPtr))
                    {
                        searchedContainers.insert(itemPtr);
                        if (isDatumInContainerRecurse(itemPtr, value, cs))
                            return true;
                    }
                }
            }
            break;
        }
        case Datum::typeList:
        {
            List *list = reinterpret_cast<List*>(container);
            ListIterator iter = list->newIterator();
            while (iter.elementExists())
            {
                Datum *itemPtr = iter.element().datumValue();
                if (areDatumsEqual(itemPtr, value, cs))
                    return true;
                if ((itemPtr->isa & (Datum::typeArray | Datum::typeList)) != 0)
                {
                    if ( ! searchedContainers.contains(itemPtr))
                    {
                        searchedContainers.insert(itemPtr);
                        if (isDatumInContainerRecurse(itemPtr, value, cs))
                            return true;
                    }
                }
            }
            break;
        }
        default:
            Q_ASSERT(false);
            break;
    }
    return false;
}

bool Evaluator::varCASEIGNOREDP()
{
    QString name = QObject::tr("CASEIGNOREDP");
    DatumPtr val = Config::get().mainKernel()->callStack.datumForName(name);
    bool retval = false;
    if (val.isa() == Datum::typeWord)
    {
        QString word = val.wordValue()->keyValue();
        retval = word == QObject::tr("TRUE");
    }
    return retval;
}
