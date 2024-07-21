#ifndef LIBRARY_H
#define LIBRARY_H

//===-- qlogo/library.h - Library text declaration -------*- C++ -*-===//
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
/// This file contains the declaration of the QLogo library interface, which
/// provides the standard library (supporting functions to the QLogo language),
/// and the help facility.
///
//===----------------------------------------------------------------------===//

#include <QStringList>

#include <QSqlDatabase>

class Library
{
    bool connectionIsValid = false;

    void getConnection();

    QStringList allProcedures;

    const QString connectionName = "libDB";
public:

    Library() {}
    ~Library();

    /// Return the text of library procedure of the given name.
    /// Returns null string if no procedure found.
    QString procedureText(QString alias);

    /// Returns a list of all procedure names available in the library.
    QStringList allProcedureNames();
};

class Help
{
    bool connectionIsValid = false;

    void getConnection();

    const QString connectionName = "help";
public:

    Help() {}
    ~Help();

    /// Return a list of all command names that have a help text entry.
    QStringList allCommands();

    /// Return the help text for a command.
    QString helpText(QString alias);
};


#endif // LIBRARY_H
