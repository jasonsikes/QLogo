
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

DatumPtr Kernel::excRun(DatumPtr node) {
  ProcedureHelper h(this, node);

  DatumPtr instructionList = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    return candidate.isWord() || candidate.isList();
  });

  return h.ret(runList(instructionList));
}

DatumPtr Kernel::excRunresult(DatumPtr node) {
  ProcedureHelper h(this, node);

  DatumPtr instructionList = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    return candidate.isWord() || candidate.isList();
  });

  List* retval = List::alloc();
  DatumPtr temp = runList(instructionList);

  if (temp.isASTNode()) {
    temp = Error::insideRunresult(temp.astnodeValue()->nodeName);
  }

  if (temp != nothing) {
    retval->append(temp);
  }

  return h.ret(retval);
}

DatumPtr Kernel::excBye(DatumPtr node) {
  ProcedureHelper h(this, node);

  Error::throwError(DatumPtr(k.system()), nothing);

  return nothing;
}

DatumPtr Kernel::excRepeat(DatumPtr node) {
  ProcedureHelper h(this, node);
  int countValue = h.validatedIntegerAtIndex(
      0, [](int candidate) { return candidate >= 0; });
  DatumPtr commandList =
      h.listAtIndex(1); // TODO: this can execute a word, too, right?

  int tempRepcount = repcount;
  repcount = 1;

  DatumPtr retval;
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

DatumPtr Kernel::excForever(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr commandList = h.listAtIndex(0);

  int tempRepcount = repcount;
  repcount = 1;

  DatumPtr retval;
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

DatumPtr Kernel::excRepcount(DatumPtr node) {
  ProcedureHelper h(this, node);

  return h.ret(repcount);
}

DatumPtr Kernel::excIf(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval;
  if (h.boolAtIndex(0, true)) {
    retval = runList(h.datumAtIndex(1));
  }
  return h.ret(retval);
}

DatumPtr Kernel::excIfelse(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval;
  if (h.boolAtIndex(0, true)) {
    retval = runList(h.datumAtIndex(1));
  } else {
    retval = runList(h.datumAtIndex(2));
  }
  return h.ret(retval);
}

DatumPtr Kernel::excTest(DatumPtr node) {
  ProcedureHelper h(this, node);

  bool testVal = h.boolAtIndex(0, true);
  variables.setTest(testVal);
  return nothing;
}

DatumPtr Kernel::excIftrue(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval;
  if (!variables.isTested())
    return h.ret(Error::noTest(node.astnodeValue()->nodeName));
  if (variables.testedState()) {
    retval = runList(h.datumAtIndex(0));
  }
  return h.ret(retval);
}

DatumPtr Kernel::excIffalse(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval;
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

DatumPtr Kernel:: excStop(DatumPtr node) {
  if (currentProcedure == nothing) {
    Error::notInsideProcedure(node.astnodeValue()->nodeName);
  }
  return node;
}

DatumPtr Kernel::excOutput(DatumPtr node) {
  if (currentProcedure == nothing) {
    Error::notInsideProcedure(node.astnodeValue()->nodeName);
  }

  return node;
}

DatumPtr Kernel::excDotMaybeoutput(DatumPtr node) {
  if (currentProcedure == nothing) {
    Error::notInsideProcedure(node.astnodeValue()->nodeName);
  }
  return node;
}

DatumPtr Kernel::excCatch(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString tag = h.wordAtIndex(0).wordValue()->keyValue();
  DatumPtr instructionlist = h.listAtIndex(1);
  DatumPtr retval;
  DatumPtr tempErract = variables.datumForName(k.erract());

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
            DatumPtr p = retval.astnodeValue()->childAtIndex(0);
            KernelMethod temp_method = p.astnodeValue()->kernel;
            DatumPtr temp_retval = (this->*temp_method)(p);
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
      DatumPtr retval = e->output;
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

DatumPtr Kernel::excThrow(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr tag = h.wordAtIndex(0);
  DatumPtr value;
  if (h.countOfChildren() > 1) {
    value = h.datumAtIndex(1);
    if (!value.isWord())
      value = DatumPtr(value.printValue());
  }

  Error::throwError(tag, value);

  return nothing;
}

DatumPtr Kernel::excError(DatumPtr node) {
  ProcedureHelper h(this, node);

  List *retval = List::alloc();
  if (currentError != nothing) {
    Error *e = currentError.errorValue();
    retval->append(DatumPtr(e->code));
    retval->append(e->errorText);
    if (e->procedure != nothing)
      retval->append(e->procedure.astnodeValue()->nodeName);
    else
      retval->append(DatumPtr(List::alloc()));
    if (e->instructionLine != nothing)
      retval->append(e->instructionLine);
    else
      retval->append(DatumPtr(List::alloc()));
    currentError = nothing;
  }
  return h.ret(retval);
}

DatumPtr Kernel::excPause(DatumPtr node) {
  ProcedureHelper h(this, node);
  if (currentProcedure == nothing) {
      Error::notInsideProcedure(node.astnodeValue()->nodeName);
  }
  return h.ret(pause());
}

DatumPtr Kernel::excContinue(DatumPtr node) {
  ProcedureHelper h(this, node);

  DatumPtr retval;
  if (h.countOfChildren() > 0) {
    retval = h.datumAtIndex(0);
    if (!retval.isWord()) {
      retval = DatumPtr(retval.printValue());
    }
  }

  Error::throwError(DatumPtr(k.pause()), retval);

  return nothing;
}

DatumPtr Kernel::excTag(DatumPtr) { return nothing; }

DatumPtr Kernel::excGoto(DatumPtr node) {
  ProcedureHelper h(this, node);
  if (currentProcedure == nothing)
    Error::notInsideProcedure(node.astnodeValue()->nodeName);
  DatumPtr tagP = h.validatedDatumAtIndex(0, [this](DatumPtr candidate) {
    if (!candidate.isWord())
      return false;
    QString tag = candidate.wordValue()->keyValue();
    return currentProcedure.astnodeValue()
        ->childAtIndex(0)
        .procedureValue()
        ->tagToLine.contains(tag);
  });
  ASTNode *a = ASTNode::alloc(k.kgoto());
  a->kernel = &Kernel::excGotoToken;
  a->addChild(tagP);
  return DatumPtr(a);
}

// TEMPLATE-BASED ITERATION

DatumPtr Kernel::excApply(DatumPtr node) {
  ProcedureHelper h(this, node);
  enum Form { explicit_slot, named_procedure, lambda, procedure };
  Form f;

  DatumPtr tmplate = h.validatedDatumAtIndex(0, [&f](DatumPtr candidate) {
    if (candidate.isWord()) {
      f = Form::named_procedure;
      return true;
    }
    if (!candidate.isList() || candidate.listValue()->size() == 0)
      return false;
    DatumPtr first = candidate.listValue()->first();

    if (first.isWord()) {
      f = Form::explicit_slot;
      return true;
    }
    if (!first.isList() || (candidate.listValue()->size() < 2))
      return false;
    DatumPtr procedureFirst = candidate.datumValue()->datumAtIndex(2);
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
  DatumPtr params = h.listAtIndex(1);

  switch (f) {
  case named_procedure: {
    DatumPtr a = parser->astnodeWithLiterals(tmplate, params);
    KernelMethod method = a.astnodeValue()->kernel;
    DatumPtr retval = (this->*method)(a);
    return h.ret(retval);
  }
  case explicit_slot: {
    VarFrame s(&variables);
    variables.setExplicitSlotList(params);
    DatumPtr retval = runList(tmplate);
    return h.ret(retval);
  }
  case lambda: {
    VarFrame s(&variables);
    DatumPtr varList = tmplate.listValue()->first();
    DatumPtr procedureList = tmplate.listValue()->butfirst();
    if (varList.listValue()->size() > params.listValue()->size())
      Error::notEnough(tmplate);
    if (varList.listValue()->size() < params.listValue()->size())
      Error::tooMany(tmplate);

    ListIterator nameIter = varList.listValue()->newIterator();
    ListIterator parmIter = params.listValue()->newIterator();
    while (nameIter.elementExists()) {
      DatumPtr nameP = nameIter.element();
      if (!nameP.isWord())
        Error::doesntLike(node.astnodeValue()->nodeName, nameP);
      DatumPtr param = parmIter.element();
      QString name = nameP.wordValue()->keyValue();
      variables.setVarAsLocal(name);
      variables.setDatumForName(param, name);
    }
    DatumPtr retval = runList(procedureList);
    return h.ret(retval);
  }
  case procedure: {
    DatumPtr anonyProcedure = parser->createProcedure(
        node.astnodeValue()->nodeName, tmplate, nothing);
    ASTNode *procnode = ASTNode::alloc(node.astnodeValue()->nodeName);
    DatumPtr procnodeP(procnode);
    procnode->addChild(anonyProcedure);
    if (params.listValue()->size() >
        anonyProcedure.procedureValue()->countOfMaxParams)
      Error::tooMany(node.astnodeValue()->nodeName);
    if (params.listValue()->size() <
        anonyProcedure.procedureValue()->countOfMinParams)
      Error::notEnough(node.astnodeValue()->nodeName);

    ListIterator paramIter = params.listValue()->newIterator();
    while (paramIter.elementExists()) {
      DatumPtr p = paramIter.element();
      DatumPtr a = DatumPtr(ASTNode::alloc(k.literal()));
      a.astnodeValue()->kernel = &Kernel::executeLiteral;
      a.astnodeValue()->addChild(p);
      procnode->addChild(a);
    }

    DatumPtr retval = executeProcedure(procnodeP);

    return h.ret(retval);
  }
  }
  return nothing;
}

// '?' operator
DatumPtr Kernel::excNamedSlot(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr inputList = variables.explicitSlotList();
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

DatumPtr Kernel::excMacrop(DatumPtr node) {
  ProcedureHelper h(this, node);
  bool retval = parser->isMacro(h.wordAtIndex(0).wordValue()->keyValue());
  return h.ret(retval);
}
