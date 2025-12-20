#ifndef LIBRARY_H
#define LIBRARY_H

//===-- qlogo/library.h - Library text declaration -------*- C++ -*-===//
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
/// This file contains the declaration of the QLogo library interface, which
/// provides the standard library (supporting functions to the QLogo language),
/// and the help facility interface, which provides access to the help text for
/// QLogo library routines. Both classes are implemented using the SQLite
/// database interface.
///
//===----------------------------------------------------------------------===//

#include <QStringList>
#include <QSqlDatabase>

/// @brief The Library class provides access to the QLogo standard library.
class Library
{
    bool connectionIsValid = false;
    QStringList allProcedures;
    const QString connectionName = "libDB";

    void getConnection();

  public:

    /// @brief Constructor for the Library class.
    Library()
    {
    }

    /// @brief Destructor for the Library class.
    ~Library();

    /// @brief Return the text of library procedure of the given name.
    /// @param cmdName The name of the procedure to return the text of.
    /// @returns Null string if no procedure found.
    QString procedureText(const QString &cmdName);

    /// @brief Return a list of all procedure names available in the library.
    /// @returns A list of all procedure names available in the library.
    QStringList allProcedureNames() const;
};

class Help
{
    bool connectionIsValid = false;
    const QString connectionName = "help";

    void getConnection();

  public:

    /// @brief Constructor for the Help class.
    Help()
    {
    }

    /// @brief Destructor for the Help class.
    ~Help();

    /// @brief Return a list of all command names that have a help text entry.
    /// @returns A list of all command names that have a help text entry.
    QStringList allCommands();

    /// @brief Return the help text for a command.
    /// @param cmdName The name of the command to return the help text for.
    /// @returns The help text for the command.
    QString helpText(const QString &cmdName);
};

#endif // LIBRARY_H
