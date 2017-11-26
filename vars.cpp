
//===-- qlogo/vars.cpp - Vars class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Vars class, which holds the
/// variables in the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "vars.h"
#include "QDebug"
#include <QSet>

const QString tf = "*tf*";

Vars::Vars() { upScope(); }

void Vars::setDatumForName(DatumP &aDatum, const QString &name) {
  for (auto &variables : levels) {
    if (variables.contains(name)) {
      variables[name] = aDatum;
      return;
    }
  }
  levels.last().insert(name, aDatum);
}

DatumP Vars::datumForName(const QString &name) {
  for (auto &variables : levels) {
    auto result = variables.find(name);
    if (result != variables.end()) {
      return *result;
    }
  }
  return nothing;
}

void Vars::setVarAsLocal(const QString &name) {
  levels.first().insert(name, nothing);
}

void Vars::setVarAsGlobal(const QString &name) {
  levels.last().insert(name, nothing);
}

void Vars::upScope() {
  QHash<QString, DatumP> a;
  levels.push_front(a);
}

void Vars::downScope() { levels.pop_front(); }

int Vars::currentScope() { return levels.size(); }

bool Vars::doesExist(const QString &name) {
  for (auto &variables : levels) {
    if (variables.contains(name))
      return true;
  }
  return false;
}

DatumP Vars::allVariables(showContents_t showWhat) {
  List *retval = new List;
  QSet<const QString> seenVars;

  for (auto &levelIter : levels) {
    for (auto &varname : levelIter.keys()) {
      if (!seenVars.contains(varname)) {
        seenVars.insert(varname);

        if (shouldInclude(showWhat, varname))
          retval->append(DatumP(new Word(varname)));
      }
    }
  }

  return DatumP(retval);
}

void Vars::eraseAll() {
  for (auto &levelIter : levels) {
    for (auto &varname : levelIter.keys()) {
      if (!isBuried(varname))
        levelIter.remove(varname);
    }
  }
}

void Vars::eraseVar(const QString &name) {
  for (auto &variables : levels) {
    if (variables.remove(name) > 0)
      return;
  }
}

void Vars::setTest(bool isTrue) {
  DatumP t = new Word(isTrue ? 0 : 1);
  levels.first().insert(tf, t);
}

bool Vars::isTested() { return datumForName(tf).isWord(); }

bool Vars::isTrue() {
  DatumP retval = datumForName(tf);
  if (retval.isWord() && (retval.wordValue()->numberValue() == 0))
    return true;
  return false;
}

bool Vars::isFalse() {
  DatumP retval = datumForName(tf);
  if (retval.isWord() && (retval.wordValue()->numberValue() == 1))
    return true;
  return false;
}

Scope::Scope(Vars *aVars) {
  v = aVars;
  v->upScope();
}

Scope::~Scope() { v->downScope(); }
