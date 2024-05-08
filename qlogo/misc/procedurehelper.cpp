
//===-- qlogo/procedurehelper.cpp - ProcedureHelper class implementation
//-------*- C++ -*-===//
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
/// This file contains the implementation of the ProcedureHelper class, which
/// provides the functionality required by QLogo primative functions
///
//===----------------------------------------------------------------------===//

#include "procedurehelper.h"
#include "error.h"
#include "kernel.h"
#include "parser.h"
#include "datum/astnode.h"
#include "datum/word.h"
#include <QDebug>
#include <math.h>
#include "stringconstants.h"


bool isErroring = false;
int traceIndent = 0;
const int dIndent = 1;

ProcedureHelper::ProcedureHelper(Kernel *aParent, DatumPtr sourceNode) {
  parent = aParent;
  node = sourceNode.astnodeValue();
  parameters.reserve(node->countOfChildren());
  isTraced = mainProcedures()->isTraced(node->nodeName.wordValue()->keyValue());

  for (int i = 0; i < node->countOfChildren(); ++i) {
    if (node->childAtIndex(i).isa() == Datum::procedureType) {
      parameters.push_back(node->childAtIndex(i));
    } else {
      ASTNode *child = node->childAtIndex(i).astnodeValue();
      KernelMethod method = child->kernel;
      DatumPtr param = (parent->*method)(child);
      if (param == nothing) {
        Error::didntOutput(child->nodeName, node->nodeName);
      }
      if (param.isASTNode()) {
        Error::notInsideProcedure(param.astnodeValue()->nodeName);
      }
      parameters.push_back(param);
    }
  }

  if (isTraced) {
    QString line = indent() + "( %1 ";
    line = line.arg(node->nodeName.wordValue()->printValue());
    for (int i = 0; i < parameters.size(); ++i) {
      DatumPtr param = parameters[i];
      if (param.isa() != Datum::procedureType)
        line += mainProcedures()->unreadDatum(parameters[i]) + " ";
    }
    parent->sysPrint(line + ")\n");
    traceIndent += dIndent;
  }
}

ProcedureHelper::~ProcedureHelper() {
  if (isTraced) {
    traceIndent -= dIndent;
    if (!isErroring) {
      if (returnValue == nothing) {
        parent->sysPrint(indent() + node->nodeName.wordValue()->printValue() +
                         k._stops());
      } else {
        parent->sysPrint(indent() + node->nodeName.wordValue()->printValue() +
                         k._outputs_() + returnValue.datumValue()->printValue() +
                         '\n');
      }
    }
  }
}

DatumPtr ProcedureHelper::validatedDatumAtIndex(int index, validatorP v) {
  DatumPtr retval = parameters.at(index);
  while (!v(retval)) {
    retval = reject(retval, true, true);
  }
  return retval;
}

double ProcedureHelper::validatedNumberAtIndex(int index, validatorD v,
                                               bool canRunList) {
  DatumPtr retvalP = wordAtIndex(index, canRunList);
  forever {
    double retval = retvalP.wordValue()->numberValue();
    if (retvalP.wordValue()->didNumberConversionSucceed() && v(retval))
      return retval;
    do {
      retvalP = reject(retvalP, true, true);
    } while (!retvalP.isWord());
  }
  return 0;
}

int ProcedureHelper::validatedIntegerAtIndex(int index, validatorI v) {
  DatumPtr retvalP = wordAtIndex(index);
  forever {
    double retvalD = retvalP.wordValue()->numberValue();
    int retvalI = (int)retvalD;
    if (retvalP.wordValue()->didNumberConversionSucceed() &&
        (floor(retvalD) == retvalD) && v(retvalI))
      return retvalI;
    do {
      retvalP = reject(retvalP, true, true);
    } while (!retvalP.isWord());
  }
  return 0;
}

DatumPtr ProcedureHelper::validatedListAtIndex(int index, validatorL v) {
  DatumPtr retvalP = listAtIndex(index);
  while (!retvalP.isList() || !v(retvalP.listValue())) {
    retvalP = reject(index, true, true);
  }
  return retvalP;
}

DatumPtr ProcedureHelper::datumAtIndex(int index, bool canRunlist) {
  DatumPtr retval = parameters.at(index);
  if (canRunlist && retval.isList()) {
    retval = parent->runList(retval);
  }
  return retval;
}

DatumPtr ProcedureHelper::wordAtIndex(int index, bool canRunlist) {
  DatumPtr retval = datumAtIndex(index, canRunlist);
  while (!retval.isWord())
    retval = reject(retval, true, true);
  return retval;
}

bool ProcedureHelper::boolAtIndex(int index, bool canRunlist) {
  DatumPtr retval = wordAtIndex(index, canRunlist);
  forever {
    QString word = retval.wordValue()->keyValue();
    if (word == k.kctrue())
      return true;
    if (word == k.kcfalse())
      return false;
    do {
      retval = reject(retval, true, true);
    } while (!retval.isWord());
  }
  return false;
}

DatumPtr ProcedureHelper::listAtIndex(int index) {
  DatumPtr retval = datumAtIndex(index);
  while (!retval.isList())
    retval = reject(retval, true, true);
  return retval;
}

DatumPtr ProcedureHelper::arrayAtIndex(int index) {
  DatumPtr retval = datumAtIndex(index);
  while (!retval.isArray())
    retval = reject(retval, true, true);
  return retval;
}

double ProcedureHelper::numberAtIndex(int index, bool canRunList) {
  DatumPtr retvalP = wordAtIndex(index, canRunList);
  forever {
    double retval = retvalP.wordValue()->numberValue();
    if (retvalP.wordValue()->didNumberConversionSucceed())
      return retval;
    do {
      retvalP = reject(retvalP, true, true);
    } while (!retvalP.isWord());
  }
  return 0;
}

int ProcedureHelper::integerAtIndex(int index) {
  DatumPtr retvalP = datumAtIndex(index);
  forever {
    double retvalD = retvalP.wordValue()->numberValue();
    int retval = (int)retvalD;
    if (retvalP.wordValue()->didNumberConversionSucceed() &&
        (floor(retvalD) == retvalD))
      return retval;
    do {
      retvalP = reject(retvalP, true, true);
    } while (!retvalP.isWord());
  }
  return 0;
}

DatumPtr ProcedureHelper::reject(DatumPtr value, bool allowErract,
                               bool allowRecovery) {
  return Error::doesntLike(node->nodeName, value, allowErract, allowRecovery);
}

DatumPtr ProcedureHelper::reject(int index, bool allowErract,
                               bool allowRecovery) {
  return reject(parameters[index], allowErract, allowRecovery);
}

DatumPtr ProcedureHelper::ret(Datum *aVal) {
  returnValue = DatumPtr(aVal);
  return returnValue;
}

DatumPtr ProcedureHelper::ret(DatumPtr aVal) {
  returnValue = aVal;
  return returnValue;
}

DatumPtr ProcedureHelper::ret(bool aVal) {
  returnValue = DatumPtr(aVal);
  return returnValue;
}


DatumPtr ProcedureHelper::ret(int aVal) {
  returnValue = DatumPtr(aVal);
  return returnValue;
}


DatumPtr ProcedureHelper::ret(double aVal) {
  returnValue = DatumPtr(aVal);
  return returnValue;
}


DatumPtr ProcedureHelper::ret(QString aVal) {
  returnValue = DatumPtr(aVal);
  return returnValue;
}

DatumPtr ProcedureHelper::ret(void) {
  returnValue = nothing;
  return nothing;
}

void ProcedureHelper::setIsErroring(bool aIsErroring) {
  isErroring = aIsErroring;
}

QString ProcedureHelper::indent() { return QString(traceIndent, ' '); }
