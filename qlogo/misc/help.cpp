#include "help.h"
#include "error.h"
#include <QSqlError>
#include <QDebug>
#include <QSqlQuery>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>

//===-- qlogo/help.cpp - Help class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Help class, which is responsible
/// for searching and retrieving help texts and commands.
///
//===----------------------------------------------------------------------===//

// If the help location was given as a command line parameter,
// This is where it would be.
extern QString helpdb;

// TODO: This should be set in the CMake file
const char* helpDbFilename = "qlogo_help.db";

Help::Help()
{
    connectionIsValid = false;
    db = QSqlDatabase::addDatabase("QSQLITE");
}


Help::~Help()
{
    db.removeDatabase("QSQLITE");
}


QString Help::findHelpDB()
{
    // If the helpDB location was passed as a parameter, use that.
    if ( ! helpdb.isNull()) {
        return helpdb;
    }

    // else, build a list of candidate locations to try.
    QStringList candidates;

    // The share directory relative to wherever the app binary is.
    candidates << QCoreApplication::applicationDirPath()
                      + QDir::separator() + ".."
                      + QDir::separator() + "share"
                      + QDir::separator() + "qlogo"
                      + QDir::separator() + helpDbFilename;
    // The Resources directory relative to wherever the app binary is.
    candidates << QCoreApplication::applicationDirPath()
                      + QDir::separator() + ".."
                      + QDir::separator() + "Resources"
                      + QDir::separator() + helpDbFilename;
    // The same directory as the app binary.
    candidates << QCoreApplication::applicationDirPath()
                      + QDir::separator() + helpDbFilename;

    for (auto &c : candidates) {
        // qDebug() << "Checking: " << c;
        if (QFileInfo::exists(c))
            return c;
    }

    // TODO: How do we handle this gracefully?
    return helpdb;
}


void Help::getConnection()
{
    if ( ! connectionIsValid) {
        QString path = findHelpDB();
        db.setDatabaseName(path);
        if (db.open())
        {
            QStringList tables = db.tables();
             if (tables.contains("ALIASES", Qt::CaseSensitive)
                && tables.contains("HELPTEXT", Qt::CaseSensitive)) {
                connectionIsValid = true;
             } else {
                qDebug() << "help db format is wrong";
             }
        } else {
            qDebug() <<"DB Error: " << db.lastError().text();
        }
    }
}


QStringList Help::allCommands()
{
    getConnection();
    QStringList retval;
    if (connectionIsValid) {
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
        } else {
            goto bailout;
        }


    }

bailout:
    return retval;
}
