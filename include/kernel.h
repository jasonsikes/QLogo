#ifndef EXECUTOR_H
#define EXECUTOR_H

//===-- qlogo/kernel.h - Kernel class definition -------*- C++ -*-===//
//
// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Kernel class, which is the
/// executor proper of the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "datum_ptr.h"
#include "workspace/callframe.h"
#include "library.h"
#include "workspace/propertylists.h"
#include <QColor>
#include <QSet>
#include <QVector>

class Parser;
class Turtle;
class ProcedureScope;
class Compiler;
class TextStream;


/// @brief Special variables.
/// @note These are variable names that are used to store special values.
enum SpecialNames
{
    ERRACT
};




/// @brief The Kernel class does most of the work for the QLogo interpreter.
/// @details The Kernel class is the core of the QLogo interpreter. It is the evaluator of the QLogo
/// language. It maintains the state of execution of the Logo code. It owns the objects that
/// support the execution of the code, such as the parser, the procedures, and
/// the turtle. 
class Kernel
{
    Procedures *procedures;
    Compiler *theCompiler;
    DatumPtr filePrefix;

    Turtle *turtle;

    PropertyLists plists;

    Help help;

    QHash<QString, TextStream *> fileStreams;
    QSet<TextStream *> writableStreams;
    QSet<TextStream *> readableStreams;
    TextStream *readStream;
    TextStream *systemReadStream;
    TextStream *writeStream;
    TextStream *systemWriteStream;
    TextStream *stdioStream;

    DatumPtr currentLine;
    DatumPtr editFileName;
    QString workspaceText;

    void inputProcedure(DatumPtr nodeP);

    void closeAll();

    void initPalette(void);

    /// Initialize LOGO system variables
    void initVariables(void);


  public:

    /// @brief Parser.
    Parser *parser;


    /// @brief The current error, if any.
    DatumPtr currentError;

    /// @brief Constructor.
    Kernel();

    Kernel(const Kernel &) = delete;
    Kernel(Kernel &&) = delete;
    Kernel &operator=(const Kernel &) = delete;
    Kernel &operator=(Kernel &&) = delete;

    /// @brief Destructor.
    ~Kernel();

    /// @brief The procedure frame stack
    ///
    /// @todo
    CallFrameStack callStack;


    /// @brief The palette of colors.
    /// @details The first 16 colors [0-15] are the standard Logo colors. The first 8
    /// are immutable. The rest [8-100] are user-assignable.
    QVector<QColor> palette;

    /// @brief READ a line of input, EVALUATE it, PRINT the result, LOOP.
    /// @param isPausing Whether we are in a PAUSE loop.
    /// @param prompt The prompt to display to the user.
    /// @return The result of the last expression entered.
    /// @note The return value is useful only in the case of PAUSE.
    DatumPtr readEvalPrintLoop(bool isPausing, const QString &prompt = QString());


    /// @brief Input the body of a procedure.
    /// @param node the ASTNode that holds the command, the procedure name and parameters.
    /// @return the given node on success or Error on error.
    Datum *inputProcedure(ASTNode *node);

    /// @brief Print a string to the standard output.
    /// @param text The text to print.
    /// @details This method prints a string to the standard output. The standard
    /// output can either be the console or a file, or both in the case of dribbling.
    void stdPrint(const QString &text);

    /// @brief Print a string to the system output.
    /// @param text The text to print.
    void sysPrint(const QString &text);

    /// @brief Run a list.
    /// @param listP The list to run.
    /// @param startTag If not null, search for the tag in the list and run from there.
    /// @return The result of the last expression in the list.
    DatumPtr runList(const DatumPtr &listP, const QString &startTag = QString());

    /// @brief Convert a Datum to a QColor.
    /// @param colorP The Datum to convert.
    /// @return The QColor.
    /// @details This method converts a Datum to a QColor. The Datum can be a
    /// color number, color name, an RGB list or an RGBA list.
    bool colorFromDatumPtr(QColor &retval, const DatumPtr &colorP) const;

    /// @brief Convert a Datum to a QVector<double>.
    /// @param v The Datum to convert.
    /// @return The QVector<double>.
    bool numbersFromList(QVector<double> &retval, const DatumPtr &listP) const;

    /// @brief Get the filepath for a filename.
    /// @param filenameP The filename to get the filepath for.
    /// @return The filepath for the filename with the current file prefix.
    QString filepathForFilename(const DatumPtr &filenameP) const;

    // SPECIAL VARIABLES
    Datum* specialVar(SpecialNames name);

    /// @brief Perform pause, essentially a REPL loop.
    /// @return The value passed to CONTINUE, if any.
    DatumPtr pause();
};

#endif // EXECUTOR_H
