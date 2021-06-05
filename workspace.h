#ifndef WORKSPACE_H
#define WORKSPACE_H

//===-- qlogo/workspace.h - Workspace class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Workspace class, which is the
/// superclass for classes that need QLogo language workspace functionality
/// (Vars, PropertyLists, Parser).
///
//===----------------------------------------------------------------------===//

#include <QSet>
#include <QString>

enum showContents_t { showUnburied, showBuried, showTraced, showStepped };

class Workspace {
  QSet<QString> buriedNames;
  QSet<QString> steppedNames;
  QSet<QString> tracedNames;

public:
  Workspace();

  void bury(const QString &aName);
  bool isBuried(const QString &aName);
  void unbury(const QString &aName);

  void step(const QString &aName);
  bool isStepped(const QString &aName);
  void unstep(const QString &aName);

  void trace(const QString &aName);
  bool isTraced(const QString &aName);
  void untrace(const QString &aName);

  bool shouldInclude(showContents_t showWhat, const QString &name);
};

#endif // WORKSPACE_H
