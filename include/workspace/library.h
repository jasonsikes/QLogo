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
    mutable bool connectionIsValid_ = false;
    const QString connectionName_;
    const QString paramFilePath_;
    const QString defaultFilePath_;

    /// @brief Constructor for DatabaseConnection.
    /// @param aConnectionName The name of the database connection.
    /// @param paramFilePath The path to the database file, may be empty.
    /// @param defaultFilePath The path to search for the database file if paramFilePath is empty.
    explicit DatabaseConnection(const QString &aConnectionName, const QString &paramFilePath, const QString &defaultFilePath)
        : connectionName_(aConnectionName), paramFilePath_(paramFilePath), defaultFilePath_(defaultFilePath)
    {
    }

    /// @brief Validate that the database has the correct schema.
    /// @param tables The list of tables in the database.
    /// @return True if the schema is valid, false otherwise.
    virtual bool validateSchema(const QStringList &tables) const = 0;

    /// @brief Initialize and validate the database connection.
    void getConnection() const;

  public:
    /// @brief Close the named Qt SQL connection while QCoreApplication is alive.
    void closeConnection() const;

    virtual ~DatabaseConnection();

    DatabaseConnection(const DatabaseConnection &) = delete;
    DatabaseConnection &operator=(const DatabaseConnection &) = delete;
};

/// @brief The Library class provides access to the QLogo standard library.
class Library : public DatabaseConnection
{
    /// @brief Cached COMMAND keys from LIBRARY. Empty if the DB is missing or invalid.
    /// @note Populated once; the table is read-only. Names are uppercase (Key form).
    mutable QStringList allProcedures_;
    mutable bool namesLoaded_ = false;

    bool validateSchema(const QStringList &tables) const override;

    /// @brief Private constructor - Singleton
    Library();

  public:
    /// @brief Get the singleton instance of the Library class.
    /// @return Reference to the singleton Library instance.
    static Library &get();

    Library(const Library &) = delete;
    Library(Library *) = delete;
    Library(Library &&) = delete;
    Library &operator=(const Library &) = delete;
    Library &operator=(Library *) = delete;
    Library &operator=(Library &&) = delete;

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

  public:
    /// @brief Get the singleton instance of the Help class.
    /// @return Reference to the singleton Help instance.
    static Help &get();

    Help(const Help &) = delete;
    Help(Help *) = delete;
    Help(Help &&) = delete;
    Help &operator=(const Help &) = delete;
    Help &operator=(Help *) = delete;
    Help &operator=(Help &&) = delete;

    /// @brief Destructor for the Help class.
    ~Help() override = default;

    /// @brief Return a list of command names for the given category.
    /// @param category The help category to list commands for.
    /// @return A list of command names in the given category.
    QStringList commandsForCategory(const QString &category);

    /// @brief Return a list of all help categories.
    /// @return A list of all help categories.
    QStringList allCategories();

    /// @brief Return the help text for a command.
    /// @param cmdName The name of the command to return the help text for.
    /// @return The help text for the command.
    QString helpText(const QString &cmdName);
};

/// @brief Set Config::cslsDirectory_ from the command line, CSLS, or defaults.
/// @details Uses --setcslsloc if given, else the CSLS environment variable, else
/// the CSLS directory next to the library and help databases (Linux
/// share/qlogo/csls, macOS Resources/csls, Windows next to the binary).
void initCslsDirectory();

#endif // LIBRARY_H
