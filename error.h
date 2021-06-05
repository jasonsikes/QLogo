#ifndef ERROR_H
#define ERROR_H

//===-- qlogo/error.h - Error class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Error class, which stores
/// error information during exceptions.
///
//===----------------------------------------------------------------------===//

#include "datum.h"

class Kernel;

enum ErrorCode : int {
    ERR_TURTLE_BOUNDS = 3,
    ERR_DOESNT_LIKE = 4,
    ERR_DIDNT_OUTPUT = 5,
    ERR_NOT_ENOUGH_INPUTS = 6,
    ERR_TOO_MANY_INPUTS = 8,
    ERR_DONT_SAY = 9,
    ERR_PAREN_NF = 10,
    ERR_NO_VALUE = 11,
    ERR_NO_HOW = 13,
    ERR_ALREADY_DEFINED = 15,
    ERR_IS_PRIMATIVE = 22,
    ERR_TO_IN_PROC = 23,
    ERR_TO_IN_PAUSE = 19,
    ERR_UNEXPECTED_SQUARE = 26,
    ERR_UNEXPECTED_BRACE = 27,
    ERR_UNEXPECTED_PAREN = 12,
    ERR_ALREADY_DRIBBLING = 17,
    ERR_FILESYSTEM = 18,
    ERR_LIST_HAS_MULTIPLE_EXPRESSIONS = 43,
    ERR_ALREADY_OPEN = 41,
    ERR_CANT_OPEN = 40,
    ERR_NOT_OPEN = 42,
    ERR_ALREADY_FILLING = 45,
    ERR_NO_GRAPHICS = 28,
    ERR_NO_TEST = 25,
    ERR_NOT_INSIDE_PROCEDURE = 31,
    ERR_MACRO_RETURNED_NOT_LIST = 29,
    ERR_INSIDE_RUNRESULT = 38,
    ERR_NO_APPLY = 44,
    ERR_STACK_OVERFLOW = 2,
    ERR_CUSTOM_THROW = 35,
    ERR_THROW = 21,
    ERR_NO_CATCH = 14
};

class Error : public Datum {
protected:
  Error(int aNumber, const QString &aErrorText);

  Error(int aNumber, DatumP aErrorText);

public:
  int code;
  DatumP tag;
  DatumP errorText;
  DatumP output;
  DatumP procedure;
  DatumP instructionLine; // The Word/List where the error occurred.

  DatumType isa() { return errorType; }

  static void setKernel(Kernel *aKernel);

  // Throwers for all the error messages
  static void turtleOutOfBounds();
  static DatumP doesntLike(DatumP who, DatumP what, bool allowErract = false,
                           bool allowRecovery = false);
  static void didntOutput(DatumP src, DatumP dest);
  static void notEnough(DatumP dest);
  static void tooMany(DatumP dest);
  static void dontSay(DatumP datum);
  static void parenNf();
  static void unexpectedCloseParen();
  static DatumP noValueRecoverable(DatumP datum);
  static void noValue(DatumP datum);
  static void noHow(DatumP datum);
  static DatumP noHowRecoverable(DatumP datum);
  static void procDefined(DatumP procname);
  static void toInProc(DatumP cmd);
  static void toInPause(DatumP cmd);
  static void unexpectedCloseSquare();
  static void unexpectedCloseBrace();
  static void listHasMultExp(DatumP list);
  static void alreadyOpen(DatumP what);
  static void cantOpen(DatumP what);
  static void notOpen(DatumP what);
  static void alreadyDribbling();
  static void fileSystem();
  static DatumP fileSystemRecoverable();
  static void alreadyFilling();
  static void isPrimative(DatumP what);
  static DatumP noTest(DatumP what);
  static void notInsideProcedure(DatumP what);
  static void throwError(DatumP aTag, DatumP aOutput);
  static DatumP macroReturned(DatumP aOutput);
  static DatumP insideRunresult(DatumP cmdName);
  static DatumP noApply(DatumP what);
  static void stackOverflow();
  static void noGraphics();
};

#endif // ERROR_H
