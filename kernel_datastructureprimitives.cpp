
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

#include "kernel.h"
#include "parser.h"
#include "datum_word.h"
#include "datum_astnode.h"
#include "datum_array.h"
#include <QTextStream>

#include "logocontroller.h"

// CONSTRUCTORS

DatumP Kernel::excWord(DatumP node) {
  ProcedureHelper h(this, node);
  QString retval = "";
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumP value = h.wordAtIndex(i);
    retval.append(value.wordValue()->rawValue());
  }
  return h.ret(retval);
}

DatumP Kernel::excList(DatumP node) {
  ProcedureHelper h(this, node);
  List *retval = emptyList();
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumP value = h.datumAtIndex(i);
    retval->append(value);
  }
  return h.ret(retval);
}

DatumP Kernel::excSentence(DatumP node) {
  ProcedureHelper h(this, node);
  List *retval = emptyList();
  for (int i = 0; i < node.astnodeValue()->countOfChildren(); ++i) {
    DatumP value = h.datumAtIndex(i);
    if (value.isList()) {
      ListIterator iter = value.listValue()->newIterator();
      while (iter.elementExists()) {
        DatumP element = iter.element();
        retval->append(element);
      }
    } else {
      retval->append(value);
    }
  }
  return h.ret(retval);
}

DatumP Kernel::excFput(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP thing = h.datumAtIndex(0);
  DatumP list = h.validatedDatumAtIndex(1, [&thing](DatumP candidate) {
          if (candidate.isWord()) return thing.isWord();
    return candidate.isList() || candidate.isWord();
  });
  if (list.isList()) {
    DatumP retval = list.listValue()->fput(thing);
    return h.ret(retval);
  }
  QString retval = thing.wordValue()->rawValue();
  retval.append(list.wordValue()->rawValue());
  return h.ret(retval);
}

DatumP Kernel::excLput(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP thing = h.datumAtIndex(0);
  DatumP list = h.validatedDatumAtIndex(1, [&thing](DatumP candidate) {
    if (candidate.isWord())
      return thing.isWord();
    return candidate.isList();
  });
  if (list.isList()) {
    List *retval = emptyList();
    ListIterator iter = list.listValue()->newIterator();
    while (iter.elementExists()) {
      retval->append(iter.element());
    }
    retval->append(thing);
    return h.ret(retval);
  }
  QString retval = list.wordValue()->rawValue();
  retval.append(thing.wordValue()->rawValue());
  return h.ret(retval);
}

DatumP Kernel::excArray(DatumP node) {
  ProcedureHelper h(this, node);
  int origin = 1;
  int size = h.integerAtIndex(0);
  if (h.countOfChildren() > 1) {
    origin = h.integerAtIndex(1);
  }
  return h.ret(new Array(origin, size));
}

DatumP Kernel::excListtoarray(DatumP node) {
  ProcedureHelper h(this, node);
  int origin = 1;
  DatumP source = h.listAtIndex(0);
  if (h.countOfChildren() > 1) {
    origin = h.integerAtIndex(1);
  }
  return h.ret(new Array(origin, source.listValue()));
}

DatumP Kernel::excArraytolist(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP source = h.arrayAtIndex(0);
  List *retval = List::listFromArray(source.arrayValue());
  return h.ret(retval);
}

// SELECTORS

DatumP Kernel::excFirst(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP value = h.validatedDatumAtIndex(
      0, [](DatumP candidate) { return candidate.datumValue()->size() >= 1; });
  return h.ret(value.datumValue()->first());
}

DatumP Kernel::excFirsts(DatumP node) {
  ProcedureHelper h(this, node);
  List *retval = emptyList();
  h.validatedListAtIndex(0, [retval](DatumP candidate) {
    ListIterator iter = candidate.listValue()->newIterator();
    while (iter.elementExists()) {
      DatumP item = iter.element();
      if (item.datumValue()->size() < 1)
        return false;
      retval->append(item.datumValue()->first());
    }
    return true;
  });
  return h.ret(retval);
}

DatumP Kernel::excLast(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP value = h.validatedDatumAtIndex(
      0, [](DatumP candidate) { return candidate.datumValue()->size() > 0; });
  return h.ret(value.datumValue()->last());
}

DatumP Kernel::excButfirst(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP value = h.validatedDatumAtIndex(
      0, [](DatumP candidate) { return candidate.datumValue()->size() > 0; });
  return h.ret(value.datumValue()->butfirst());
}

DatumP Kernel::excButfirsts(DatumP node) {
  ProcedureHelper h(this, node);
  List *retval = emptyList();
  h.validatedListAtIndex(0, [retval](DatumP candidate) {
    ListIterator iter = candidate.listValue()->newIterator();
    while (iter.elementExists()) {
      DatumP item = iter.element();
      if (item.datumValue()->size() < 1)
        return false;
      retval->append(item.datumValue()->butfirst());
    }
    return true;
  });
  return h.ret(retval);
}

DatumP Kernel::excButlast(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP value = h.validatedDatumAtIndex(
      0, [](DatumP candidate) { return candidate.datumValue()->size() > 0; });
  return h.ret(value.datumValue()->butlast());
}

DatumP Kernel::excItem(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP thing = h.datumAtIndex(1);
  int index = h.validatedIntegerAtIndex(0, [&thing](int candidate) {
    return thing.datumValue()->isIndexInRange((int)candidate);
  });

  return h.ret(thing.datumValue()->datumAtIndex((int)index));
}

// MUTATORS

DatumP Kernel::excSetitem(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP array = h.validatedDatumAtIndex(1, [](DatumP candidate) {
    return candidate.isList() || candidate.isArray();
  });
  int index = h.validatedIntegerAtIndex(0, [&array](int candidate) {
    return array.datumValue()->isIndexInRange(candidate);
  });
  DatumP thing = h.validatedDatumAtIndex(2, [&array, this](DatumP candidate) {
    if (candidate.isArray() || candidate.isList()) {
      if (candidate == array)
        return false;
      return !candidate.datumValue()->containsDatum(array, varCASEIGNOREDP());
    }
    return true;
  });
  array.datumValue()->setItem(index, thing);
  return nothing;
}

DatumP Kernel::excDotSetfirst(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP array = h.validatedDatumAtIndex(0, [](DatumP candidate) {
    if (!candidate.isList() && !candidate.isArray())
      return false;
    return candidate.datumValue()->size() > 0;
  });
  DatumP thing = h.datumAtIndex(1);
  array.datumValue()->setFirstItem(thing);
  return nothing;
}

DatumP Kernel::excDotSetbf(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP thing = h.datumAtIndex(1);
  DatumP array = h.validatedDatumAtIndex(0, [&thing](DatumP candidate) {
    if (!candidate.isList() && !candidate.isArray())
      return false;
    if (candidate.datumValue()->size() == 0)
      return false;
    return candidate.isa() == thing.isa();
  });
  array.datumValue()->setButfirstItem(thing);
  return nothing;
}

DatumP Kernel::excDotSetitem(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP array = h.validatedDatumAtIndex(1, [](DatumP candidate) {
    return candidate.isList() || candidate.isArray();
  });
  int index = (int)h.validatedIntegerAtIndex(0, [&array](int candidate) {
    return array.datumValue()->isIndexInRange(candidate);
  });
  DatumP thing = h.datumAtIndex(2);
  array.datumValue()->setItem(index, thing);
  return nothing;
}

// PREDICATES

DatumP Kernel::excWordp(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP src = h.datumAtIndex(0);
  return h.ret(src.isWord());
}

DatumP Kernel::excListp(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP src = h.datumAtIndex(0);
  return h.ret(src.isList());
}

DatumP Kernel::excArrayp(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP src = h.datumAtIndex(0);
  return h.ret(src.isArray());
}

DatumP Kernel::excEmptyp(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP src = h.datumAtIndex(0);
  return h.ret(src.datumValue()->size() == 0);
}

DatumP Kernel::excEqualp(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP a = h.datumAtIndex(0);
  DatumP b = h.datumAtIndex(1);
  return h.ret(a.isEqual(b, varCASEIGNOREDP()));
}

DatumP Kernel::excNotequal(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP a = h.datumAtIndex(0);
  DatumP b = h.datumAtIndex(1);
  return h.ret(!a.isEqual(b, varCASEIGNOREDP()));
}

// TODO case-sensitivity
DatumP Kernel::excBeforep(DatumP node) {
  ProcedureHelper h(this, node);
  const QString &a = h.wordAtIndex(0).wordValue()->printValue();
  const QString &b = h.wordAtIndex(1).wordValue()->printValue();
  return h.ret(a < b);
}

DatumP Kernel::excDotEq(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP a = h.datumAtIndex(0);
  DatumP b = h.datumAtIndex(1);
  return h.ret(a.isDotEqual(b));
}

DatumP Kernel::excMemberp(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP container = h.validatedDatumAtIndex(1, [](DatumP candidate) {
    return candidate.isList() || candidate.isWord();
  });
  DatumP thing = h.validatedDatumAtIndex(0, [&container](DatumP candidate) {
    if (container.isWord())
      return candidate.isWord();
    return true;
  });
  if (container.isWord() && (thing.wordValue()->size() != 1))
    return h.ret(false);
  return h.ret(container.datumValue()->isMember(thing, varCASEIGNOREDP()));
}

DatumP Kernel::excSubstringp(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP thing = h.datumAtIndex(0);
  DatumP container = h.datumAtIndex(1);
  if (!container.isWord() || !thing.isWord())
    return h.ret(false);
  return h.ret(container.datumValue()->isMember(thing, varCASEIGNOREDP()));
}

DatumP Kernel::excNumberp(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP thing = h.datumAtIndex(0);
  if (!thing.isWord())
    return h.ret(false);
  thing.wordValue()->numberValue();
  return h.ret(thing.wordValue()->didNumberConversionSucceed());
}

DatumP Kernel::excVbarredp(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP thing = h.validatedDatumAtIndex(0, [](DatumP candidate) {
    if (!candidate.isWord())
      return false;
    return candidate.wordValue()->size() == 1;
  });
  QChar c = thing.wordValue()->rawValue()[0];
  return h.ret(c != rawToChar(c));
}

// QUERIES

DatumP Kernel::excCount(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP thing = h.datumAtIndex(0);
  int count = thing.datumValue()->size();
  return h.ret(count);
}

DatumP Kernel::excAscii(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP chr = h.validatedDatumAtIndex(0, [](DatumP candidate) {
    return candidate.isWord() && candidate.wordValue()->size() == 1;
  });
  QChar c = chr.printValue()[0];
  int asc = c.unicode();
  return h.ret(asc);
}

DatumP Kernel::excRawascii(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP chr = h.validatedDatumAtIndex(0, [](DatumP candidate) {
    return candidate.isWord() && candidate.wordValue()->size() == 1;
  });
  QChar c = chr.wordValue()->rawValue()[0];
  int asc = c.unicode();
  return h.ret(asc);
}

DatumP Kernel::excChar(DatumP node) {
  ProcedureHelper h(this, node);
  int n = h.validatedIntegerAtIndex(0, [](int candidate) {
    return (candidate >= 0) && (candidate <= USHRT_MAX);
  });
  return h.ret(QString(QChar((ushort)n)));
}

DatumP Kernel::excMember(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP container = h.datumAtIndex(1);
  DatumP thing = h.validatedDatumAtIndex(0, [&container](DatumP candidate) {
    return container.isArray() || container.isList() || candidate.isWord();
  });
  return h.ret(container.datumValue()->fromMember(thing, varCASEIGNOREDP()));
}

DatumP Kernel::excLowercase(DatumP node) {
  ProcedureHelper h(this, node);
  const QString &phrase = h.wordAtIndex(0).wordValue()->printValue();
  QString retval = phrase.toLower();
  return h.ret(retval);
}

DatumP Kernel::excUppercase(DatumP node) {
  ProcedureHelper h(this, node);
  const QString &phrase = h.wordAtIndex(0).wordValue()->printValue();
  QString retval = phrase.toUpper();
  return h.ret(retval);
}

DatumP Kernel::excStandout(DatumP node) {
  ProcedureHelper h(this, node);
  const QString &phrase = h.wordAtIndex(0).wordValue()->printValue();
  QString t = mainController()->addStandoutToString(phrase);
  return h.ret(t);
}

DatumP Kernel::excParse(DatumP node) {
  ProcedureHelper h(this, node);
  Parser p(this);
  DatumP word = h.validatedDatumAtIndex(
      0, [](DatumP candidate) { return candidate.isWord(); });
  QString text = word.wordValue()->rawValue();
  QTextStream src(&text, QIODevice::ReadOnly);

  return h.ret(p.readlistWithPrompt("", false, &src));
}

DatumP Kernel::excRunparse(DatumP node) {
  ProcedureHelper h(this, node);
  Parser p(this);
  DatumP wordOrList = h.validatedDatumAtIndex(0, [](DatumP candidate) {
    return candidate.isWord() || candidate.isList();
  });
  return h.ret(p.runparse(wordOrList));
}
