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

#include "datum/datump.h"
#include "workspace/workspace.h"
#include <QHash>
#include <QList>

class Vars;


/// A frame to hold local variables.
class VarFrame
{
  friend class Vars;

  bool isTested = false;
  bool testState;
  DatumPtr explicitSlotList;
  QHash<QString, DatumPtr> variables;
  Vars *vars;
public:
  VarFrame(Vars *aVars);
  ~VarFrame();
};


/// A stack that holds variables during procedure calls.
///
/// The "bottom" of the stack holds the global variables while the "top" of the
/// stack holds the local variables.
///
/// The topmost frame is at position zero while the global frame is at the end
/// of the globalFrame list.
class Vars : public Workspace {
  QList<VarFrame *> frames;
  VarFrame *globalFrame;

public:
  Vars();
  ~Vars();

  /// Search downward through the variable stack for the first occurrence of
  /// 'name'. Returns the stored value associated with 'name'.
  DatumPtr datumForName(const QString &name);

  /// Search downward through the variable stack for the first occurrence of
  /// 'name'. Replace the stored value with aDatum. If 'name' is not found,
  /// store value at the bottom of the stack.
  void setDatumForName(DatumPtr &aDatum, const QString &name);

  /// Insert an entry for 'name' at the top of the variable stack. Store
  /// 'nothing' for the entry if name wasn't already present.
  void setVarAsLocal(const QString &name);

  /// Insert an entry for 'name' at the bottom of the variable stack. Store
  /// 'nothing' for the entry if name wasn't already present.
  void setVarAsGlobal(const QString &name);

  /// Push the pointer to a VarFrame object to the top of the variable stack.
  /// (Called by VarFrame class constructor.)
  void upScope(VarFrame *aFrame);

  /// Pop the pointer to a frame object from the top of the variable stack.
  /// (Called by VarFrame class destructor.)
  void downScope();

  /// Return true if value keyed by name exists somewhere in the stack.
  bool doesExist(const QString &name);

  /// Erase every occurrence of name from the variable stack.
  void eraseVar(const QString &name);

  /// Erase every unburied variable from the variable stack.
  void eraseAll();

  /// Returns the size of the stack, i.e. the number of stack frames.
  int size();

  /// Set the test state to TRUE or FALSE
  void setTest(bool isTrue);

  /// Return true if a test state has been registered in any stack frame.
  bool isTested();

  /// Return true if the highest registered test state is true.
  bool testedState();

  /// Returns true if the named variable exists in the lowest stack frame.
  bool isVarGlobal(const QString &name);

  /// Return a list of all variables defined and buried/unburied (determined by
  /// 'showWhat'.)
  DatumPtr allVariables(showContents_t showWhat);


  /// In "explicit slot" APPLY command, sets the list of values of the explicit
  /// slot variables ("?1", "?2", etc.)
  void setExplicitSlotList(DatumPtr aList);

  /// In "explicit slot" APPLY command, retrieves the list of values of the
  /// explicit slot variables ("?1", "?2", etc.)
  DatumPtr explicitSlotList();
};

#endif // VARS_H
