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

#include "flowcontrol.h"
#include "astnode.h"
#include "sharedconstants.h"
#include "kernel.h"
#include "workspace/procedures.h"
#include <QObject>

void FCError::commonInit()
{
    Kernel *k = Config::get().mainKernel();

    // If the error is a bad default expression, don't report the procedure
    // or line.
    if (code != ERR_BAD_DEFAULT_EXPRESSION)
    {
        CallFrame *cf = k->callStack.localFrame();
        if ( cf->sourceNode.isASTNode())
        {
            procedure() = cf->sourceNode.astnodeValue()->nodeName;
            line() = cf->localEvaluator()->list;
        }
    }
    k->currentError = DatumPtr(this);
}

FCError* FCError::custom(DatumPtr tag, DatumPtr message, DatumPtr output)
{
    ErrCode code;
    if (tag.toString(ToStringFlags_Key) == QObject::tr("ERROR"))
    {
        if (message.isNothing())
        {
            code = ERR_THROW;
            message = DatumPtr(QObject::tr("Throw \"Error"));
        }
        else
        {
            code = ERR_CUSTOM_THROW;
        }
    }
    else
    {
        code = ERR_NO_CATCH;
        message = DatumPtr(QObject::tr("Can't find catch tag for %1").arg(tag.toString(Datum::ToStringFlags_Show)));
    }
    return new FCError(code, message, tag, output);
}

FCError* FCError::turtleOutOfBounds()
{
    return new FCError(ERR_TURTLE_BOUNDS, QObject::tr("Turtle out of bounds"));
}

FCError* FCError::noGraphics()
{
    return new FCError(ERR_NO_GRAPHICS, QObject::tr("Graphics not initialized"));
}

FCError* FCError::toInProc(DatumPtr cmd)
{
    QString message = QObject::tr("can't use %1 inside a procedure").arg(cmd.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_TO_IN_PROC, message);
}

FCError* FCError::unexpectedCloseSquare()
{
    return new FCError(ERR_UNEXPECTED_SQUARE, QObject::tr("unexpected ']'"));
}

FCError* FCError::unexpectedCloseBrace()
{
    return new FCError(ERR_UNEXPECTED_BRACE, QObject::tr("unexpected '}'"));
}

FCError* FCError::unexpectedCloseParen()
{
    return new FCError(ERR_UNEXPECTED_PAREN, QObject::tr("unexpected ')'"));
}


FCError* FCError::fileSystem()
{
    return new FCError(ERR_FILESYSTEM, QObject::tr("File system error"));
}

FCError* FCError::notInsideProcedure(DatumPtr cmd)
{
    QString message = QObject::tr("Can only use %1 inside a procedure").arg(cmd.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_NOT_INSIDE_PROCEDURE, message);
}


FCError* FCError::noHow(DatumPtr cmd)
{
    QString message = QObject::tr("I don't know how to %1").arg(cmd.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_NO_HOW, message);
}


FCError* FCError::doesntLike(DatumPtr x, DatumPtr y)
{
    QString message = QObject::tr("%1 doesn't like %2 as input").arg(x.toString(Datum::ToStringFlags_Show), y.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_DOESNT_LIKE, message);
}

FCError* FCError::dontSay(DatumPtr x)
{
    QString message = QObject::tr("You don't say what to do with %1").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_DONT_SAY, message);
}

FCError* FCError::noTest(DatumPtr x)
{
    QString message = QObject::tr("%1 without TEST").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_NO_TEST, message);
}

FCError* FCError::didntOutput(DatumPtr x, DatumPtr y)
{
    QString message = QObject::tr("%1 didn't output to %2").arg(x.toString(Datum::ToStringFlags_Show), y.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_DIDNT_OUTPUT, message);
}

FCError* FCError::tooManyInputs(DatumPtr x)
{
    QString message = QObject::tr("too many inputs to %1").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_TOO_MANY_INPUTS, message);
}

FCError* FCError::notEnoughInputs(DatumPtr x)
{
    QString message = QObject::tr("not enough inputs to %1").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_NOT_ENOUGH_INPUTS, message);
}

FCError* FCError::noValue(DatumPtr x)
{
    QString message = QObject::tr("%1 has no value").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_NO_VALUE, message);
}

FCError* FCError::alreadyFilling()
{
    QString message = QObject::tr("Already filling");
    return new FCError(ERR_ALREADY_FILLING, message);
}

FCError* FCError::procDefined(DatumPtr x)
{
    QString message = QObject::tr("%1 is already defined").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_ALREADY_DEFINED, message);
}

FCError* FCError::badDefault(DatumPtr x)
{
    QString message = QObject::tr("Bad default expression for optional input: %1").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_BAD_DEFAULT_EXPRESSION, message);
}

FCError* FCError::parenNf()
{
    QString message = QObject::tr("')' not found");
    return new FCError(ERR_PAREN_NF, message);
}


FCError* FCError::isPrimitive(DatumPtr x)
{
    QString message = QObject::tr("%1 is a primitive").arg(x.toString(Datum::ToStringFlags_Show));
    return new FCError(ERR_IS_PRIMITIVE, message);
}

QString FCError::toString( ToStringFlags, int, int, VisitedSet *) const
{
    QString retval = message().toString();
    if ( ! procedure().isNothing())
    {
        retval = QString("%1 in %2\n%3").arg(retval, procedure().toString(), line().toString(ToStringFlags_Show));
    }
    return retval;
}