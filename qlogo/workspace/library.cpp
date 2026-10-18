
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
/// This file contains the implementation of the QLogo library interface, which
/// provides the standard library (supporting functions to the QLogo language),
/// and the help facility interface, which provides access to the help text for
/// QLogo library routines. Both classes are implemented using the SQLite
/// database interface.
///
//===----------------------------------------------------------------------===//

#include "workspace/library.h"
#include "sharedconstants.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>

/// @brief Find a data file or directory next to the library and help databases.
/// @param name The file or directory name to search for.
/// @returns The path if found, or an empty string.
/// @note Candidates, in order:
/// - The share directory relative to wherever the app binary is. (Linux)
/// - The Resources directory relative to wherever the app binary is. (macOS)
/// - The same directory as the app binary. (Windows)
QString findDataPath(const QString &name)
{
    QStringList candidates;
    QString appDir = QCoreApplication::applicationDirPath();

    candidates << QDir::cleanPath(appDir + "/../share/qlogo/" + name);
    candidates << QDir::cleanPath(appDir + "/../Resources/" + name);
    candidates << QDir::cleanPath(appDir + "/" + name);

    for (auto &c : candidates)
    {
        if (QFileInfo::exists(c))
            return c;
    }

    return {};
}

/// @brief Find the path to the database file.
/// @param defaultDBName The default name of the database file.
/// @returns The path to the database file, or an empty string if the file is not found.
QString findDBPath(const QString &defaultDBName)
{
    return findDataPath(defaultDBName);
}

void initCslsDirectory()
{
    Config &cfg = Config::get();
    if (!cfg.paramCslsDirectory_.isEmpty())
    {
        cfg.cslsDirectory_ = QDir::cleanPath(cfg.paramCslsDirectory_);
        return;
    }

    const QString env = qEnvironmentVariable("CSLS");
    if (!env.isEmpty())
    {
        cfg.cslsDirectory_ = QDir::cleanPath(env);
        return;
    }

    cfg.cslsDirectory_ = findDataPath(QString::fromLatin1(cfg.defaultCslsDirName_));
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

void DatabaseConnection::closeConnection() const
{
    if (!connectionIsValid_)
        return;

    if (QCoreApplication::instance() == nullptr)
    {
        connectionIsValid_ = false;
        return;
    }

    {
        QSqlDatabase db = QSqlDatabase::database(connectionName_);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(connectionName_);
    connectionIsValid_ = false;
}

DatabaseConnection::~DatabaseConnection()
{
    closeConnection();
}

void DatabaseConnection::getConnection() const
{
    if (connectionIsValid_)
    {
        return;
    }
    bool isOpen = initDBConnection(connectionName_, paramFilePath_, defaultFilePath_);
    if (isOpen)
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName_);
        QStringList tables = db.tables();
        if (validateSchema(tables))
        {
            connectionIsValid_ = true;
        }
        else
        {
            qWarning() << QString("Database connection '%1' has the wrong schema.").arg(connectionName_);
        }
    }
}

Library::Library()
    : DatabaseConnection("libDB", Config::get().paramLibraryDatabaseFilepath_, Config::get().defaultLibraryDbFilename_)
{
}

Library &Library::get()
{
    static Library instance;
    return instance;
}

bool Library::validateSchema(const QStringList &tables) const
{
    return tables.contains("LIBRARY", Qt::CaseSensitive);
}

QString Library::procedureText(const QString &cmdName)
{
    QString retval;

    getConnection();

    if (connectionIsValid_)
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName_);
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
    if (namesLoaded_)
        return allProcedures_;

    getConnection();

    if (connectionIsValid_)
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName_);
        QSqlQuery query(QStringLiteral("SELECT COMMAND FROM LIBRARY"), db);
        while (query.next())
        {
            allProcedures_.append(query.value(0).toString().toUpper());
        }
    }

    namesLoaded_ = true;
    return allProcedures_;
}

Help::Help()
    : DatabaseConnection("help", Config::get().paramHelpDatabaseFilepath_, Config::get().defaultHelpDbFilename_)
{
}

Help &Help::get()
{
    static Help instance;
    return instance;
}

bool Help::validateSchema(const QStringList &tables) const
{
    return tables.contains("ALIASES", Qt::CaseSensitive) && tables.contains("HELPTEXT", Qt::CaseSensitive);
}

QStringList Help::commandsForCategory(const QString &category)
{
    getConnection();
    QStringList retval;
    if (connectionIsValid_)
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName_);
        QSqlQuery query(db);
        query.prepare("SELECT ALIAS FROM ALIASES WHERE CATEGORY = ? ORDER BY ALIAS");
        query.addBindValue(category);
        query.exec();
        while (query.next())
        {
            retval.append(query.value(0).toString());
        }
    }
    return retval;
}

QStringList Help::allCategories()
{
    getConnection();
    QStringList retval;
    if (connectionIsValid_)
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName_);
        QSqlQuery query("SELECT DISTINCT CATEGORY FROM ALIASES ORDER BY CATEGORY", db);
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

    if (connectionIsValid_)
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName_);
        // Every command has an alias
        // even if the alias is the same as the command.
        // Use the alias to get the command name.
        QSqlQuery query(db);
        query.prepare("SELECT COMMAND FROM ALIASES WHERE ALIAS = ?");
        query.addBindValue(name);
        query.exec();
        if ( ! query.next())
        {
            return {};
        }
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
