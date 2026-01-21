
//===-- qlogo/library.cpp - Library text implementation -------*- C++ -*-===//
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

#include "workspace/library.h"
#include "flowcontrol.h"
#include "sharedconstants.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>

/// @brief Find the path to the database file.
/// @param defaultDBName The default name of the database file.
/// @returns The path to the database file, or an empty string if the file is not found.
/// @note this functions checks for the database file in the following locations:
/// - The share directory relative to wherever the app binary is. (Linux)
/// - The Resources directory relative to wherever the app binary is. (macOS)
/// - The same directory as the app binary. (Windows)
QString findDBPath(const QString &defaultDBName)
{
    // Build a list of candidate locations to try.
    QStringList candidates;
    QString appDir = QCoreApplication::applicationDirPath();

    // The share directory relative to wherever the app binary is. (Linux)
    candidates << QDir::cleanPath(appDir + "/../share/qlogo/" + defaultDBName);
    // The Resources directory relative to wherever the app binary is. (macOS)
    candidates << QDir::cleanPath(appDir + "/../Resources/" + defaultDBName);
    // The same directory as the app binary. (Windows)
    candidates << QDir::cleanPath(appDir + "/" + defaultDBName);

    for (auto &c : candidates)
    {
        if (QFileInfo::exists(c))
            return c;
    }

    return {};
}

/// @brief Initialize a database connection.
/// @param connectionName The name of the connection.
/// @param paramFilePath The path to the database file, may be null.
/// @param defaultFilePath The default name of the database file.
/// @returns True if the connection is successful, false otherwise.
/// @note If `paramFilePath` is empty, the function will search for the database
/// file using `findDBPath()` using `defaultFilePath` as the default name of the
/// database file.
bool initDBConnection(const QString &connectionName, const QString &paramFilePath, const QString &defaultFilePath)
{
    QString path = (!paramFilePath.isEmpty()) ? paramFilePath : findDBPath(defaultFilePath);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(path);
    bool isSuccessful = db.open();
    if (!isSuccessful)
    {
        qWarning() << "DB Error: " << db.lastError().text();
    }
    return isSuccessful;
}

Library::~Library()
{
    if (connectionIsValid)
        QSqlDatabase::removeDatabase(connectionName);
}

void Library::getConnection() const
{
    if (connectionIsValid)
        return;
    bool isOpen = initDBConnection(
        connectionName, Config::get().paramLibraryDatabaseFilepath, Config::get().defaultLibraryDbFilename);
    if (isOpen)
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        QStringList tables = db.tables();
        if (tables.contains("LIBRARY", Qt::CaseSensitive))
        {
            connectionIsValid = true;
        }
        else
        {
            qDebug() << "library db format is wrong";
        }
    }
}

QString Library::procedureText(const QString &cmdName)
{
    QString retval;

    getConnection();

    if (connectionIsValid)
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        QSqlQuery query(db);
        query.prepare("SELECT CODE FROM LIBRARY WHERE COMMAND = ?");
        query.addBindValue(cmdName);
        query.exec();
        if (query.next())
        {
            retval = query.value(0).toString();
        }
    }

    return retval;
}

QStringList Library::allProcedureNames() const
{
    // This is disabled to prevent confusion.
    // We shouldn't read procedure names if we can't yet input procedure bodies.

    // if (allProcedures.isEmpty())
    // {
    //     getConnection();

    //     if (connectionIsValid)
    //     {
    //         QSqlDatabase db = QSqlDatabase::database(connectionName);
    //         QSqlQuery query("SELECT COMMAND FROM LIBRARY", db);
    //         while (query.next())
    //         {
    //             allProcedures.append(query.value(0).toString());
    //         }
    //     }
    // }
    return {}; // Return allProcedures;
}

Help::~Help()
{
    if (connectionIsValid)
        QSqlDatabase::removeDatabase(connectionName);
}

void Help::getConnection() const
{
    if (connectionIsValid)
        return;
    bool isOpen =
        initDBConnection(connectionName, Config::get().paramHelpDatabaseFilepath, Config::get().defaultHelpDbFilename);
    if (isOpen)
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        QStringList tables = db.tables();
        if (tables.contains("ALIASES", Qt::CaseSensitive) && tables.contains("HELPTEXT", Qt::CaseSensitive))
        {
            connectionIsValid = true;
        }
        else
        {
            qDebug() << "help db format is wrong";
        }
    }
}

QStringList Help::allCommands()
{
    getConnection();
    QStringList retval;
    if (connectionIsValid)
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        QSqlQuery query("SELECT ALIAS FROM ALIASES", db);
        while (query.next())
        {
            retval.append(query.value(0).toString());
        }
    }
    return retval;
}

QString Help::helpText(const QString &name)
{
    QString retval;
    QString cmdName;

    getConnection();

    if (connectionIsValid)
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        // Every command has an alias
        // even if the alias is the same as the command.
        // Use the alias to get the command name.
        QSqlQuery query(db);
        query.prepare("SELECT COMMAND FROM ALIASES WHERE ALIAS = ?");
        query.addBindValue(name);
        query.exec();
        if ( ! query.next())
            return {};
        cmdName = query.value(0).toString();
        query.finish();

        // Get the help text for the command name.
        query.prepare("SELECT DESCRIPTION FROM HELPTEXT WHERE COMMAND = ?");
        query.addBindValue(cmdName);
        query.exec();
        if (query.next())
        {
            retval = query.value(0).toString();
        }
    }
    return retval;
}
