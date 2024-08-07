
//===-- qlogo/kernel.cpp - Kernel class implementation -------*- C++ -*-===//
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
/// This file contains a part of the implementation of the Kernel class, which is the
/// executor proper of the QLogo language. Specifically, this file contains the
/// implementations for operations involving control structure commands, such as
/// REPEAT, IF, STOP, OUTPUT, etc.
///
/// See README.md in this directory for information about the documentation
/// structure for each Kernel::exc* method.
///
//===----------------------------------------------------------------------===//

#include "QtCore/qdatetime.h"
#include "astnode.h"
#include "datum.h"
#include "error.h"
#include "kernel.h"
#include "parser.h"
#include <QObject>

// CONTROL STRUCTURES

/***DOC RUN
RUN instructionlist

    command or operation.  Runs the Logo instructions in the input
    list; outputs if the list contains an expression that outputs.

COD***/
// CMD RUN 1 1 1
DatumPtr Kernel::excRun(DatumPtr node)
{
    ProcedureHelper h(this, node);

    DatumPtr instructionList =
        h.validatedDatumAtIndex(0, [](DatumPtr candidate) { return candidate.isWord() || candidate.isList(); });

    return h.ret(runList(instructionList));
}

/***DOC TIME
TIME instructionlist

    runs the instructions in the input; prints the amount of time
    (in seconds) the command or operation takes to complete; outputs
    if the list contains an expression that outputs.

COD***/
// CMD TIME 1 1 1
DatumPtr Kernel::excTime(DatumPtr node)
{
    ProcedureHelper h(this, node);

    DatumPtr instructionList =
        h.validatedDatumAtIndex(0, [](DatumPtr candidate) { return candidate.isWord() || candidate.isList(); });

    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    DatumPtr retval = runList(instructionList);
    qint64 endTime = QDateTime::currentMSecsSinceEpoch();
    double timeInSeconds = ((double)(endTime - startTime)) / 1000.0;
    QString report = "Time: %1 seconds\n";
    stdPrint(report.arg(timeInSeconds));
    return h.ret(retval);
}

/***DOC MARK
MARK value

    Takes the input, places a mark on it, and outputs whatever was input. The
    address of the marked value will be printed out. Later if or when the value
    is no longer being used by QLogo, a message will again be printed out when
    the internal representation of the value is destroyed, including the address.

    Example: MAKE "A MARK 2+2

    or:

    PRINT MARK "Hello

COD***/
// CMD MARK 1 1 1
DatumPtr Kernel::excMark(DatumPtr node)
{
    ProcedureHelper h(this, node);

    DatumPtr item = h.datumAtIndex(0);
    item.alertOnDelete();

    return item;
}

/***DOC RUNRESULT
RUNRESULT instructionlist

    runs the instructions in the input; outputs an empty list if
    those instructions produce no output, or a list whose only
    member is the output from running the input instructionlist.
    Useful for inventing command-or-operation control structures:

        local "result
        make "result runresult [something]
        if emptyp :result [stop]
        output first :result

COD***/
// CMD RUNRESULT 1 1 1
DatumPtr Kernel::excRunresult(DatumPtr node)
{
    ProcedureHelper h(this, node);

    DatumPtr instructionList =
        h.validatedDatumAtIndex(0, [](DatumPtr candidate) { return candidate.isWord() || candidate.isList(); });

    List *retval = new List();
    DatumPtr temp = runList(instructionList);

    if (temp.isASTNode())
    {
        temp = Error::insideRunresult(temp.astnodeValue()->nodeName);
    }

    if (temp != nothing)
    {
        retval->append(temp);
    }

    return h.ret(retval);
}

/***DOC BYE
BYE

    command.  Exits from Logo.

COD***/
// CMD BYE 0 0 0
DatumPtr Kernel::excBye(DatumPtr node)
{
    ProcedureHelper h(this, node);

    Error::throwError(DatumPtr(QObject::tr("SYSTEM")), nothing);

    return nothing;
}

/***DOC REPEAT
REPEAT num instructionlist

    command.  Runs the "instructionlist" repeatedly, "num" times.

COD***/
// CMD REPEAT 2 2 2
DatumPtr Kernel::excRepeat(DatumPtr node)
{
    ProcedureHelper h(this, node);
    int countValue = h.validatedIntegerAtIndex(0, [](int candidate) { return candidate >= 0; });
    DatumPtr commandList = h.listAtIndex(1); // TODO: this can execute a word, too, right?

    int tempRepcount = repcount;
    repcount = 1;

    DatumPtr retval;
    try
    {
        while ((countValue > 0) && (retval == nothing))
        {
            retval = runList(commandList);
            --countValue;
            ++repcount;
        }
    }
    catch (Error *e)
    {
        // TODO: RAII
        repcount = tempRepcount;
        throw e;
    }
    repcount = tempRepcount;
    return h.ret(retval);
}

/***DOC FOREVER
FOREVER instructionlist

    command.  Runs the "instructionlist" repeatedly, until something
    inside the instructionlist (such as STOP or THROW) makes it stop.

COD***/
// CMD FOREVER 1 1 1
DatumPtr Kernel::excForever(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr commandList = h.listAtIndex(0);

    int tempRepcount = repcount;
    repcount = 1;

    DatumPtr retval;
    try
    {
        while (retval == nothing)
        {
            retval = runList(commandList);
            ++repcount;
        }
    }
    catch (Error *e)
    {
        repcount = tempRepcount;
        throw e;
    }
    repcount = tempRepcount;
    return h.ret(retval);
}

/***DOC REPCOUNT #
REPCOUNT
#

    outputs the repetition count of the innermost current REPEAT or
    FOREVER, starting from 1.  If no REPEAT or FOREVER is active,
    outputs -1.

    The abbreviation # can be used for REPCOUNT unless the REPEAT is
    inside the template input to a higher order procedure such as
    FOREACH, in which case # has a different meaning.

COD***/
// CMD REPCOUNT 0 0 0
DatumPtr Kernel::excRepcount(DatumPtr node)
{
    ProcedureHelper h(this, node);

    return h.ret(repcount);
}

/***DOC IF
IF tf instructionlist
(IF tf instructionlist1 instructionlist2)

    command.  If the first input has the value TRUE, then IF runs
    the second input.  If the first input has the value FALSE, then
    IF does nothing.  (If given a third input, IF acts like IFELSE,
    as described below.)  It is an error if the first input is not
    either TRUE or FALSE.

COD***/

/***DOC IFELSE
IFELSE tf instructionlist1 instructionlist2

    command or operation.  If the first input has the value TRUE, then
    IFELSE runs the second input.  If the first input has the value FALSE,
    then IFELSE runs the third input.  IFELSE outputs a value if the
    instructionlist contains an expression that outputs a value.

COD***/
// CMD IF 2 2 3
// CMD IFELSE 3 3 3
DatumPtr Kernel::excIfelse(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr retval;
    if (h.boolAtIndex(0, true))
    {
        retval = runList(h.datumAtIndex(1));
    }
    else
    {
        if (h.countOfChildren() == 3)
            retval = runList(h.datumAtIndex(2));
    }
    return h.ret(retval);
}

/***DOC TEST
TEST tf

    command.  Remembers its input, which must be TRUE or FALSE, for use
    by later IFTRUE or IFFALSE instructions.  The effect of TEST is local
    to the procedure in which it is used; any corresponding IFTRUE or
    IFFALSE must be in the same procedure or a subprocedure.

COD***/
// CMD TEST 1 1 1
DatumPtr Kernel::excTest(DatumPtr node)
{
    ProcedureHelper h(this, node);

    bool testVal = h.boolAtIndex(0, true);
    callStack.setTest(testVal);
    return nothing;
}

/***DOC IFTRUE IFT
IFTRUE instructionlist
IFT instructionlist

    command.  Runs its input if the most recent TEST instruction had
    a TRUE input.  The TEST must have been in the same procedure or a
    superprocedure.

COD***/
// CMD IFTRUE 1 1 1
// CMD IFT 1 1 1
DatumPtr Kernel::excIftrue(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr retval;
    if (!callStack.isTested())
        return h.ret(Error::noTest(node.astnodeValue()->nodeName));
    if (callStack.testedState())
    {
        retval = runList(h.datumAtIndex(0));
    }
    return h.ret(retval);
}

/***DOC IFFALSE IFF
IFFALSE instructionlist
IFF instructionlist

    command.  Runs its input if the most recent TEST instruction had
    a FALSE input.  The TEST must have been in the same procedure or a
    superprocedure.

COD***/
// CMD IFFALSE 1 1 1
// CMD IFF 1 1 1
DatumPtr Kernel::excIffalse(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr retval;
    if (!callStack.isTested())
        return h.ret(Error::noTest(node.astnodeValue()->nodeName));
    if (!callStack.testedState())
    {
        retval = runList(h.datumAtIndex(0));
    }
    return h.ret(retval);
}

// The commands STOP, OUTPUT, and .MAYBEOUTPUT return an ASTNode instead of a
// Word, List, or Array.
//
// The caller is responsible for dissecting the node and acting appropriately.
//

/***DOC STOP
STOP

    command.  Ends the running of the procedure in which it appears.
    Control is returned to the context in which that procedure was
    invoked.  The stopped procedure does not output a value.

COD***/
// CMD STOP 0 0 1
DatumPtr Kernel::excStop(DatumPtr node)
{
    Q_ASSERT(callStack.size() > 0);
    if (callStack.localFrame()->sourceNode.isNothing())
    {
        Error::notInsideProcedure(node.astnodeValue()->nodeName);
    }
    return node;
}

/***DOC OUTPUT OP
OUTPUT value
OP value

    command.  Ends the running of the procedure in which it appears.
    That procedure outputs the value "value" to the context in which
    it was invoked.  Don't be confused: OUTPUT itself is a command,
    but the procedure that invokes OUTPUT is an operation.

COD***/
// CMD OUTPUT 1 1 1
// CMD OP 1 1 1
DatumPtr Kernel::excOutput(DatumPtr node)
{
    Q_ASSERT(callStack.size() > 1);
    if (callStack.localFrame()->sourceNode.isNothing())
    {
        Error::notInsideProcedure(node.astnodeValue()->nodeName);
    }
    return node;
}

/***DOC .MAYBEOUTPUT
.MAYBEOUTPUT value					(special form)

    works like OUTPUT except that the expression that provides the
    input value might not, in fact, output a value, in which case
    the effect is like STOP.  This is intended for use in control
    structure definitions, for cases in which you don't know whether
    or not some expression produces a value.  Example:

        to invoke :function [:inputs] 2
        .maybeoutput apply :function :inputs
        end

        ? (invoke "print "a "b "c)
        a b c
        ? print (invoke "word "a "b "c)
        abc

    This is an alternative to RUNRESULT.  It's fast and easy to use,
    at the cost of being an exception to Logo's evaluation rules.
    (Ordinarily, it should be an error if the expression that's
    supposed to provide an input to something doesn't have a value.)

COD***/
// CMD .MAYBEOUTPUT 1 1 1
DatumPtr Kernel::excDotMaybeoutput(DatumPtr node)
{
    Q_ASSERT(callStack.size() > 0);
    if (callStack.localFrame()->sourceNode.isNothing())
    {
        Error::notInsideProcedure(node.astnodeValue()->nodeName);
    }
    return node;
}

/***DOC CATCH
CATCH tag instructionlist

    command or operation.  Runs its second input.  Outputs if that
    instructionlist outputs.  If, while running the instructionlist,
    a THROW instruction is executed with a tag equal to the first
    input (case-insensitive comparison), then the running of the
    instructionlist is terminated immediately.  In this case the CATCH
    outputs if a value input is given to THROW.  The tag must be a word.

    If the tag is the word ERROR, then any error condition that arises
    during the running of the instructionlist has the effect of THROW
    "ERROR instead of printing an error message and returning to
    toplevel.  The CATCH does not output if an error is caught.  Also,
    during the running of the instructionlist, the variable ERRACT is
    temporarily unbound.  (If there is an error while ERRACT has a
    value, that value is taken as an instructionlist to be run after
    printing the error message.  Typically the value of ERRACT, if any,
    is the list [PAUSE].)

COD***/
// CMD CATCH 2 2 2
DatumPtr Kernel::excCatch(DatumPtr node)
{
    ProcedureHelper h(this, node);
    QString tag = h.wordAtIndex(0).wordValue()->keyValue();
    DatumPtr instructionlist = h.listAtIndex(1);
    DatumPtr retval;
    DatumPtr tempErract = callStack.datumForName(QObject::tr("ERRACT"));

    if (callStack.doesExist(QObject::tr("ERRACT")))
    {
        callStack.setDatumForName(nothing, QObject::tr("ERRACT"));
    }

    try
    {
        retval = runList(instructionlist);
        if (retval.isASTNode())
        {
            KernelMethod method = retval.astnodeValue()->kernel;
            if (method == &Kernel::excStop)
            {
                retval = nothing;
            }
            else if ((method == &Kernel::excOutput) || (method == &Kernel::excDotMaybeoutput) ||
                     ((method == &Kernel::excStop) && (retval.astnodeValue()->countOfChildren() > 0)))
            {
                DatumPtr p = retval.astnodeValue()->childAtIndex(0);
                KernelMethod temp_method = p.astnodeValue()->kernel;
                DatumPtr temp_retval = (this->*temp_method)(p);
                if ((temp_retval == nothing) && (method == &Kernel::excOutput))
                {
                    Error::didntOutput(p.astnodeValue()->nodeName, retval.astnodeValue()->nodeName);
                }
                if ((temp_retval != nothing) && (method == &Kernel::excStop))
                {
                    Error::dontSay(retval.astnodeValue()->nodeName);
                }
                retval = temp_retval;
            }
            else
            {
                retval = (this->*method)(retval);
            }
        }
    }
    catch (Error *e)
    {
        if (callStack.doesExist(QObject::tr("ERRACT")))
        {
            callStack.setDatumForName(tempErract, QObject::tr("ERRACT"));
        }

        if ((tag == QObject::tr("ERROR")) &&
            (((e->code == 14) && (e->tag.wordValue()->keyValue()) == QObject::tr("ERROR")) || (e->code != 14)))
        {
            ProcedureHelper::setIsErroring(false);
            return nothing;
        }
        else if ((e->code == 14) && (tag == e->tag.wordValue()->keyValue()))
        {
            DatumPtr retval = e->output;
            registerError(nothing);
            return h.ret(retval);
        }
        throw e;
    }

    if (callStack.doesExist(QObject::tr("ERRACT")))
    {
        callStack.setDatumForName(tempErract, QObject::tr("ERRACT"));
    }
    return h.ret(retval);
}

/***DOC THROW
THROW tag
(THROW tag value)

    command.  Must be used within the scope of a CATCH with an equal
    tag.  Ends the running of the instructionlist of the CATCH.  If
    THROW is used with only one input, the corresponding CATCH does
    not output a value.  If THROW is used with two inputs, the second
    provides an output for the CATCH.

    THROW "TOPLEVEL can be used to terminate all running procedures and
    interactive pauses, and return to the toplevel instruction prompt.
    Typing the system interrupt character (alt-S for wxWidgets; otherwise
    normally control-C for Unix, control-Q for DOS, or command-period for
    Mac) has the same effect.

    THROW "ERROR can be used to generate an error condition.  If the
    error is not caught, it prints a message (THROW "ERROR) with the
    usual indication of where the error (in this case the THROW)
    occurred.  If a second input is used along with a tag of ERROR,
    that second input is used as the text of the error message
    instead of the standard message.  Also, in this case, the location
    indicated for the error will be, not the location of the THROW,
    but the location where the procedure containing the THROW was
    invoked.  This allows user-defined procedures to generate error
    messages as if they were primitives.  Note: in this case the
    corresponding CATCH "ERROR, if any, does not output, since the second
    input to THROW is not considered a return value.

    THROW "SYSTEM immediately leaves Logo, returning to the operating
    system, without printing the usual parting message and without
    deleting any editor temporary file written by EDIT.

COD***/
// CMD THROW 1 1 2
DatumPtr Kernel::excThrow(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr tag = h.wordAtIndex(0);
    DatumPtr value;
    if (h.countOfChildren() > 1)
    {
        value = h.datumAtIndex(1);
        if (!value.isWord())
            value = DatumPtr(value.printValue());
    }

    Error::throwError(tag, value);

    return nothing;
}

/***DOC ERROR
ERROR

    outputs a list describing the error just caught, if any.  If there was
    not an error caught since the last use of ERROR, the empty list will
    be output.  The error list contains four members: an integer code
    corresponding to the type of error, the text of the error message (as
    a single word including spaces), the name of the procedure in which
    the error occurred, and the instruction line on which the error
    occurred.

COD***/
// CMD ERROR 0 0 0
DatumPtr Kernel::excError(DatumPtr node)
{
    ProcedureHelper h(this, node);

    List *retval = new List();
    if (currentError != nothing)
    {
        Error *e = currentError.errorValue();
        retval->append(DatumPtr(e->code));
        retval->append(e->errorText);
        if (e->procedure != nothing)
            retval->append(e->procedure.astnodeValue()->nodeName);
        else
            retval->append(DatumPtr(new List()));
        if (e->instructionLine != nothing)
            retval->append(e->instructionLine);
        else
            retval->append(DatumPtr(new List()));
        currentError = nothing;
    }
    return h.ret(retval);
}

/***DOC PAUSE
PAUSE

    command or operation.  Enters an interactive pause.  The user is
    prompted for instructions, as at toplevel, but with a prompt that
    includes the name of the procedure in which PAUSE was invoked.
    Local variables of that procedure are available during the pause.
    PAUSE outputs if the pause is ended by a CONTINUE with an input.

    If the variable ERRACT exists, and an error condition occurs,
    an interactive pause will be entered.  This allows the user to check
    values of local variables at the time of the error.

COD***/
// CMD PAUSE 0 0 0
DatumPtr Kernel::excPause(DatumPtr node)
{
    ProcedureHelper h(this, node);
    Q_ASSERT(callStack.size() > 0);
    if (callStack.globalFrame()->sourceNode.isNothing())
    {
        Error::notInsideProcedure(node.astnodeValue()->nodeName);
    }
    return h.ret(pause());
}

/***DOC CONTINUE CO
CONTINUE value
CO value
(CONTINUE)
(CO)

    command.  Ends the current interactive pause, returning to the
    context of the PAUSE invocation that began it.  If CONTINUE is
    given an input, that value is used as the output from the PAUSE.
    If not, the PAUSE does not output.

    Exceptionally, the CONTINUE command can be used without its default
    input and without parentheses provided that nothing follows it on
    the instruction line.

COD***/
// CMD CONTINUE 0 -1 1
// CMD CO 0 -1 1
DatumPtr Kernel::excContinue(DatumPtr node)
{
    ProcedureHelper h(this, node);

    DatumPtr retval;
    if (h.countOfChildren() > 0)
    {
        retval = h.datumAtIndex(0);
        if (!retval.isWord())
        {
            retval = DatumPtr(retval.printValue());
        }
    }

    Error::throwError(DatumPtr(QObject::tr("PAUSE")), retval);

    return nothing;
}

/***DOC TAG
TAG quoted.word

    command.  Does nothing.  The input must be a literal word following
    a quotation mark ("), not the result of a computation.  Tags are
    used by the GOTO command.

COD***/
// CMD TAG 1 1 1
DatumPtr Kernel::excTag(DatumPtr)
{
    return nothing;
}

/***DOC GOTO
GOTO word

    command.  Looks for a TAG command with the same input in the same
    procedure, and continues running the procedure from the location of
    that TAG.  It is meaningless to use GOTO outside of a procedure.

COD***/
// CMD GOTO 1 1 1
DatumPtr Kernel::excGoto(DatumPtr node)
{
    ProcedureHelper h(this, node);
    if (callStack.localFrame()->sourceNode.isNothing())
        Error::notInsideProcedure(node.astnodeValue()->nodeName);
    DatumPtr tagP = h.validatedDatumAtIndex(0, [this](DatumPtr candidate) {
        if (!candidate.isWord())
            return false;
        QString tag = candidate.wordValue()->keyValue();
        return callStack.localFrame()->sourceNode.astnodeValue()->childAtIndex(0).procedureValue()->tagToLine.contains(
            tag);
    });
    ASTNode *a = new ASTNode(QObject::tr("GOTO"));
    a->kernel = &Kernel::excGotoToken;
    a->addChild(tagP);
    return DatumPtr(a);
}

// TEMPLATE-BASED ITERATION

/***DOC APPLY
APPLY template inputlist

    command or operation.  Runs the "template," filling its slots with
    the members of "inputlist."  The number of members in "inputlist"
    must be an acceptable number of slots for "template."  It is
    illegal to apply the primitive TO as a template, but anything else
    is okay.  APPLY outputs what "template" outputs, if anything.

COD***/
// CMD APPLY 2 2 2
DatumPtr Kernel::excApply(DatumPtr node)
{
    ProcedureHelper h(this, node);
    enum Form
    {
        explicit_slot,
        named_procedure,
        lambda,
        procedure
    };
    Form f;

    DatumPtr tmplate = h.validatedDatumAtIndex(0, [&f](DatumPtr candidate) {
        if (candidate.isWord())
        {
            f = Form::named_procedure;
            return true;
        }
        if (!candidate.isList() || candidate.listValue()->isEmpty())
            return false;
        DatumPtr first = candidate.listValue()->head;

        if (first.isWord())
        {
            f = Form::explicit_slot;
            return true;
        }
        if (!first.isList() || (candidate.listValue()->count() < 2))
            return false;
        DatumPtr butfirst = candidate.listValue()->tail;
        DatumPtr procedureFirst = butfirst.listValue()->head;
        if (procedureFirst.isWord())
        {
            f = Form::lambda;
            return true;
        }
        if (procedureFirst.isList())
        {
            f = Form::procedure;
            return true;
        }
        return false;
    });
    DatumPtr params = h.listAtIndex(1);

    switch (f)
    {
    case named_procedure:
    {
        DatumPtr a = procedures->astnodeWithLiterals(tmplate, params);
        KernelMethod method = a.astnodeValue()->kernel;
        DatumPtr retval = (this->*method)(a);
        return h.ret(retval);
    }
    case explicit_slot:
    {
        CallFrame s(&callStack, node.astnodeValue());
        callStack.setExplicitSlotList(params);
        DatumPtr retval = runList(tmplate);
        return h.ret(retval);
    }
    case lambda:
    {
        CallFrame s(&callStack, node.astnodeValue());
        DatumPtr varList = tmplate.listValue()->head;
        DatumPtr procedureList = butfirst(tmplate);
        if (varList.listValue()->count() > params.listValue()->count())
            Error::notEnough(tmplate);
        if (varList.listValue()->count() < params.listValue()->count())
            Error::tooMany(tmplate);

        ListIterator nameIter = varList.listValue()->newIterator();
        ListIterator parmIter = params.listValue()->newIterator();
        while (nameIter.elementExists())
        {
            DatumPtr nameP = nameIter.element();
            if (!nameP.isWord())
                Error::doesntLike(node.astnodeValue()->nodeName, nameP);
            DatumPtr param = parmIter.element();
            QString name = nameP.wordValue()->keyValue();
            callStack.setVarAsLocal(name);
            callStack.setDatumForName(param, name);
        }
        DatumPtr retval = runList(procedureList);
        return h.ret(retval);
    }
    case procedure:
    {
        DatumPtr anonyProcedure = procedures->createProcedure(node.astnodeValue()->nodeName, tmplate, nothing);
        ASTNode *procnode = new ASTNode(node.astnodeValue()->nodeName);
        DatumPtr procnodeP(procnode);
        procnode->addChild(anonyProcedure);
        if (params.listValue()->count() > anonyProcedure.procedureValue()->countOfMaxParams)
            Error::tooMany(node.astnodeValue()->nodeName);
        if (params.listValue()->count() < anonyProcedure.procedureValue()->countOfMinParams)
            Error::notEnough(node.astnodeValue()->nodeName);

        ListIterator paramIter = params.listValue()->newIterator();
        while (paramIter.elementExists())
        {
            DatumPtr p = paramIter.element();
            DatumPtr a = DatumPtr(new ASTNode(QObject::tr("literal")));
            a.astnodeValue()->kernel = &Kernel::executeLiteral;
            a.astnodeValue()->addChild(p);
            procnode->addChild(a);
        }

        DatumPtr retval = executeProcedure(procnodeP);

        return h.ret(retval);
    }
    }
    return nothing;
}

// '?' operator
// CMD ? 0 0 1
DatumPtr Kernel::excNamedSlot(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr inputList = callStack.explicitSlotList();
    if (!inputList.isList())
        return Error::noApply(node.astnodeValue()->nodeName);
    int index = 1;
    if (h.countOfChildren() > 0)
    {
        h.integerAtIndex(0);
        index = h.validatedIntegerAtIndex(
            0, [&inputList, this](int candidate) { return doesListHaveCountOrMore(inputList.listValue(), candidate); });
    }
    return h.ret(inputList.listValue()->itemAtIndex(index));
}

/***DOC MACROP MACRO?
MACROP name
MACRO? name

    outputs TRUE if its input is the name of a macro.

COD***/
// CMD MACROP 1 1 1
// CMD MACRO? 1 1 1
DatumPtr Kernel::excMacrop(DatumPtr node)
{
    ProcedureHelper h(this, node);
    bool retval = procedures->isMacro(h.wordAtIndex(0).wordValue()->keyValue());
    return h.ret(retval);
}
