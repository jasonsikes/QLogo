
//===-- qlogo/propertylists.cpp - PropertyLists class implementation --*- C++ -*-===//
//
// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the PropertyLists class, which
/// provides property list functionality for the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "workspace/propertylists.h"
#include "datum.h"

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
  return emptyList;
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
  ListBuilder builder;
  if (plists.contains(plistname)) {
    QList<QString> keys = plists[plistname].keys();
    QList<DatumPtr> values = plists[plistname].values();
    QList<QString>::iterator kIter = keys.begin();
    for (auto &vIter : values) {
      builder.append(DatumPtr(*kIter));
      builder.append(vIter);
      ++kIter;
    }
  }
  return builder.finishedList();
}

void PropertyLists::erasePropertyList(const QString &plistname) {
  plists.remove(plistname);
}

bool PropertyLists::isPropertyList(const QString &plistname) {
  return plists.contains(plistname);
}

DatumPtr PropertyLists::allPLists() {
  ListBuilder builder;
  for (auto name : plists.asKeyValueRange()) {
      builder.append(DatumPtr(name.first));
  }
  return builder.finishedList();
}
