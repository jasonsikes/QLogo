
//===-- qlogo/kernel.cpp - Kernel class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Kernel class, which is the
/// executor proper of the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "error.h"
#include "kernel.h"
#include "parser.h"
#include "datum_word.h"
#include "datum_astnode.h"
#include "stringconstants.h"
#include "logocontroller.h"

QString Kernel::executeText(const QString &text) {
  QString inText = text;
  QString outText;

  QTextStream inQStream(&inText, QIODevice::ReadOnly);
  QTextStream outQStream(&outText, QIODevice::WriteOnly);

  TextStream inStream(&inQStream);
  TextStream outStream(&outQStream);

  StreamRedirect sr(this, &inStream, &outStream);

  bool shouldContinue = true;
  while (shouldContinue) {
    shouldContinue = getLineAndRunIt(false);
  }
  outStream.flush();
  return outText;
}

void Kernel::editAndRunWorkspaceText() {
  const QString textRetval = mainController()->editText(workspaceText);
  if (textRetval != workspaceText) {
    workspaceText = textRetval;
    QString output = executeText(textRetval);
    if (varLOADNOISILY()) {
      sysPrint(output);
    }
  }
}

void Kernel::editAndRunFile() {
  QString filepath = filepathForFilename(editFileName);
  QFile file(filepath);
  if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
    Error::cantOpen(editFileName);
  }
  QTextStream in(&file);
  QString fileText = in.readAll();

  const QString textRetval = mainController()->editText(fileText);
  if (textRetval != "") {
    fileText = textRetval;
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

DatumPtr Kernel::buildContentsList(showContents_t showWhat) {
  List *retval = List::alloc();
  retval->append(parser->allProcedureNames(showWhat));
  retval->append(variables.allVariables(showWhat));
  retval->append(plists.allPLists(showWhat));
  return retval;
}

DatumPtr Kernel::contentslistFromDatumPtr(DatumPtr sourceNode) {
  List *sublists[3];
  DatumPtr locker[3];
  for (int i = 0; i < 3; ++i) {
    sublists[i] = List::alloc();
    locker[i] = DatumPtr(sublists[i]);
  }

  if (sourceNode.isWord()) {
    sublists[0]->append(sourceNode);
  } else if (sourceNode.isList()) {
    unsigned parseLevel = 0;
    ListIterator i = sourceNode.listValue()->newIterator();
    while (i.elementExists()) {
      if (parseLevel > 2)
        return nothing;
      DatumPtr d = i.element();
      if (d.isWord()) {
        sublists[parseLevel]->append(d);
      } else if (d.isList()) {
        ListIterator j = d.listValue()->newIterator();
        while (j.elementExists()) {
          DatumPtr e = j.element();
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

  List *retval = List::alloc();
  for (int i = 0; i < 3; ++i) {
    retval->append(DatumPtr(sublists[i]));
  }
  return DatumPtr(retval);
}

void Kernel::processContentsListWithMethod(
    DatumPtr contentslist, void (Workspace::*method)(const QString &)) {
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
    DatumPtr listnameP = i.element();
    QString listname = listnameP.wordValue()->keyValue();
    (plists.*method)(listname);
  }
}

DatumPtr Kernel::queryContentsListWithMethod(
    DatumPtr contentslist, bool (Workspace::*method)(const QString &)) {
  List *proceduresList = contentslist.listValue()->datumAtIndex(1).listValue();

  if (proceduresList->size() > 0) {
    QString procname = proceduresList->first().wordValue()->keyValue();
    return DatumPtr((parser->*method)(procname));
  }

  List *variablesList = contentslist.listValue()->datumAtIndex(2).listValue();

  if (variablesList->size() > 0) {
    QString varname = variablesList->first().wordValue()->keyValue();
    return DatumPtr((variables.*method)(varname));
  }

  List *propertiesList = contentslist.listValue()->datumAtIndex(3).listValue();

  if (propertiesList->size() > 0) {
    QString pname = propertiesList->first().wordValue()->keyValue();
    return DatumPtr((plists.*method)(pname));
  }
  return nothing;
}

QString Kernel::createPrintoutFromContentsList(DatumPtr contentslist,
                                               bool shouldValidate) {
  QString retval("");

  List *proceduresList = contentslist.listValue()->datumAtIndex(1).listValue();
  List *variablesList = contentslist.listValue()->datumAtIndex(2).listValue();
  List *propertiesList = contentslist.listValue()->datumAtIndex(3).listValue();

  ListIterator i = proceduresList->newIterator();
  while (i.elementExists()) {
    DatumPtr procedureText =
        parser->procedureFulltext(i.element(), shouldValidate);
    ListIterator j = procedureText.listValue()->newIterator();
    while (j.elementExists()) {
      QString line = j.element().wordValue()->printValue();
      line.append('\n');
      retval += line;
    }
  }

  i = variablesList->newIterator();
  while (i.elementExists()) {
    DatumPtr varnameP = i.element();
    QString varname = varnameP.wordValue()->keyValue();
    DatumPtr value = variables.datumForName(varname);
    if ((value == nothing) && shouldValidate) {
      Error::noValue(varnameP);
    } else {
      QString line = k.make12().arg(varname,
                                    parser->printoutDatum(value));
      retval += line;
    }
  }

  i = propertiesList->newIterator();
  while (i.elementExists()) {
    DatumPtr listnameP = i.element();
    QString listname = listnameP.wordValue()->keyValue();
    DatumPtr proplist = plists.getPropertyList(listname);
    ListIterator j = proplist.listValue()->newIterator();
    while (j.elementExists()) {
      DatumPtr nameP = j.element();
      DatumPtr valueP = j.element();
      QString line = k.pprop123()
                         .arg(parser->printoutDatum(listnameP),
                              parser->printoutDatum(nameP),
                              parser->printoutDatum(valueP));
      retval += line;
    }
  }
  return retval;
}

// SPECIAL VARIABLES

bool Kernel::varLOADNOISILY() {
  DatumPtr retvalP = variables.datumForName(k.loadnoisily());
  if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == k.kctrue()))
    return true;
  return false;
}

bool Kernel::varALLOWGETSET() {
  DatumPtr retvalP = variables.datumForName(k.allowGetSet());
  if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == k.kctrue()))
    return true;
  return false;
}

DatumPtr Kernel::varBUTTONACT() { return variables.datumForName(k.buttonact()); }

DatumPtr Kernel::varKEYACT() { return variables.datumForName(k.keyact()); }

bool Kernel::varFULLPRINTP() {
  DatumPtr retvalP = variables.datumForName(k.fullprintp());
  if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == k.kctrue()))
    return true;
  return false;
}

int Kernel::varPRINTDEPTHLIMIT() {
  DatumPtr retvalP = variables.datumForName(k.printdepthlimit());
  if (retvalP.isWord()) {
    double retval = retvalP.wordValue()->numberValue();
    if (retvalP.wordValue()->didNumberConversionSucceed()) {
      return (int)retval;
    }
  }
  return -1;
}

int Kernel::varPRINTWIDTHLIMIT() {
  DatumPtr retvalP = variables.datumForName(k.printwidthlimit());
  if (retvalP.isWord()) {
    double retval = retvalP.wordValue()->numberValue();
    if (retvalP.wordValue()->didNumberConversionSucceed()) {
      return (int)retval;
    }
  }
  return -1;
}

DatumPtr Kernel::varSTARTUP() { return variables.datumForName(k.startup()); }

bool Kernel::varUNBURYONEDIT() {
  DatumPtr retvalP = variables.datumForName(k.unburyonedit());
  if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == k.kctrue()))
    return true;
  return false;
}

bool Kernel::varCASEIGNOREDP() {
  DatumPtr retvalP = variables.datumForName(k.caseignoredp());
  if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == k.kctrue()))
    return true;
  return false;
}

// PROCEDURE DEFINITION

DatumPtr Kernel::excTo(DatumPtr node) {
  // None of the children of node are ASTNode. They have to be literal so there
  // is no procedurehelper here.
  if (currentProcedure != nothing) {
    Error::toInProc(node.astnodeValue()->nodeName);
  }
  parser->inputProcedure(node, systemReadStream);
  return nothing;
}

DatumPtr Kernel::excDefine(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr text = h.validatedListAtIndex(1, [](DatumPtr candidate) {
    ListIterator iter = candidate.listValue()->newIterator();
    while (iter.elementExists()) {
      DatumPtr line = iter.element();
      if (!line.isList())
        return false;
    }
    return true;
  });
  DatumPtr cmd = node.astnodeValue()->nodeName;
  DatumPtr procnameP = h.wordAtIndex(0);

  parser->defineProcedure(cmd, procnameP, text, nothing);

  return nothing;
}

DatumPtr Kernel::excText(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr procnameP = h.wordAtIndex(0);
  return h.ret(parser->procedureText(procnameP));
}

DatumPtr Kernel::excFulltext(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr procnameP = h.wordAtIndex(0);
  return h.ret(parser->procedureFulltext(procnameP));
}

DatumPtr Kernel::excCopydef(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr newname = h.wordAtIndex(0);
  DatumPtr oldname = h.wordAtIndex(1);

  parser->copyProcedure(newname, oldname);

  return nothing;
}

// VARIABLE DEFINITION

DatumPtr Kernel::excMake(DatumPtr node) {
  ProcedureHelper h(this, node);

  QString lvalue = h.wordAtIndex(0).wordValue()->keyValue();
  DatumPtr rvalue = h.datumAtIndex(1);

  variables.setDatumForName(rvalue, lvalue);

  if (variables.isTraced(lvalue)) {
    QString line = k.make12()
                       .arg(h.wordAtIndex(0).wordValue()->printValue(),
                            parser->unreadDatum(rvalue));
    sysPrint(line);
  }

  return nothing;
}

DatumPtr Kernel::excSetfoo(DatumPtr node) {
  ProcedureHelper h(this, node);

  DatumPtr nodeName = node.astnodeValue()->nodeName;
  QString foo = nodeName.wordValue()->keyValue();

  QString lvalue = foo.right(foo.size() - 3);
  DatumPtr rvalue = h.datumAtIndex(0);

  if (!variables.doesExist(lvalue)) {
    Error::noHow(nodeName);
  }

  variables.setDatumForName(rvalue, lvalue);

  if (variables.isTraced(lvalue.toUpper())) {
    QString line =
        QString("%1 %2\n")
            .arg(node.astnodeValue()->nodeName.wordValue()->printValue(),
            parser->unreadDatum(rvalue));
    sysPrint(line);
  }

  return nothing;
}

DatumPtr Kernel::excFoo(DatumPtr node) {
  DatumPtr fooP = node.astnodeValue()->nodeName;
  QString foo = fooP.wordValue()->keyValue();

  DatumPtr retval = variables.datumForName(foo);
  if (retval == nothing)
    return Error::noHowRecoverable(fooP);
  return retval;
}

DatumPtr Kernel::excLocal(DatumPtr node) {
  ProcedureHelper h(this, node);
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumPtr var = h.validatedDatumAtIndex(i, [](DatumPtr candidate) {
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
        DatumPtr v = j.element();
        makeVarLocal(v.wordValue()->keyValue());
      }
    }
  }
  return nothing;
}

DatumPtr Kernel::excThing(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString varName = h.wordAtIndex(0).wordValue()->keyValue();
  DatumPtr retval = h.ret(variables.datumForName(varName));
  if (retval == nothing)
    return h.ret(Error::noValueRecoverable(h.datumAtIndex(0)));
  return retval;
}

DatumPtr Kernel::excGlobal(DatumPtr node) {
  ProcedureHelper h(this, node);
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumPtr var = h.validatedDatumAtIndex(i, [](DatumPtr candidate) {
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
        DatumPtr v = j.element();
        variables.setVarAsGlobal(v.wordValue()->keyValue());
      }
    }
  }
  return nothing;
}

// PROPERTY LISTS

DatumPtr Kernel::excPprop(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString plistname = h.wordAtIndex(0).wordValue()->keyValue();
  QString propname = h.wordAtIndex(1).wordValue()->keyValue();
  DatumPtr value = h.datumAtIndex(2);
  plists.addProperty(plistname, propname, value);
  if (plists.isTraced(plistname)) {
    QString line = k.pprop123()
                       .arg(parser->unreadDatum(h.datumAtIndex(0)),
                            parser->unreadDatum(h.datumAtIndex(1)),
                            parser->unreadDatum(value));
    sysPrint(line);
  }
  return nothing;
}

DatumPtr Kernel::excGprop(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString plistname = h.wordAtIndex(0).wordValue()->keyValue();
  QString propname = h.wordAtIndex(1).wordValue()->keyValue();
  return h.ret(plists.getProperty(plistname, propname));
}

DatumPtr Kernel::excRemprop(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString plistname = h.wordAtIndex(0).wordValue()->keyValue();
  QString propname = h.wordAtIndex(1).wordValue()->keyValue();
  plists.removeProperty(plistname, propname);

  return nothing;
}

DatumPtr Kernel::excPlist(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString plistname = h.wordAtIndex(0).wordValue()->keyValue();
  return h.ret(plists.getPropertyList(plistname));
}

// PREDICATES

DatumPtr Kernel::excProcedurep(DatumPtr node) {
  ProcedureHelper h(this, node);
  bool retval = parser->isProcedure(h.wordAtIndex(0).wordValue()->keyValue());
  return h.ret(retval);
}

DatumPtr Kernel::excPrimitivep(DatumPtr node) {
  ProcedureHelper h(this, node);
  bool retval = parser->isPrimitive(h.wordAtIndex(0).wordValue()->keyValue());
  return h.ret(retval);
}

DatumPtr Kernel::excDefinedp(DatumPtr node) {
  ProcedureHelper h(this, node);
  bool retval = parser->isDefined(h.wordAtIndex(0).wordValue()->keyValue());
  return h.ret(retval);
}

DatumPtr Kernel::excNamep(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString varname = h.wordAtIndex(0).wordValue()->keyValue();
  bool retval = (variables.doesExist(varname));
  return h.ret(retval);
}

DatumPtr Kernel::excPlistp(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString listName = h.wordAtIndex(0).wordValue()->keyValue();
  bool retval = plists.isPropertyList(listName);
  return h.ret(retval);
}

// QUERIES

DatumPtr Kernel::excContents(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(buildContentsList(showUnburied));
}

DatumPtr Kernel::excBuried(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(buildContentsList(showBuried));
}

DatumPtr Kernel::excTraced(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(buildContentsList(showTraced));
}

DatumPtr Kernel::excStepped(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(buildContentsList(showStepped));
}

DatumPtr Kernel::excProcedures(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(parser->allProcedureNames(showUnburied));
}

DatumPtr Kernel::excPrimitives(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(parser->allPrimitiveProcedureNames());
}

DatumPtr Kernel::excNames(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = List::alloc();
  retval->append(DatumPtr(List::alloc()));
  retval->append(variables.allVariables(showUnburied));
  return h.ret(retval);
}

DatumPtr Kernel::excPlists(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = List::alloc();
  retval->append(DatumPtr(List::alloc()));
  retval->append(DatumPtr(List::alloc()));
  retval->append(plists.allPLists(showUnburied));
  return h.ret(retval);
}

DatumPtr Kernel::excArity(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(parser->arity(h.wordAtIndex(0)));
}

DatumPtr Kernel::excNodes(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(nodes());
}

// INSPECTION

DatumPtr Kernel::excPrintout(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
    contentslist = contentslistFromDatumPtr(candidate);
    return contentslist != nothing;
  });

  QString output = createPrintoutFromContentsList(contentslist);
  stdPrint(output);

  return nothing;
}

DatumPtr Kernel::excPot(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
    contentslist = contentslistFromDatumPtr(candidate);
    return contentslist != nothing;
  });

  List *proceduresList = contentslist.listValue()->datumAtIndex(1).listValue();
  List *variablesList = contentslist.listValue()->datumAtIndex(2).listValue();
  List *propertiesList = contentslist.listValue()->datumAtIndex(3).listValue();

  ListIterator i = proceduresList->newIterator();
  while (i.elementExists()) {
    QString procedureTitle = parser->procedureTitle(i.element());
    stdPrint(procedureTitle);
    stdPrint("\n");
  }

  i = variablesList->newIterator();
  while (i.elementExists()) {
    DatumPtr varnameP = i.element();
    QString varname = varnameP.wordValue()->keyValue();
    DatumPtr value = variables.datumForName(varname);
    if (value == nothing)
      Error::noValue(varnameP);
    QString line = k.make12().arg(varname, parser->unreadDatum(value));
    stdPrint(line);
  }

  i = propertiesList->newIterator();
  while (i.elementExists()) {
    DatumPtr listnameP = i.element();
    QString listname = listnameP.wordValue()->keyValue();
    DatumPtr proplist = plists.getPropertyList(listname);
    if (proplist.listValue()->size() > 0) {
      QString line = k.plist12()
                         .arg(parser->unreadDatum(listnameP),
                              parser->unreadDatum(proplist, true));
      stdPrint(line);
    }
  }

  return nothing;
}

// WORKSPACE CONTROL

DatumPtr Kernel::excErase(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
    contentslist = contentslistFromDatumPtr(candidate);
    return contentslist != nothing;
  });

  List *proceduresList = contentslist.listValue()->datumAtIndex(1).listValue();
  List *variablesList = contentslist.listValue()->datumAtIndex(2).listValue();
  List *propertiesList = contentslist.listValue()->datumAtIndex(3).listValue();

  ListIterator i = proceduresList->newIterator();
  while (i.elementExists()) {
    DatumPtr nameP = i.element();
    parser->eraseProcedure(nameP);
  }

  i = variablesList->newIterator();
  while (i.elementExists()) {
    QString varname = i.element().wordValue()->keyValue();
    variables.eraseVar(varname);
  }

  i = propertiesList->newIterator();
  while (i.elementExists()) {
    DatumPtr listnameP = i.element();
    QString listname = listnameP.wordValue()->keyValue();
    plists.erasePropertyList(listname);
  }

  return nothing;
}

DatumPtr Kernel::excErall(DatumPtr node) {
  ProcedureHelper h(this, node);
  parser->eraseAllProcedures();
  variables.eraseAll();
  plists.eraseAll();

  return nothing;
}

DatumPtr Kernel::excErps(DatumPtr node) {
  ProcedureHelper h(this, node);
  parser->eraseAllProcedures();

  return nothing;
}

DatumPtr Kernel::excErns(DatumPtr node) {
  ProcedureHelper h(this, node);
  variables.eraseAll();

  return nothing;
}

DatumPtr Kernel::excErpls(DatumPtr node) {
  ProcedureHelper h(this, node);
  plists.eraseAll();

  return nothing;
}

DatumPtr Kernel::excBury(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
    contentslist = contentslistFromDatumPtr(candidate);
    return contentslist != nothing;
  });

  processContentsListWithMethod(contentslist, &Workspace::bury);

  return nothing;
}

DatumPtr Kernel::excUnbury(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
    contentslist = contentslistFromDatumPtr(candidate);
    return contentslist != nothing;
  });

  processContentsListWithMethod(contentslist, &Workspace::unbury);

  return nothing;
}

DatumPtr Kernel::excBuriedp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval;
  h.validatedDatumAtIndex(0, [&retval, this](DatumPtr candidate) {
    DatumPtr contentslist = contentslistFromDatumPtr(candidate);
    if (contentslist == nothing)
      return false;
    retval = queryContentsListWithMethod(contentslist, &Workspace::isBuried);

    return retval != nothing;
  });

  return h.ret(retval);
}

DatumPtr Kernel::excTrace(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
    contentslist = contentslistFromDatumPtr(candidate);
    return contentslist != nothing;
  });

  processContentsListWithMethod(contentslist, &Workspace::trace);

  return nothing;
}

DatumPtr Kernel::excUntrace(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
    contentslist = contentslistFromDatumPtr(candidate);
    return contentslist != nothing;
  });

  processContentsListWithMethod(contentslist, &Workspace::untrace);

  return nothing;
}

DatumPtr Kernel::excTracedp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval;
  h.validatedDatumAtIndex(0, [&retval, this](DatumPtr candidate) {
    DatumPtr contentslist = contentslistFromDatumPtr(candidate);
    if (contentslist == nothing)
      return false;
    retval = queryContentsListWithMethod(contentslist, &Workspace::isTraced);

    return retval != nothing;
  });

  return h.ret(retval);
}

DatumPtr Kernel::excStep(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
    contentslist = contentslistFromDatumPtr(candidate);
    return contentslist != nothing;
  });

  processContentsListWithMethod(contentslist, &Workspace::step);

  return nothing;
}

DatumPtr Kernel::excUnstep(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr contentslist;
  h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
    contentslist = contentslistFromDatumPtr(candidate);
    return contentslist != nothing;
  });

  processContentsListWithMethod(contentslist, &Workspace::unstep);

  return nothing;
}

DatumPtr Kernel::excSteppedp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval;
  h.validatedDatumAtIndex(0, [&retval, this](DatumPtr candidate) {
    DatumPtr contentslist = contentslistFromDatumPtr(candidate);
    if (contentslist == nothing)
      return false;
    retval = queryContentsListWithMethod(contentslist, &Workspace::isStepped);

    return retval != nothing;
  });

  return h.ret(retval);
}

DatumPtr Kernel::excEdit(DatumPtr node) {
  ProcedureHelper h(this, node);
  if (h.countOfChildren() > 0) {
    DatumPtr contentslist;
    h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
      contentslist = contentslistFromDatumPtr(candidate);
      return contentslist != nothing;
    });

    workspaceText = createPrintoutFromContentsList(contentslist, false);

    editAndRunWorkspaceText();
  } else if (editFileName.isWord() &&
             editFileName.wordValue()->printValue() != "") {
    editAndRunFile();
  } else {
      workspaceText = "";
    editAndRunWorkspaceText();
  }

  return nothing;
}

DatumPtr Kernel::excEditfile(DatumPtr node) {
  ProcedureHelper h(this, node);
  editFileName = h.wordAtIndex(0);
  editAndRunFile();
  return nothing;
}

DatumPtr Kernel::excSave(DatumPtr node) {
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

  DatumPtr contentList = buildContentsList(showUnburied);
  QString fileText = createPrintoutFromContentsList(contentList);

  file.seek(0);
  file.resize(0);
  QTextStream out(&file);
  out << fileText;

  return nothing;
}

DatumPtr Kernel::excLoad(DatumPtr node) {
  ProcedureHelper h(this, node);
  editFileName = h.wordAtIndex(0);
  DatumPtr oldStartup = varSTARTUP();
  DatumPtr retval;

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
  DatumPtr startup = varSTARTUP();
  if (oldStartup != startup) {
    if (startup.isWord() || startup.isList())
      retval = runList(startup);
  }

  return h.ret(retval);
}

DatumPtr Kernel::excHelp(DatumPtr node) {
  ProcedureHelper h(this, node);

  sysPrint("Sorry, help is not available in this version of QLogo.\n"
	   "The UCBLogo manual, from which QLogo is based, is available\n"
	   "at https://people.eecs.berkeley.edu/~bh/usermanual\n");

  return nothing;
}
