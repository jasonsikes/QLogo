
//===-- qlogo/library.cpp - Library text implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the QLogo library interface, which
/// provides the standard library (supporting functions to the QLogo language),
/// and the help facility. They are included here because they share use of the
/// QSqlDatabase.
///
//===----------------------------------------------------------------------===//

#include "library.h"
#include <QStringList>
#include <QCoreApplication>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include "error.h"
#include "sharedconstants.h"


QString findDBPath(QString defaultDBName)
{
    // Build a list of candidate locations to try.
    QStringList candidates;

    // The share directory relative to wherever the app binary is.
    candidates << QCoreApplication::applicationDirPath()
                      + QDir::separator() + ".."
                      + QDir::separator() + "share"
                      + QDir::separator() + "qlogo"
                      + QDir::separator() + defaultDBName;
    // The Resources directory relative to wherever the app binary is.
    candidates << QCoreApplication::applicationDirPath()
                      + QDir::separator() + ".."
                      + QDir::separator() + "Resources"
                      + QDir::separator() + defaultDBName;
    // The same directory as the app binary.
    candidates << QCoreApplication::applicationDirPath()
                      + QDir::separator() + defaultDBName;

    for (auto &c : candidates) {
        // qDebug() << "Checking: " << c;
        if (QFileInfo::exists(c))
            return c;
    }

    // TODO: How do we handle this gracefully?
    return defaultDBName;
}


// Returns true if successful
bool initDBConnection(QString connectionName,
                      QString paramFilePath,
                      QString defaultFilePath)
{
    QString path = paramFilePath;
    if (path.isNull())
        path = findDBPath(defaultFilePath);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(path);
    bool isSuccessful = db.open();
    if ( ! isSuccessful)
    {
        qWarning() <<"DB Error: " << db.lastError().text();
    }
    return isSuccessful;
}

Library::~Library()
{
    if (connectionIsValid)
        QSqlDatabase::removeDatabase(connectionName);
}


void Library::getConnection()
{
    if (connectionIsValid) return;
    bool isOpen = initDBConnection(connectionName,
                                   Config::get().paramLibraryDatabaseFilepath,
                                   Config::get().defaultLibraryDbFilename);
    if (isOpen) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        QStringList tables = db.tables();
        if (tables.contains("LIBRARY", Qt::CaseSensitive)) {
            connectionIsValid = true;
        } else {
            qDebug() << "library db format is wrong";
        }
    }
}


QString Library::procedureText(QString cmdName)
{
    QString retval;

    getConnection();

    if (connectionIsValid) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        QSqlQuery query(db);
        query.prepare("SELECT CODE FROM LIBRARY WHERE COMMAND = ?");
        query.addBindValue(cmdName);
        query.exec();
        if (query.next()) {
            retval = query.value(0).toString();
        }
    }

    return retval;
}


QStringList Library::allProcedureNames()
{
    if (allProcedures.isEmpty()) {
        getConnection();

        if (connectionIsValid) {
            QSqlDatabase db = QSqlDatabase::database(connectionName);
            QSqlQuery query("SELECT COMMAND FROM LIBRARY", db);
            while (query.next()) {
                allProcedures.append(query.value(0).toString());
            }
        }
    }
    return allProcedures;
}


Help::~Help()
{
    if (connectionIsValid)
        QSqlDatabase::removeDatabase(connectionName);
}


void Help::getConnection()
{
    if (connectionIsValid) return;
    bool isOpen = initDBConnection(connectionName,
                                   Config::get().paramHelpDatabaseFilepath,
                                   Config::get().defaultHelpDbFilename);
    if (isOpen) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        QStringList tables = db.tables();
        if (tables.contains("ALIASES", Qt::CaseSensitive)
            && tables.contains("HELPTEXT", Qt::CaseSensitive)) {
            connectionIsValid = true;
        } else {
            qDebug() << "help db format is wrong";
        }
    }
}


QStringList Help::allCommands()
{
    getConnection();
    QStringList retval;
    if (connectionIsValid) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        QSqlQuery query("SELECT ALIAS FROM ALIASES", db);
        while (query.next()) {
            retval.append(query.value(0).toString());
        }
    } else {
        Error::fileSystem();
    }
    return retval;
}


QString Help::helpText(QString alias)
{
    QString retval;
    QString cmdName;

    getConnection();

    if (connectionIsValid) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        // Every command has an alias
        // even if the alias is the same as the command.
        QSqlQuery query(db);
        query.prepare("SELECT COMMAND FROM ALIASES WHERE ALIAS = ?");
        query.addBindValue(alias);
        query.exec();
        if (query.next()) {
            cmdName = query.value(0).toString();
        } else {
            goto bailout;
        }
        query.finish();

        query.prepare("SELECT DESCRIPTION FROM HELPTEXT WHERE COMMAND = ?");
        query.addBindValue(cmdName);
        query.exec();
        if (query.next()) {
            retval = query.value(0).toString();
        }
    }

bailout:
    return retval;
}
