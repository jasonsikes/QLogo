
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

//#include "error.h"
#include "kernel.h"
//#include "parser.h"

//#include "logocontroller.h"

// RAII for temporary default object change.
// Restores object when this instance is destroyed.
class RaiiSetObject
{
protected:
  Kernel *theKernel;
  DatumP originalObject;

public:
  RaiiSetObject (Kernel *aKernel, Object *tempObject) {
    theKernel = aKernel;
    originalObject = theKernel->currentObject;
    theKernel->currentObject = tempObject;
  }

  ~RaiiSetObject()
  {
    theKernel->currentObject = originalObject;
  }
};

DatumP Kernel::excSomething(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP retval(new Object(logoObject));
  return h.ret(retval);
}


DatumP Kernel::excKindof(DatumP node) {
  ProcedureHelper h(this, node);

  // Input may be a list of objects
  if ((h.countOfChildren() == 1) && (h.datumAtIndex(0).isList())) {
      DatumP listP = h.validatedListAtIndex(0, [](DatumP candidate) {
          List *list = candidate.listValue();
          if (list->size() == 0) return false;
          ListIterator i = list->newIterator();
          while (i.elementExists()) {
          if ( ! i.element().isObject())
          return false;
    }
          return true;
    });
      DatumP retval(new Object(listP.listValue()));
      return h.ret(retval);
    }

  // Otherwise each input is a list
  List parents;
  for (int i = 0; i < h.countOfChildren(); ++i) {
      DatumP o = h.objectAtIndex(i);
      parents.append(o);
    }
  DatumP retval(new Object(&parents));
  return h.ret(retval);
}


DatumP Kernel::excAsk(DatumP node) {
  ProcedureHelper h(this, node);
  Object *obj = h.objectAtIndex(0).objectValue();
  DatumP list = h.listAtIndex(1);
  DatumP retval;
  {
    RaiiSetObject o(this, obj);
    retval = runList(list);
  }
  return h.ret(retval);
}


DatumP Kernel::excSelf(DatumP node) {
  ProcedureHelper h(this, node);
  return h.ret(currentObject);
}


DatumP Kernel::excLogo(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP retval(logoObject);
  return h.ret(retval);
}


DatumP Kernel::excParents(DatumP node) {
  ProcedureHelper h(this, node);
  List *parents = currentObject.objectValue()->getParents();
  DatumP retval(parents);
  return h.ret(retval);
}


DatumP Kernel::excHave(DatumP node) {
  ProcedureHelper h(this, node);
  QString key = h.wordAtIndex(0).wordValue()->keyValue();
  // Only add the key if it doesn't already exist
  // (We don't want to replace an existing value with 'nothing'.)
  if (currentObject.objectValue()->hasVar(key) == NULL)
    currentObject.objectValue()->havemake(key, nothing);
  return nothing;
}


DatumP Kernel::excHavemake(DatumP node) {
  ProcedureHelper h(this, node);
  QString key = h.wordAtIndex(0).wordValue()->keyValue();
  DatumP value;
  if ((key == "NAME") || (key == "LICENSEPLATE")) {
      value = h.wordAtIndex(1);
    } else {
      value = h.datumAtIndex(1);
    }
  currentObject.objectValue()->havemake(key, value);
  return nothing;
}


DatumP Kernel::excMynames(DatumP node) {
  ProcedureHelper h(this, node);
  List *names = currentObject.objectValue()->getVarnames();
  DatumP retval(names);
  return h.ret(retval);
}


DatumP Kernel::excMynamep(DatumP node) {
  ProcedureHelper h(this, node);
  QString key = h.wordAtIndex(0).wordValue()->keyValue();
  Object *hasVar = currentObject.objectValue()->hasVar(key);
  DatumP retval(hasVar != NULL);
  return h.ret(retval);
}


