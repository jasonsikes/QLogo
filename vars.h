#ifndef VARS_H
#define VARS_H

//===-- qlogo/vars.h - Vars class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Vars class, which holds the
/// variables in the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "datum.h"
#include "workspace.h"
#include <QHash>
#include <QList>

class Vars : public Workspace {
  QList<QHash<QString, DatumP>> levels;

public:
  Vars();
  DatumP datumForName(const QString &name);
  void setDatumForName(DatumP &aDatum, const QString &name);

  void setVarAsLocal(const QString &name);
  void setVarAsGlobal(const QString &name);
  void upScope();   // more local
  void downScope(); // more global
  bool doesExist(const QString &name);
  void eraseVar(const QString &name);
  void eraseAll();
  int currentScope();
  void setTest(bool isTrue);
  bool isTested();
  bool isTrue();
  bool isFalse();
  bool isVarGlobal(const QString &name);

  DatumP allVariables(showContents_t showWhat);
};

class Scope {
  Vars *v;

public:
  Scope(Vars *aVars);
  ~Scope();
};

#endif //
