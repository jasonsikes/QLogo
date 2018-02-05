
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

Error::Error(int aNumber, const QString &aErrorText) {
  code = aNumber;
  errorText = DatumP(new Word(aErrorText));
}

Error::Error(int aNumber, DatumP aErrorText) {
  code = aNumber;
  errorText = aErrorText;
}

void Error::setKernel(Kernel *aKernel) { mainKernel = aKernel; }

void Error::turtleOutOfBounds() {
  QString message("Turtle out of bounds");
  mainKernel->registerError(new Error(3, message), true);
}

DatumP Error::doesntLike(DatumP who, DatumP what, bool allowErract,
                         bool allowRecovery) {
  QString message("%1 doesn't like %2 as input");
  message = message.arg(who.showValue()).arg(what.showValue());
  return mainKernel->registerError(new Error(4, message), allowErract,
                                   allowRecovery);
}

void Error::didntOutput(DatumP src, DatumP dest) {
  QString message("%1 didn't output to %2");
  message = message.arg(src.showValue()).arg(dest.showValue());
  mainKernel->registerError(new Error(5, message), true);
}

void Error::notEnough(DatumP dest) {
  QString message("not enough inputs to %1");
  message = message.arg(dest.showValue());
  mainKernel->registerError(new Error(6, message));
}

void Error::tooMany(DatumP dest) {
  QString message("too many inputs to %1");
  message = message.arg(dest.showValue());
  mainKernel->registerError(new Error(8, message));
}

void Error::dontSay(DatumP datum) {
  QString message("You don't say what to do with %1");
  message = message.arg(datum.showValue());
  mainKernel->registerError(new Error(9, message));
}

void Error::parenNf() {
  QString message("')' not found");
  mainKernel->registerError(new Error(10, message));
}

DatumP Error::noValueRecoverable(DatumP datum) {
  QString message("%1 has no value");
  message = message.arg(datum.showValue());
  return mainKernel->registerError(new Error(11, message), true, true);
}

void Error::noValue(DatumP datum) {
  QString message("%1 has no value");
  message = message.arg(datum.showValue());
  mainKernel->registerError(new Error(11, message));
}

void Error::noHow(DatumP dest) {
  QString message("I don't know how to %1");
  message = message.arg(dest.showValue());
  mainKernel->registerError(new Error(13, message));
}

DatumP Error::noHowRecoverable(DatumP dest) {
  QString message("I don't know how to %1");
  message = message.arg(dest.showValue());
  return mainKernel->registerError(new Error(13, message), true, true);
}

void Error::procDefined(DatumP procname) {
  QString message("%1 is already defined");
  message = message.arg(procname.showValue());
  mainKernel->registerError(new Error(15, message));
}

void Error::isPrimative(DatumP procname) {
  QString message("%1 is a primitive");
  message = message.arg(procname.showValue());
  mainKernel->registerError(new Error(22, message));
}

void Error::toInProc(DatumP cmd) {
  QString message("can't use %1 inside a procedure");
  message = message.arg(cmd.showValue());
  mainKernel->registerError(new Error(23, message));
}

void Error::toInPause(DatumP cmd) {
  QString message("Can't use %1 within PAUSE");
  message = message.arg(cmd.showValue());
  mainKernel->registerError(new Error(19, message));
}

void Error::unexpectedCloseSquare() {
  QString message("unexpected ']'");
  mainKernel->registerError(new Error(26, message));
}

void Error::unexpectedCloseBrace() {
  QString message("unexpected '}'");
  mainKernel->registerError(new Error(27, message));
}

void Error::unexpectedCloseParen() {
  QString message("unexpected ')'");
  mainKernel->registerError(new Error(12, message));
}

void Error::alreadyDribbling() {
  QString message("already dribbling");
  mainKernel->registerError(new Error(17, message), true);
}

void Error::fileSystem() {
  QString message("File system error");
  mainKernel->registerError(new Error(18, message));
}

DatumP Error::fileSystemRecoverable() {
  QString message("File system error");
  return mainKernel->registerError(new Error(18, message), true, true);
}

void Error::listHasMultExp(DatumP list) {
  QString message("Runlist %1 has more than one expression");
  message = message.arg(list.showValue());
  mainKernel->registerError(new Error(43, message));
}

void Error::alreadyOpen(DatumP what) {
  QString message("File %1 already open");
  message = message.arg(what.showValue());
  mainKernel->registerError(new Error(41, message), true);
}

void Error::cantOpen(DatumP what) {
  QString message("I can't open file %1");
  message = message.arg(what.showValue());
  mainKernel->registerError(new Error(40, message), true);
}

void Error::notOpen(DatumP what) {
  QString message("File %1 not open");
  message = message.arg(what.showValue());
  mainKernel->registerError(new Error(42, message), true);
}

void Error::alreadyFilling() {
  QString message = "Already filling";
  mainKernel->registerError(new Error(28, message), true);
}

DatumP Error::noTest(DatumP what) {
  QString message = "%1 without TEST";
  message = message.arg(what.showValue());
  return mainKernel->registerError(new Error(25, message), true, true);
}

void Error::notInsideProcedure(DatumP what) {
  QString message = "Can only use %1 inside a procedure";
  message = message.arg(what.showValue());
  mainKernel->registerError(new Error(31, message));
}

DatumP Error::macroReturned(DatumP aOutput) {
  QString message = "Macro returned %1 instead of a list";
  message = message.arg(aOutput.showValue());
  return mainKernel->registerError(new Error(29, message), true, true);
}

DatumP Error::insideRunresult(DatumP cmdName) {
  QString message = "Can't use %1 inside RUNRESULT";
  message = message.arg(cmdName.showValue());
  return mainKernel->registerError(new Error(38, message), true, true);
}

DatumP Error::noApply(DatumP what) {
  QString message = "Can't use %1 without APPLY";
  message = message.arg(what.showValue());
  return mainKernel->registerError(new Error(44, message), true, true);
}

void Error::throwError(DatumP aTag, DatumP aOutput) {
  Error *e;
  if (aTag.wordValue()->keyValue() == "ERROR") {
    if (aOutput == nothing) {
      e = new Error(21, "Throw \"Error");
      e->tag = aTag;
    } else {
      e = new Error(35, aOutput);
      e->tag = aTag;
    }
  } else {
    QString message = "Can't find catch tag for %1";
    message = message.arg(aTag.showValue());
    e = new Error(14, message);
    e->tag = aTag;
    e->output = aOutput;
  }
  mainKernel->registerError(e);
}
