
//===-- qlogo/vars.cpp - Vars class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Vars class, which holds the
/// variables in the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "vars.h"
#include "datum_list.h"
#include "QDebug"
#include <QSet>


Vars::Vars()
{
  // Add one frame to the stack so that it is never empty
  globalFrame = new VarFrame(this);
}


Vars::~Vars()
{
  delete globalFrame;
}

void Vars::setDatumForName(DatumP &aDatum, const QString &name) {
  for (auto frame : frames) {
    if (frame->variables.contains(name)) {
      frame->variables[name] = aDatum;
      return;
    }
  }
  frames.last()->variables.insert(name, aDatum);
}

DatumP Vars::datumForName(const QString &name) {
  for (auto frame : frames) {
    auto result = frame->variables.find(name);
    if (result != frame->variables.end()) {
      return *result;
    }
  }
  return nothing;
}

void Vars::setVarAsLocal(const QString &name) {
  if ( ! frames.first()->variables.contains(name))
    frames.first()->variables.insert(name, nothing);
}

void Vars::setVarAsGlobal(const QString &name) {
  if ( ! frames.last()->variables.contains(name))
  frames.last()->variables.insert(name, nothing);
}

bool Vars::isVarGlobal(const QString &name)
{
  return frames.last()->variables.contains(name);
}

void Vars::upScope(VarFrame *aFrame) {
  frames.push_front(aFrame);
}

void Vars::downScope() { frames. pop_front(); }

int Vars::size() { return frames.size(); }

bool Vars::doesExist(const QString &name) {
  for (auto frame : frames) {
    if (frame->variables.contains(name))
      return true;
  }
  return false;
}

DatumP Vars::allVariables(showContents_t showWhat) {
  List *retval = emptyList();
  QSet<const QString> seenVars;

  for (auto frame : frames) {
      QStringList varnames = frame->variables.keys();
    for (auto &varname : varnames) {
      if (!seenVars.contains(varname)) {
        seenVars.insert(varname);

        if (shouldInclude(showWhat, varname))
          retval->append(DatumP(varname));
      }
    }
  }

  return DatumP(retval);
}

void Vars::eraseAll() {
  for (auto frame : frames) {
      QStringList varnames = frame->variables.keys();
    for (auto &varname : varnames) {
      if (!isBuried(varname))
        frame->variables.remove(varname);
    }
  }
}

void Vars::eraseVar(const QString &name) {
  for (auto frame : frames) {
    if (frame->variables.remove(name) > 0)
      return;
  }
}

void Vars::setTest(bool isTrue) {
  frames.first()->isTested = true;
  frames.first()->testState = isTrue;
}

bool Vars::isTested() {
  for (auto frame : frames) {
      if (frame->isTested)
        return true;
    }
  return false;
}


void Vars::setExplicitSlotList(DatumP aList)
{
  frames.last()->explicitSlotList = aList;
}


DatumP Vars::explicitSlotList()
{
  return frames.last()->explicitSlotList;
}

bool Vars::testedState() {
  for (auto frame : frames) {
      if (frame->isTested)
        return frame->testState;
    }
  Q_ASSERT(false);
}

VarFrame::VarFrame(Vars *aVars) {
  vars = aVars;
  vars->upScope(this);
}

VarFrame::~VarFrame() { vars->downScope(); }
