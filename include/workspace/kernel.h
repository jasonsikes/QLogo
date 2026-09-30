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
#include "workspace/callframe.h"
#include "workspace/evaluator.h"
#include "workspace/propertylists.h"
#include "workspace/specialvars.h"
#include <QColor>
#include <QList>
#include <QSet>
#include <QStack>
#include <QVector>
#include <deque>
#include <memory>

/// @brief One variable snapshot for a lambda APPLY binding.
struct ApplyLambdaSavedVar
{
    QString name;
    DatumPtr previous;
    bool existed = false;
};

/// @brief Saved variable bindings for one nested lambda APPLY.
struct ApplyLambdaBinding
{
    QList<ApplyLambdaSavedVar> saved;
};

class ProcedureScope;
class TextStream;
struct Evaluator;

/// @brief The Kernel class does most of the work for the QLogo interpreter.
/// @details The Kernel class is the evaluator of the QLogo
/// language and maintains the state of execution of the QLogo code.
class Kernel
{
  public:
    /// @brief Result of evaluating one top-level instruction list.
    enum class TopLevelResult
    {
        /// @brief The instruction finished; read another from this source.
        Continue,
        /// @brief The commander is exhausted.
        EndOfInput,
        /// @brief A runtime error or THROW "TOPLEVEL.
        /// @details The REPL reads another line. A file stops, and Kernel::run
        /// then enters the prompt.
        StopThenPrompt,
        /// @brief THROW "SYSTEM (BYE). Do not enter the prompt.
        ExitProcess,
        /// @brief THROW "PAUSE while already pausing.
        PauseReturn
    };

  private:
    bool isPausing_;

    /// @brief True when the next PAUSE should print the error that invoked ERRACT.
    bool erractMessagePending_ = false;

    /// @brief Text of the error that invoked ERRACT, printed when PAUSE runs.
    QString erractMessage_;

    /// @brief True after ERRACT has handled an error and the user has not typed a line since.
    bool erractLoopArmed_ = false;

    /// @brief Error code of the ERRACT attempt recorded by armErractLoop().
    int erractLoopCode_ = -1;

    /// @brief Datum most recently output by ERRACT, while it is still alive.
    Datum *erractReturned_ = nullptr;

    /// @brief True when @c erractReturned_ is a word with a valid number.
    bool erractReturnedIsNumber_ = false;

    /// @brief Number stored in @c erractReturned_, when @c erractReturnedIsNumber_ is set.
    double erractReturnedNumber_ = 0;

    /// @brief True when that output came from PAUSE, so a bad CONTINUE may try again.
    bool erractReturnedFromUser_ = false;

    /// @brief How many times PAUSE has been entered. Used to notice user input during ERRACT.
    uint32_t pauseCount_ = 0;

    void initPalette();

    /// @brief Run the explicit control evaluator.
    /// @return The result of the last expression in the list.
    DatumPtr runECE();

    /// Initialize LOGO system variables
    void initVariables();

    /// @brief Evaluate one instruction list from the commander.
    /// @param prompt Prompt shown only when the commander is the terminal.
    /// @param isPausing Whether this is a PAUSE session.
    /// @param resultOut Optional; receives the instruction result (PAUSE value).
    /// @return What the session should do next.
    /// @note The caller must already have a base call frame on the stack.
    TopLevelResult executeTopLevelList(const QString &prompt, bool isPausing, DatumPtr *resultOut = nullptr);

    /// @brief Classify a top-level result: print errors, map THROW tags.
    /// @param resultIn The value returned by runECE or a caught FCError.
    /// @param isPausing Whether this is a PAUSE session.
    /// @param resultOut Optional; receives the instruction result (PAUSE value).
    /// @return What the session should do next.
    TopLevelResult finishTopLevelResult(const DatumPtr &resultIn, bool isPausing, DatumPtr *resultOut);

    /// @brief Run STARTUP if this load assigned it a list.
    /// @param startupBefore The STARTUP binding before the file ran.
    /// @return EndOfInput if STARTUP was not assigned a new list; otherwise the
    /// result of running that list.
    TopLevelResult runAssignedStartup(const DatumPtr &startupBefore);

    /// @brief Run the current commander, then STARTUP if this load assigned it.
    /// @param startupBefore The STARTUP binding before the commander ran.
    /// @return Why the load stopped.
    /// @note Shared by loadFile and loadText. The caller must already have
    /// redirected the commander.
    TopLevelResult runLoadedProgram(const DatumPtr &startupBefore);

    /// @brief Open @p path as the commander and run it.
    /// @param path The file to run.
    /// @param exitCode Set to 1 if the file cannot be opened.
    /// @return False if the caller should skip the prompt (BYE or open failure).
    bool runFile(const QString &path, int &exitCode);

    /// @brief Private constructor for singleton pattern.
    Kernel();

  public:
    /// @brief Get the singleton instance of the Kernel class.
    /// @return The singleton instance of the Kernel class.
    static Kernel &get()
    {
        static Kernel instance;
        return instance;
    }

    // Deleted operators to prevent copying.
    Kernel(const Kernel &) = delete;
    Kernel(Kernel *) = delete;
    Kernel(Kernel &&) = delete;
    Kernel &operator=(const Kernel &) = delete;
    Kernel &operator=(Kernel &&) = delete;

    /// @brief Destructor.
    ~Kernel();

    /// @brief The call frame stack.
    /// @note This stack is used to store the evaluation state of procedures while they are executing.
    std::deque<CallFrame> callFrameStack_;

    /// @brief The evaluation stack.
    /// @note This stack is used to store the evaluation state of lists while they are executing.
    std::deque<Evaluator> evaluationStack_;

    /************ variables ************/

    /// @brief The variables hash.
    QHash<QString, DatumPtr> variables_;

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

    /// @brief True if the special variable is the word TRUE.
    /// @param id The interpreter-owned variable.
    /// @return True only if the stored value is a word whose key is TRUE.
    /// A missing name, a non-word, or any other word is false.
    bool specialVarIsTrue(SpecialVars::Id id) const;

    /// @brief Erase name and its value from the variables hash.
    /// @param name The name of the variable to erase.
    void eraseVar(const QString &name);

    /// @brief Return variable names from the variables hash.
    /// @param unburiedOnly When true, omit buried variables. A variable with no
    /// value is included either way. Traced and stepped variables are included.
    /// @return A list of words.
    DatumPtr allVariables(bool unburiedOnly = false) const;

    /// @brief Parse a PRINTOUT/ERASE-style contents-list input into three lists.
    /// @param who The primitive that rejected the input, used in doesn't-like.
    /// @param source A word (one procedure name), a list of words (procedure
    /// names), or a list of up to three lists: procedures, then variables, then
    /// property lists. A word at a given level is one name of that kind.
    /// Members past the third list are rejected.
    /// @return A list of three lists. Throws FCError::doesntLike if a member is
    /// not a word or a list of words, or if the input is an array.
    DatumPtr contentsListFromDatum(const DatumPtr &who, const DatumPtr &source) const;

    /// @brief Logo source for the procedures, variables, and property lists named
    /// in a contents list from contentsListFromDatum.
    /// @param contentslist A list of three lists of names.
    /// @param shouldValidate When true, a primitive or unknown procedure, or a
    /// variable with no value, is an error. When false, FULLTEXT of a missing
    /// procedure is a stand-in empty definition and a missing variable is omitted.
    /// A missing property list prints nothing either way.
    /// @return Text suitable for the write stream or a SAVE file, each definition
    /// ending with a newline.
    QString printoutFromContentsList(const DatumPtr &contentslist, bool shouldValidate = true) const;

    /// @brief POT text for the names in a contents list from contentsListFromDatum.
    /// @param contentslist A list of three lists of names.
    /// @return Title lines for procedures, MAKE lines for variables, and one
    /// Plist line per nonempty property list. A primitive or unknown procedure,
    /// or a variable with no value, is an error. A missing property list is omitted.
    QString titlesFromContentsList(const DatumPtr &contentslist) const;

    /// @brief Erase the procedures, variables, and property lists named in a
    /// contents list from contentsListFromDatum.
    /// @param contentslist A list of three lists of names.
    /// @details A missing name is ignored. A primitive procedure is an error
    /// (REDEFP is not consulted). Names before a rejected primitive stay erased.
    /// Buried, traced, and stepped marks on a name are left in place.
    void eraseFromContentsList(const DatumPtr &contentslist);

    /// @brief Unburied workspace names as a contents list of three lists.
    /// @param procedures When true, include unburied user procedures.
    /// @param variables When true, include unburied variables. A variable with
    /// no value is included.
    /// @param plists When true, include unburied nonempty property lists.
    /// @return A list of three lists. An omitted kind is an empty list.
    /// CONTENTS is all three. NAMES is two lists and is not this method.
    DatumPtr unburiedContents(bool procedures, bool variables, bool plists) const;

    /// @brief Erase the unburied names that unburiedContents would report.
    /// @details Buried names are left in place. ERALL, ERPS, ERNS, and ERPLS
    /// select which kinds to erase. Special variables that CONTENTS still
    /// lists are erased with the rest of the unburied names.
    void eraseUnburied(bool procedures, bool variables, bool plists);

    /// @brief Whether a workspace name is buried, traced, or stepped.
    /// @note Count is the number of marks, not a set to pass to the accessors.
    enum class NameMark
    {
        Buried,
        Traced,
        Stepped,
        Count
    };

    /// @brief Which kind of workspace name a mark applies to.
    /// @note Count is the number of kinds, not a set to pass to the accessors.
    enum class NameKind
    {
        Procedures,
        Variables,
        PropertyLists,
        Count
    };

    /// @brief Add @p name to the set for @p mark and @p kind.
    /// @param mark Buried, traced, or stepped.
    /// @param kind Procedures, variables, or property lists.
    /// @param name The workspace name. A name already in the set is left there.
    /// @note For example, addName(NameMark::Buried, NameKind::Procedures, "FOO").
    void addName(NameMark mark, NameKind kind, const QString &name);

    /// @brief Remove @p name from the set for @p mark and @p kind.
    /// @param mark Buried, traced, or stepped.
    /// @param kind Procedures, variables, or property lists.
    /// @param name The workspace name. A name that is not in the set is ignored.
    void removeName(NameMark mark, NameKind kind, const QString &name);

    /// @brief Return true if @p name is in the set for @p mark and @p kind.
    /// @param mark Buried, traced, or stepped.
    /// @param kind Procedures, variables, or property lists.
    /// @param name The workspace name.
    bool containsName(NameMark mark, NameKind kind, const QString &name) const;

    /// @brief The names in the set for @p mark and @p kind.
    /// @param mark Buried, traced, or stepped.
    /// @param kind Procedures, variables, or property lists.
    /// @note Iteration order is hash order and need not stay the same across runs.
    const QSet<QString> &nameList(NameMark mark, NameKind kind) const;

    /// @brief Spaces printed before a TRACE line.
    /// @details stdTraceBegin increments this after printing the enter line.
    /// stdTraceEnd decrements it before printing the exit line. An Evaluator
    /// snapshots the value on construction and restores it if that list
    /// finishes with an error, so a skipped TRACE exit cannot leak indent.
    int traceDepth() const { return traceDepth_; }

    /// @brief Enter a traced call after its begin line has been printed.
    void incrementTraceDepth() { ++traceDepth_; }

    /// @brief Leave a traced call before its exit line is printed.
    void decrementTraceDepth();

    /// @brief Set TRACE indent back to a saved value.
    /// @param depth A snapshot taken when an Evaluator started. Must be between
    /// 0 and the current depth, inclusive.
    void restoreTraceDepth(int depth);

    /// @brief Repcount is for use in looping functions (e.g. REPEAT)
    double repcount_ = -1;

    /// @brief Explicit-slot lists for `?` (APPLY / MAP / FOREACH).
    /// @note APPLY pushes a list and pops it when the template returns.
    QStack<DatumPtr> explicitSlotListStack_;

    /// @brief Saved variable bindings for nested lambda APPLY.
    /// @note beginApplyLambda pushes; endApplyLambda pops and restores.
    QStack<ApplyLambdaBinding> applyLambdaBindingStack_;

    /// @brief The current explicit-slot list, or nothing if the stack is empty.
    DatumPtr explicitSlotList() const;

    /// @brief Set the test state for TEST/IFTRUE/IFFALSE.
    /// @note Sets the test state for the current call frame.
    /// @param isTrue The value to set the test state to.
    void setTest(bool isTrue);

    /// @brief Get the test state for TEST/IFTRUE/IFFALSE.
    /// @note Searches the call frame stack for the first non-zero test state.
    /// @note 0 = not tested, 3 = tested true, 2 = tested false.
    /// @return The test state for the highest non-zero test state in the call frame stack.
    int8_t testedState() const;

    /************ ECE ************/

    /// @brief The next operation to perform.
    void (Kernel::*nextOperation_)() = nullptr;

    /// @brief The source list that returned the return value.
    DatumPtr retvalSourceList_;

    /// @brief Begin a procedure.
    /// @param node The ASTNode that holds the command, the procedure name and parameters.
    /// @param paramAry The parameters to apply to the procedure.
    /// @param paramCount The number of parameters to the procedure.
    void beginProcedure(ASTNode *node, Datum **paramAry, uint32_t paramCount);

    /// @brief Get the current call frame.
    CallFrame *currentCallFrame();

    /// @brief Get the topmost evaluator from the evaluation stack.
    Evaluator *currentEvaluator();

    /// @brief Begin or continue evaluating a list. Will return when execution is suspended.
    void ece_evaluateStack();

    /// @brief Chain the current list being evaluated to the next list.
    void ece_chainList();

    /// @brief Decide what to do next after emptying the evaluation stack.
    void ece_decideEmptyEvaluationStack();

    /// @brief Begin or continue processing procedure arguments.
    void ece_processArguments();

    /// @brief Pop the topmost evaluator from the evaluation stack.
    void ece_popEvaluator();

    /// Begin the procedure.
    void ece_beginProcedure();

    /// @brief Exit the current procedure.
    void ece_exitProcedure();

    /// @brief Pop a macro frame and run its output list in the caller's context.
    /// @param value The macro's output. Must be a list of instructions, otherwise an error.
    void ece_expandMacroReturn(const DatumPtr &value);

    /// @brief Resume the caller with @p result as the value of the expression it is waiting on.
    void ece_resumeCaller(const DatumPtr &result);

    /// @brief Process the parameters and arguments for the current procedure.
    void ece_processParameters();

    /************ miscellaneous ************/

    /// @brief The palette of colors.
    /// @details The first 16 colors [0-15] are the standard Logo colors. The first 8
    /// are immutable. The rest [8-100] are user-assignable.
    QVector<QColor> palette_;

    /// @brief The current error, if any.
    DatumPtr currentError_;

    /// @brief The collection of property lists.
    PropertyLists plists_;

  private:
    /// @brief Nested TRACE indent. See traceDepth().
    int traceDepth_ = 0;

    /// @brief The nine name sets, indexed by NameMark then NameKind.
    /// @details A name may be in more than one set.
    QList<QSet<QString>> nameSets_ =
        QList<QSet<QString>>(static_cast<qsizetype>(NameMark::Count) * static_cast<qsizetype>(NameKind::Count));

    /// @brief The set selected by @p mark and @p kind.
    QSet<QString> &namesFor(NameMark mark, NameKind kind);
    const QSet<QString> &namesFor(NameMark mark, NameKind kind) const;

    /// @brief SETEDITOR command: a word, or a list of words.
    DatumPtr editorCommand_;

    /// @brief SETTEMPLOC directory. Empty means QDir::tempPath().
    QString tempLoc_;

    /// @brief Absolute path of the current EDIT temp file, or empty.
    QString editorTempPath_;

    /// @brief True after THROW "SYSTEM so the EDIT temp file is left on disk.
    bool keepEditorTemp_ = false;

    /// @brief Filename of the most recent LOAD or SAVE command, or nothing().
    DatumPtr lastWorkspaceFile_;

    /// @brief The directory EDIT uses for its temporary file.
    QString resolvedTempLoc() const;

    /// @brief Write @p text to a new temp file in resolvedTempLoc().
    QString createEditorTempFile(const QString &text);

    /// @brief Open the editor on @p path. Psychi uses the built-in editor.
    /// @return False if the user reverted (Psychi); true to LOAD the file.
    bool runEditorOnFile(const QString &path, const QString &startText, const QString &title);

    /// @brief LOAD @p path, then delete it unless @p deleteAfter is false or
    /// the load was THROW "SYSTEM.
    TopLevelResult loadEditedFile(const QString &path, bool deleteAfter);

  public:
    /// @brief READ a line of input, EVALUATE it, PRINT the result, LOOP.
    /// @param isPausing Whether we are in a PAUSE loop.
    /// @param prompt The prompt to display to the user.
    /// @return The result of the last expression entered.
    /// @note The return value is useful only in the case of PAUSE.
    DatumPtr readEvalPrintLoop(bool isPausing, const QString &prompt = QString());

    /// @brief Execute instructions from the current commander until they stop.
    /// @return Why the loop stopped.
    /// @note The caller must already have redirected the commander.
    TopLevelResult runProgram();

    /// @brief Open @p filename as the commander, run it, then run STARTUP if
    /// this file assigned that variable a list.
    /// @param filename A word naming the file. PREFIX applies to relative names.
    /// @return Why the load stopped.
    /// @note Shared by LOAD, EDIT, EDITFILE, and Kernel::runFile. Failure to open the
    /// file is thrown. Errors inside the file are printed and returned as
    /// StopThenPrompt.
    TopLevelResult loadFile(const DatumPtr &filename);

    /// @brief SAVE: write unburied procedures, variables, and property lists
    /// to @p filename.
    /// @param filename A word naming the file. PREFIX applies to relative names.
    /// @details Same text as PRINTOUT of CONTENTS. Does not change WRITER. A
    /// name already in ALLOPEN is already-open. Remembers @p filename for a
    /// later SAVE with no input.
    void saveWorkspace(const DatumPtr &filename);

    /// @brief Remember @p filename as the most recent LOAD or SAVE name.
    /// @details SAVE with no input uses this. EDIT / EDITFILE / library
    /// autoload / Kernel::runFile do not update it.
    void rememberWorkspaceFile(const DatumPtr &filename);

    /// @brief The filename of the most recent LOAD or SAVE, or nothing().
    DatumPtr lastWorkspaceFile() const;

    /// @brief Run @p text as the commander, then run STARTUP if this text
    /// assigned that variable a list.
    /// @param text Logo source (library CODE).
    /// @return Why the load stopped.
    /// @note Same TO / LOADNOISILY / STARTUP rules as loadFile. Used by
    /// library autoload. EDIT writes a temp file and calls loadFile.
    TopLevelResult loadText(const QString &text);

    /// @brief SETEDITOR: the shell command or program used by EDIT / EDITFILE.
    /// @param command A word, or a non-empty list of words. The filename is
    /// appended when the editor is launched.
    void setEditor(const DatumPtr &command);

    /// @brief EDITOR: the current SETEDITOR command.
    DatumPtr editor() const;

    /// @brief SETTEMPLOC: directory for EDIT's temporary file.
    /// @param path A word naming a directory, or the empty list for the
    /// system temp directory.
    void setTempLoc(const DatumPtr &path);

    /// @brief TEMPLOC: the directory EDIT uses for its temporary file.
    DatumPtr tempLoc() const;

    /// @brief EDIT: write the named workspace items to a temp file, open the
    /// editor, LOAD the file, then delete it (unless THROW "SYSTEM).
    /// @param contentslist A contents list from contentsListFromDatum, or
    /// nothing() to edit unburied procedures.
    TopLevelResult editContents(const DatumPtr &contentslist);

    /// @brief EDITFILE: open @p filename in the editor, then LOAD it.
    /// @param filename A word. PREFIX applies to relative names. A missing
    /// file is created empty. The file is not deleted.
    TopLevelResult editFile(const DatumPtr &filename);

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

    /// @brief Perform pause, essentially a REPL loop.
    /// @return The value passed to CONTINUE, if any. An uncaught TOPLEVEL or
    /// SYSTEM throw is returned as an FCError so the caller can unwind.
    /// @note If ERRACT armed an error message, it is printed before the pause prompt.
    DatumPtr pause();

    /// @brief Run the ERRACT instruction in the current dynamic scope.
    /// @param instruction The value of ERRACT. A word is runparsed; a list is run as-is.
    /// @return The instruction's output, an error, or nothing if it did not output.
    /// @details ERRACT is locally unbound for the duration of the instruction, then restored.
    /// Local variables of the procedure that raised the error remain visible.
    DatumPtr runErractInstruction(const DatumPtr &instruction);

    /// @brief Remember @p code so a second identical error without user input is an ERRACT loop.
    /// @param code The ErrCode of the error ERRACT is about to handle.
    void armErractLoop(int code);

    /// @brief True if @p code repeats the error ERRACT just handled, with no typed line in between.
    /// @param code The ErrCode of the error about to be handled.
    /// @return True if running ERRACT again would repeat the same error without user input.
    bool isErractLoop(int code) const;

    /// @brief Allow the next error to run ERRACT.
    /// @note Also called when a line is read from the terminal, which is the user-input
    /// reset used so a bad CONTINUE can try again.
    void clearErractLoop();

    /// @brief Drop the datum remembered by noteErractReturn().
    /// @note Call this before that datum can be freed.
    void clearErractReturnGuard();

    /// @brief Remember a value ERRACT just output, so an immediate rejection is an ERRACT loop.
    /// @param value The output. Must stay alive until clearErractReturnGuard().
    /// @param fromUser True if PAUSE ran while producing @p value.
    void noteErractReturn(Datum *value, bool fromUser);

    /// @brief True if @p value is the output ERRACT just supplied and the user did not type it.
    /// @param value The input a primitive is rejecting.
    /// @return True when accepting @p value would repeat the same ERRACT recovery.
    bool isErractReturnRetry(const DatumPtr &value) const;

    /// @brief How many times PAUSE has been entered.
    /// @return The pause entry count.
    uint32_t pauseCount() const { return pauseCount_; }

    /// @brief Ask the next PAUSE to print @p message before its prompt.
    /// @param message The error text to print.
    void armErractMessage(const QString &message);

    /// @brief Drop a pending ERRACT message.
    /// @return True if PAUSE has not printed the message yet.
    bool takeErractMessagePending();

    /// @brief Consume a pending terminal signal on the interpreter thread.
    /// @param resultOut Receives the FCError for TOPLEVEL/SYSTEM, or the return
    /// value of pause() for PAUSE. Unchanged meaning: nothing() if no signal.
    /// @return True if a signal was pending and consumed.
    /// @note Call this only from the interpreter thread. The signal handler
    /// never calls it.
    bool consumePendingSignal(DatumPtr *resultOut);

    /// @brief Runs the main loop of the QLogo interpreter.
    /// @return The exit code of the application.
    /// @note This method initializes the interface, sets up signal handlers,
    /// runs ~/startup.lg if that file exists, optionally runs a command-line
    /// script, runs the read-eval-print loop, and at termination restores
    /// signal handlers.
    int run();
};

#endif // KERNEL_H
