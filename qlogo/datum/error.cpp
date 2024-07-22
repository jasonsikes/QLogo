
//===-- qlogo/error.cpp - Error class implementation -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// QLogo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with QLogo.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the Error class, which stores
/// and reports error information during exceptions.
///
//===----------------------------------------------------------------------===//

#include "error.h"
#include "datum.h"
#include "kernel.h"
#include <QDebug>
#include <unistd.h>

Error::Error()
{
}

Error *Error::createError(int aNumber, const QString &aErrorText)
{
    return Error::createError(aNumber, DatumPtr(aErrorText));
}

Error *Error::createError(int aNumber, DatumPtr aErrorText)
{
    Error *retval = new Error();
    retval->code = aNumber;
    retval->errorText = aErrorText;
    retval->tag = nothing;
    retval->output = nothing;
    retval->procedure = nothing;
    retval->instructionLine = nothing;
    return retval;
}

void Error::turtleOutOfBounds()
{
    Config::get().mainKernel()->registerError(createError(ERR_TURTLE_BOUNDS, QObject::tr("Turtle out of bounds")),
                                              true);
}

DatumPtr Error::doesntLike(DatumPtr who, DatumPtr what, bool allowErract, bool allowRecovery)
{
    QString message = QObject::tr("%1 doesn't like %2 as input").arg(who.showValue(), what.showValue());
    return Config::get().mainKernel()->registerError(createError(ERR_DOESNT_LIKE, message), allowErract, allowRecovery);
}

void Error::didntOutput(DatumPtr src, DatumPtr dest)
{
    QString message = QObject::tr("%1 didn't output to %2").arg(src.showValue(), dest.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_DIDNT_OUTPUT, message), true);
}

void Error::notEnough(DatumPtr dest)
{
    QString message = QObject::tr("not enough inputs to %1").arg(dest.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_NOT_ENOUGH_INPUTS, message));
}

void Error::tooMany(DatumPtr dest)
{
    QString message = QObject::tr("too many inputs to %1").arg(dest.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_TOO_MANY_INPUTS, message));
}

void Error::dontSay(DatumPtr datum)
{
    QString message = QObject::tr("You don't say what to do with %1").arg(datum.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_DONT_SAY, message));
}

void Error::parenNf()
{
    Config::get().mainKernel()->registerError(createError(ERR_PAREN_NF, QObject::tr("')' not found")));
}

DatumPtr Error::noValueRecoverable(DatumPtr datum)
{
    QString message = QObject::tr("%1 has no value").arg(datum.showValue());
    return Config::get().mainKernel()->registerError(createError(ERR_NO_VALUE, message), true, true);
}

void Error::noValue(DatumPtr datum)
{
    QString message = QObject::tr("%1 has no value").arg(datum.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_NO_VALUE, message));
}

void Error::noHow(DatumPtr dest)
{
    QString message = QObject::tr("I don't know how to %1").arg(dest.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_NO_HOW, message));
}

DatumPtr Error::noHowRecoverable(DatumPtr dest)
{
    QString message = QObject::tr("I don't know how to %1").arg(dest.showValue());
    return Config::get().mainKernel()->registerError(createError(ERR_NO_HOW, message), true, true);
}

void Error::procDefined(DatumPtr procname)
{
    QString message = QObject::tr("%1 is already defined").arg(procname.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_ALREADY_DEFINED, message));
}

void Error::isPrimative(DatumPtr procname)
{
    QString message = QObject::tr("%1 is a primitive").arg(procname.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_IS_PRIMATIVE, message));
}

void Error::toInProc(DatumPtr cmd)
{
    QString message = QObject::tr("can't use %1 inside a procedure").arg(cmd.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_TO_IN_PROC, message));
}

void Error::toInPause(DatumPtr cmd)
{
    QString message = QObject::tr("Can't use %1 within PAUSE").arg(cmd.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_TO_IN_PAUSE, message));
}

void Error::unexpectedCloseSquare()
{
    Config::get().mainKernel()->registerError(createError(ERR_UNEXPECTED_SQUARE, QObject::tr("unexpected ']'")));
}

void Error::unexpectedCloseBrace()
{
    Config::get().mainKernel()->registerError(createError(ERR_UNEXPECTED_BRACE, QObject::tr("unexpected '}'")));
}

void Error::unexpectedCloseParen()
{
    Config::get().mainKernel()->registerError(createError(ERR_UNEXPECTED_PAREN, QObject::tr("unexpected ')'")));
}

void Error::alreadyDribbling()
{
    Config::get().mainKernel()->registerError(createError(ERR_ALREADY_DRIBBLING, QObject::tr("already dribbling")),
                                              true);
}

void Error::fileSystem()
{
    Config::get().mainKernel()->registerError(createError(ERR_FILESYSTEM, QObject::tr("File system error")));
}

DatumPtr Error::fileSystemRecoverable()
{
    return Config::get().mainKernel()->registerError(
        createError(ERR_FILESYSTEM, QObject::tr("File system error")), true, true);
}

void Error::listHasMultExp(DatumPtr list)
{
    QString message = QObject::tr("Runlist %1 has more than one expression").arg(list.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_LIST_HAS_MULTIPLE_EXPRESSIONS, message));
}

void Error::alreadyOpen(DatumPtr what)
{
    QString message = QObject::tr("File %1 already open").arg(what.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_ALREADY_OPEN, message), true);
}

void Error::cantOpen(DatumPtr what)
{
    QString message = QObject::tr("I can't open file %1").arg(what.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_CANT_OPEN, message), true);
}

void Error::notOpen(DatumPtr what)
{
    QString message = QObject::tr("File %1 not open").arg(what.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_NOT_OPEN, message), true);
}

void Error::alreadyFilling()
{
    Config::get().mainKernel()->registerError(createError(ERR_ALREADY_FILLING, QObject::tr("Already filling")), true);
}

void Error::noGraphics()
{
    Config::get().mainKernel()->registerError(createError(ERR_NO_GRAPHICS, QObject::tr("Graphics not initialized")),
                                              true);
}

DatumPtr Error::noTest(DatumPtr what)
{
    QString message = QObject::tr("%1 without TEST").arg(what.showValue());
    return Config::get().mainKernel()->registerError(createError(ERR_NO_TEST, message), true, true);
}

void Error::notInsideProcedure(DatumPtr what)
{
    QString message = QObject::tr("Can only use %1 inside a procedure").arg(what.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_NOT_INSIDE_PROCEDURE, message));
}

DatumPtr Error::macroReturned(DatumPtr aOutput)
{
    QString message = QObject::tr("Macro returned %1 instead of a list").arg(aOutput.showValue());
    return Config::get().mainKernel()->registerError(createError(ERR_MACRO_RETURNED_NOT_LIST, message), true, true);
}

DatumPtr Error::insideRunresult(DatumPtr cmdName)
{
    QString message = QObject::tr("Can't use %1 inside RUNRESULT").arg(cmdName.showValue());
    return Config::get().mainKernel()->registerError(createError(ERR_INSIDE_RUNRESULT, message), true, true);
}

DatumPtr Error::noApply(DatumPtr what)
{
    QString message = QObject::tr("Can't use %1 without APPLY").arg(what.showValue());
    return Config::get().mainKernel()->registerError(createError(ERR_NO_APPLY, message), true, true);
}

void Error::stackOverflow()
{
    Config::get().mainKernel()->registerError(createError(ERR_STACK_OVERFLOW, QObject::tr("Stack overflow")));
}

void Error::badDefaultExpression(DatumPtr what)
{
    QString message = QObject::tr("Bad default expression for optional input: %1").arg(what.showValue());
    Config::get().mainKernel()->registerError(createError(ERR_BAD_DEFAULT_EXPRESSION, message), true);
}

void Error::throwError(DatumPtr aTag, DatumPtr aOutput)
{
    Error *e;
    if (aTag.wordValue()->keyValue() == QObject::tr("ERROR"))
    {
        if (aOutput == nothing)
        {
            e = createError(ERR_THROW, QObject::tr("Throw \"Error"));
            e->tag = aTag;
        }
        else
        {
            e = createError(ERR_CUSTOM_THROW, aOutput);
            e->tag = aTag;
        }
    }
    else
    {
        QString message = QObject::tr("Can't find catch tag for %1").arg(aTag.showValue());
        e = createError(ERR_NO_CATCH, message);
        e->tag = aTag;
        e->output = aOutput;
    }
    Config::get().mainKernel()->registerError(e);
}
