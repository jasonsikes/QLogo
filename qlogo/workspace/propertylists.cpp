
//===-- qlogo/propertylists.cpp - PropertyLists class implementation -------*-
// C++ -*-===//
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
/// This file contains the implementation of the PropertyLists class, which
/// provides property list functionality for the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "propertylists.h"
#include "datum/datump.h"
#include "datum/list.h"
#include "datum/word.h"

PropertyLists::PropertyLists() {}

void PropertyLists::addProperty(const QString &plistname,
                                const QString &propname, DatumPtr value) {
  if (!plists.contains(plistname)) {
    plists.insert(plistname, QHash<QString, DatumPtr>());
  }

  plists[plistname][propname] = value;
}

DatumPtr PropertyLists::getProperty(const QString &plistname,
                                  const QString &propname) {
  if (plists.contains(plistname) && plists[plistname].contains(propname))
    return plists[plistname][propname];
  return DatumPtr(List::alloc());
}

void PropertyLists::removeProperty(const QString &plistname,
                                   const QString &propname) {
  if (plists.contains(plistname)) {
    plists[plistname].remove(propname);
    if (plists[plistname].isEmpty())
      plists.remove(plistname);
  }
}

DatumPtr PropertyLists::getPropertyList(const QString &plistname) {
  List *retval = List::alloc();
  if (plists.contains(plistname)) {
    QList<QString> keys = plists[plistname].keys();
    QList<DatumPtr> values = plists[plistname].values();
    QList<QString>::iterator kIter = keys.begin();
    for (auto &vIter : values) {
      retval->append(DatumPtr(*kIter));
      retval->append(vIter);
      ++kIter;
    }
  }
  return DatumPtr(retval);
}

void PropertyLists::erasePropertyList(const QString &plistname) {
  plists.remove(plistname);
}

bool PropertyLists::isPropertyList(const QString &plistname) {
  return plists.contains(plistname);
}

DatumPtr PropertyLists::allPLists(showContents_t showWhat) {
  List *retval = List::alloc();
  for (auto &name : plists.keys()) {
    if (shouldInclude(showWhat, name))
      retval->append(DatumPtr(name));
  }
  return DatumPtr(retval);
}

void PropertyLists::eraseAll() {
  QStringList listnames = plists.keys();

  for (auto &name : listnames) {
    if (!isBuried(name)) {
      plists.remove(name);
    }
  }
}
