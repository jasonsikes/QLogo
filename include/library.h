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
    QString procedureText(QString cmdName);

    /// @brief Return a list of all procedure names available in the library.
    /// @returns A list of all procedure names available in the library.
    QStringList allProcedureNames();
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
    QString helpText(QString cmdName);
};

#endif // LIBRARY_H
