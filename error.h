#ifndef ERROR_H
#define ERROR_H

//===-- qlogo/error.h - Error class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Error class, which stores
/// error information during exceptions.
///
//===----------------------------------------------------------------------===//

#include "datum.h"

class Kernel;

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
};

#endif // ERROR_H
