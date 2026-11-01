// ===-- qlogo/exports/exports_controlstructures.cpp - Exports for the QLogo language -------*- C++ -*-===//
//
// Copyright 2017-2026 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the C library routines that are used by the QLogo compiler
/// for program flow control structures.
///
//===----------------------------------------------------------------------===//

#include "astnode.h"
#include "cmd_strings.h"
#include "compiler.h"
#include "datum_types.h"
#include "flowcontrol.h"
#include "interface/streammanager.h"
#include "workspace/exports_internal.h"
#include "workspace/kernel.h"
#include "workspace/procedures.h"
#include "workspace/specialvars.h"

#include <QElapsedTimer>
#include <QObject>
#include <QStack>
#include <QString>

/// Push a list onto the evaluation stack for explicit-control evaluation.
/// The caller will then suspend; the driver runs this list to completion, then resumes the caller.
/// If the list is invalid (e.g. not a list, or word that runparses to error), sets lastSubExecResult and does not push.
/// @param eAddr a pointer to the Evaluator object context.
/// @param listAddr a pointer to the list (or word to runparse) to execute.
EXPORTC void pushListOntoEvaluationStack(addr_t eAddr, addr_t listAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *aList = reinterpret_cast<Datum *>(listAddr);
    e->pushSublist(aList);
}

/// Tell evaluator to run this list after the current list has completed.
/// @param eAddr a pointer to the Evaluator object context.
/// @param listAddr a pointer to the list (or word to runparse) to execute.
/// @return Result for this evaluation (error, nothing, or nothing when a continuation was scheduled).
EXPORTC addr_t runListAfterCurrentList(addr_t eAddr, addr_t listAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *aList = reinterpret_cast<Datum *>(listAddr);
    return reinterpret_cast<addr_t>(e->runListAfterCurrentList(aList));
}

/// After a suspend that pushed a list or called a procedure, return that result.
/// @param eAddr a pointer to the Evaluator object context.
/// @return the result of the list/procedure execution.
EXPORTC addr_t getLastEvaluationResult(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *result = e->retvalFromChild_.datumValue();
    if ((result->isa_ & Datum::typeDataMask) != 0)
        e->watch(result);
    return reinterpret_cast<addr_t>(result);
}

namespace
{

QStack<QElapsedTimer> runtimeTimers;

/// Seconds from nanoseconds, with only as many fractional digits as needed.
QString secondsFromNsecs(qint64 nsecs)
{
    QString text = QString::number(nsecs / 1000000000LL);
    const qint64 frac = nsecs % 1000000000LL;
    if (frac == 0)
        return text;

    QString fracStr = QString::number(frac).rightJustified(9, QLatin1Char('0'));
    while (fracStr.endsWith(QLatin1Char('0')))
        fracStr.chop(1);
    return text + QLatin1Char('.') + fracStr;
}

} // namespace

/// RUNTIME: start a timer before the instruction list runs.
/// Nested RUNTIME calls push another timer so each reports its own elapsed time.
EXPORTC void stdRuntimeStart(void) noexcept
{
    QElapsedTimer timer;
    timer.start();
    runtimeTimers.push(timer);
}

/// RUNTIME: print elapsed seconds after the instruction list finishes.
/// Always paired with stdRuntimeStart in genRuntime, including when the list
/// returns flow control.
EXPORTC addr_t stdRuntimeEnd(addr_t eAddr, addr_t astnodeAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    Q_ASSERT(!runtimeTimers.isEmpty());
    const qint64 nsecs = runtimeTimers.pop().nsecsElapsed();
    return printOrError(e, reinterpret_cast<addr_t>(node), secondsFromNsecs(nsecs) + QObject::tr(" seconds\n"));
}

/// @brief Execute a procedure.
/// @param eAddr a pointer to the Evaluator object context.
/// @param astnodeAddr a pointer to the ASTNode object which is the procedure to execute.
/// @param paramAryAddr a pointer to an array of pointers to Datum objects which are the parameters to the procedure.
/// @param paramCount the number of parameters to the procedure.
/// @param outputRequiredMode 1 when the caller requires a value, 2 when the call is
/// the tail of a line and should inherit that requirement from a last body line,
/// 0 when a missing value is acceptable.
EXPORTC void beginProcedure(
    addr_t eAddr, addr_t astnodeAddr, addr_t paramAryAddr, uint32_t paramCount, int32_t outputRequiredMode) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto **paramAry = reinterpret_cast<Datum **>(paramAryAddr);
    e->beginProcedure(node, paramAry, paramCount, outputRequiredMode);
}

/// @brief True when the current procedure's caller requires a value.
/// @details .MAYBEOUTPUT uses this to turn a missing input into "didn't output".
EXPORTC bool procedureOutputRequired(void) noexcept
{
    Kernel &kernel = Kernel::get();
    if (kernel.callFrameStack_.empty())
        return false;
    return kernel.currentCallFrame()->outputRequired_;
}

/// @brief Create and return a RETURN control object.
/// @param eAddr a pointer to the Evaluator object context.
/// @param astNodeAddr a pointer to the ASTNode which is the source node of the RETURN.
/// @param retvalAddr a pointer to the Datum object which is the value to return.
/// @return a pointer to the RETURN control object that was generated.
EXPORTC addr_t getCtrlReturn(addr_t eAddr, addr_t astNodeAddr, addr_t retvalAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto retval = DatumPtr(reinterpret_cast<Datum *>(retvalAddr));
    auto *control = new FCReturn(DatumPtr(reinterpret_cast<Datum *>(astNodeAddr)), retval);
    e->watch(control);
    return reinterpret_cast<addr_t>(control);
}

/// @brief Create and return a CONTINUATION control object.
/// @param eAddr a pointer to the Evaluator object context.
/// @param astNodeAddr the procedure call to tail-call.
/// @param argAryAddr a pointer to an array of pointers to Datum objects which are the inputs.
/// @param argCount the number of inputs.
/// @param demandNothing true when the tail call is STOP (command position): the
///        continued procedure must not output a value.
/// @param sourceNodeAddr OUTPUT, STOP, or .MAYBEOUTPUT. The procedure to run is
///        astNodeAddr; this node is the command that tail-called it.
/// @param allowNothing true for STOP and .MAYBEOUTPUT, where the continued
///        procedure may produce no value. OUTPUT leaves this false.
/// @return a pointer to the CONTINUATION control object that was generated.
EXPORTC addr_t getCtrlContinuation(addr_t eAddr,
                                   addr_t astNodeAddr,
                                   addr_t argAryAddr,
                                   uint32_t argCount,
                                   bool demandNothing,
                                   addr_t sourceNodeAddr,
                                   bool allowNothing) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astNodeAddr);
    auto *sourceNode = reinterpret_cast<ASTNode *>(sourceNodeAddr);
    auto **argAry = reinterpret_cast<Datum **>(argAryAddr);
    auto nodePtr = DatumPtr(node);

    DatumPtr arguments = emptyList();
    for (int i = argCount - 1; i >= 0; i--)
    {
        arguments = DatumPtr(new List(argAry[i], arguments.listValue()));
    }
    // Assign the Logo-style linked list to the first element of the parameters QList.
    QList<DatumPtr> args{arguments};
    auto *control = new FCContinuation(DatumPtr(sourceNode), nodePtr, args, demandNothing, allowNothing);
    e->watch(control);
    return reinterpret_cast<addr_t>(control);
}

/// @brief Create and return a GOTO control object.
/// @param eAddr a pointer to the Evaluator object context.
/// @param astNodeAddr a pointer to the ASTNode which is the source node of the GOTO.
/// @param tagAddr a pointer to the Datum object which is the tag of the GOTO.
/// @return a pointer to the GOTO control object that was generated, or error if the tag is not found.
EXPORTC addr_t getCtrlGoto(addr_t eAddr, addr_t astNodeAddr, addr_t tagAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto tag = DatumPtr(reinterpret_cast<Datum *>(tagAddr));
    CallFrame *cf = Kernel::get().currentCallFrame();

    auto astNode = reinterpret_cast<ASTNode *>(astNodeAddr);

    // If node is nullptr, return error for not being in a procedure.
    if (cf->sourceNode_.isNothing())
    {
        return reinterpret_cast<addr_t>(FCError::notInsideProcedure(astNode->nodeName_));
    }
    ASTNode *node = cf->sourceNode_.astnodeValue();

    DatumPtr procedure = node->procedure_;
    auto &tagToLineAndBlockId = procedure.procedureValue()->tagToLineAndBlockId_;
    const QString tagKey = tag.toString(Datum::ToStringFlags_Key);

    auto makeGoto = [&](const DatumPtr &line, int32_t blockId) -> addr_t {
        auto *control = new FCGoto(DatumPtr(astNodeAddr), line, blockId);
        e->watch(control);
        return reinterpret_cast<addr_t>(control);
    };

    // If the tag was registered on a previous run/compile, jump using the stored line.
    auto blockIdIterator = tagToLineAndBlockId.find(tagKey);
    if (blockIdIterator != tagToLineAndBlockId.end())
    {
        auto [line, blockId] = blockIdIterator.value();
        return makeGoto(line, blockId);
    }

    // Tag not yet known: compile following lines until it is registered (or we run out).
    // Head of runningSourceList_ is the GOTO line still in progress; search its tail.
    DatumPtr runningList = cf->runningSourceList_.listValue()->tail;
    while (!runningList.listValue()->isEmpty())
    {
        Compiler::get().setTagLineLocation(runningList);
        addr_t compiled = catchError(e, static_cast<addr_t>(0), [&] {
            Compiler::get().compiledTextFromList(runningList.listValue()->head.listValue());
        });
        if (compiled != 0)
        {
            Compiler::get().clearTagLineLocation();
            return compiled;
        }
        Compiler::get().clearTagLineLocation();

        blockIdIterator = tagToLineAndBlockId.find(tagKey);
        if (blockIdIterator != tagToLineAndBlockId.end())
        {
            auto [line, blockId] = blockIdIterator.value();
            return makeGoto(line, blockId);
        }
        runningList = runningList.listValue()->tail;
    }

    return reinterpret_cast<addr_t>(FCError::doesntLike(astNode->nodeName_, tag));
}

/// @brief return the address of the repcount variable.
EXPORTC addr_t repcountAddr(void) noexcept
{
    void *retval = &Kernel::get().repcount_;
    return (addr_t)retval;
}

EXPORTC addr_t beginCatch(void) noexcept
{
    Word *erractName = SpecialVars::word(SpecialVars::Id::ErrAct);
    Datum *erractValue = Kernel::get().datumForName(erractName).datumValue();

    // Save the erract value.
    if ((erractValue->isa_ & Datum::typeNothing) == 0)
    {
        erractValue->retainCount_++;
        Kernel::get().setDatumForName(nothing(), erractName);
    }
    return reinterpret_cast<addr_t>(erractValue);
}

EXPORTC addr_t endCatch(addr_t eAddr, addr_t nodeAddr, addr_t errActAddr, addr_t resultAddr, addr_t tagAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    Word *erractName = SpecialVars::word(SpecialVars::Id::ErrAct);
    auto *erractValue = reinterpret_cast<Datum *>(errActAddr);
    auto *result = reinterpret_cast<Datum *>(resultAddr);
    auto *tag = reinterpret_cast<Word *>(tagAddr);

    // Restore the erract value.
    if ((erractValue->isa_ & Datum::typeNothing) == 0)
    {
        DatumPtr erractValuePtr = DatumPtr(erractValue);
        Kernel::get().setDatumForName(erractValuePtr, erractName);
        erractValue->retainCount_--;
    }

    if ((result->isa_ & Datum::typeError) != 0)
    {
        auto *err = reinterpret_cast<FCError *>(result);
        QString tagStr = tag->toString(Datum::ToStringFlags_Key);

        if ((tagStr == QObject::tr("ERROR")) &&
            (((err->code_ == ErrCode::ERR_NO_CATCH) &&
              (err->tag().toString(Datum::ToStringFlags_Key) == QObject::tr("ERROR"))) ||
             (err->code_ != ErrCode::ERR_NO_CATCH)))
        {
            e->watch(err);
            return nodeAddr;
        }
        else if ((err->code_ == ErrCode::ERR_NO_CATCH) && (err->tag().toString(Datum::ToStringFlags_Key) == tagStr))
        {
            e->watch(err);
            auto retval = reinterpret_cast<addr_t>(err->output().datumValue());
            Kernel::get().currentError_ = nothing();
            return retval;
        }
        return resultAddr;
    }

    return reinterpret_cast<addr_t>(result);
}

EXPORTC addr_t getCurrentError(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr errPtr = Kernel::get().currentError_;

    ListBuilder retvalBuilder;
    if (!errPtr.isNothing())
    {
        auto *err = reinterpret_cast<FCError *>(errPtr.datumValue());
        retvalBuilder.append(DatumPtr(err->code_));
        retvalBuilder.append(err->message());
        retvalBuilder.append(err->procedure());
        retvalBuilder.append(err->line());
    }
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t callPause(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    return catchError(e, [&]() -> addr_t {
        DatumPtr retval = Kernel::get().pause();
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    });
}

EXPORTC addr_t generateContinue(addr_t eAddr, addr_t outputAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *output = reinterpret_cast<Datum *>(outputAddr);

    FCError *err = FCError::custom(DatumPtr(QObject::tr("PAUSE")), nothing(), DatumPtr(output));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// MARK: set a debug flag on the datum so its destruction is logged.
/// Prints MARK immediately, then DELE when the last DatumPtr drops.
/// @param thingAddr a pointer to the Datum to mark
/// @return the same Datum
EXPORTC addr_t stdMark(addr_t thingAddr) noexcept
{
    DatumPtr thing(reinterpret_cast<Datum *>(thingAddr));
    thing.alertOnDelete();
    return thingAddr;
}

EXPORTC addr_t processRunresult(addr_t eAddr, addr_t resultAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *result = reinterpret_cast<Datum *>(resultAddr);
    Datum *retval;

    if ((result->isa_ & Datum::typeDataMask) != 0)
    {
        retval = new List(result, EmptyList::instance());
    }
    else if ((result->isa_ & Datum::typeUnboundMask) != 0)
    {
        retval = EmptyList::instance();
    }
    else
    {
        // Pass through whatever we got because it's not good.
        return resultAddr;
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC void saveTestResult(bool tf) noexcept
{
    Kernel::get().setTest(tf);
}

EXPORTC int8_t getTestResult(void) noexcept
{
    return Kernel::get().testedState();
}

/// @brief Determine the template form for the `APPLY` operator.
/// @param eAddr the address of the Evaluator object
/// @param templateAddr a pointer to the template
/// @return the template form (as an integer)
EXPORTC int8_t getTemplateForm(addr_t eAddr, addr_t templateAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *aTemplate = reinterpret_cast<Datum *>(templateAddr);

    if (aTemplate->isWord())
    {
        return TemplateFormNamedProcedure; // "sum
    }
    if ((!aTemplate->isList()) || (aTemplate->listValue()->isEmpty()))
    {
        return TemplateFormInvalid;
    }

    DatumPtr first = aTemplate->listValue()->head;
    if (first.isWord())
    {
        return TemplateFormExplicitSlot; // [? * ?]
    }
    if ((!first.isList()) || (aTemplate->listValue()->count() < 2))
    {
        return TemplateFormInvalid;
    }

    DatumPtr butfirst = aTemplate->listValue()->tail;
    DatumPtr procedureFirst = butfirst.listValue()->head;

    if (procedureFirst.isList())
    {
        return TemplateFormProcedureText; // [[x y] [OUTPUT :x + :y]]
    }

    return TemplateFormLambda; // [[x y] :x + :y]
}

/// @brief Build a call AST for named-procedure APPLY.
/// @param eAddr the address of the Evaluator object
/// @param templateAddr a pointer to the procedure name (a Word)
/// @param inputlistAddr a pointer to the already-evaluated input list
/// @return a one-node List whose head is the call AST, or an error
EXPORTC addr_t buildApplyNamedCall(addr_t eAddr, addr_t astNodeAddr, addr_t templateAddr, addr_t inputlistAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *astNode = reinterpret_cast<Datum *>(astNodeAddr);
    auto *aTemplate = reinterpret_cast<Datum *>(templateAddr);
    auto *inputlist = reinterpret_cast<Datum *>(inputlistAddr);

    Q_ASSERT(aTemplate->isWord());
    Q_ASSERT(inputlist->isList());

    QString name = aTemplate->toString(Datum::ToStringFlags_Key);
    if (name == StringConstants::cmdStrTO() || name == StringConstants::cmdStr_dot_MACRO())
    {
        FCError *err = FCError::doesntLike(DatumPtr(astNode), DatumPtr(aTemplate));
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }

    return catchError(e, [&]() -> addr_t {
        DatumPtr node = Procedures::get().astnodeWithLiterals(DatumPtr(aTemplate), DatumPtr(inputlist));
        auto *callList = new List(node, EmptyList::instance());
        e->watch(callList);
        return reinterpret_cast<addr_t>(callList);
    });
}

// TODO: We should consider including the exports header, because this is kinda out of place.
EXPORTC addr_t handleBadDatum(addr_t eAddr, addr_t parentAddr, addr_t valueAddr) noexcept;

/// @brief Return the explicit-slot value for `?` / `(? n)`.
/// @param eAddr the address of the Evaluator object
/// @param parentAddr a pointer to the `?` ASTNode (for errors)
/// @param indexAddr a 1-based slot index, or notADatum if `?` was given no input
/// @return the slot datum, or an error
EXPORTC addr_t getExplicitSlot(addr_t eAddr, addr_t parentAddr, addr_t indexAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *parent = reinterpret_cast<ASTNode *>(parentAddr);
    DatumPtr who = parent->nodeName_;
    DatumPtr index(reinterpret_cast<Datum *>(indexAddr));
    FCError *err = nullptr;

    qsizetype i = 1;
    if (!index.isNothing())
    {
        if (!index.isWord())
            goto recover;
        Word *w = index.wordValue();
        double n = w->numberValue();
        i = static_cast<qsizetype>(n);
        if (!w->numberIsValid || i != n || i < 1)
            goto recover;
    }

    {
        DatumPtr slotList = Kernel::get().explicitSlotList();
        if (!slotList.isList())
        {
            err = FCError::notEnoughInputs(who);
            goto bailout;
        }

        ListIterator iter = slotList.listValue()->newIterator();
        qsizetype remaining = i;
        while (iter.elementExists())
        {
            DatumPtr element = iter.element();
            remaining--;
            if (remaining == 0)
            {
                e->watch(element);
                return reinterpret_cast<addr_t>(element.datumValue());
            }
        }

        goto recover;
    }

recover:
{
    addr_t recovered = handleBadDatum(eAddr, parentAddr, indexAddr);
    DatumPtr result(reinterpret_cast<Datum *>(recovered));
    if (result.isErr())
        return recovered;
    return getExplicitSlot(eAddr, parentAddr, recovered);
}

bailout:
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// @brief Push an explicit-slot list for `?`.
EXPORTC void pushExplicitSlotList(addr_t listAddr) noexcept
{
    auto *list = reinterpret_cast<Datum *>(listAddr);
    Kernel::get().explicitSlotListStack_.push(DatumPtr(list));
}

/// @brief Pop the current explicit-slot list.
EXPORTC void popExplicitSlotList(void) noexcept
{
    Q_ASSERT(!Kernel::get().explicitSlotListStack_.isEmpty());
    Kernel::get().explicitSlotListStack_.pop();
}

/// @brief Bind lambda APPLY names to inputlist members and return the template body.
/// @param eAddr the address of the Evaluator object
/// @param astNodeAddr a pointer to the APPLY command name (for errors)
/// @param templateAddr a pointer to the lambda template list
/// @param inputlistAddr a pointer to the already-evaluated input list
/// @return the template tail (instruction list) on success, or an error
EXPORTC addr_t beginApplyLambda(addr_t eAddr, addr_t astNodeAddr, addr_t templateAddr, addr_t inputlistAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *who = reinterpret_cast<Datum *>(astNodeAddr);
    auto *aTemplate = reinterpret_cast<Datum *>(templateAddr);
    auto *inputlist = reinterpret_cast<Datum *>(inputlistAddr);

    Q_ASSERT(aTemplate->isList());
    Q_ASSERT(inputlist->isList());

    List *tmpl = aTemplate->listValue();
    Q_ASSERT(!tmpl->isEmpty() && tmpl->head.isList());

    List *names = tmpl->head.listValue();
    List *body = tmpl->tail.listValue();
    List *inputs = inputlist->listValue();
    DatumPtr whoP(who);

    ListIterator nameIter = names->newIterator();
    while (nameIter.elementExists())
    {
        DatumPtr nameP = nameIter.element();
        if (!nameP.isWord())
        {
            FCError *err = FCError::doesntLike(whoP, nameP);
            e->watch(err);
            return reinterpret_cast<addr_t>(err);
        }
    }

    const int nameCount = names->count();
    const int inputCount = inputs->count();
    if (nameCount < inputCount)
    {
        FCError *err = FCError::tooManyInputs(whoP);
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    if (nameCount > inputCount)
    {
        FCError *err = FCError::notEnoughInputs(whoP);
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }

    Kernel &k = Kernel::get();
    ApplyLambdaBinding frame;
    nameIter = names->newIterator();
    ListIterator inputIter = inputs->newIterator();
    while (nameIter.elementExists())
    {
        Word *name = nameIter.element().wordValue()->interned();
        DatumPtr value = inputIter.element();
        ApplyLambdaSavedVar saved;
        saved.name = name;
        saved.existed = k.doesExist(name);
        saved.previous = saved.existed ? k.datumForName(name) : nothing();
        frame.saved.append(saved);
        k.setDatumForName(value, name);
    }

    k.applyLambdaBindingStack_.push(frame);
    e->watch(body);
    return reinterpret_cast<addr_t>(body);
}

/// @brief Build a call AST for procedure-text APPLY.
/// @param eAddr the address of the Evaluator object
/// @param astNodeAddr a pointer to the APPLY command name (for errors)
/// @param templateAddr a pointer to the procedure-text template list
/// @param inputlistAddr a pointer to the already-evaluated input list
/// @return a one-node List whose head is the call AST, or an error
EXPORTC addr_t buildApplyProcedureText(addr_t eAddr,
                                       addr_t astNodeAddr,
                                       addr_t templateAddr,
                                       addr_t inputlistAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *who = reinterpret_cast<Datum *>(astNodeAddr);
    auto *aTemplate = reinterpret_cast<Datum *>(templateAddr);
    auto *inputlist = reinterpret_cast<Datum *>(inputlistAddr);

    Q_ASSERT(aTemplate->isList());
    Q_ASSERT(inputlist->isList());

    DatumPtr whoP(who);
    DatumPtr templateP(aTemplate);
    List *tmpl = aTemplate->listValue();
    Q_ASSERT(!tmpl->isEmpty() && tmpl->head.isList());

    return catchError(e, [&]() -> addr_t {
        for (List *lineList = tmpl->tail.listValue(); !lineList->isEmpty(); lineList = lineList->tail.listValue())
        {
            if (!lineList->head.isList())
                throw FCError::doesntLike(whoP, lineList->head);
        }

        auto [minParams, defaultParams, maxParams] = Procedures::get().validateParameters(whoP, tmpl->head);

        int countOfChildren = inputlist->listValue()->count();
        if (countOfChildren < minParams)
            throw FCError::notEnoughInputs(whoP);
        if ((countOfChildren > maxParams) && (maxParams != -1))
            throw FCError::tooManyInputs(whoP);

        auto *body = new Procedure();
        DatumPtr bodyP(body);
        body->countOfMinParams_ = minParams;
        body->countOfDefaultParams_ = defaultParams;
        body->countOfMaxParams_ = maxParams;
        body->instructionList_ = templateP;

        auto node = DatumPtr(new ASTNode(whoP));
        node.astnodeValue()->genExpression_ = &Compiler::genExecProcedure;
        node.astnodeValue()->produces_ = TypeDN;
        node.astnodeValue()->procedure_ = bodyP;

        ListIterator iter = inputlist->listValue()->newIterator();
        while (iter.elementExists())
        {
            DatumPtr p = iter.element();
            auto datumNode = DatumPtr(new ASTNode(QObject::tr("literal")));
            datumNode.astnodeValue()->genExpression_ = &Compiler::genLiteral;
            datumNode.astnodeValue()->produces_ = TypeDatum;
            datumNode.astnodeValue()->addChild(p);
            node.astnodeValue()->addChild(datumNode);
        }

        auto *callList = new List(node, EmptyList::instance());
        e->watch(callList);
        return reinterpret_cast<addr_t>(callList);
    });
}

/// @brief Restore variable bindings from the most recent beginApplyLambda.
EXPORTC void endApplyLambda(void) noexcept
{
    Kernel &k = Kernel::get();
    Q_ASSERT(!k.applyLambdaBindingStack_.isEmpty());
    ApplyLambdaBinding frame = k.applyLambdaBindingStack_.pop();
    for (auto it = frame.saved.crbegin(); it != frame.saved.crend(); ++it)
    {
        if (!it->existed)
            k.eraseVar(it->name);
        else
            k.setDatumForName(it->previous, it->name);
    }
}
