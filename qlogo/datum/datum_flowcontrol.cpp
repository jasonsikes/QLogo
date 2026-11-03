//===-- qlogo/datum_flowcontrol.cpp - FlowControl subclasses implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the FlowControl subclasses, which are the general
/// container of flow control data. The subclasses are as follows:
/// - FCError: An error object.
/// - FCGoto: A goto object.
/// - FCReturn: A return object.
/// - FCContinuation: A continuation object.
///
//===----------------------------------------------------------------------===//

#include "astnode.h"
#include "flowcontrol.h"
#include "sharedconstants.h"
#include "workspace/kernel.h"
#include "workspace/procedures.h"
#include <QObject>

void FCError::commonInit()
{
    Kernel *k = &Kernel::get();

    // ERROR's procedure and line members are the empty list when this error is
    // not inside a procedure, or when no instruction line was running.
    procedure() = emptyList();
    line() = emptyList();

    // If the error is a bad default expression, don't report the procedure
    // or line. There is also no procedure when an error is created before
    // the interpreter has a call frame (e.g. a missing startup script).
    if (code_ != ERR_BAD_DEFAULT_EXPRESSION && !k->callFrameStack_.empty())
    {
        CallFrame *cf = k->currentCallFrame();
        // (THROW "ERROR message) is reported where the throwing procedure was
        // invoked, so a user procedure can look like a primitive. One-input
        // THROW "ERROR stays on the THROW line.
        const bool blameCaller = code_ == ERR_CUSTOM_THROW && k->callFrameStack_.size() >= 2;
        if (blameCaller)
            cf = &k->callFrameStack_[k->callFrameStack_.size() - 2];

        if (cf->sourceNode_.isASTNode())
        {
            procedure() = cf->sourceNode_.astnodeValue()->nodeName_;
            if (Evaluator *ev = cf->topOwnEvaluator())
            {
                line() = ev->list_;
                lineCaptured_ = true;
            }
        }
    }
    k->currentError_ = DatumPtr(this);
}

FCError *FCError::fromThrow(const DatumPtr &tag, const DatumPtr &second)
{
    if (tag.toString(ToStringFlags_Key) == QObject::tr("ERROR"))
    {
        if (second.isNothing())
            return new FCError(ERR_THROW, QObject::tr("Throw \"Error"), tag, nothing());

        // The second input is the message, not a value for CATCH "ERROR to output.
        DatumPtr message = second.isWord() ? second : DatumPtr(new Word(second.toString(ToStringFlags_None)));
        return new FCError(ERR_CUSTOM_THROW, message, tag, nothing());
    }

    QString message = QObject::tr("Can't find catch tag for %1").arg(tag.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_NO_CATCH, message, tag, second);
}

FCError *FCError::fatalInternal()
{
    auto *err = new FCError(ERR_FATAL_INTERNAL, QObject::tr("Fatal internal error"));
    qFatal("%s\n", err->toString().toUtf8().constData());
    return err;
}

FCError *FCError::turtleOutOfBounds()
{
    return new FCError(ERR_TURTLE_BOUNDS, QObject::tr("Turtle out of bounds"));
}

FCError *FCError::noGraphics()
{
    return new FCError(ERR_NO_GRAPHICS, QObject::tr("Graphics not initialized"));
}

FCError *FCError::toInProc(const DatumPtr &cmd)
{
    QString message = QObject::tr("can't use %1 inside a procedure").arg(cmd.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_TO_IN_PROC, message);
}

FCError *FCError::unexpectedCloseSquare()
{
    return new FCError(ERR_UNEXPECTED_SQUARE, QObject::tr("unexpected ']'"));
}

FCError *FCError::unexpectedCloseBrace()
{
    return new FCError(ERR_UNEXPECTED_BRACE, QObject::tr("unexpected '}'"));
}

FCError *FCError::unexpectedCloseParen()
{
    return new FCError(ERR_UNEXPECTED_PAREN, QObject::tr("unexpected ')'"));
}

FCError *FCError::fileSystem()
{
    return new FCError(ERR_FILESYSTEM, QObject::tr("File system error"));
}

FCError *FCError::notInsideProcedure(const DatumPtr &cmd)
{
    QString message = QObject::tr("Can only use %1 inside a procedure").arg(cmd.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_NOT_INSIDE_PROCEDURE, message);
}

FCError *FCError::noHow(const DatumPtr &cmd)
{
    QString message = QObject::tr("I don't know how to %1").arg(cmd.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_NO_HOW, message);
}

FCError *FCError::doesntLike(const DatumPtr &x, const DatumPtr &y)
{
    QString message = QObject::tr("%1 doesn't like %2 as input")
                          .arg(x.toString(Datum::ToStringFlags_Show), y.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_DOESNT_LIKE, message);
}

FCError *FCError::dontSay(const DatumPtr &x)
{
    QString message = QObject::tr("You don't say what to do with %1").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_DONT_SAY, message);
}

FCError *FCError::noTest(const DatumPtr &x)
{
    QString message = QObject::tr("%1 without TEST").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_NO_TEST, message);
}

FCError *FCError::didntOutput(const DatumPtr &x, const DatumPtr &y)
{
    QString message = QObject::tr("%1 didn't output to %2")
                          .arg(x.toString(Datum::ToStringFlags_Show), y.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_DIDNT_OUTPUT, message);
}

FCError *FCError::tooManyInputs(const DatumPtr &x)
{
    QString message = QObject::tr("too many inputs to %1").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_TOO_MANY_INPUTS, message);
}

FCError *FCError::notEnoughInputs(const DatumPtr &x)
{
    QString message = QObject::tr("not enough inputs to %1").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_NOT_ENOUGH_INPUTS, message);
}

FCError *FCError::noValue(const DatumPtr &x)
{
    QString message = QObject::tr("%1 has no value").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_NO_VALUE, message);
}

FCError *FCError::macroReturnedNotList(const DatumPtr &value)
{
    QString shown = value.toString(Datum::ToStringFlags_Show);
    QString message = QObject::tr("Macro returned %1 instead of a list").arg(shown);
    return new FCError(ERR_MACRO_RETURNED_NOT_LIST, message);
}

FCError *FCError::multipleExpressions(const DatumPtr &runlist)
{
    QString message =
        QObject::tr("Runlist %1 has more than one expression").arg(runlist.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_LIST_HAS_MULTIPLE_EXPRESSIONS, message);
}

FCError *FCError::alreadyFilling()
{
    QString message = QObject::tr("Already filling");
    return new FCError(ERR_ALREADY_FILLING, message);
}

FCError *FCError::procDefined(const DatumPtr &x)
{
    QString message = QObject::tr("%1 is already defined").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_ALREADY_DEFINED, message);
}

FCError *FCError::badDefault(const DatumPtr &x)
{
    QString message =
        QObject::tr("Bad default expression for optional input: %1").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_BAD_DEFAULT_EXPRESSION, message);
}

FCError *FCError::parenNf()
{
    QString message = QObject::tr("')' not found");
    return new FCError(ERR_PAREN_NF, message);
}

FCError *FCError::stackOverflow()
{
    return new FCError(ERR_STACK_OVERFLOW, QObject::tr("Stack overflow"));
}

FCError *FCError::isPrimitive(const DatumPtr &x)
{
    QString message = QObject::tr("%1 is a primitive").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_IS_PRIMITIVE, message);
}

FCError *FCError::alreadyDribbling()
{
    return new FCError(ERR_ALREADY_DRIBBLING, QObject::tr("already dribbling"));
}

FCError *FCError::notOpen(const DatumPtr &x)
{
    QString message = QObject::tr("File %1 not open").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_NOT_OPEN, message);
}

FCError *FCError::alreadyOpen(const DatumPtr &x)
{
    QString message = QObject::tr("File %1 already open").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_ALREADY_OPEN, message);
}

FCError *FCError::cantOpen(const DatumPtr &x)
{
    QString message = QObject::tr("I can't open file %1").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_CANT_OPEN, message);
}

QString FCError::toString(ToStringFlags, int, int, VisitedSet *) const
{
    QString retval = message().toString();
    if (procedure().isWord())
    {
        retval = QString("%1 in %2").arg(retval, procedure().toString());
        if (lineCaptured_)
            retval += QLatin1Char('\n') + line().toString(ToStringFlags_Show);
    }
    return retval;
}