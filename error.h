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
public:

  enum errorCode {
    ecFatalInternal =  0, //Fatal internal error (can't be caught)
    ecOutOfMemory =  1, //Out of memory
    ecStackOverflow =  2, //Stack overflow
    ecTurtleOutOfBounds =  3, //Turtle out of bounds
    ecDoesntLike =  4, //PROC doesn't like DATUM as input (not recoverable)
    ecDidntOutput =  5, //PROC didn't output to PROC
    ecNotEnoughInputs = 6, //Not enough inputs to PROC
    ecDowsntLikeRec = 7, //PROC doesn't like DATUM as input (recoverable)
    ecTooManyParens = 8, //Too much inside ()'s
    ecNoSay = 9, //You don't say what to do with DATUM
    ecNoCloseParen = 10, //')' not found
    ecNoValue = 11, //VAR has no value
    ecUnexpectedCloseParen = 12, //Unexpected ')'
    ecNoHowRec = 13, //I don't know how to PROC (recoverable)
    ecNoCatch = 14, //Can't find catch tag for THROWTAG
    ecAlreadyDefined = 15, //PROC is already defined
    ecStopped = 16, //Stopped
    ecAlreadyDribbling = 17, //Already dribbling
    ecFileSystem = 18, //File system error
    ecCmdInPause = 19, //Can't use Command within PAUSE
    //was: Assuming you mean IFELSE, not IF (warning only)
    ecShadowed = 20, //VAR shadowed by local in procedure call (warning only)
    ecThrow = 21, //Throw "Error
    ecPrimitive = 22, //PROC is a primitive
    ecToInsideProc = 23, //Can't use TO inside a procedure
    ecNoHow = 24, //I don't know how to PROC (not recoverable)
    ecNoTest = 25, //IFTRUE/IFFALSE without TEST
    ecUnexpectedCloseSquare = 26, //Unexpected ']'
    ecUnexpectedCloseCurly = 27, //Unexpected '}'
    ecAlreadyFilling = 28, //Already filling
    //was: Couldn't initialize graphics
    ecMacroReturned = 29, //Macro returned VALUE instead of a list
    ecNoSay2 = 30, //You don't say what to do with VALUE
    ecNotInsideProc = 31, //Can only use STOP or OUTPUT inside a procedure
    ecDoesntLike2 = 32, //APPLY doesn't like BADTHING as input
    ecEnd = 33, //END inside multi-line instruction
    ecReallyOutOfMem = 34, //Really out of memory (can't be caught)
    ecUserGen = 35, //user-generated error message (THROW "ERROR [message])
    ecEnd2 = 36, //END inside multi-line instruction
    ecBadDefault = 37, //Bad default expression for optional input: EXPR
    ecNoStop = 38, //Can't use OUTPUT or STOP inside RUNRESULT
    ecAssume = 39 , //Assuming you meant 'FD 100', not FD100 (or similar)
    ecCantOpen = 40, //I can't open file FILENAME
    ecAlreadyOpen = 41, //File FILENAME already open
    ecNotOpen = 42, //File FILENAME not open
    ecListMultExpr = 43, //Runlist [EXPR EXPR] has more than one expression.
    ecNoApply = 44 // Can't use '?' without APPLY
  };


protected:
  Error(errorCode aNumber, const QString &aErrorText);

  Error(errorCode aNumber, DatumP aErrorText);

public:

  errorCode code;
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
  static DatumP insideRunresult(DatumP cmdName, DatumP listName);
  static DatumP noApply(DatumP what);
  static void stackOverflow();
};

#endif // ERROR_H
