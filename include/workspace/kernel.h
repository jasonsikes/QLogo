#ifndef KERNEL_H
#define KERNEL_H

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
#include "workspace/library.h"
#include "workspace/callframe.h"
#include "workspace/evaluator.h"
#include "workspace/propertylists.h"
#include <QColor>
#include <QSet>
#include <stack>
#include <QVector>
#include <memory>

class ProcedureScope;
class TextStream;
class NewEvaluator;

/// @brief Special variables.
/// @note These are variable names that are used to store special values.
enum SpecialNames
{
    ERRACT
};

/// @brief The Kernel class does most of the work for the QLogo interpreter.
/// @details The Kernel class is the evaluator of the QLogo
/// language and maintains the state of execution of the QLogo code.
class Kernel
{
    DatumPtr filePrefix_;

    PropertyLists plists_;

    QHash<QString, TextStream *> fileStreams_;
    QSet<TextStream *> writableStreams_;
    QSet<TextStream *> readableStreams_;
    TextStream *readStream_;
    TextStream *systemReadStream_;
    TextStream *writeStream_;
    TextStream *systemWriteStream_;
    TextStream *stdioStream_;

    bool isPausing_;

    void closeAll();

    void initPalette();

    /// @brief Run the explicit control evaluator.
    /// @param listP The list to run.
    /// @return The result of the last expression in the list.
    DatumPtr runECE();

    /// Initialize LOGO system variables
    void initVariables();

    /// @brief Private constructor for singleton pattern.
    Kernel();

    Kernel(const Kernel &) = delete;
    Kernel(Kernel &&) = delete;
    Kernel &operator=(const Kernel &) = delete;
    Kernel &operator=(Kernel &&) = delete;

  public:
    /// @brief Get the singleton instance of the Kernel class.
    /// @return The singleton instance of the Kernel class.
    static Kernel &get()
    {
        static Kernel instance;
        return instance;
    }

    /// @brief Destructor.
    ~Kernel();

    /// @brief The call frame stack.
    /// @note This stack is used to store the evaluation state of procedures and subprocedures while they are executing.
    std::stack<std::unique_ptr<NewCallFrame>> callFrameStack_;

    /************ variables ************/

    /// @brief The variables hash.
    QHash<QString, DatumPtr> variables_;

    // SPECIAL VARIABLES
    Datum *specialVar(SpecialNames name) const;

    /// @brief Set a value for a variable.
    /// @param aDatum The value to store.
    /// @param name The name of the variable to set.
    void setDatumForName(const DatumPtr &aDatum, const QString &name);

    /// @brief Return the value of a variable.
    /// @param name The name of the variable to search for.
    /// @return The stored value associated with 'name' or 'nothing' if the variable is not found.
    DatumPtr datumForName(const QString &name) const;

    /// @brief Return true if value keyed by name exists in the variables hash.
    /// @param name The name of the variable to search for.
    /// @return True if the variable exists, false otherwise.
    bool doesExist(const QString &name) const;

    /// @brief Erase name and its value from the variables hash.
    /// @param name The name of the variable to erase.
    void eraseVar(const QString &name);

    /// @brief Return a list of all variables defined.
    /// @return A list of all variables defined.
    DatumPtr allVariables() const;

    /// @brief Repcount is for use in looping functions (e.g. REPEAT)
    double repcount_ = -1;

    /************ ECE ************/

    /// @brief The next operation to perform.
    void(Kernel::*nextOperation_)() = nullptr;
    /// @brief The jump location to start execution from.
    int32_t jumpLocation_ = 0;
    /// @brief The return value when an operation completes.
    DatumPtr retval_;

    /// @brief Get the current call frame.
    NewCallFrame *currentCallFrame() const;

    /// @brief Get the topmost evaluator from the evaluation stack.
    NewEvaluator *topEvaluator() const;

    /// @brief Begin or continue evaluating a list. Will return when execution is suspended.
    void ece_evaluateList();

    /// @brief Decide what to do next after emptying the evaluation stack.
    void ece_decideEmptyEvaluationStack();

    /// @brief Pop the topmost evaluator from the evaluation stack.
    void ece_popEvaluator();

    /************ miscellaneous ************/

    /// @brief The palette of colors.
    /// @details The first 16 colors [0-15] are the standard Logo colors. The first 8
    /// are immutable. The rest [8-100] are user-assignable.
    QVector<QColor> palette_;

    /// @brief The current error, if any.
    DatumPtr currentError_;

    /// @brief READ a line of input, EVALUATE it, PRINT the result, LOOP.
    /// @param isPausing Whether we are in a PAUSE loop.
    /// @param prompt The prompt to display to the user.
    /// @return The result of the last expression entered.
    /// @note The return value is useful only in the case of PAUSE.
    DatumPtr readEvalPrintLoop(bool isPausing, const QString &prompt = QString());

    /// @brief Get the procedure name from a TO or .MACRO node.
    /// @param node The ASTNode that holds the command, the procedure name and parameters.
    /// @return The procedure name.
    DatumPtr procnameFromNode(ASTNode *node);

    /// @brief Get the procedure parameters from a TO or .MACRO node.
    /// @param node The ASTNode that holds the command, the procedure name and parameters.
    /// @return The procedure parameters as a list with ':' and '"' filered out from the parameter names.
    DatumPtr procParametersFromNode(ASTNode *node);

    /// @brief Input the body of a procedure.
    /// @param node the ASTNode that holds the command, the procedure name and parameters.
    /// @return the given node on success or Error on error.
    Datum *inputProcedure(ASTNode *node);

    /// @brief Print a string to the standard output.
    /// @param text The text to print.
    /// @details This method prints a string to the standard output. The standard
    /// output can either be the console or a file, or both in the case of dribbling.
    void stdPrint(const QString &text) const;

    /// @brief Print a string to the system output.
    /// @param text The text to print.
    void sysPrint(const QString &text) const;

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


    /// @brief Perform pause, essentially a REPL loop.
    /// @return The value passed to CONTINUE, if any.
    DatumPtr pause();

    /// @brief Runs the main loop of the QLogo interpreter.
    /// @return The exit code of the application.
    /// @note This method initializes the interface, sets up signal handlers,
    /// runs the read-eval-print loop, and at termination restores signal handlers.
    int run();
};

#endif // KERNEL_H
