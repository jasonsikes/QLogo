#ifndef PROCEDURES_H
#define PROCEDURES_H

//===-- qlogo/procedures.h - Procedures class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Procedures class, which is responsible
/// for organizing all procedures in QLogo: primitives, user-defined, and library.
///
//===----------------------------------------------------------------------===//

#include "compiler_types.h"
#include "datum_ptr.h"
#include "workspace/library.h"
#include <QHash>

/// @brief A structure to hold a command's details for the treeifyer.
/// @note This is used to map a command name to its method, minimum, default, maximum
/// parameter counts, and return data type.
struct Cmd_t
{
    /// @brief The compiler method to generate code for this command.
    Generator method_;

    /// @brief The minimum number of parameters this command expects.
    /// @note -1 means no minimum number of tokens, all tokens are parsed without expression parsing, "raw" tokens.
    int countOfMinParams_;

    /// @brief The number of default parameters this command expects.
    /// @note -1 means special form, read until EOL.
    int countOfDefaultParams_;

    /// @brief The maximum number of parameters this command expects.
    /// @note -1 means unlimited.
    int countOfMaxParams_;

    /// @brief The data type(s) that this procedure is expected to return.
    RequestReturnType returnType_;
};

/// @brief The procedures class.
/// @note This is the main class for managing procedures in QLogo. It holds all
/// user-defined and library procedures.
class Procedures
{
    QHash<QString, Cmd_t> stringToCmd;

    QHash<QString, DatumPtr> procedures_;
    qint64 lastProcedureCreatedTimestamp_;

    DatumPtr procedureForName(const QString &aName) const;
    bool isNamedProcedure(const QString &aName) const;


    /// @brief Private constructor for singleton pattern.
    Procedures();

    Procedures(const Procedures &) = delete;
    Procedures(Procedures &&) = delete;
    Procedures &operator=(const Procedures &) = delete;
    Procedures &operator=(Procedures &&) = delete;

  public:
    /// @brief Get the singleton instance of the Procedures class.
    /// @return The singleton instance of the Procedures class.
    static Procedures &get()
    {
        static Procedures instance;
        return instance;
    }

    /// @brief Destructor.
    ~Procedures() = default;

    /// @brief Return the timestamp of the last procedure creation.
    /// @return The timestamp of the last procedure creation.
    qint64 timeOfLastProcedureCreation() const
    {
        return lastProcedureCreatedTimestamp_;
    }

    /// @brief Validate a procedure arguments list.
    /// @param cmd The command to validate the arguments list for.
    /// @param argumentsList The arguments list to validate.
    /// @return A tuple representing the arity of the procedure. Throws an exception if the arguments list is invalid.
    std::tuple<int, int, int> validateArguments(const DatumPtr &cmd, const DatumPtr &argumentsList) const;

    /// @brief Create a Procedure object and save it to the procedures hash table.
    /// @param cmd The command used to define the procedure (TO or .MACRO).
    /// @param procnameP The name of the procedure to define.
    /// @param text The text to define a procedure from, in the form of a list of sublists.
    /// @param sourceText The source text to define a procedure from, or an empty list if there
    /// was no source text.
    void defineProcedure(const DatumPtr &cmd,
                         const DatumPtr &procnameP,
                         const DatumPtr &text,
                         const QList<DatumPtr> &sourceText);

    /// @brief Copy a procedure to a new name.
    /// @param newnameP The new name to copy the procedure to.
    /// @param oldnameP The name of the procedure to copy.
    void copyProcedure(const DatumPtr &newnameP, const DatumPtr &oldnameP);

    /// @brief Erase a procedure.
    /// @param procnameP The name of the procedure to erase.
    void eraseProcedure(const DatumPtr &procnameP);

    /// @brief Get an AST node from a procedure.
    /// @param cmdP The name of the procedure to search for.
    /// @return A tuple containing a pointer to the created AST node, and three integers representing the arity of the procedure.
    ///         If the procedure is not found, returns a tuple with nothing() as the first element.
    std::tuple<DatumPtr, int, int, int> astnodeFromProcedure(const DatumPtr &cmdP) const;

    /// @brief Get an AST node from a primitive command.
    /// @param cmdP The name of the command to search for.
    /// @return A tuple containing a pointer to the created AST node, and three integers representing the arity of the command.
    ///         If the command is not found, returns a tuple with nothing() as the first element.
    std::tuple<DatumPtr, int, int, int> astnodeFromPrimitive(const DatumPtr &cmdP) const;

    /// @brief Get an AST node from a command, either a primitive or user-defined procedure.
    /// @param command The name of the command to search for.
    /// @return A tuple containing a pointer to the created AST node, and three integers representing the arity of the command.
    std::tuple<DatumPtr, int, int, int> astnodeFromCommand(const DatumPtr &command) const;

    /// @brief Get the text of a procedure.
    /// @param procnameP The name of the procedure to get the text of.
    /// @return A pointer to the text of the procedure, in the form of a list of sublists.
    DatumPtr procedureText(const DatumPtr &procnameP) const;

    /// @brief Get the full text of a procedure.
    /// @param procnameP The name of the procedure to get the full text of.
    /// @param shouldValidate Whether to validate the procedure.
    /// @return A list of the full text of the procedure, in the form of a list of
    /// words.
    DatumPtr procedureFulltext(const DatumPtr &procnameP, bool shouldValidate = true) const;

    /// @brief Generate the title line of a procedure.
    /// @param procnameP The name of the procedure to generate the title line for.
    /// @return A string containing the title of the procedure. A title is the first line
    /// of the procedure's source text, starting with 'to' or '.macro'.
    QString generateProcedureTitleLine(const DatumPtr &procnameP) const;

    /// @brief Check if a name is a procedure.
    /// @param procname The name to check.
    /// @return True if the name is a procedure, false otherwise.
    bool isProcedure(const QString &procname) const;

    /// @brief Check if a name is a macro.
    /// @param procname The name to check.
    /// @return True if the name is a macro, false otherwise.
    bool isMacro(const QString &procname) const;

    /// @brief Check if a name is a primitive.
    /// @param procname The name to check.
    /// @return True if the name is a primitive, false otherwise.
    bool isPrimitive(const QString &procname) const;

    /// @brief Check if a name is defined.
    /// @param procname The name to check.
    /// @return True if the name is defined, false otherwise.
    /// @note This checks both user-defined and primitive procedures.
    bool isDefined(const QString &procname) const;

    /// @brief Get all procedure names.
    /// @return A pointer to a list of all procedure names.
    DatumPtr allProcedureNames() const;

    /// @brief Get all primitive procedure names.
    /// @return A pointer to a list of all primitive procedure names.
    DatumPtr allPrimitiveProcedureNames() const;

    /// @brief Get the arity of a procedure.
    /// @param nameP The name of the procedure to get the arity of.
    /// @return A pointer to the arity of the procedure, in the form of a list of three
    /// integers: the minimum, default, and maximum number of parameters.
    DatumPtr arity(const DatumPtr &nameP) const;

    /// @brief Create an AST node from a command and its parameters.
    /// @param cmd The command to create an AST node from.
    /// @param params The parameters to create an AST node from.
    /// @return A pointer to the created AST node.
    /// @note This creates an AST node from a command and its parameters, in a form suitable
    /// for use in the APPLY command.
    DatumPtr astnodeWithLiterals(const DatumPtr &cmd, const DatumPtr &params);
};

/// @brief The procedure class.
/// @note This class maintains the details of a procedure, including its arity,
/// parameters, instruction list, and source text.
class Procedure : public Datum
{

  public:
    /// @brief Constructor.
    Procedure()
    {
        isa_ = typeProcedure;
    }

    /// @brief The minimum number of parameters this procedure accepts.
    int countOfMinParams_ = 0;
    /// @brief The number of default parameters this procedure expects.
    int countOfDefaultParams_ = 0;
    /// @brief The maximum number of parameters this procedure accepts.
    int countOfMaxParams_ = -1;

    /// @brief A mapping of tag names to source lines and block IDs.
    QHash<QString, std::pair<DatumPtr, int32_t>> tagToLineAndBlockId_;

    /// @brief Whether this procedure is a macro.
    bool isMacro_ = false;

    /// @brief The source text of the procedure.
    /// @note This is a list of sublists, with each sublist representing a line of the
    /// source text. The source text begins with the word 'TO' or '.MACRO' and ends with
    /// the word 'END'.
    // TODO: Should this be a list of words, since each line is a word?
    QList<DatumPtr> sourceText_;

    /// @brief The instruction list of the procedure.
    /// @note This is a list of lists.
    /// The first list is the arguments list, and the rest are the instruction lists.
    /// TODO This should be a deep copy of the source lists, to prevent direct modification.
    DatumPtr instructionList_;
};

#endif // PROCEDURES_H
