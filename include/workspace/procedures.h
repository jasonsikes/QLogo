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

#include "datum_ptr.h"
#include "library.h"
#include "compiler_types.h"
#include <QHash>

/// @brief A structure to hold a command's details for the parser.
/// @note This is used to map a command name to its method, minimum, default, maximum
/// parameter counts, and return data type.
struct Cmd_t
{
    /// @brief The compiler method to call for this command.
    Generator method;

    /// @brief The minimum number of parameters this command expects.
    int countOfMinParams;

    /// @brief The number of default parameters this command expects.
    int countOfDefaultParams;

    /// @brief The maximum number of parameters this command expects.
    int countOfMaxParams;

    /// @brief The data type(s) that this procedure is expected to return.
    RequestReturnType returnType;
};


/// @brief The procedures class.
/// @note This is the main class for managing procedures in QLogo. It holds all
/// user-defined and library procedures.
class Procedures
{
    QHash<QString, Cmd_t> stringToCmd;

    QHash<QString, DatumPtr> procedures;
    qint64 lastProcedureCreatedTimestamp;

    DatumPtr procedureForName(QString aName) const;
    bool isNamedProcedure(QString aName) const;

    Library stdLib;

  public:

    /// @brief Constructor.
    Procedures();

    /// @brief Destructor.
    ~Procedures();

    /// @brief Return the timestamp of the last procedure creation.
    /// @return The timestamp of the last procedure creation.
    qint64 timeOfLastProcedureCreation() const
    {
        return lastProcedureCreatedTimestamp;
    }

    /// @brief Create an AST from a list.
    /// @param aList The list to create an AST from.
    /// @return A pointer to the created ASTList.
    /// @note A list can contain several commands, so this returns a list of ASTNode roots,
    /// with each root representing a command.
    QList<DatumPtr> *astFromList(List *aList);

    /// @brief Create a procedure from a command and its text.
    /// @param cmd The name of the command.
    /// @param text The text to create a procedure from, in the form of a list of sublists.
    /// @param sourceText The source text to create a procedure from, or 'nothing' if there
    /// was no source text.
    /// @return A pointer to the created procedure.
    /// @note This creates and returns a Procedure object from a command and its text. It
    /// does not save the procedure to the procedures hash table.
    DatumPtr createProcedure(DatumPtr cmd, DatumPtr text, DatumPtr sourceText);

    /// @brief Define a procedure.
    /// @param cmd The command used to define the procedure (TO or .MACRO).
    /// @param procnameP The name of the procedure to define.
    /// @param text The text to define a procedure from, in the form of a list of sublists.
    /// @param sourceText The source text to define a procedure from, or 'nothing' if there
    /// was no source text.
    /// @note This creates a Procedure object and saves it to the procedures hash table.
    void defineProcedure(DatumPtr cmd, DatumPtr procnameP, DatumPtr text, DatumPtr sourceText);

    /// @brief Copy a procedure to a new name.
    /// @param newnameP The new name to copy the procedure to.
    /// @param oldnameP The name of the procedure to copy.
    void copyProcedure(DatumPtr newnameP, DatumPtr oldnameP);

    /// @brief Erase a procedure.
    /// @param procnameP The name of the procedure to erase.
    void eraseProcedure(DatumPtr procnameP);

    /// @brief Get an AST node from a procedure.
    /// @param cmdP The name of the procedure to search for.
    /// @param minParams The minimum number of parameters this procedure expects.
    /// @param defaultParams The number of default parameters this procedure expects.
    /// @param maxParams The maximum number of parameters this procedure expects.
    /// @return A pointer to the created AST node.
    DatumPtr astnodeFromProcedure(DatumPtr cmdP, int &minParams, int &defaultParams, int &maxParams);

    /// @brief Get an AST node from a primitive command.
    /// @param cmdP The name of the command to search for.
    /// @param minParams The minimum number of parameters this command expects.
    /// @param defaultParams The number of default parameters this command expects.
    /// @param maxParams The maximum number of parameters this command expects.
    /// @return A pointer to the created AST node.
    /// @note The primitive name should be tested to be in the stringToCmd hash table.
    DatumPtr astnodeFromPrimitive(DatumPtr cmdP, int &minParams, int &defaultParams, int &maxParams);

    /// @brief Get an AST node from a command, either a primitive or user-defined procedure.
    /// @param command The name of the command to search for.
    /// @param minParams The minimum number of parameters this command expects.
    /// @param defaultParams The number of default parameters this command expects.
    /// @param maxParams The maximum number of parameters this command expects.
    DatumPtr astnodeFromCommand(DatumPtr command, int &minParams, int &defaultParams, int &maxParams);

    /// @brief Get the text of a procedure.
    /// @param procnameP The name of the procedure to get the text of.
    /// @return A pointer to the text of the procedure, in the form of a list of sublists.
    DatumPtr procedureText(DatumPtr procnameP) const;

    /// @brief Get the full text of a procedure.
    /// @param procnameP The name of the procedure to get the full text of.
    /// @param shouldValidate Whether to validate the procedure.
    /// @return A pointer to the full text of the procedure, in the form of a list of
    /// sublists.
    DatumPtr procedureFulltext(DatumPtr procnameP, bool shouldValidate = true) const;

    /// @brief Get the title of a procedure.
    /// @param procnameP The name of the procedure to get the title of.
    /// @return A string containing the title of the procedure. A title is the first line
    /// of the procedure's source text, starting with 'TO' or '>MACRO'.
    QString procedureTitle(DatumPtr procnameP) const;

    /// @brief Check if a name is a procedure.
    /// @param procname The name to check.
    /// @return True if the name is a procedure, false otherwise.
    bool isProcedure(QString procname) const;

    /// @brief Check if a name is a macro.
    /// @param procname The name to check.
    /// @return True if the name is a macro, false otherwise.
    bool isMacro(QString procname) const;

    /// @brief Check if a name is a primitive.
    /// @param procname The name to check.
    /// @return True if the name is a primitive, false otherwise.
    bool isPrimitive(QString procname) const;

    /// @brief Check if a name is defined.
    /// @param procname The name to check.
    /// @return True if the name is defined, false otherwise.
    /// @note This checks both user-defined and primitive procedures.
    bool isDefined(QString procname) const;

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
    DatumPtr arity(DatumPtr nameP) const;

    /// @brief Create an AST node from a command and its parameters.
    /// @param cmd The command to create an AST node from.
    /// @param params The parameters to create an AST node from.
    /// @return A pointer to the created AST node.
    /// @note This creates an AST node from a command and its parameters, in a form suitable
    /// for use in the APPLY command.
    DatumPtr astnodeWithLiterals(DatumPtr cmd, DatumPtr params);
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
        isa = typeProcedure;
    }

    /// @brief The parameter names of the required inputs of the procedure.
    QStringList requiredInputs;

    /// @brief The parameter names of the optional inputs of the procedure.
    QStringList optionalInputs;

    /// @brief The default values of the optional inputs of the procedure.
    QList<DatumPtr> optionalDefaults;

    /// @brief The parameter name for the rest of the inputs.
    QString restInput;

    /// @brief The minimum number of parameters this procedure accepts.
    int countOfMinParams = 0;
    /// @brief The number of default parameters this procedure expects.
    int countOfDefaultParams = 0;
    /// @brief The maximum number of parameters this procedure accepts  .
    int countOfMaxParams = -1;

    /// @brief A hash table to map tag names to the lines in the source text.
    QHash<const QString, DatumPtr> tagToLine;

    /// @brief A hash table to map tag names to the block ID for efficient execution.
    QHash<const QString, int32_t> tagToBlockId;

    /// @brief Whether this procedure is a macro.
    bool isMacro = false;

    /// @brief The source text of the procedure.
    /// @note This is a list of sublists, with each sublist representing a line of the
    /// source text. The source text begins with the word 'TO' or '.MACRO' and ends with
    /// the word 'END'.
    // TODO: Should this be a list of words, since each line is a word?
    DatumPtr sourceText;

    /// @brief The instruction list of the procedure.
    /// @note This is a list of lists, with each sublist representing a line of instruction.
    /// @TODO This should be a deep copy of the source lists, to prevent direct modification.
    DatumPtr instructionList;

};

#endif // PROCEDURES_H
