#ifndef PROCEDUREHELPER_H
#define PROCEDUREHELPER_H

//===-- qlogo/procedurehelper.h - ProcedureHelper class definition -------*- C++
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
/// This file contains the declaration of the ProcedureHelper class, which
/// provides the functionality required by QLogo primative functions
///
//===----------------------------------------------------------------------===//

#include "datum/datump.h"
#include <QVector>
#include <functional>

class Kernel;
class Parser;
class Procedures;

typedef std::function<bool(DatumPtr)> validatorP;
typedef std::function<bool(double)> validatorD;
typedef std::function<bool(int)> validatorI;
typedef std::function<bool(List *)> validatorL;

class ProcedureHelper {
  ASTNode *node;
  Kernel *parent;
  QVector<DatumPtr> parameters;
  DatumPtr returnValue;

public:
  QString indent();
  bool isTraced;
  static void setParser(Parser *aParser);
  static void setProcedures(Procedures *aProcedures);
  ProcedureHelper() { exit(1); }
  ProcedureHelper(Kernel *aParent, DatumPtr sourceNode);
  ~ProcedureHelper();

  int countOfChildren() { return parameters.size(); }

  DatumPtr validatedDatumAtIndex(int index, validatorP v);
  DatumPtr datumAtIndex(int index, bool canRunlist = false);
  DatumPtr wordAtIndex(int index, bool canRunlist = false);
  DatumPtr listAtIndex(int index);
  DatumPtr validatedListAtIndex(int index, validatorL v);
  DatumPtr arrayAtIndex(int index);
  double numberAtIndex(int index, bool canRunList = false);
  double validatedNumberAtIndex(int index, validatorD v,
                                bool canRunList = false);
  int integerAtIndex(int index);
  int validatedIntegerAtIndex(int index, validatorI v);
  bool boolAtIndex(int index, bool canRunlist = false);
  DatumPtr reject(int index, bool allowErract = false,
                bool allowRecovery = false);
  DatumPtr reject(DatumPtr value, bool allowErract = false,
                bool allowRecovery = false);
  DatumPtr ret(int aVal);
  DatumPtr ret(double aVal);
  DatumPtr ret(QString aVal);
  DatumPtr ret(Datum *aVal);
  DatumPtr ret(DatumPtr aVal);
  DatumPtr ret(bool aVal);
  DatumPtr ret(void);
  static void setIsErroring(bool aIsErroring);
};

#endif // PROCEDUREHELPER_H
