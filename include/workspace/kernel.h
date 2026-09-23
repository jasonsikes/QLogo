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
#include <QColor>
#include <QList>
#include <QSet>
#include <QStack>
#include <deque>
#include <QVector>
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
    bool isPausing_;

    void initPalette();

    /// @brief Run the explicit control evaluator.
    /// @param listP The list to run.
    /// @return The result of the last expression in the list.
    DatumPtr runECE();

    /// Initialize LOGO system variables
    void initVariables();

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
    void(Kernel::*nextOperation_)() = nullptr;

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
    /// @brief The nine name sets, indexed by NameMark then NameKind.
    /// @details A name may be in more than one set.
    QList<QSet<QString>> nameSets_ = QList<QSet<QString>>(
        static_cast<qsizetype>(NameMark::Count) * static_cast<qsizetype>(NameKind::Count));

    /// @brief The set selected by @p mark and @p kind.
    QSet<QString> &namesFor(NameMark mark, NameKind kind);
    const QSet<QString> &namesFor(NameMark mark, NameKind kind) const;

  public:
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
    /// @return The value passed to CONTINUE, if any.
    DatumPtr pause();

    /// @brief Runs the main loop of the QLogo interpreter.
    /// @return The exit code of the application.
    /// @note This method initializes the interface, sets up signal handlers,
    /// runs the read-eval-print loop, and at termination restores signal handlers.
    int run();
};

#endif // KERNEL_H
