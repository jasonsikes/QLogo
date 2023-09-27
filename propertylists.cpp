
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
#include "datum_datump.h"
#include "datum_list.h"
#include "datum_word.h"

PropertyLists::PropertyLists() {}

void PropertyLists::addProperty(const QString &plistname,
                                const QString &propname, DatumP value) {
  if (!plists.contains(plistname)) {
    plists.insert(plistname, QHash<QString, DatumP>());
  }

  plists[plistname][propname] = value;
}

DatumP PropertyLists::getProperty(const QString &plistname,
                                  const QString &propname) {
  if (plists.contains(plistname) && plists[plistname].contains(propname))
    return plists[plistname][propname];
  return DatumP(List::alloc());
}

void PropertyLists::removeProperty(const QString &plistname,
                                   const QString &propname) {
  if (plists.contains(plistname)) {
    plists[plistname].remove(propname);
    if (plists[plistname].isEmpty())
      plists.remove(plistname);
  }
}

DatumP PropertyLists::getPropertyList(const QString &plistname) {
  List *retval = List::alloc();
  if (plists.contains(plistname)) {
    QList<QString> keys = plists[plistname].keys();
    QList<DatumP> values = plists[plistname].values();
    QList<QString>::iterator kIter = keys.begin();
    for (auto &vIter : values) {
      retval->append(DatumP(*kIter));
      retval->append(vIter);
      ++kIter;
    }
  }
  return DatumP(retval);
}

void PropertyLists::erasePropertyList(const QString &plistname) {
  plists.remove(plistname);
}

bool PropertyLists::isPropertyList(const QString &plistname) {
  return plists.contains(plistname);
}

DatumP PropertyLists::allPLists(showContents_t showWhat) {
  List *retval = List::alloc();
  for (auto &name : plists.keys()) {
    if (shouldInclude(showWhat, name))
      retval->append(DatumP(name));
  }
  return DatumP(retval);
}

void PropertyLists::eraseAll() {
  QStringList listnames = plists.keys();

  for (auto &name : listnames) {
    if (!isBuried(name)) {
      plists.remove(name);
    }
  }
}
