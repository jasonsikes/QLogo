
//===-- qlogo/workspace.cpp - Workspace class implementation -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
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
/// This file contains the implementation of the Workspace class, which is the
/// superclass for classes that need QLogo language workspace functionality
/// (Vars, PropertyLists, Parser).
///
//===----------------------------------------------------------------------===//

#include "workspace.h"

Workspace::Workspace() {}

void Workspace::bury(const QString &aName) { buriedNames.insert(aName); }

bool Workspace::isBuried(const QString &aName) {
  return buriedNames.contains(aName);
}

void Workspace::unbury(const QString &aName) { buriedNames.remove(aName); }

void Workspace::step(const QString &aName) { steppedNames.insert(aName); }

bool Workspace::isStepped(const QString &aName) {
  return steppedNames.contains(aName);
}

void Workspace::unstep(const QString &aName) { steppedNames.remove(aName); }

void Workspace::trace(const QString &aName) { tracedNames.insert(aName); }

bool Workspace::isTraced(const QString &aName) {
  return tracedNames.contains(aName);
}

void Workspace::untrace(const QString &aName) { tracedNames.remove(aName); }

bool Workspace::shouldInclude(showContents_t showWhat, const QString &name) {
  switch (showWhat) {
  case showUnburied:
    if (!isBuried(name))
      return true;
    break;
  case showBuried:
    if (isBuried(name))
      return true;
    break;
  case showTraced:
    if (isTraced(name))
      return true;
    break;
  case showStepped:
    if (isStepped(name))
      return true;
    break;
  default:
    Q_ASSERT(false);
    break;
  } // /switch
  return false;
}
