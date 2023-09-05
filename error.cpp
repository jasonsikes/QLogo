
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
#include "kernel.h"
#include "datum_word.h"
#include <QDebug>

Kernel *mainKernel;

Error::Error(int aNumber, const QString &aErrorText) {
  code = aNumber;
  errorText = DatumP(aErrorText);
}

Error::Error(int aNumber, DatumP aErrorText) {
  code = aNumber;
  errorText = aErrorText;
}

void Error::setKernel(Kernel *aKernel) { mainKernel = aKernel; }

void Error::turtleOutOfBounds() {
  QString message("Turtle out of bounds");
  mainKernel->registerError(new Error(ERR_TURTLE_BOUNDS, message), true);
}

DatumP Error::doesntLike(DatumP who, DatumP what, bool allowErract,
                         bool allowRecovery) {
  QString message("%1 doesn't like %2 as input");
  message = message.arg(who.showValue(),what.showValue());
  return mainKernel->registerError(new Error(ERR_DOESNT_LIKE, message), allowErract,
                                   allowRecovery);
}

void Error::didntOutput(DatumP src, DatumP dest) {
  QString message("%1 didn't output to %2");
  message = message.arg(src.showValue(),dest.showValue());
  mainKernel->registerError(new Error(ERR_DIDNT_OUTPUT, message), true);
}

void Error::notEnough(DatumP dest) {
  QString message("not enough inputs to %1");
  message = message.arg(dest.showValue());
  mainKernel->registerError(new Error(ERR_NOT_ENOUGH_INPUTS, message));
}

void Error::tooMany(DatumP dest) {
  QString message("too many inputs to %1");
  message = message.arg(dest.showValue());
  mainKernel->registerError(new Error(ERR_TOO_MANY_INPUTS, message));
}

void Error::dontSay(DatumP datum) {
  QString message("You don't say what to do with %1");
  message = message.arg(datum.showValue());
  mainKernel->registerError(new Error(ERR_DONT_SAY, message));
}

void Error::parenNf() {
  QString message("')' not found");
  mainKernel->registerError(new Error(ERR_PAREN_NF, message));
}

DatumP Error::noValueRecoverable(DatumP datum) {
  QString message("%1 has no value");
  message = message.arg(datum.showValue());
  return mainKernel->registerError(new Error(ERR_NO_VALUE, message), true, true);
}

void Error::noValue(DatumP datum) {
  QString message("%1 has no value");
  message = message.arg(datum.showValue());
  mainKernel->registerError(new Error(ERR_NO_VALUE, message));
}

void Error::noHow(DatumP dest) {
  QString message("I don't know how to %1");
  message = message.arg(dest.showValue());
  mainKernel->registerError(new Error(ERR_NO_HOW, message));
}

DatumP Error::noHowRecoverable(DatumP dest) {
  QString message("I don't know how to %1");
  message = message.arg(dest.showValue());
  return mainKernel->registerError(new Error(ERR_NO_HOW, message), true, true);
}

void Error::procDefined(DatumP procname) {
  QString message("%1 is already defined");
  message = message.arg(procname.showValue());
  mainKernel->registerError(new Error(ERR_ALREADY_DEFINED, message));
}

void Error::isPrimative(DatumP procname) {
  QString message("%1 is a primitive");
  message = message.arg(procname.showValue());
  mainKernel->registerError(new Error(ERR_IS_PRIMATIVE, message));
}

void Error::toInProc(DatumP cmd) {
  QString message("can't use %1 inside a procedure");
  message = message.arg(cmd.showValue());
  mainKernel->registerError(new Error(ERR_TO_IN_PROC, message));
}

void Error::toInPause(DatumP cmd) {
  QString message("Can't use %1 within PAUSE");
  message = message.arg(cmd.showValue());
  mainKernel->registerError(new Error(ERR_TO_IN_PAUSE, message));
}

void Error::unexpectedCloseSquare() {
  QString message("unexpected ']'");
  mainKernel->registerError(new Error(ERR_UNEXPECTED_SQUARE, message));
}

void Error::unexpectedCloseBrace() {
  QString message("unexpected '}'");
  mainKernel->registerError(new Error(ERR_UNEXPECTED_BRACE, message));
}

void Error::unexpectedCloseParen() {
  QString message("unexpected ')'");
  mainKernel->registerError(new Error(ERR_UNEXPECTED_PAREN, message));
}

void Error::alreadyDribbling() {
  QString message("already dribbling");
  mainKernel->registerError(new Error(ERR_ALREADY_DRIBBLING, message), true);
}

void Error::fileSystem() {
  QString message("File system error");
  mainKernel->registerError(new Error(ERR_FILESYSTEM, message));
}

DatumP Error::fileSystemRecoverable() {
  QString message("File system error");
  return mainKernel->registerError(new Error(ERR_FILESYSTEM, message), true, true);
}

void Error::listHasMultExp(DatumP list) {
  QString message("Runlist %1 has more than one expression");
  message = message.arg(list.showValue());
  mainKernel->registerError(new Error(ERR_LIST_HAS_MULTIPLE_EXPRESSIONS, message));
}

void Error::alreadyOpen(DatumP what) {
  QString message("File %1 already open");
  message = message.arg(what.showValue());
  mainKernel->registerError(new Error(ERR_ALREADY_OPEN, message), true);
}

void Error::cantOpen(DatumP what) {
  QString message("I can't open file %1");
  message = message.arg(what.showValue());
  mainKernel->registerError(new Error(ERR_CANT_OPEN, message), true);
}

void Error::notOpen(DatumP what) {
  QString message("File %1 not open");
  message = message.arg(what.showValue());
  mainKernel->registerError(new Error(ERR_NOT_OPEN, message), true);
}

void Error::alreadyFilling() {
  QString message = "Already filling";
  mainKernel->registerError(new Error(ERR_ALREADY_FILLING, message), true);
}

void Error::noGraphics() {
  QString message = "Graphics not initialized";
  mainKernel->registerError(new Error(ERR_NO_GRAPHICS, message), true);
}

DatumP Error::noTest(DatumP what) {
  QString message = "%1 without TEST";
  message = message.arg(what.showValue());
  return mainKernel->registerError(new Error(ERR_NO_TEST, message), true, true);
}

void Error::notInsideProcedure(DatumP what) {
  QString message = "Can only use %1 inside a procedure";
  message = message.arg(what.showValue());
  mainKernel->registerError(new Error(ERR_NOT_INSIDE_PROCEDURE, message));
}

DatumP Error::macroReturned(DatumP aOutput) {
  QString message = "Macro returned %1 instead of a list";
  message = message.arg(aOutput.showValue());
  return mainKernel->registerError(new Error(ERR_MACRO_RETURNED_NOT_LIST, message), true, true);
}

DatumP Error::insideRunresult(DatumP cmdName) {
  QString message = "Can't use %1 inside RUNRESULT";
  message = message.arg(cmdName.showValue());
  return mainKernel->registerError(new Error(ERR_INSIDE_RUNRESULT, message), true, true);
}

DatumP Error::noApply(DatumP what) {
  QString message = "Can't use %1 without APPLY";
  message = message.arg(what.showValue());
  return mainKernel->registerError(new Error(ERR_NO_APPLY, message), true, true);
}

void Error::stackOverflow()
{
  QString message("Stack overflow");
  mainKernel->registerError(new Error(ERR_STACK_OVERFLOW, message));
}

void Error::throwError(DatumP aTag, DatumP aOutput) {
  Error *e;
  if (aTag.wordValue()->keyValue() == "ERROR") {
    if (aOutput == nothing) {
      e = new Error(ERR_THROW, "Throw \"Error");
      e->tag = aTag;
    } else {
      e = new Error(ERR_CUSTOM_THROW, aOutput);
      e->tag = aTag;
    }
  } else {
    QString message = "Can't find catch tag for %1";
    message = message.arg(aTag.showValue());
    e = new Error(ERR_NO_CATCH, message);
    e->tag = aTag;
    e->output = aOutput;
  }
  mainKernel->registerError(e);
}
