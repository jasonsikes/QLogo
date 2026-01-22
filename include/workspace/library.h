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

#include <QSqlDatabase>
#include <QStringList>

/// @brief Base class for database connection management.
///
/// This class provides common functionality for managing SQLite database connections,
/// including connection initialization, validation, and cleanup.
class DatabaseConnection
{
  protected:
    mutable bool connectionIsValid = false;
    const QString connectionName;
    const QString paramFilePath;
    const QString defaultFilePath;

    /// @brief Constructor for DatabaseConnection.
    /// @param aConnectionName The name of the database connection.
    /// @param paramFilePath The path to the database file, may be empty.
    /// @param defaultFilePath The path to search for the database file if paramFilePath is empty.
    explicit DatabaseConnection(const QString &aConnectionName, const QString &paramFilePath, const QString &defaultFilePath)
        : connectionName(aConnectionName), paramFilePath(paramFilePath), defaultFilePath(defaultFilePath)
    {
    }

    /// @brief Validate that the database has the correct schema.
    /// @param tables The list of tables in the database.
    /// @return True if the schema is valid, false otherwise.
    virtual bool validateSchema(const QStringList &tables) const = 0;

    /// @brief Initialize and validate the database connection.
    void getConnection() const;

  public:
    virtual ~DatabaseConnection();

    DatabaseConnection(const DatabaseConnection &) = delete;
    DatabaseConnection &operator=(const DatabaseConnection &) = delete;
};

/// @brief The Library class provides access to the QLogo standard library.
class Library : public DatabaseConnection
{
    // Commented out because it is never populated. Restore it when we get it working again.
    // QStringList allProcedures;

    bool validateSchema(const QStringList &tables) const override;

    /// @brief Private constructor - Singleton
    Library();
    Library(const Library &) = delete;
    Library &operator=(const Library &) = delete;

  public:
    /// @brief Get the singleton instance of the Library class.
    /// @return Reference to the singleton Library instance.
    static Library &get();

    /// @brief Destructor for the Library class.
    ~Library() override = default;

    /// @brief Return the text of library procedure of the given name.
    /// @param cmdName The name of the procedure to return the text of.
    /// @return Null string if no procedure found.
    QString procedureText(const QString &cmdName);

    /// @brief Return a list of all procedure names available in the library.
    /// @return A list of all procedure names available in the library.
    QStringList allProcedureNames() const;
};

/// @brief The Help class provides access to help text for QLogo commands.
class Help : public DatabaseConnection
{
    bool validateSchema(const QStringList &tables) const override;

    /// @brief Private constructor - Singleton
    Help();
    Help(const Help &) = delete;
    Help &operator=(const Help &) = delete;

  public:
    /// @brief Get the singleton instance of the Help class.
    /// @return Reference to the singleton Help instance.
    static Help &get();

    /// @brief Destructor for the Help class.
    ~Help() override = default;

    /// @brief Return a list of all command names that have a help text entry.
    /// @return A list of all command names that have a help text entry.
    QStringList allCommands();

    /// @brief Return the help text for a command.
    /// @param cmdName The name of the command to return the help text for.
    /// @return The help text for the command.
    QString helpText(const QString &cmdName);
};

#endif // LIBRARY_H
