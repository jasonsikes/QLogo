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

#include "datum.h"
#include <QVector>
#include <functional>

class Kernel;
class Parser;

typedef std::function<bool(DatumP)> validatorP;
typedef std::function<bool(double)> validatorD;
typedef std::function<bool(int)> validatorI;
typedef std::function<bool(List *)> validatorL;

class ProcedureHelper {
  ASTNode *node;
  Kernel *parent;
  QVector<DatumP> parameters;
  DatumP returnValue;

public:
  QString indent();
  bool isTraced;
  static void setParser(Parser *aParser);
  ProcedureHelper() { exit(1); }
  ProcedureHelper(Kernel *aParent, DatumP sourceNode);
  ~ProcedureHelper();

  int countOfChildren() { return parameters.size(); }

  DatumP validatedDatumAtIndex(int index, validatorP v);
  DatumP datumAtIndex(int index, bool canRunlist = false);
  DatumP wordAtIndex(int index, bool canRunlist = false);
  DatumP listAtIndex(int index);
  DatumP validatedListAtIndex(int index, validatorL v);
  DatumP arrayAtIndex(int index);
  double numberAtIndex(int index, bool canRunList = false);
  double validatedNumberAtIndex(int index, validatorD v,
                                bool canRunList = false);
  long integerAtIndex(int index);
  long validatedIntegerAtIndex(int index, validatorI v);
  bool boolAtIndex(int index, bool canRunlist = false);
  DatumP reject(int index, bool allowErract = false,
                bool allowRecovery = false);
  DatumP reject(DatumP value, bool allowErract = false,
                bool allowRecovery = false);
  DatumP ret(Datum *aVal);
  DatumP ret(DatumP aVal);
  DatumP ret(bool aVal);
  DatumP ret(void);
  static void setIsErroring(bool aIsErroring);
};

#endif // PROCEDUREHELPER_H
