#ifndef EXECUTOR_H
#define EXECUTOR_H

//===-- qlogo/kernel.h - Kernel class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Kernel class, which is the
/// executor proper of the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "controller/textstream.h"
#include "datum.h"
#include "procedurehelper.h"
#include "sharedconstants.h"
#include "workspace/callframe.h"
#include "workspace/procedures.h"
#include "workspace/propertylists.h"
#include <QColor>
#include <QFile>
#include <QFont>
#include <QRandomGenerator>
#include <QSet>
#include <QVector>

class Parser;
class Turtle;
class ProcedureScope;

/// @brief The Kernel class does most of the work for the QLogo interpreter.
/// @details The Kernel class is the core of the QLogo interpreter. It is the evaluator of the QLogo
/// language. It maintains the state of execution of the Logo code. It owns the objects that
/// support the execution of the code, such as the parser, the procedures, and
/// the turtle. 
class Kernel
{
    friend class ProcedureHelper;
    friend class StreamRedirect;
    Parser *parser;
    Procedures *procedures;
    DatumPtr filePrefix;
    int repcount = -1;
    int pauseLevel = 0;
    bool isPausing = false;

    Turtle *turtle;

    QVector<QColor> palette;
    PropertyLists plists;
    QRandomGenerator randomGenerator;

    Help help;

    CallFrameStack callStack;

    QHash<QString, TextStream *> fileStreams;
    QSet<TextStream *> writableStreams;
    QSet<TextStream *> readableStreams;
    TextStream *readStream;
    TextStream *systemReadStream;
    TextStream *writeStream;
    TextStream *systemWriteStream;
    TextStream *stdioStream;

    DatumPtr currentError;
    DatumPtr currentLine;
    DatumPtr editFileName;
    QString workspaceText;

    // Recursive searches need to make sure we don't get caught in infinite loops.
    // Remember what we searched so we don't search it again.
    QSet<void *> searchedContainers;
    QSet<void *> comparedContainers;

    ASTNode *astnodeValue(DatumPtr caller, DatumPtr value);
    bool numbersFromList(QVector<double> &retval, DatumPtr l);
    DatumPtr contentslistFromDatumPtr(DatumPtr sourceNode);
    void processContentsListWithMethod(DatumPtr contentsList, void (Workspace::*method)(const QString &aName));
    DatumPtr queryContentsListWithMethod(DatumPtr contentslist, bool (Workspace::*method)(const QString &aName));
    void makeVarLocal(const QString &varname);
    DatumPtr executeProcedureCore(DatumPtr node);
    void inputProcedure(DatumPtr nodeP);

    bool colorFromDatumPtr(QColor &retval, DatumPtr colorP);

    QString filepathForFilename(DatumPtr filenameP);
    TextStream *openFileStream(DatumPtr filenameP, QIODevice::OpenMode mode);
    TextStream *createStringStream(DatumPtr filenameP, QIODevice::OpenMode mode);
    TextStream *getStream(ProcedureHelper &h);
    TextStream *open(ProcedureHelper &h, QIODevice::OpenMode openFlags);
    void close(const QString &filename);
    void closeAll();
    void editAndRunFile();
    void editAndRunWorkspaceText();

    void initPalette(void);

    /// Initialize LOGO system variables
    void initVariables(void);

    DatumPtr buildContentsList(showContents_t showWhat);
    QString createPrintoutFromContentsList(DatumPtr contentslist, bool shouldValidate = true);

    /// Check for interrupts and handle them accordingly.
    SignalsEnum_t interruptCheck();

    bool searchContainerForDatum(DatumPtr containerP, DatumPtr thingP, bool ignoreCase);

    // Compare two datums, return true iff equal.
    bool areDatumsEqual(DatumPtr datumP1, DatumPtr datumP2, bool ignoreCase);

    // Return the butfirst of a word or list
    DatumPtr butfirst(DatumPtr srcValue);

    // Determine if the given list contains at least as many items as the
    // integer given.
    bool doesListHaveCountOrMore(List *list, int count);

  public:

    /// @brief Constructor.
    Kernel();

    /// @brief Destructor.
    ~Kernel();

    /// @brief Get the next line of input and run it.
    /// @param shouldHandleError Set to true to tell the method to handle errors.
    /// @return True if line was read and executed successfully, false otherwise.
    bool getLineAndRunIt(bool shouldHandleError = true);

    /// @brief Execute text. Can be any number of lines of text.
    /// @param text The text to execute.
    /// @return The result of the execution.
    QString executeText(const QString &text);

    /// @brief Print a string to the standard output.
    /// @param text The text to print.
    /// @details This method prints a string to the standard output. The standard
    /// output can either be the console or a file, or both in the case of dribbling.
    void stdPrint(const QString &text);

    /// @brief Print a string to the system output.
    /// @param text The text to print.
    /// @details This method prints a string to the system output. The system
    /// output is the console, except in the case of executing text from a file,
    /// where the output is also a file.
    void sysPrint(const QString &text);

    /// @brief Register an error.
    /// @param anError The error to register.
    /// @param allowErract Set to true to allow the error to be recovered.
    /// @param allowRecovery Set to true to allow the error to be recovered.
    /// @return The error that was registered.
    DatumPtr registerError(DatumPtr anError, bool allowErract = false, bool allowRecovery = false);

    /// @brief Pause execution. Enable the user to interact with the execution environment.
    /// @return The result of the pause if user provided a parameter with the continue command.
    DatumPtr pause();

    /// @brief Return true if input is something other than standard input.
    /// @return false if input is standard input; true otherwise.
    bool isInputRedirected();

    /// @brief Run a list.
    /// @param listP The list to run.
    /// @param startTag If not null, search for the tag in the list and run from there.
    /// @return The result of the last expression in the list.
    DatumPtr runList(DatumPtr listP, QString startTag = QString());

    /// @brief NOOP
    /// @details This is a no-op. It is a token that gets passed when GOTO is used.
    DatumPtr excGotoToken(DatumPtr);

    /// @brief Execute a procedure.
    /// @param node The procedure to execute.
    /// @return The output of the procedure.
    DatumPtr executeProcedure(DatumPtr node);

    /// @brief Execute a macro.
    /// @param node The macro to execute.
    /// @return The result of the macro.
    /// @details The macro is a procedure that outputs a list. The list is run in the caller's stack frame
    /// after the procedure's stack frame is torn down.
    DatumPtr executeMacro(DatumPtr node);

    /// @brief Simply return a literal.
    /// @param node The literal.
    /// @return The literal.
    DatumPtr executeLiteral(DatumPtr node);

    /// @brief Return the value of a variable.
    /// @param node The variable to return the value of.
    /// @return The value of the variable.
    DatumPtr executeValueOf(DatumPtr node);

    /// @brief Set the value of a variable.
    /// @param node The variable to set the value of.
    /// @return The new value of the variable.
    DatumPtr excSetfoo(DatumPtr node);

    /// @brief Return the value of a variable.
    /// @param node The variable to return the value of.
    /// @return The value of the variable.
    DatumPtr excFoo(DatumPtr node);

// Since every primitive requires a declaration, a help file entry, a function definition, and an entry in the pimitives table.
// It is far easier to include all the information about a primitive in one place. In QLogo, all the information about
// every primitive can be found in its implimentation file. Various scripts extract the relevant information.
// The python script, 'generate_command_table.py', generates the declarations for the primitives and places them in
// the 'primitives.h' file.
#include "primitives.h"

    /// @brief No operation.
    /// @param node Dummy value.
    /// @return Nothing.
    /// @details This is a no-op. Some UCBLogo commands have no action in QLogo.
    DatumPtr excNoop(DatumPtr node);

    /// @brief Throw an error because the GUI is not available.
    /// @param node Dummy value.
    /// @return Nothing.
    /// @details In environments that do not support the GUI, this primitive throws an error.
    DatumPtr excErrorNoGui(DatumPtr node);

    // SPECIAL VARIABLES


    /// if TRUE, prints the names of procedures defined when loading
	  /// from a file (including the temporary file made by EDIT).
    bool varLOADNOISILY();

    /// if TRUE, indicates that an attempt to use a procedure that doesn't
    /// exist should be taken as an implicit getter or setter procedure
    /// (setter if the first three letters of the name are SET) for a variable
    /// of the same name (without the SET if appropriate).
    bool varALLOWGETSET();

    /// if nonempty, should be an instruction list that will be evaluated
    /// whenever a mouse button is pressed.  Note that the user may have
    DatumPtr varBUTTONACT();

    /// if nonempty, should be an instruction list that will be evaluated
    /// whenever a key is pressed.
    DatumPtr varKEYACT();

    /// if TRUE, any output will be printed in a manner suitable for re-reading
    /// by QLogo to produce the same value.
    bool varFULLPRINTP();

    /// indicates the maximum depth of sublist structure that will be printed.
    int varPRINTDEPTHLIMIT();

    /// indicates the maximum number of members in any one list that will be printed.
    int varPRINTWIDTHLIMIT();

    /// if assigned a list value in a file loaded by LOAD, that value is
    /// run as an instructionlist after loading.
    DatumPtr varSTARTUP();

    /// if TRUE, causes any procedure defined during EDIT or LOAD to be
    /// unburied when editing a file.
    bool varUNBURYONEDIT();

    /// if TRUE, indicates that lower case and upper case letters should be
    /// considered equal by EQUALP, BEFOREP, MEMBERP, etc.
    bool varCASEIGNOREDP();
};

/// @brief Redirects the standard input and output streams.
/// @details This class redirects the standard input and output streams to a
/// new stream for RAII. When the redirection is done, the original streams are
/// saved so they can be restored later when the object is deallocated.
class StreamRedirect
{
    TextStream *originalWriteStream;
    TextStream *originalSystemWriteStream;
    TextStream *originalReadStream;
    TextStream *originalSystemReadStream;

    Parser *originalParser;

  public:

    /// @brief Constructor.
    /// @param newReadStream The new read stream.
    /// @param newWriteStream The new write stream.
    /// @param newParser The new parser.
    /// @details The constructor redirects the standard input and output streams
    /// to a new stream.
    StreamRedirect(TextStream *newReadStream, TextStream *newWriteStream, Parser *newParser);

    /// @brief Destructor.
    /// @details The destructor restores the original standard input and output
    /// streams.
    ~StreamRedirect();
};

#endif // EXECUTOR_H
