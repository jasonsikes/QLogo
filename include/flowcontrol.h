#ifndef FLOWCONTROL_H
#define FLOWCONTROL_H

//===-- qlogo/datum/flowcontrol.h - FlowControl class definition -------*- C++ -*-===//
//
// Copyright 2017-2025 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the FlowControl class and its subclasses,
/// which are used to signal the end of execution of a list or procedure.
///
//===----------------------------------------------------------------------===//

#include "datum_ptr.h"
#include <QString>

// The subclasses of FlowControl have a FC prefix because "goto", "return", and "continue"
// are reserved words in C++, and because "Error" is a LLVM class name.
struct FCError;
struct FCGoto;
struct FCReturn;
struct FCContinuation;


/// @brief The flow control instruction.
///
/// @details This class is used to signal the end of execution of a list or procedure. The base class does nothing.
/// The subclasses contain the methods for the various flow control instructions.
/// The FlowControl subtypes are as follows:
/// - typeGoto: Goto the specified tag.
/// - typeReturn: Return from the current procedure.
/// - typeContinuation: Similar to typeReturn but scope is unaffected.
/// - typeError: Signal an error.
/// 
struct FlowControl : public Datum {

    /// @brief The source node of the flow control instruction, for blame tracking.
    DatumPtr sourceNode;
    /// @brief Most flow control types have one datum.
    DatumPtr data;
    /// @details FCError and FCContinuation have a list of data. This is where it is stored.
    QList<DatumPtr> dataAry;
};

struct FCGoto : public FlowControl {

    FCGoto(DatumPtr aSourceNode, DatumPtr aTag)
    {
        isa = Datum::typeGoto;
        data = aTag;
        sourceNode = aSourceNode;
    }

    /// @brief The tag to goto.
    DatumPtr& tag()
    {
        Q_ASSERT(isa == Datum::typeGoto);
        Q_ASSERT(dataAry.isEmpty());
        return data;
    }
};

struct FCReturn : public FlowControl {

    FCReturn(DatumPtr aSourceNode, DatumPtr aValue)
    {
        isa = Datum::typeReturn;
        data = aValue;
        sourceNode = aSourceNode;
    }

    /// @brief The value to return.
    DatumPtr& returnValue()
    {
        Q_ASSERT(isa == Datum::typeReturn);
        return data;
    }
};

struct FCContinuation : public FlowControl {

    FCContinuation(DatumPtr aSourceNode, DatumPtr aProcedure, QList<DatumPtr> aParams)
    {
        isa = Datum::typeContinuation;
        data = aProcedure;
        dataAry = aParams;
        sourceNode = aSourceNode;
    }

    /// @brief The procedure to continue.
    DatumPtr& procedure()
    {
        Q_ASSERT(isa == Datum::typeContinuation);
        return data;
    }

    /// @brief The parameters to pass to the procedure.
    QList<DatumPtr>& params()
    {
        Q_ASSERT(isa == Datum::typeContinuation);
        return dataAry;
    }
};

/// @brief The error code for an error.
enum ErrCode : int
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
    ERR_IS_PRIMITIVE = 22,
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

/// @brief An error object.
struct FCError : public FlowControl
{
private:
    /// @brief Initialize the error object with information from the kernel object.
    void commonInit();

public:
    ErrCode code;

    /// @brief The text string message of the error.
    DatumPtr& message()
    {
        Q_ASSERT(isa == Datum::typeError);
        Q_ASSERT(dataAry.size() == 5);
        return dataAry[0];
    }

    /// @brief The text string message of the error (const version).
    const DatumPtr& message() const
    {
        Q_ASSERT(isa == Datum::typeError);
        Q_ASSERT(dataAry.size() == 5);
        return dataAry[0];
    }

    /// @brief The error tag.
    DatumPtr& tag()
    {
        Q_ASSERT(isa == Datum::typeError);
        Q_ASSERT(dataAry.size() == 5);
        return dataAry[1];
    }

    /// @brief The error tag (const version).
    const DatumPtr& tag() const
    {
        Q_ASSERT(isa == Datum::typeError);
        Q_ASSERT(dataAry.size() == 5);
        return dataAry[1];
    }

    /// @brief The output of the error.
    /// @details This is used to store the output of the error. The Logo library uses the
    /// throw/catch mechanism as a means of passing execution control.
    /// The output is the value that is thrown to and used by the catch block.
    DatumPtr& output()
    {
        Q_ASSERT(isa == Datum::typeError);
        Q_ASSERT(dataAry.size() == 5);
        return dataAry[2];
    }

    /// @brief The output of the error (const version).
    const DatumPtr& output() const
    {
        Q_ASSERT(isa == Datum::typeError);
        Q_ASSERT(dataAry.size() == 5);
        return dataAry[2];
    }

    /// @brief The line where the error occurred.
    DatumPtr& line()
    {
        Q_ASSERT(isa == Datum::typeError);
        Q_ASSERT(dataAry.size() == 5);
        return dataAry[3];
    }

    /// @brief The line where the error occurred (const version).
    const DatumPtr& line() const
    {
        Q_ASSERT(isa == Datum::typeError);
        Q_ASSERT(dataAry.size() == 5);
        return dataAry[3];
    }

    /// @brief The procedure where the error occurred.
    DatumPtr& procedure()
    {
        Q_ASSERT(isa == Datum::typeError);
        Q_ASSERT(dataAry.size() == 5);
        return dataAry[4];
    }

    /// @brief The procedure where the error occurred (const version).
    const DatumPtr& procedure() const
    {
        Q_ASSERT(isa == Datum::typeError);
        Q_ASSERT(dataAry.size() == 5);
        return dataAry[4];
    }

    /// @brief Create an error object.
    /// @param aCode The error code.
    /// @param aMessage The error message.
    /// @param aTag The tag of the error. Can be used with `CATCH tag` and/or `SYSTEM/TOPLEVEL/PAUSE` to handle the error.
    /// @note The other elements are fetched from the procedure.
    FCError(ErrCode aCode, DatumPtr aMessage, DatumPtr aTag = nothing, DatumPtr aOutput = nothing)
        : code(aCode)
    {
        isa = Datum::typeError;
        dataAry.resize(5);
        tag() = aTag;
        message() = aMessage;
        output() = aOutput;
        commonInit();
    }

    /// @brief Create an error object.
    /// @param aCode The error code.
    /// @param aMessage The error message.
    /// @param aTag The tag of the error. Can be used with `CATCH tag` and/or `SYSTEM/TOPLEVEL/PAUSE` to handle the error.
    /// @note The other elements are fetched from the procedure.
    FCError(ErrCode aCode, QString aMessage, DatumPtr aTag = nothing, DatumPtr aOutput = nothing)
        : code(aCode)
    {
        isa = Datum::typeError;
        dataAry.resize(5);
        tag() = aTag;
        message() = DatumPtr(aMessage);
        output() = aOutput;
        commonInit();
    }

    /// @brief Create an error object for a custom error.
    /// @param tag The tag of the error. Can be used with `CATCH tag` and/or `SYSTEM/TOPLEVEL/PAUSE` to handle the error.
    /// @param message An optional message to accompany the error.
    /// @param output An optional output to accompany the error.
    static FCError* custom(const DatumPtr &tag, DatumPtr message = nothing, const DatumPtr &output = nothing);

    /// @brief Create an error object for a turtle out of bounds error.
    static FCError* turtleOutOfBounds();

    /// @brief Create an error object for attempting to use a graphics/turtle command with no active graphics window.
    static FCError* noGraphics();

    /// @brief Create an error object for attempting to use a forbidden command inside a procedure.
    static FCError* toInProc(const DatumPtr &cmd);

    /// @brief Create an error object for an unexpected closing square during parsing.
    static FCError* unexpectedCloseSquare();

    /// @brief Create an error object for an unexpected closing brace during parsing.
    static FCError* unexpectedCloseBrace();

    /// @brief Create an error object for an unexpected closing parenthesis during parsing.
    static FCError* unexpectedCloseParen();

    /// @brief Create an error object for a file system error.
    static FCError* fileSystem();

    /// @brief Create an error object for attempting to use a command that is forbidden outside a procedure.
    /// @param cmd The command that was attempted to be used.
    static FCError* notInsideProcedure(const DatumPtr &cmd);

    /// @brief Create an error object for an unknown command or procedure.
    /// @param cmd The name of the unknown command or procedure.
    static FCError* noHow(const DatumPtr &cmd);

    /// @brief Create an error object for an unknown catch tag.
    /// @param tag The name of the unknown catch tag.
    static FCError* noCatch(DatumPtr tag);

    /// @brief Create an error object for an input that is not accepted by a command.
    /// @param x The name of the command that rejected the input.
    /// @param y The input that was rejected.
    static FCError* doesntLike(const DatumPtr &x, const DatumPtr &y);

    /// @brief create an error object for a command output that has no destination.
    /// @param x The output that has no destination.
    static FCError* dontSay(const DatumPtr &x);

    /// @brief create an error object for a command that was called without a TEST.
    /// @param x The command that was called without a TEST.
    static FCError* noTest(const DatumPtr &x);

    /// @brief Create an error object for a command that didn't output to another that was expecting input.
    /// @param x The command that didn't output.
    /// @param y The destination that expected input.
    static FCError* didntOutput(const DatumPtr &x, const DatumPtr &y);

    /// @brief Create an error object for a command that has too many inputs.
    /// @param x The command that has too many inputs.
    static FCError* tooManyInputs(const DatumPtr &x);

    /// @brief Create an error object for a command that has too few inputs.
    /// @param x The command that has too few inputs.
    static FCError* notEnoughInputs(const DatumPtr &x);

    /// @brief Create an error object for a variable that has no value.
    /// @param x The variable that has no value.
    static FCError* noValue(const DatumPtr &x);

    /// @brief Create an error object for trying to nest FILLED blocks.
    static FCError* alreadyFilling();

    /// @brief Create an error object for a procedure that is already defined.
    /// @param x The procedure that is already defined.
    static FCError* procDefined(const DatumPtr &x);

    /// @brief Create an error object for a bad default expression for an optional input.
    /// @param x The bad default expression.
    static FCError* badDefault(const DatumPtr &x);

    /// @brief Create an error object for a closing parenthesis not found when expected during parsing.
    static FCError* parenNf();

    /// @brief Create an error object for trying to manipulate a primitive.
    /// @param x The primitive that is being manipulated.
    static FCError* isPrimitive(const DatumPtr &x);


    /// @brief Return a string suitable for the PRINT command
    ///
    /// @param flags Flags to control the output. See ToStringFlags for possible values.
    /// @param printDepthLimit Limit the depth of sublists for readability. (ignored for errors)
    /// @param printWidthLimit Limit the length of a string or list for readability. (ignored for errors)
    /// @param visited Set of visited nodes to prevent cycles.
    /// @return A string suitable for the PRINT command
    QString toString( ToStringFlags flags = ToStringFlags_None, int printDepthLimit = -1, int printWidthLimit = -1, VisitedSet *visited = nullptr) const override;

};

#endif // FLOWCONTROL_H
