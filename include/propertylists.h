#ifndef PROPERTYLISTS_H
#define PROPERTYLISTS_H

//===-- qlogo/propertylists.h - PropertyLists class definition -------*- C++
//-*-===//
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
/// This file contains the declaration of the PropertyLists class, which
/// provides property list functionality for the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "workspace.h"
#include "datum/datum.h"

class PropertyLists : public Workspace {
  QHash<QString, QHash<QString, DatumPtr>> plists;

public:
  PropertyLists();

  void addProperty(const QString &plistname, const QString &propname,
                   DatumPtr value);
  DatumPtr getProperty(const QString &plistname, const QString &propname);
  void removeProperty(const QString &plistname, const QString &propname);
  DatumPtr getPropertyList(const QString &plistname);
  void erasePropertyList(const QString &plistname);
  void eraseAll();

  bool isPropertyList(const QString &plistname);

  DatumPtr allPLists(showContents_t showWhat);
};

#endif // PROPERTYLISTS_H
