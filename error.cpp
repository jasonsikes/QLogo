
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
#include "stringconstants.h"

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
  mainKernel->registerError(new Error(ERR_TURTLE_BOUNDS, k.eOutBounds()), true);
}

DatumP Error::doesntLike(DatumP who, DatumP what, bool allowErract,
                         bool allowRecovery) {
  QString message = k.eDontlike().arg(who.showValue(),what.showValue());
  return mainKernel->registerError(new Error(ERR_DOESNT_LIKE, message), allowErract,
                                   allowRecovery);
}

void Error::didntOutput(DatumP src, DatumP dest) {
  QString message = k.eDidntOutput().arg(src.showValue(),dest.showValue());
  mainKernel->registerError(new Error(ERR_DIDNT_OUTPUT, message), true);
}

void Error::notEnough(DatumP dest) {
  QString message = k.eNotEnoughInputs().arg(dest.showValue());
  mainKernel->registerError(new Error(ERR_NOT_ENOUGH_INPUTS, message));
}

void Error::tooMany(DatumP dest) {
  QString message = k.eTooManyInputs().arg(dest.showValue());
  mainKernel->registerError(new Error(ERR_TOO_MANY_INPUTS, message));
}

void Error::dontSay(DatumP datum) {
  QString message = k.eNoSay().arg(datum.showValue());
  mainKernel->registerError(new Error(ERR_DONT_SAY, message));
}

void Error::parenNf() {
  mainKernel->registerError(new Error(ERR_PAREN_NF, k.eCParenNotFound()));
}

DatumP Error::noValueRecoverable(DatumP datum) {
  QString message = k.eNoValue().arg(datum.showValue());
  return mainKernel->registerError(new Error(ERR_NO_VALUE, message), true, true);
}

void Error::noValue(DatumP datum) {
  QString message = k.eNoValue().arg(datum.showValue());
  mainKernel->registerError(new Error(ERR_NO_VALUE, message));
}

void Error::noHow(DatumP dest) {
  QString message = k.eNoHow().arg(dest.showValue());
  mainKernel->registerError(new Error(ERR_NO_HOW, message));
}

DatumP Error::noHowRecoverable(DatumP dest) {
  QString message = k.eNoHow().arg(dest.showValue());
  return mainKernel->registerError(new Error(ERR_NO_HOW, message), true, true);
}

void Error::procDefined(DatumP procname) {
  QString message = k.eAlreadyDefined().arg(procname.showValue());
  mainKernel->registerError(new Error(ERR_ALREADY_DEFINED, message));
}

void Error::isPrimative(DatumP procname) {
  QString message = k.eIsPrimitive().arg(procname.showValue());
  mainKernel->registerError(new Error(ERR_IS_PRIMATIVE, message));
}

void Error::toInProc(DatumP cmd) {
  QString message = k.eCantInProcedure().arg(cmd.showValue());
  mainKernel->registerError(new Error(ERR_TO_IN_PROC, message));
}

void Error::toInPause(DatumP cmd) {
  QString message = k.eCantInPause().arg(cmd.showValue());
  mainKernel->registerError(new Error(ERR_TO_IN_PAUSE, message));
}

void Error::unexpectedCloseSquare() {
  mainKernel->registerError(new Error(ERR_UNEXPECTED_SQUARE, k.eUnexpectedBracket()));
}

void Error::unexpectedCloseBrace() {
  mainKernel->registerError(new Error(ERR_UNEXPECTED_BRACE, k.eUnexpectedBrace()));
}

void Error::unexpectedCloseParen() {
  mainKernel->registerError(new Error(ERR_UNEXPECTED_PAREN, k.eUnexpectedParen()));
}

void Error::alreadyDribbling() {
  mainKernel->registerError(new Error(ERR_ALREADY_DRIBBLING, k.eAlreadyDribbling()), true);
}

void Error::fileSystem() {
  mainKernel->registerError(new Error(ERR_FILESYSTEM, k.eFileError()));
}

DatumP Error::fileSystemRecoverable() {
  return mainKernel->registerError(new Error(ERR_FILESYSTEM, k.eFileError()), true, true);
}

void Error::listHasMultExp(DatumP list) {
  QString message = k.eRunlistMultExpressions().arg(list.showValue());
  mainKernel->registerError(new Error(ERR_LIST_HAS_MULTIPLE_EXPRESSIONS, message));
}

void Error::alreadyOpen(DatumP what) {
  QString message = k.eAlreadyOpen().arg(what.showValue());
  mainKernel->registerError(new Error(ERR_ALREADY_OPEN, message), true);
}

void Error::cantOpen(DatumP what) {
  QString message = k.eCantOpen().arg(what.showValue());
  mainKernel->registerError(new Error(ERR_CANT_OPEN, message), true);
}

void Error::notOpen(DatumP what) {
  QString message = k.eNotOpen().arg(what.showValue());
  mainKernel->registerError(new Error(ERR_NOT_OPEN, message), true);
}

void Error::alreadyFilling() {
  mainKernel->registerError(new Error(ERR_ALREADY_FILLING, k.eAlreadyFilling()), true);
}

void Error::noGraphics() {
  mainKernel->registerError(new Error(ERR_NO_GRAPHICS, k.eNoGraphics()), true);
}

DatumP Error::noTest(DatumP what) {
  QString message = k.eNoTest().arg(what.showValue());
  return mainKernel->registerError(new Error(ERR_NO_TEST, message), true, true);
}

void Error::notInsideProcedure(DatumP what) {
  QString message = k.eNotInProcedure().arg(what.showValue());
  mainKernel->registerError(new Error(ERR_NOT_INSIDE_PROCEDURE, message));
}

DatumP Error::macroReturned(DatumP aOutput) {
  QString message = k.eNotList().arg(aOutput.showValue());
  return mainKernel->registerError(new Error(ERR_MACRO_RETURNED_NOT_LIST, message), true, true);
}

DatumP Error::insideRunresult(DatumP cmdName) {
  QString message = k.eCantInsideRunresult().arg(cmdName.showValue());
  return mainKernel->registerError(new Error(ERR_INSIDE_RUNRESULT, message), true, true);
}

DatumP Error::noApply(DatumP what) {
  QString message = k.eCantNoApply().arg(what.showValue());
  return mainKernel->registerError(new Error(ERR_NO_APPLY, message), true, true);
}

void Error::stackOverflow()
{
  mainKernel->registerError(new Error(ERR_STACK_OVERFLOW, k.eStackOverflow()));
}

void Error::throwError(DatumP aTag, DatumP aOutput) {
  Error *e;
  if (aTag.wordValue()->keyValue() == k.error()) {
    if (aOutput == nothing) {
          e = new Error(ERR_THROW, k.throwError());
      e->tag = aTag;
    } else {
      e = new Error(ERR_CUSTOM_THROW, aOutput);
      e->tag = aTag;
    }
  } else {
    QString message = k.eNoCatch().arg(aTag.showValue());
    e = new Error(ERR_NO_CATCH, message);
    e->tag = aTag;
    e->output = aOutput;
  }
  mainKernel->registerError(e);
}
