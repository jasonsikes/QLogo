
//===-- qlogo/kernel.cpp - Kernel class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Kernel class, which is the
/// executor proper of the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "error.h"
#include "kernel.h"
#include "parser.h"

#include CONTROLLER_HEADER

QString Kernel::executeText(const QString &text) {
  QString inText = text;
  QString outText;

  QTextStream inStream(&inText, QIODevice::ReadOnly);
  QTextStream outStream(&outText, QIODevice::WriteOnly);

  StreamRedirect sr(this, &inStream, &outStream);

  bool shouldContinue = true;
  while (shouldContinue) {
    shouldContinue = getLineAndRunIt(false);
  }
  outStream.flush();
  return outText;
}

void Kernel::editAndRunWorkspaceText() {
  const QString *textRetval = mainController()->editText(&workspaceText);
  if (textRetval != NULL) {
    workspaceText = *textRetval;
    QString output = executeText(*textRetval);
    if (varLOADNOISILY()) {
      sysPrint(output);
    }
  }
}

void Kernel::editAndRunFile() {
  QString filepath = filepathForFilename(editFileName.wordValue());
  QFile file(filepath);
  if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
    Error::cantOpen(editFileName);
  }
  QTextStream in(&file);
  QString fileText = in.readAll();

  const QString *textRetval = mainController()->editText(&fileText);
  if (textRetval != NULL) {
    fileText = *textRetval;
    file.seek(0);
    file.resize(0);
    QTextStream out(&file);
    out << fileText;
    QString output = executeText(fileText);
    if (varLOADNOISILY()) {
      sysPrint(output);
    }
  }
}

DatumP Kernel::buildContentsList(showContents_t showWhat) {
  List *retval = new List;
  retval->append(parser->allProcedureNames(showWhat));
  retval->append(variables.allVariables(showWhat));
  retval->append(plists.allPLists(showWhat));
  return retval;
}

DatumP Kernel::contentslistFromDatumP(DatumP sourceNode) {
  List *sublists[3];
  DatumP locker[3];
  for (int i = 0; i < 3; ++i) {
    sublists[i] = new List;
    locker[i] = DatumP(sublists[i]);
  }

  if (sourceNode.isWord()) {
    sublists[0]->append(sourceNode);
  } else if (sourceNode.isList()) {
    unsigned parseLevel = 0;
    ListIterator i = sourceNode.listValue()->newIterator();
    while (i.elementExists()) {
      if (parseLevel > 2)
        return nothing;
      DatumP d = i.element();
      if (d.isWord()) {
        sublists[parseLevel]->append(d);
      } else if (d.isList()) {
        ListIterator j = d.listValue()->newIterator();
        while (j.elementExists()) {
          DatumP e = j.element();
          if (!e.isWord())
            return nothing;
          sublists[parseLevel]->append(e);
        }
        ++parseLevel;
      } else {
        return nothing;
      }
    }
  } else {
    return nothing;
  }

  List *retval = new List;
  for (int i = 0; i < 3; ++i) {
    retval->append(DatumP(sublists[i]));
  }
  return DatumP(retval);
}

void Kernel::processContentsListWithMethod(
    DatumP contentslist, void (Workspace::*method)(const QString &)) {
  List *proceduresList = contentslist.listValue()->datumAtIndex(1).listValue();
  List *variablesList = contentslist.listValue()->datumAtIndex(2).listValue();
  List *propertiesList = contentslist.listValue()->datumAtIndex(3).listValue();

  ListIterator i = proceduresList->newIterator();
  while (i.elementExists()) {
    QString procname = i.element().wordValue()->keyValue();
    (parser->*method)(procname);
  }

  i = variablesList->newIterator();
  while (i.elementExists()) {
    QString varname = i.element().wordValue()->keyValue();
    (variables.*method)(varname);
  }

  i = propertiesList->newIterator();
  while (i.elementExists()) {
    DatumP listnameP = i.element();
    QString listname = listnameP.wordValue()->keyValue();
    (plists.*method)(listname);
  }
}

DatumP Kernel::queryContentsListWithMethod(
    DatumP contentslist, bool (Workspace::*method)(const QString &)) {
  List *proceduresList = contentslist.listValue()->datumAtIndex(1).listValue();

  if (proceduresList->size() > 0) {
    QString procname = proceduresList->first().wordValue()->keyValue();
    return DatumP((parser->*method)(procname));
  }

  List *variablesList = contentslist.listValue()->datumAtIndex(2).listValue();

  if (variablesList->size() > 0) {
    QString varname = variablesList->first().wordValue()->keyValue();
    return DatumP((variables.*method)(varname));
  }

  List *propertiesList = contentslist.listValue()->datumAtIndex(3).listValue();

  if (propertiesList->size() > 0) {
    QString pname = propertiesList->first().wordValue()->keyValue();
    return DatumP((plists.*method)(pname));
  }
  return nothing;
}

QString Kernel::createPrintoutFromContentsList(DatumP contentslist,
                                               bool shouldValidate) {
  QString retval("");

  List *proceduresList = contentslist.listValue()->datumAtIndex(1).listValue();
  List *variablesList = contentslist.listValue()->datumAtIndex(2).listValue();
  List *propertiesList = contentslist.listValue()->datumAtIndex(3).listValue();

  ListIterator i = proceduresList->newIterator();
  while (i.elementExists()) {
    DatumP procedureText =
        parser->procedureFulltext(i.element(), shouldValidate);
    ListIterator j = procedureText.listValue()->newIterator();
    while (j.elementExists()) {
      QString line = j.element().wordValue()->printValue();
      line.append("\n");
      retval += line;
    }
  }

  i = variablesList->newIterator();
  while (i.elementExists()) {
    DatumP varnameP = i.element();
    QString varname = varnameP.wordValue()->keyValue();
    DatumP value = variables.datumForName(varname);
    if ((value == nothing) && shouldValidate) {
      Error::noValue(varnameP);
    } else {
      QString line = QString("Make \"%1 %2\n")
                         .arg(varname)
                         .arg(parser->unreadDatum(value));
      retval += line;
    }
  }

  i = propertiesList->newIterator();
  while (i.elementExists()) {
    DatumP listnameP = i.element();
    QString listname = listnameP.wordValue()->keyValue();
    DatumP proplist = plists.getPropertyList(listname);
    ListIterator j = proplist.listValue()->newIterator();
    while (j.elementExists()) {
      DatumP nameP = j.element();
      DatumP valueP = j.element();
      QString line = QString("Pprop %1 %2 %3\n")
                         .arg(parser->unreadDatum(listnameP))
                         .arg(parser->unreadDatum(nameP))
                         .arg(parser->unreadDatum(valueP));
      retval += line;
    }
  }
  return retval;
}

// SPECIAL VARIABLES

bool Kernel::varLOADNOISILY() {
  DatumP retvalP = variables.datumForName("LOADNOISILY");
  if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == QString("TRUE")))
    return true;
  return false;
}

bool Kernel::varALLOWGETSET() {
  DatumP retvalP = variables.datumForName("ALLOWGETSET");
  if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == QString("TRUE")))
    return true;
  return false;
}

DatumP Kernel::varBUTTONACT() { return variables.datumForName("BUTTONACT"); }

DatumP Kernel::varKEYACT() { return variables.datumForName("KEYACT"); }

bool Kernel::varFULLPRINTP() {
  DatumP retvalP = variables.datumForName("FULLPRINTP");
  if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == QString("TRUE")))
    return true;
  return false;
}

int Kernel::varPRINTDEPTHLIMIT() {
  DatumP retvalP = variables.datumForName("PRINTDEPTHLIMIT");
  if (retvalP.isWord()) {
    double retval = retvalP.wordValue()->numberValue();
    if (retvalP.wordValue()->didNumberConversionSucceed()) {
      return (int)retval;
    }
  }
  return -1;
}

int Kernel::varPRINTWIDTHLIMIT() {
  DatumP retvalP = variables.datumForName("PRINTWIDTHLIMIT");
  if (retvalP.isWord()) {
    double retval = retvalP.wordValue()->numberValue();
    if (retvalP.wordValue()->didNumberConversionSucceed()) {
      return (int)retval;
    }
  }
  return -1;
}

DatumP Kernel::varSTARTUP() { return variables.datumForName("STARTUP"); }

bool Kernel::varUNBURYONEDIT() {
  DatumP retvalP = variables.datumForName("UNBURYONEDIT");
  if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == QString("TRUE")))
    return true;
  return false;
}

bool Kernel::varCASEIGNOREDP() {
  DatumP retvalP = variables.datumForName("CASEIGNOREDP");
  if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == QString("TRUE")))
    return true;
  return false;
}

// PROCEDURE DEFINITION

DatumP Kernel::excTo(DatumP node) {
  // None of the children of node are ASTNode. They have to be literal so there
  // is no procedurehelper here.
  if (currentProcedure != nothing) {
    Error::toInProc(node.astnodeValue()->nodeName);
  }
  if (pauseLevel > 0) {
    Error::toInPause(node.astnodeValue()->nodeName);
  }
  parser->inputProcedure(node, systemReadStream);
  return nothing;
}

DatumP Kernel::excDefine(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP text = h.validatedListAtIndex(1, [](DatumP candidate) {
    ListIterator iter = candidate.listValue()->newIterator();
    while (iter.elementExists()) {
      DatumP line = iter.element();
      if (!line.isList())
        return false;
    }
    return true;
  });
  DatumP cmd = node.astnodeValue()->nodeName;
  DatumP procnameP = h.wordAtIndex(0);

  parser->defineProcedure(cmd, procnameP, text, nothing);

  return nothing;
}

DatumP Kernel::excText(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP procnameP = h.wordAtIndex(0);
  return h.ret(parser->procedureText(procnameP));
}

DatumP Kernel::excFulltext(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP procnameP = h.wordAtIndex(0);
  return h.ret(parser->procedureFulltext(procnameP));
}

DatumP Kernel::excCopydef(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP newname = h.wordAtIndex(0);
  DatumP oldname = h.wordAtIndex(1);

  parser->copyProcedure(newname, oldname);

  return nothing;
}

// VARIABLE DEFINITION

DatumP Kernel::excMake(DatumP node) {
  ProcedureHelper h(this, node);

  QString lvalue = h.wordAtIndex(0).wordValue()->keyValue();
  DatumP rvalue = h.datumAtIndex(1);

  variables.setDatumForName(rvalue, lvalue);

  if (variables.isTraced(lvalue.toUpper())) {
    QString line = QString("Make \"%1 %2\n")
                       .arg(h.wordAtIndex(0).wordValue()->printValue())
                       .arg(parser->unreadDatum(rvalue));
    sysPrint(line);
  }

  return nothing;
}

DatumP Kernel::excSetfoo(DatumP node) {
  ProcedureHelper h(this, node);

  QString foo = node.astnodeValue()->nodeName.wordValue()->keyValue();

  QString lvalue = foo.right(foo.size() - 3);
  DatumP rvalue = h.datumAtIndex(0);

  if (!variables.doesExist(lvalue)) {
    variables.setVarAsLocal(lvalue);
  }

  variables.setDatumForName(rvalue, lvalue);

  if (variables.isTraced(lvalue.toUpper())) {
    QString line =
        QString("%1 %2\n")
            .arg(node.astnodeValue()->nodeName.wordValue()->printValue())
            .arg(parser->unreadDatum(rvalue));
    sysPrint(line);
  }

  return nothing;
}

DatumP Kernel::excFoo(DatumP node) {
  DatumP fooP = node.astnodeValue()->nodeName;
  QString foo = fooP.wordValue()->keyValue();

  DatumP retval = variables.datumForName(foo);
  if (retval == nothing)
    return Error::noHowRecoverable(fooP);
  return retval;
}

DatumP Kernel::excLocal(DatumP node) {
  ProcedureHelper h(this, node);
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumP var = h.validatedDatumAtIndex(i, [](DatumP candidate) {
      if (candidate.isWord())
        return true;
      if (candidate.isList()) {
        ListIterator j = candidate.listValue()->newIterator();
        while (j.elementExists())
          if (!j.element().isWord())
            return false;
        return true;
      }
      return false;
    });
    if (var.isWord()) {
      makeVarLocal(var.wordValue()->keyValue());
    } else {
      ListIterator j = var.listValue()->newIterator();
      while (j.elementExists()) {
        DatumP v = j.element();
        makeVarLocal(v.wordValue()->keyValue());
      }
    }
  }
  return nothing;
}

DatumP Kernel::excThing(DatumP node) {
  ProcedureHelper h(this, node);
  QString varName = h.wordAtIndex(0).wordValue()->keyValue();
  DatumP retval = h.ret(variables.datumForName(varName));
  if (retval == nothing)
    return h.ret(Error::noValueRecoverable(h.datumAtIndex(0)));
  return retval;
}

DatumP Kernel::excGlobal(DatumP node) {
  ProcedureHelper h(this, node);
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumP var = h.validatedDatumAtIndex(i, [](DatumP candidate) {
      if (candidate.isWord())
        return true;
      if (candidate.isList()) {
        ListIterator j = candidate.listValue()->newIterator();
        while (j.elementExists())
          if (!j.element().isWord())
            return false;
        return true;
      }
      return false;
    });
    if (var.isWord()) {
      variables.setVarAsGlobal(var.wordValue()->keyValue());
    } else {
      ListIterator j = var.listValue()->newIterator();
      while (j.elementExists()) {
        DatumP v = j.element();
        variables.setVarAsGlobal(v.wordValue()->keyValue());
      }
    }
  }
  return nothing;
}

// PROPERTY LISTS

DatumP Kernel::excPprop(DatumP node) {
  ProcedureHelper h(this, node);
  QString plistname = h.wordAtIndex(0).wordValue()->keyValue();
  QString propname = h.wordAtIndex(1).wordValue()->keyValue();
  DatumP value = h.datumAtIndex(2);
  plists.addProperty(plistname, propname, value);
  if (plists.isTraced(plistname)) {
    QString line = QString("Pprop %1 %2 %3\n")
                       .arg(parser->unreadDatum(h.datumAtIndex(0)))
                       .arg(parser->unreadDatum(h.datumAtIndex(1)))
                       .arg(parser->unreadDatum(value));
    sysPrint(line);
  }
  return nothing;
}

DatumP Kernel::excGprop(DatumP node) {
  ProcedureHelper h(this, node);
  QString plistname = h.wordAtIndex(0).wordValue()->keyValue();
  QString propname = h.wordAtIndex(1).wordValue()->keyValue();
  DatumP retval = h.ret(plists.getProperty(plistname, propname));

  return retval;
}

DatumP Kernel::excRemprop(DatumP node) {
  ProcedureHelper h(this, node);
  QString plistname = h.wordAtIndex(0).wordValue()->keyValue();
  QString propname = h.wordAtIndex(1).wordValue()->keyValue();
  plists.removeProperty(plistname, propname);

  return nothing;
}

DatumP Kernel::excPlist(DatumP node) {
  ProcedureHelper h(this, node);
  QString plistname = h.wordAtIndex(0).wordValue()->keyValue();
  DatumP retval = plists.getPropertyList(plistname);

  return retval;
}

// PREDICATES

DatumP Kernel::excProcedurep(DatumP node) {
  ProcedureHelper h(this, node);
  bool retval = parser->isProcedure(h.wordAtIndex(0).wordValue()->keyValue());
  return h.ret(retval);
}

DatumP Kernel::excPrimitivep(DatumP node) {
  ProcedureHelper h(this, node);
  bool retval = parser->isPrimitive(h.wordAtIndex(0).wordValue()->keyValue());
  return h.ret(retval);
}

DatumP Kernel::excDefinedp(DatumP node) {
  ProcedureHelper h(this, node);
  bool retval = parser->isDefined(h.wordAtIndex(0).wordValue()->keyValue());
  return h.ret(retval);
}

DatumP Kernel::excNamep(DatumP node) {
  ProcedureHelper h(this, node);
  QString varname = h.wordAtIndex(0).wordValue()->keyValue();
  bool retval = (variables.doesExist(varname));
  return h.ret(retval);
}

DatumP Kernel::excPlistp(DatumP node) {
  ProcedureHelper h(this, node);
  QString listName = h.wordAtIndex(0).wordValue()->keyValue();
  bool retval = plists.isPropertyList(listName);
  return h.ret(retval);
}

// QUERIES

DatumP Kernel::excContents(DatumP node) {
  ProcedureHelper h(this, node);
  return h.ret(buildContentsList(showUnburied));
}

DatumP Kernel::excBuried(DatumP node) {
  ProcedureHelper h(this, node);
  return h.ret(buildContentsList(showBuried));
}

DatumP Kernel::excTraced(DatumP node) {
  ProcedureHelper h(this, node);
  return h.ret(buildContentsList(showTraced));
}

DatumP Kernel::excStepped(DatumP node) {
  ProcedureHelper h(this, node);
  return h.ret(buildContentsList(showStepped));
}

DatumP Kernel::excProcedures(DatumP node) {
  ProcedureHelper h(this, node);
  return h.ret(parser->allProcedureNames(showUnburied));
}

DatumP Kernel::excPrimitives(DatumP node) {
  ProcedureHelper h(this, node);
  return h.ret(parser->allPrimitiveProcedureNames());
}

DatumP Kernel::excNames(DatumP node) {
  ProcedureHelper h(this, node);
  List *retval = new List;
  retval->append(DatumP(new List));
  retval->append(variables.allVariables(showUnburied));
  return h.ret(retval);
}

DatumP Kernel::excPlists(DatumP node) {
  ProcedureHelper h(this, node);
  List *retval = new List;
  retval->append(DatumP(new List));
  retval->append(DatumP(new List));
  retval->append(plists.allPLists(showUnburied));
  return h.ret(retval);
}

DatumP Kernel::excArity(DatumP node) {
  ProcedureHelper h(this, node);
  return h.ret(parser->arity(h.wordAtIndex(0)));
}

DatumP Kernel::excNodes(DatumP node) {
  ProcedureHelper h(this, node);
  return h.ret(nodes());
}

// INSPECTION

DatumP Kernel::excPrintout(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumP candidate) {
    contentslist = contentslistFromDatumP(candidate);
    return contentslist != nothing;
  });

  QString output = createPrintoutFromContentsList(contentslist);
  stdPrint(output);

  return nothing;
}

DatumP Kernel::excPot(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumP candidate) {
    contentslist = contentslistFromDatumP(candidate);
    return contentslist != nothing;
  });

  List *proceduresList = contentslist.listValue()->datumAtIndex(1).listValue();
  List *variablesList = contentslist.listValue()->datumAtIndex(2).listValue();
  List *propertiesList = contentslist.listValue()->datumAtIndex(3).listValue();

  ListIterator i = proceduresList->newIterator();
  while (i.elementExists()) {
    QString procedureTitle = parser->procedureTitle(i.element()) + "\n";
    stdPrint(procedureTitle);
  }

  i = variablesList->newIterator();
  while (i.elementExists()) {
    DatumP varnameP = i.element();
    QString varname = varnameP.wordValue()->keyValue();
    DatumP value = variables.datumForName(varname);
    if (value == nothing)
      Error::noValue(varnameP);
    QString line =
        QString("Make \"%1 %2\n").arg(varname).arg(parser->unreadDatum(value));
    stdPrint(line);
  }

  i = propertiesList->newIterator();
  while (i.elementExists()) {
    DatumP listnameP = i.element();
    QString listname = listnameP.wordValue()->keyValue();
    DatumP proplist = plists.getPropertyList(listname);
    if (proplist.listValue()->size() > 0) {
      QString line = QString("Plist %1 = %2\n")
                         .arg(parser->unreadDatum(listnameP))
                         .arg(parser->unreadDatum(proplist, true));
      stdPrint(line);
    }
  }

  return nothing;
}

// WORKSPACE CONTROL

DatumP Kernel::excErase(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumP candidate) {
    contentslist = contentslistFromDatumP(candidate);
    return contentslist != nothing;
  });

  List *proceduresList = contentslist.listValue()->datumAtIndex(1).listValue();
  List *variablesList = contentslist.listValue()->datumAtIndex(2).listValue();
  List *propertiesList = contentslist.listValue()->datumAtIndex(3).listValue();

  ListIterator i = proceduresList->newIterator();
  while (i.elementExists()) {
    DatumP nameP = i.element();
    parser->eraseProcedure(nameP);
  }

  i = variablesList->newIterator();
  while (i.elementExists()) {
    QString varname = i.element().wordValue()->keyValue();
    variables.eraseVar(varname);
  }

  i = propertiesList->newIterator();
  while (i.elementExists()) {
    DatumP listnameP = i.element();
    QString listname = listnameP.wordValue()->keyValue();
    plists.erasePropertyList(listname);
  }

  return nothing;
}

DatumP Kernel::excErall(DatumP node) {
  ProcedureHelper h(this, node);
  parser->eraseAllProcedures();
  variables.eraseAll();
  plists.eraseAll();

  return nothing;
}

DatumP Kernel::excErps(DatumP node) {
  ProcedureHelper h(this, node);
  parser->eraseAllProcedures();

  return nothing;
}

DatumP Kernel::excErns(DatumP node) {
  ProcedureHelper h(this, node);
  variables.eraseAll();

  return nothing;
}

DatumP Kernel::excErpls(DatumP node) {
  ProcedureHelper h(this, node);
  plists.eraseAll();

  return nothing;
}

DatumP Kernel::excBury(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumP candidate) {
    contentslist = contentslistFromDatumP(candidate);
    return contentslist != nothing;
  });

  processContentsListWithMethod(contentslist, &Workspace::bury);

  return nothing;
}

DatumP Kernel::excUnbury(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumP candidate) {
    contentslist = contentslistFromDatumP(candidate);
    return contentslist != nothing;
  });

  processContentsListWithMethod(contentslist, &Workspace::unbury);

  return nothing;
}

DatumP Kernel::excBuriedp(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP retval;
  h.validatedDatumAtIndex(0, [&retval, this](DatumP candidate) {
    DatumP contentslist = contentslistFromDatumP(candidate);
    if (contentslist == nothing)
      return false;
    retval = queryContentsListWithMethod(contentslist, &Workspace::isBuried);

    return retval != nothing;
  });

  return h.ret(retval);
}

DatumP Kernel::excTrace(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumP candidate) {
    contentslist = contentslistFromDatumP(candidate);
    return contentslist != nothing;
  });

  processContentsListWithMethod(contentslist, &Workspace::trace);

  return nothing;
}

DatumP Kernel::excUntrace(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumP candidate) {
    contentslist = contentslistFromDatumP(candidate);
    return contentslist != nothing;
  });

  processContentsListWithMethod(contentslist, &Workspace::untrace);

  return nothing;
}

DatumP Kernel::excTracedp(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP retval;
  h.validatedDatumAtIndex(0, [&retval, this](DatumP candidate) {
    DatumP contentslist = contentslistFromDatumP(candidate);
    if (contentslist == nothing)
      return false;
    retval = queryContentsListWithMethod(contentslist, &Workspace::isTraced);

    return retval != nothing;
  });

  return h.ret(retval);
}

DatumP Kernel::excStep(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumP candidate) {
    contentslist = contentslistFromDatumP(candidate);
    return contentslist != nothing;
  });

  processContentsListWithMethod(contentslist, &Workspace::step);

  return nothing;
}

DatumP Kernel::excUnstep(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumP candidate) {
    contentslist = contentslistFromDatumP(candidate);
    return contentslist != nothing;
  });

  processContentsListWithMethod(contentslist, &Workspace::unstep);

  return nothing;
}

DatumP Kernel::excSteppedp(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP retval;
  h.validatedDatumAtIndex(0, [&retval, this](DatumP candidate) {
    DatumP contentslist = contentslistFromDatumP(candidate);
    if (contentslist == nothing)
      return false;
    retval = queryContentsListWithMethod(contentslist, &Workspace::isStepped);

    return retval != nothing;
  });

  return h.ret(retval);
}

DatumP Kernel::excEdit(DatumP node) {
  ProcedureHelper h(this, node);
  if (h.countOfChildren() > 0) {
    DatumP contentslist;
    h.validatedDatumAtIndex(0, [&contentslist, this](DatumP candidate) {
      contentslist = contentslistFromDatumP(candidate);
      return contentslist != nothing;
    });

    workspaceText = createPrintoutFromContentsList(contentslist, false);

    editAndRunWorkspaceText();
  } else if (editFileName.isWord() &&
             editFileName.wordValue()->printValue() != "") {
    editAndRunFile();
  } else {
    editAndRunWorkspaceText();
  }

  return nothing;
}

DatumP Kernel::excEditfile(DatumP node) {
  ProcedureHelper h(this, node);
  editFileName = h.wordAtIndex(0);
  editAndRunFile();
  return nothing;
}

DatumP Kernel::excSave(DatumP node) {
  ProcedureHelper h(this, node);
  if (h.countOfChildren() > 0) {
    editFileName = h.wordAtIndex(0);
  } else {
    if (editFileName == nothing)
      Error::notEnough(node.astnodeValue()->nodeName);
  }

  QString filepath = filepathForFilename(editFileName.wordValue());
  QFile file(filepath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    Error::cantOpen(editFileName);
  }

  DatumP contentList = buildContentsList(showUnburied);
  QString fileText = createPrintoutFromContentsList(contentList);

  file.seek(0);
  file.resize(0);
  QTextStream out(&file);
  out << fileText;

  return nothing;
}

DatumP Kernel::excLoad(DatumP node) {
  ProcedureHelper h(this, node);
  editFileName = h.wordAtIndex(0);
  DatumP oldStartup = varSTARTUP();
  DatumP retval;

  QString filepath = filepathForFilename(editFileName.wordValue());
  QFile file(filepath);
  if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
    Error::cantOpen(editFileName);
  }
  QTextStream in(&file);
  QString fileText = in.readAll();
  QString output = executeText(fileText);
  if (varLOADNOISILY()) {
    sysPrint(output);
  }
  DatumP startup = varSTARTUP();
  if (oldStartup != startup) {
    if (startup.isWord() || startup.isList())
      retval = runList(startup);
  }

  return h.ret(retval);
}

DatumP Kernel::excHelp(DatumP node) {
  ProcedureHelper h(this, node);
  if (h.countOfChildren() > 0) {
    QString cmdName = h.wordAtIndex(0).wordValue()->keyValue();
    DatumP textP = help.helpForKeyword(cmdName);
    if (textP == nothing) {
      QString message = QString("No help available on %1\n")
                            .arg(h.wordAtIndex(0).wordValue()->printValue());
      sysPrint(message);
    } else {
      QString message = help.helpForKeyword(cmdName).wordValue()->printValue();
      sysPrint(message);
    }
  } else {
    DatumP keywordsP = help.allKeywords();
    ListIterator iter = keywordsP.listValue()->newIterator();
    while (iter.elementExists()) {
      sysPrint(iter.element().wordValue()->printValue() + "\n");
    }
  }

  return nothing;
}
