
//===-- qlogo/workspace.cpp - Workspace class implementation --*- C++ -*-===//
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
/// This file contains the implementation of the Workspace class, which is the
/// superclass for classes that need QLogo language workspace functionality
/// (Vars, PropertyLists, Parser).
///
//===----------------------------------------------------------------------===//

#include "workspace/workspace.h"

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
