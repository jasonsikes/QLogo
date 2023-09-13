
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

// CONTROL STRUCTURES

DatumP Kernel::excRun(DatumP node) {
  ProcedureHelper h(this, node);

  DatumP instructionList = h.validatedDatumAtIndex(0, [](DatumP candidate) {
    return candidate.isWord() || candidate.isList();
  });

  return h.ret(runList(instructionList));
}

DatumP Kernel::excRunresult(DatumP node) {
  ProcedureHelper h(this, node);

  DatumP instructionList = h.validatedDatumAtIndex(0, [](DatumP candidate) {
    return candidate.isWord() || candidate.isList();
  });

  List* retval = emptyList();
  DatumP temp = runList(instructionList);

  if (temp.isASTNode()) {
    temp = Error::insideRunresult(temp.astnodeValue()->nodeName);
  }

  if (temp != nothing) {
    retval->append(temp);
  }

  return h.ret(retval);
}

DatumP Kernel::excBye(DatumP node) {
  ProcedureHelper h(this, node);

  Error::throwError(DatumP(k.system()), nothing);

  return nothing;
}

DatumP Kernel::excRepeat(DatumP node) {
  ProcedureHelper h(this, node);
  int countValue = h.validatedIntegerAtIndex(
      0, [](int candidate) { return candidate >= 0; });
  DatumP commandList =
      h.listAtIndex(1); // TODO: this can execute a word, too, right?

  int tempRepcount = repcount;
  repcount = 1;

  DatumP retval;
  try {
    while ((countValue > 0) && (retval == nothing)) {
      retval = runList(commandList);
      --countValue;
      ++repcount;
    }
  } catch (Error *e) {
    // TODO: RAII
    repcount = tempRepcount;
    throw e;
  }
  repcount = tempRepcount;
  return h.ret(retval);
}

DatumP Kernel::excForever(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP commandList = h.listAtIndex(0);

  int tempRepcount = repcount;
  repcount = 1;

  DatumP retval;
  try {
    while (retval == nothing) {
      retval = runList(commandList);
      ++repcount;
    }
  } catch (Error *e) {
    repcount = tempRepcount;
    throw e;
  }
  repcount = tempRepcount;
  return h.ret(retval);
}

DatumP Kernel::excRepcount(DatumP node) {
  ProcedureHelper h(this, node);

  return h.ret(repcount);
}

DatumP Kernel::excIf(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP retval;
  if (h.boolAtIndex(0, true)) {
    retval = runList(h.datumAtIndex(1));
  }
  return h.ret(retval);
}

DatumP Kernel::excIfelse(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP retval;
  if (h.boolAtIndex(0, true)) {
    retval = runList(h.datumAtIndex(1));
  } else {
    retval = runList(h.datumAtIndex(2));
  }
  return h.ret(retval);
}

DatumP Kernel::excTest(DatumP node) {
  ProcedureHelper h(this, node);

  bool testVal = h.boolAtIndex(0, true);
  variables.setTest(testVal);
  return nothing;
}

DatumP Kernel::excIftrue(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP retval;
  if (!variables.isTested())
    return h.ret(Error::noTest(node.astnodeValue()->nodeName));
  if (variables.testedState()) {
    retval = runList(h.datumAtIndex(0));
  }
  return h.ret(retval);
}

DatumP Kernel::excIffalse(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP retval;
  if (!variables.isTested())
    return h.ret(Error::noTest(node.astnodeValue()->nodeName));
  if ( ! variables.testedState()) {
    retval = runList(h.datumAtIndex(0));
  }
  return h.ret(retval);
}

// The commands STOP, OUTPUT, and .MAYBEOUTPUT return an ASTNode instead of a
// Word, List, or Array.
//
// The caller is responsible for dissecting the node and acting appropriately.
//

DatumP Kernel:: excStop(DatumP node) {
  if (currentProcedure == nothing) {
    Error::notInsideProcedure(node.astnodeValue()->nodeName);
  }
  return node;
}

DatumP Kernel::excOutput(DatumP node) {
  if (currentProcedure == nothing) {
    Error::notInsideProcedure(node.astnodeValue()->nodeName);
  }

  return node;
}

DatumP Kernel::excDotMaybeoutput(DatumP node) {
  if (currentProcedure == nothing) {
    Error::notInsideProcedure(node.astnodeValue()->nodeName);
  }
  return node;
}

DatumP Kernel::excCatch(DatumP node) {
  ProcedureHelper h(this, node);
  QString tag = h.wordAtIndex(0).wordValue()->keyValue();
  DatumP instructionlist = h.listAtIndex(1);
  DatumP retval;
  DatumP tempErract = variables.datumForName(k.erract());

  if (variables.doesExist(k.erract())) {
    variables.setDatumForName(nothing, k.erract());
  }

  try {
    retval = runList(instructionlist);
    if (retval.isASTNode()) {
        KernelMethod method = retval.astnodeValue()->kernel;
        if (method == &Kernel::excStop) {
            retval = nothing;
          } else if ((method == &Kernel::excOutput) || (method == &Kernel::excDotMaybeoutput) ||
                     ((method == &Kernel::excStop) && (retval.astnodeValue()->countOfChildren() > 0))) {
            DatumP p = retval.astnodeValue()->childAtIndex(0);
            KernelMethod temp_method = p.astnodeValue()->kernel;
            DatumP temp_retval = (this->*temp_method)(p);
            if ((temp_retval == nothing) && (method == &Kernel::excOutput)) {
                Error::didntOutput(p.astnodeValue()->nodeName,
                                   retval.astnodeValue()->nodeName);
              }
            if ((temp_retval != nothing) && (method == &Kernel::excStop)) {
                Error::dontSay(retval.astnodeValue()->nodeName);
            }
            retval = temp_retval;
          } else {
            retval = (this->*method)(retval);
          }
      }
  } catch (Error *e) {
    if (variables.doesExist(k.erract())) {
      variables.setDatumForName(tempErract, k.erract());
    }

    if ((tag == k.error()) &&
        (((e->code == 14) && (e->tag.wordValue()->keyValue()) == k.error()) ||
         (e->code != 14))) {
      ProcedureHelper::setIsErroring(false);
      return nothing;
    } else if ((e->code == 14) && (tag == e->tag.wordValue()->keyValue())) {
      DatumP retval = e->output;
      registerError(nothing);
      return h.ret(retval);
    }
    throw e;
  }

  if (variables.doesExist(k.erract())) {
    variables.setDatumForName(tempErract, k.erract());
  }
  return h.ret(retval);
}

DatumP Kernel::excThrow(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP tag = h.wordAtIndex(0);
  DatumP value;
  if (h.countOfChildren() > 1) {
    value = h.datumAtIndex(1);
    if (!value.isWord())
      value = DatumP(value.printValue());
  }

  Error::throwError(tag, value);

  return nothing;
}

DatumP Kernel::excError(DatumP node) {
  ProcedureHelper h(this, node);

  List *retval = emptyList();
  if (currentError != nothing) {
    Error *e = currentError.errorValue();
    retval->append(DatumP(e->code));
    retval->append(e->errorText);
    if (e->procedure != nothing)
      retval->append(e->procedure.astnodeValue()->nodeName);
    else
      retval->append(emptyListP());
    if (e->instructionLine != nothing)
      retval->append(e->instructionLine);
    else
      retval->append(emptyListP());
    currentError = nothing;
  }
  return h.ret(retval);
}

DatumP Kernel::excPause(DatumP node) {
  ProcedureHelper h(this, node);
  if (currentProcedure == nothing) {
      Error::notInsideProcedure(node.astnodeValue()->nodeName);
  }
  return h.ret(pause());
}

DatumP Kernel::excContinue(DatumP node) {
  ProcedureHelper h(this, node);

  DatumP retval;
  if (h.countOfChildren() > 0) {
    retval = h.datumAtIndex(0);
    if (!retval.isWord()) {
      retval = DatumP(retval.printValue());
    }
  }

  Error::throwError(DatumP(k.pause()), retval);

  return nothing;
}

DatumP Kernel::excTag(DatumP) { return nothing; }

DatumP Kernel::excGoto(DatumP node) {
  ProcedureHelper h(this, node);
  if (currentProcedure == nothing)
    Error::notInsideProcedure(node.astnodeValue()->nodeName);
  DatumP tagP = h.validatedDatumAtIndex(0, [this](DatumP candidate) {
    if (!candidate.isWord())
      return false;
    QString tag = candidate.wordValue()->keyValue();
    return currentProcedure.astnodeValue()
        ->childAtIndex(0)
        .procedureValue()
        ->tagToLine.contains(tag);
  });
  ASTNode *a = new ASTNode(k.kgoto());
  a->kernel = &Kernel::excGotoToken;
  a->addChild(tagP);
  return DatumP(a);
}

// TEMPLATE-BASED ITERATION

DatumP Kernel::excApply(DatumP node) {
  ProcedureHelper h(this, node);
  enum Form { explicit_slot, named_procedure, lambda, procedure };
  Form f;

  DatumP tmplate = h.validatedDatumAtIndex(0, [&f](DatumP candidate) {
    if (candidate.isWord()) {
      f = Form::named_procedure;
      return true;
    }
    if (!candidate.isList() || candidate.listValue()->size() == 0)
      return false;
    DatumP first = candidate.listValue()->first();

    if (first.isWord()) {
      f = Form::explicit_slot;
      return true;
    }
    if (!first.isList() || (candidate.listValue()->size() < 2))
      return false;
    DatumP procedureFirst = candidate.datumValue()->datumAtIndex(2);
    if (procedureFirst.isWord()) {
      f = Form::lambda;
      return true;
    }
    if (procedureFirst.isList()) {
      f = Form::procedure;
      return true;
    }
    return false;
  });
  DatumP params = h.listAtIndex(1);

  switch (f) {
  case named_procedure: {
    DatumP a = parser->astnodeWithLiterals(tmplate, params);
    KernelMethod method = a.astnodeValue()->kernel;
    DatumP retval = (this->*method)(a);
    return h.ret(retval);
  }
  case explicit_slot: {
    VarFrame s(&variables);
    variables.setExplicitSlotList(params);
    DatumP retval = runList(tmplate);
    return h.ret(retval);
  }
  case lambda: {
    VarFrame s(&variables);
    DatumP varList = tmplate.listValue()->first();
    DatumP procedureList = tmplate.listValue()->butfirst();
    if (varList.listValue()->size() > params.listValue()->size())
      Error::notEnough(tmplate);
    if (varList.listValue()->size() < params.listValue()->size())
      Error::tooMany(tmplate);

    ListIterator nameIter = varList.listValue()->newIterator();
    ListIterator parmIter = params.listValue()->newIterator();
    while (nameIter.elementExists()) {
      DatumP nameP = nameIter.element();
      if (!nameP.isWord())
        Error::doesntLike(node.astnodeValue()->nodeName, nameP);
      DatumP param = parmIter.element();
      QString name = nameP.wordValue()->keyValue();
      variables.setVarAsLocal(name);
      variables.setDatumForName(param, name);
    }
    DatumP retval = runList(procedureList);
    return h.ret(retval);
  }
  case procedure: {
    DatumP anonyProcedure = parser->createProcedure(
        node.astnodeValue()->nodeName, tmplate, nothing);
    ASTNode *procnode = new ASTNode(node.astnodeValue()->nodeName);
    DatumP procnodeP(procnode);
    procnode->addChild(anonyProcedure);
    if (params.listValue()->size() >
        anonyProcedure.procedureValue()->countOfMaxParams)
      Error::tooMany(node.astnodeValue()->nodeName);
    if (params.listValue()->size() <
        anonyProcedure.procedureValue()->countOfMinParams)
      Error::notEnough(node.astnodeValue()->nodeName);

    ListIterator paramIter = params.listValue()->newIterator();
    while (paramIter.elementExists()) {
      DatumP p = paramIter.element();
      DatumP a = DatumP(new ASTNode(k.literal()));
      a.astnodeValue()->kernel = &Kernel::executeLiteral;
      a.astnodeValue()->addChild(p);
      procnode->addChild(a);
    }

    DatumP retval = executeProcedure(procnodeP);

    return h.ret(retval);
  }
  }
  return nothing;
}

// '?' operator
DatumP Kernel::excNamedSlot(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP inputList = variables.explicitSlotList();
  if (!inputList.isList())
    return Error::noApply(node.astnodeValue()->nodeName);
  int index = 1;
  if (h.countOfChildren() > 0) {
    h.integerAtIndex(0);
    index = h.validatedIntegerAtIndex(0, [&inputList](int candidate) {
      return (candidate >= 1) && (candidate <= inputList.listValue()->size());
    });
  }
  return h.ret(inputList.listValue()->datumAtIndex((int)index));
}

DatumP Kernel::excMacrop(DatumP node) {
  ProcedureHelper h(this, node);
  bool retval = parser->isMacro(h.wordAtIndex(0).wordValue()->keyValue());
  return h.ret(retval);
}
