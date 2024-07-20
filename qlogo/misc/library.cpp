
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
/// provides standard supporting functions to the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "library.h"
#include <QStringList>
#include <QCoreApplication>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include "sharedconstants.h"


Library::~Library()
{
    if (db.isValid())
        db.removeDatabase("QSQLITE");
}


QString Library::findLibraryDB()
{
    // If the libraryDB location was passed as a parameter, use that.
    if ( ! Config::get().paramLibraryDatabaseFilepath.isNull()) {
        return Config::get().paramLibraryDatabaseFilepath;
    }

    // else, build a list of candidate locations to try.
    QStringList candidates;

    // The share directory relative to wherever the app binary is.
    candidates << QCoreApplication::applicationDirPath()
                      + QDir::separator() + ".."
                      + QDir::separator() + "share"
                      + QDir::separator() + "qlogo"
                      + QDir::separator() + Config::get().defaultLibraryDbFilename;
    // The Resources directory relative to wherever the app binary is.
    candidates << QCoreApplication::applicationDirPath()
                      + QDir::separator() + ".."
                      + QDir::separator() + "Resources"
                      + QDir::separator() + Config::get().defaultLibraryDbFilename;
    // The same directory as the app binary.
    candidates << QCoreApplication::applicationDirPath()
                      + QDir::separator() + Config::get().defaultLibraryDbFilename;

    for (auto &c : candidates) {
        // qDebug() << "Checking: " << c;
        if (QFileInfo::exists(c))
            return c;
    }

    // TODO: How do we handle this gracefully?
    return Config::get().defaultLibraryDbFilename;
}


void Library::getConnection()
{
    if ( ! connectionIsValid) {
        if ( ! db.isValid()) {
            db = QSqlDatabase::addDatabase("QSQLITE");
        }
        QString path = findLibraryDB();
        db.setDatabaseName(path);
        if (db.open())
        {
            QStringList tables = db.tables();
            if (tables.contains("LIBRARY", Qt::CaseSensitive)) {
                connectionIsValid = true;
            } else {
                qDebug() << "library db format is wrong";
            }
        } else {
            qDebug() <<"DB Error: " << db.lastError().text();
        }
    }
}


QString Library::procedureText(QString cmdName)
{
    QString retval;

    getConnection();

    if (connectionIsValid) {
        QSqlQuery query(db);
        query.prepare("SELECT CODE FROM LIBRARY WHERE COMMAND = ?");
        query.addBindValue(cmdName);
        query.exec();
        if (query.next()) {
            retval = query.value(0).toString();
        }
    }

bailout:
    return retval;
}


QStringList Library::allProcedureNames()
{
    if (allProcedures.isEmpty()) {
        getConnection();

        if (connectionIsValid) {
            QSqlQuery query("SELECT COMMAND FROM LIBRARY", db);
            while (query.next()) {
                allProcedures.append(query.value(0).toString());
            }
        }
    }
    return allProcedures;
}
