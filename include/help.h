#ifndef HELP_H
#define HELP_H

//===-- qlogo/help.h - Help class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Help class, which is responsible
/// for searching and retrieving help texts and commands.
///
//===----------------------------------------------------------------------===//


#include <QStringList>

#include <QSqlDatabase>

class Help
{
    bool connectionIsValid = false;

    QString findHelpDB();
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

#endif // HELP_H
