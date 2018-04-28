
//===-- qlogo/error.cpp - Error class implementation -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
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
/// error information during exceptions.
///
//===----------------------------------------------------------------------===//

#include "error.h"
#include "kernel.h"
#include <QDebug>

Kernel *mainKernel;

Error::Error(Error::errorCode aNumber, const QString &aErrorText) {
  code = aNumber;
  errorText = DatumP(new Word(aErrorText));
}

Error::Error(Error::errorCode aNumber, DatumP aErrorText) {
  code = aNumber;
  errorText = aErrorText;
}

void Error::setKernel(Kernel *aKernel) { mainKernel = aKernel; }

void Error::turtleOutOfBounds() {
  QString message("Turtle out of bounds");
  mainKernel->registerError(new Error(ecTurtleOutOfBounds, message), true);
}

DatumP Error::doesntLike(DatumP who, DatumP what, bool allowErract,
                         bool allowRecovery) {
  QString message("%1 doesn't like %2 as input");
  message = message.arg(who.showValue()).arg(what.showValue());
  return mainKernel->registerError(new Error(ecDoesntLike, message), allowErract,
                                   allowRecovery);
}

void Error::didntOutput(DatumP src, DatumP dest) {
  QString message("%1 didn't output to %2");
  message = message.arg(src.showValue()).arg(dest.showValue());
  mainKernel->registerError(new Error(ecDidntOutput, message), true);
}

void Error::notEnough(DatumP dest) {
  QString message("not enough inputs to %1");
  message = message.arg(dest.showValue());
  mainKernel->registerError(new Error(ecNotEnoughInputs, message));
}

void Error::tooMany(DatumP dest) {
  QString message("too many inputs to %1");
  message = message.arg(dest.showValue());
  mainKernel->registerError(new Error(ecTooManyParens, message));
}

void Error::dontSay(DatumP datum) {
  QString message("You don't say what to do with %1");
  message = message.arg(datum.showValue());
  mainKernel->registerError(new Error(ecNoSay, message));
}

void Error::parenNf() {
  QString message("')' not found");
  mainKernel->registerError(new Error(ecNoCloseParen, message));
}

DatumP Error::noValueRecoverable(DatumP datum) {
  QString message("%1 has no value");
  message = message.arg(datum.showValue());
  return mainKernel->registerError(new Error(ecNoValue, message), true, true);
}

void Error::noValue(DatumP datum) {
  QString message("%1 has no value");
  message = message.arg(datum.showValue());
  mainKernel->registerError(new Error(ecNoValue, message));
}

void Error::noHow(DatumP dest) {
  QString message("I don't know how to %1");
  message = message.arg(dest.showValue());
  mainKernel->registerError(new Error(ecNoHow, message));
}

DatumP Error::noHowRecoverable(DatumP dest) {
  QString message("I don't know how to %1");
  message = message.arg(dest.showValue());
  return mainKernel->registerError(new Error(ecNoHowRec, message), true, true);
}

void Error::procDefined(DatumP procname) {
  QString message("%1 is already defined");
  message = message.arg(procname.showValue());
  mainKernel->registerError(new Error(ecAlreadyDefined, message));
}

void Error::isPrimative(DatumP procname) {
  QString message("%1 is a primitive");
  message = message.arg(procname.showValue());
  mainKernel->registerError(new Error(ecPrimitive, message));
}

void Error::toInProc(DatumP cmd) {
  QString message("can't use %1 inside a procedure");
  message = message.arg(cmd.showValue());
  mainKernel->registerError(new Error(ecToInsideProc, message));
}

void Error::toInPause(DatumP cmd) {
  QString message("Can't use %1 within PAUSE");
  message = message.arg(cmd.showValue());
  mainKernel->registerError(new Error(ecCmdInPause, message));
}

void Error::unexpectedCloseSquare() {
  QString message("unexpected ']'");
  mainKernel->registerError(new Error(ecUnexpectedCloseSquare, message));
}

void Error::unexpectedCloseBrace() {
  QString message("unexpected '}'");
  mainKernel->registerError(new Error(ecUnexpectedCloseCurly, message));
}

void Error::unexpectedCloseParen() {
  QString message("unexpected ')'");
  mainKernel->registerError(new Error(ecUnexpectedCloseParen, message));
}

void Error::alreadyDribbling() {
  QString message("already dribbling");
  mainKernel->registerError(new Error(ecAlreadyDribbling, message), true);
}

void Error::fileSystem() {
  QString message("File system error");
  mainKernel->registerError(new Error(ecFileSystem, message));
}

DatumP Error::fileSystemRecoverable() {
  QString message("File system error");
  return mainKernel->registerError(new Error(ecFileSystem, message), true, true);
}

void Error::listHasMultExp(DatumP list) {
  QString message("Runlist %1 has more than one expression");
  message = message.arg(list.showValue());
  mainKernel->registerError(new Error(ecListMultExpr, message));
}

void Error::alreadyOpen(DatumP what) {
  QString message("File %1 already open");
  message = message.arg(what.showValue());
  mainKernel->registerError(new Error(ecAlreadyOpen, message), true);
}

void Error::cantOpen(DatumP what) {
  QString message("I can't open file %1");
  message = message.arg(what.showValue());
  mainKernel->registerError(new Error(ecCantOpen, message), true);
}

void Error::notOpen(DatumP what) {
  QString message("File %1 not open");
  message = message.arg(what.showValue());
  mainKernel->registerError(new Error(ecNotOpen, message), true);
}

void Error::alreadyFilling() {
  QString message = "Already filling";
  mainKernel->registerError(new Error(ecAlreadyFilling, message), true);
}

DatumP Error::noTest(DatumP what) {
  QString message = "%1 without TEST";
  message = message.arg(what.showValue());
  return mainKernel->registerError(new Error(ecNoTest, message), true, true);
}

void Error::notInsideProcedure(DatumP what) {
  QString message = "Can only use %1 inside a procedure";
  message = message.arg(what.showValue());
  mainKernel->registerError(new Error(ecNotInsideProc, message));
}

DatumP Error::macroReturned(DatumP aOutput) {
  QString message = "Macro returned %1 instead of a list";
  message = message.arg(aOutput.showValue());
  return mainKernel->registerError(new Error(ecMacroReturned, message), true, true);
}

DatumP Error::insideRunresult(DatumP cmdName, DatumP listName) {
  QString message = "Can't use %1 inside %2";
  message = message.arg(cmdName.showValue()).arg(listName.showValue());
  return mainKernel->registerError(new Error(ecNoStop, message), true, true);
}

DatumP Error::noApply(DatumP what) {
  QString message = "Can't use %1 without APPLY";
  message = message.arg(what.showValue());
  return mainKernel->registerError(new Error(ecNoApply, message), true, true);
}

void Error::stackOverflow()
{
  QString message("Stack overflow");
  mainKernel->registerError(new Error(ecStackOverflow, message));
}

void Error::throwError(DatumP aTag, DatumP aOutput) {
  Error *e;
  if (aTag.wordValue()->keyValue() == "ERROR") {
    if (aOutput == nothing) {
      e = new Error(ecThrow, "Throw \"Error");
      e->tag = aTag;
    } else {
      e = new Error(ecUserGen, aOutput);
      e->tag = aTag;
    }
  } else {
    QString message = "Can't find catch tag for %1";
    message = message.arg(aTag.showValue());
    e = new Error(ecNoCatch, message);
    e->tag = aTag;
    e->output = aOutput;
  }
  mainKernel->registerError(e);
}
