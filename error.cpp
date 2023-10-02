
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

static DatumPtrool<Error> pool(3);


Error::Error()
{
}

Error * Error::createError(int aNumber, const QString &aErrorText) {
    return Error::createError(aNumber, DatumPtr(aErrorText));
}

Error * Error::createError(int aNumber, DatumPtr aErrorText) {
  Error * retval = (Error *) pool.alloc();
  retval->code = aNumber;
  retval->errorText = aErrorText;
  retval->tag = nothing;
  retval->output = nothing;
  retval->procedure = nothing;
  retval->instructionLine = nothing;
  return retval;
}

void Error::setKernel(Kernel *aKernel) { mainKernel = aKernel; }

void Error::turtleOutOfBounds() {
  mainKernel->registerError(createError(ERR_TURTLE_BOUNDS, k.eOutBounds()), true);
}

DatumPtr Error::doesntLike(DatumPtr who, DatumPtr what, bool allowErract,
                         bool allowRecovery) {
  QString message = k.eDontlike().arg(who.showValue(),what.showValue());
  return mainKernel->registerError(createError(ERR_DOESNT_LIKE, message), allowErract,
                                   allowRecovery);
}

void Error::didntOutput(DatumPtr src, DatumPtr dest) {
  QString message = k.eDidntOutput().arg(src.showValue(),dest.showValue());
  mainKernel->registerError(createError(ERR_DIDNT_OUTPUT, message), true);
}

void Error::notEnough(DatumPtr dest) {
  QString message = k.eNotEnoughInputs().arg(dest.showValue());
  mainKernel->registerError(createError(ERR_NOT_ENOUGH_INPUTS, message));
}

void Error::tooMany(DatumPtr dest) {
  QString message = k.eTooManyInputs().arg(dest.showValue());
  mainKernel->registerError(createError(ERR_TOO_MANY_INPUTS, message));
}

void Error::dontSay(DatumPtr datum) {
  QString message = k.eNoSay().arg(datum.showValue());
  mainKernel->registerError(createError(ERR_DONT_SAY, message));
}

void Error::parenNf() {
  mainKernel->registerError(createError(ERR_PAREN_NF, k.eCParenNotFound()));
}

DatumPtr Error::noValueRecoverable(DatumPtr datum) {
  QString message = k.eNoValue().arg(datum.showValue());
  return mainKernel->registerError(createError(ERR_NO_VALUE, message), true, true);
}

void Error::noValue(DatumPtr datum) {
  QString message = k.eNoValue().arg(datum.showValue());
  mainKernel->registerError(createError(ERR_NO_VALUE, message));
}

void Error::noHow(DatumPtr dest) {
  QString message = k.eNoHow().arg(dest.showValue());
  mainKernel->registerError(createError(ERR_NO_HOW, message));
}

DatumPtr Error::noHowRecoverable(DatumPtr dest) {
  QString message = k.eNoHow().arg(dest.showValue());
  return mainKernel->registerError(createError(ERR_NO_HOW, message), true, true);
}

void Error::procDefined(DatumPtr procname) {
  QString message = k.eAlreadyDefined().arg(procname.showValue());
  mainKernel->registerError(createError(ERR_ALREADY_DEFINED, message));
}

void Error::isPrimative(DatumPtr procname) {
  QString message = k.eIsPrimitive().arg(procname.showValue());
  mainKernel->registerError(createError(ERR_IS_PRIMATIVE, message));
}

void Error::toInProc(DatumPtr cmd) {
  QString message = k.eCantInProcedure().arg(cmd.showValue());
  mainKernel->registerError(createError(ERR_TO_IN_PROC, message));
}

void Error::toInPause(DatumPtr cmd) {
  QString message = k.eCantInPause().arg(cmd.showValue());
  mainKernel->registerError(createError(ERR_TO_IN_PAUSE, message));
}

void Error::unexpectedCloseSquare() {
  mainKernel->registerError(createError(ERR_UNEXPECTED_SQUARE, k.eUnexpectedBracket()));
}

void Error::unexpectedCloseBrace() {
  mainKernel->registerError(createError(ERR_UNEXPECTED_BRACE, k.eUnexpectedBrace()));
}

void Error::unexpectedCloseParen() {
  mainKernel->registerError(createError(ERR_UNEXPECTED_PAREN, k.eUnexpectedParen()));
}

void Error::alreadyDribbling() {
  mainKernel->registerError(createError(ERR_ALREADY_DRIBBLING, k.eAlreadyDribbling()), true);
}

void Error::fileSystem() {
  mainKernel->registerError(createError(ERR_FILESYSTEM, k.eFileError()));
}

DatumPtr Error::fileSystemRecoverable() {
  return mainKernel->registerError(createError(ERR_FILESYSTEM, k.eFileError()), true, true);
}

void Error::listHasMultExp(DatumPtr list) {
  QString message = k.eRunlistMultExpressions().arg(list.showValue());
  mainKernel->registerError(createError(ERR_LIST_HAS_MULTIPLE_EXPRESSIONS, message));
}

void Error::alreadyOpen(DatumPtr what) {
  QString message = k.eAlreadyOpen().arg(what.showValue());
  mainKernel->registerError(createError(ERR_ALREADY_OPEN, message), true);
}

void Error::cantOpen(DatumPtr what) {
  QString message = k.eCantOpen().arg(what.showValue());
  mainKernel->registerError(createError(ERR_CANT_OPEN, message), true);
}

void Error::notOpen(DatumPtr what) {
  QString message = k.eNotOpen().arg(what.showValue());
  mainKernel->registerError(createError(ERR_NOT_OPEN, message), true);
}

void Error::alreadyFilling() {
  mainKernel->registerError(createError(ERR_ALREADY_FILLING, k.eAlreadyFilling()), true);
}

void Error::noGraphics() {
  mainKernel->registerError(createError(ERR_NO_GRAPHICS, k.eNoGraphics()), true);
}

DatumPtr Error::noTest(DatumPtr what) {
  QString message = k.eNoTest().arg(what.showValue());
  return mainKernel->registerError(createError(ERR_NO_TEST, message), true, true);
}

void Error::notInsideProcedure(DatumPtr what) {
  QString message = k.eNotInProcedure().arg(what.showValue());
  mainKernel->registerError(createError(ERR_NOT_INSIDE_PROCEDURE, message));
}

DatumPtr Error::macroReturned(DatumPtr aOutput) {
  QString message = k.eNotList().arg(aOutput.showValue());
  return mainKernel->registerError(createError(ERR_MACRO_RETURNED_NOT_LIST, message), true, true);
}

DatumPtr Error::insideRunresult(DatumPtr cmdName) {
  QString message = k.eCantInsideRunresult().arg(cmdName.showValue());
  return mainKernel->registerError(createError(ERR_INSIDE_RUNRESULT, message), true, true);
}

DatumPtr Error::noApply(DatumPtr what) {
  QString message = k.eCantNoApply().arg(what.showValue());
  return mainKernel->registerError(createError(ERR_NO_APPLY, message), true, true);
}

void Error::stackOverflow()
{
  mainKernel->registerError(createError(ERR_STACK_OVERFLOW, k.eStackOverflow()));
}

void Error::throwError(DatumPtr aTag, DatumPtr aOutput) {
  Error *e;
  if (aTag.wordValue()->keyValue() == k.error()) {
    if (aOutput == nothing) {
          e = createError(ERR_THROW, k.throwError());
      e->tag = aTag;
    } else {
      e = createError(ERR_CUSTOM_THROW, aOutput);
      e->tag = aTag;
    }
  } else {
    QString message = k.eNoCatch().arg(aTag.showValue());
    e = createError(ERR_NO_CATCH, message);
    e->tag = aTag;
    e->output = aOutput;
  }
  mainKernel->registerError(e);
}


void Error::addToPool()
{
  tag = nothing;
  errorText = nothing;
  output = nothing;
  procedure = nothing;
  instructionLine = nothing;
  pool.dealloc(this);
}

