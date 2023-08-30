
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
#include <QDebug>
#include <math.h>

Parser *parser;

bool isErroring = false;
int traceIndent = 0;
const int dIndent = 1;

void ProcedureHelper::setParser(Parser *aParser) { parser = aParser; }

ProcedureHelper::ProcedureHelper(Kernel *aParent, DatumP sourceNode) {
  parent = aParent;
  node = sourceNode.astnodeValue();
  parameters.reserve(node->countOfChildren());
  isTraced = parser->isTraced(node->nodeName.wordValue()->keyValue());

  for (int i = 0; i < node->countOfChildren(); ++i) {
    if (node->childAtIndex(i).isa() == Datum::procedureType) {
      parameters.push_back(node->childAtIndex(i));
    } else {
      ASTNode *child = node->childAtIndex(i).astnodeValue();
      KernelMethod method = child->kernel;
      DatumP param = (parent->*method)(child);
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
      DatumP param = parameters[i];
      if (param.isa() != Datum::procedureType)
        line += parser->unreadDatum(parameters[i]) + " ";
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
                         " stops\n");
      } else {
        parent->sysPrint(indent() + node->nodeName.wordValue()->printValue() +
                         " outputs " + returnValue.datumValue()->printValue() +
                         "\n");
      }
    }
  }
}

DatumP ProcedureHelper::validatedDatumAtIndex(int index, validatorP v) {
  DatumP retval = parameters.at(index);
  while (!v(retval)) {
    retval = reject(retval, true, true);
  }
  return retval;
}

double ProcedureHelper::validatedNumberAtIndex(int index, validatorD v,
                                               bool canRunList) {
  DatumP retvalP = wordAtIndex(index, canRunList);
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

long ProcedureHelper::validatedIntegerAtIndex(int index, validatorI v) {
  DatumP retvalP = wordAtIndex(index);
  forever {
    double retvalD = retvalP.wordValue()->numberValue();
    long retvalI = (long)retvalD;
    if (retvalP.wordValue()->didNumberConversionSucceed() &&
        (floor(retvalD) == retvalD) && v(retvalI))
      return retvalI;
    do {
      retvalP = reject(retvalP, true, true);
    } while (!retvalP.isWord());
  }
  return 0;
}

DatumP ProcedureHelper::validatedListAtIndex(int index, validatorL v) {
  DatumP retvalP = listAtIndex(index);
  while (!retvalP.isList() || !v(retvalP.listValue())) {
    retvalP = reject(index, true, true);
  }
  return retvalP;
}

DatumP ProcedureHelper::datumAtIndex(int index, bool canRunlist) {
  DatumP retval = parameters.at(index);
  if (canRunlist && retval.isList()) {
    retval = parent->runList(retval);
  }
  return retval;
}

DatumP ProcedureHelper::wordAtIndex(int index, bool canRunlist) {
  DatumP retval = datumAtIndex(index, canRunlist);
  while (!retval.isWord())
    retval = reject(retval, true, true);
  return retval;
}

bool ProcedureHelper::boolAtIndex(int index, bool canRunlist) {
  DatumP retval = wordAtIndex(index, canRunlist);
  forever {
    QString word = retval.wordValue()->keyValue();
    if (word == "TRUE")
      return true;
    if (word == "FALSE")
      return false;
    do {
      retval = reject(retval, true, true);
    } while (!retval.isWord());
  }
  return false;
}

DatumP ProcedureHelper::listAtIndex(int index) {
  DatumP retval = datumAtIndex(index);
  while (!retval.isList())
    retval = reject(retval, true, true);
  return retval;
}

DatumP ProcedureHelper::arrayAtIndex(int index) {
  DatumP retval = datumAtIndex(index);
  while (!retval.isArray())
    retval = reject(retval, true, true);
  return retval;
}

double ProcedureHelper::numberAtIndex(int index, bool canRunList) {
  DatumP retvalP = wordAtIndex(index, canRunList);
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

long ProcedureHelper::integerAtIndex(int index) {
  DatumP retvalP = datumAtIndex(index);
  forever {
    double retvalD = retvalP.wordValue()->numberValue();
    long retval = (long)retvalD;
    if (retvalP.wordValue()->didNumberConversionSucceed() &&
        (floor(retvalD) == retvalD))
      return retval;
    do {
      retvalP = reject(retvalP, true, true);
    } while (!retvalP.isWord());
  }
  return 0;
}

DatumP ProcedureHelper::reject(DatumP value, bool allowErract,
                               bool allowRecovery) {
  return Error::doesntLike(node->nodeName, value, allowErract, allowRecovery);
}

DatumP ProcedureHelper::reject(int index, bool allowErract,
                               bool allowRecovery) {
  return reject(parameters[index], allowErract, allowRecovery);
}

DatumP ProcedureHelper::ret(Datum *aVal) {
  returnValue = DatumP(aVal);
  return returnValue;
}

DatumP ProcedureHelper::ret(DatumP aVal) {
  returnValue = aVal;
  return returnValue;
}

DatumP ProcedureHelper::ret(bool aVal) {
  returnValue = DatumP(aVal);
  return returnValue;
}

DatumP ProcedureHelper::ret(void) {
  returnValue = nothing;
  return nothing;
}

void ProcedureHelper::setIsErroring(bool aIsErroring) {
  isErroring = aIsErroring;
}

QString ProcedureHelper::indent() { return QString(traceIndent, ' '); }
