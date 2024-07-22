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

/// @brief The error code for an error.
enum ErrorCode : int
{
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
    ERR_BAD_DEFAULT_EXPRESSION = 37,
    ERR_INSIDE_RUNRESULT = 38,
    ERR_NO_APPLY = 44,
    ERR_STACK_OVERFLOW = 2,
    ERR_CUSTOM_THROW = 35,
    ERR_THROW = 21,
    ERR_NO_CATCH = 14
};

/// @brief The error class.
/// @details This class is used to store error information during exceptions.
class Error : public Datum
{
  protected:
    static Error *createError(int aNumber, const QString &aErrorText);

    static Error *createError(int aNumber, DatumPtr aErrorText);

  public:
    /// @brief Constructor.
    Error();

    /// @brief The ErrorCode corresponding to the type of error.
    int code;

    /// @brief The error tag, if one was provided for a custom throw.
    DatumPtr tag;

    /// @brief The error text, either one of the standard error messages or a custom error message.
    DatumPtr errorText;

    /// @brief The error output message, if one was provided.
    DatumPtr output;

    /// @brief The name of the procedure where the error occurred, if applicable.
    DatumPtr procedure;

    /// @brief The instruction line where the error occurred, if applicable.
    DatumPtr instructionLine;

    /// @brief The error type.
    DatumType isa()
    {
        return errorType;
    }

    /// @brief Throw an error for a turtle out of bounds.
    static void turtleOutOfBounds();

    /// @brief Throw an error for a procedure not accepting an input value.
    static DatumPtr doesntLike(DatumPtr who, DatumPtr what, bool allowErract = false, bool allowRecovery = false);

    /// @brief Throw an error for a procedure not outputting a value where one is expected.
    static void didntOutput(DatumPtr src, DatumPtr dest);

    /// @brief Throw an error for not enough inputs to a procedure.
    static void notEnough(DatumPtr dest);

    /// @brief Throw an error for too many inputs to a procedure.
    static void tooMany(DatumPtr dest);

    /// @brief Throw an error for ignoring the output of a procedure.
    static void dontSay(DatumPtr datum);

    /// @brief Throw an error for a parenthesis not being found.
    static void parenNf();

    /// @brief Throw an error for an unexpected close parenthesis.
    static void unexpectedCloseParen();

    /// @brief Throw an error for a procedure not returning a value. Allow user to provide a value to recover.
    static DatumPtr noValueRecoverable(DatumPtr datum);

    /// @brief Throw an error for a procedure not returning a value. User may not provide a value to recover.
    static void noValue(DatumPtr datum);

    /// @brief Throw an error for command name not found.
    static void noHow(DatumPtr datum);

    /// @brief Throw an error for command name not found. Allow user to provide a procedure definition to recover.
    static DatumPtr noHowRecoverable(DatumPtr datum);

    /// @brief Throw an error for a procedure already defined.
    static void procDefined(DatumPtr procname);

    /// @brief Throw an error for attempting to define a procedure using 'TO' while inside a procedure.
    static void toInProc(DatumPtr cmd);

    /// @brief Throw an error for attempting to define a procedure using 'TO' while inside a pause.
    static void toInPause(DatumPtr cmd);

    /// @brief Throw an error for an unexpected close square bracket.
    static void unexpectedCloseSquare();

    /// @brief Throw an error for an unexpected close curly bracket.
    static void unexpectedCloseBrace();

    /// @brief Throw an error for a list containing multiple expressions.
    static void listHasMultExp(DatumPtr list);

    /// @brief Throw an error for attempting to open a file that is already open.
    static void alreadyOpen(DatumPtr what);

    /// @brief Throw an error for attempting to open a file that cannot be opened.
    static void cantOpen(DatumPtr what);

    /// @brief Throw an error for attempting to read or write a file that is not open.
    static void notOpen(DatumPtr what);

    /// @brief Throw an error for attempting to dribble while already dribbling.
    static void alreadyDribbling();

    /// @brief Throw an error for a file system error.
    static void fileSystem();

    /// @brief Throw an error for a file system error. Allow user to recover.
    static DatumPtr fileSystemRecoverable();

    /// @brief Throw an error for attempting to fill a polygon while already filling.
    static void alreadyFilling();

    /// @brief Throw an error for defining a procedure whose name is the same as a primitive.
    static void isPrimative(DatumPtr what);

    /// @brief Throw an error for using IFTRUE/IFFALSE when TEST has not been used.
    static DatumPtr noTest(DatumPtr what);

    /// @brief Throw an error for attempting to use a procedure primitive while not inside a procedure.
    static void notInsideProcedure(DatumPtr what);

    /// @brief Throw a custom error.
    static void throwError(DatumPtr aTag, DatumPtr aOutput);

    /// @brief Throw an error for a macro returning a non-list value.
    static DatumPtr macroReturned(DatumPtr aOutput);

    /// @brief Throw an error for using a procedure command while running RUNRESULT.
    static DatumPtr insideRunresult(DatumPtr cmdName);

    /// @brief Throw an error for attempting to use '?' operator but not in APPLY context.
    static DatumPtr noApply(DatumPtr what);

    /// @brief Throw an error for a stack overflow.
    static void stackOverflow();

    /// @brief Throw an error for attempting to use a graphics/turtle command with no active graphics window.
    static void noGraphics();

    /// @brief Throw an error for a bad default expression.
    static void badDefaultExpression(DatumPtr what);
};

#endif // ERROR_H
