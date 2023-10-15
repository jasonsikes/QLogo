
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
#include "textstream.h"

#include "logocontroller.h"

// CONSTRUCTORS

DatumPtr Kernel::excWord(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString retval = "";
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumPtr value = h.wordAtIndex(i);
    retval.append(value.wordValue()->rawValue());
  }
  return h.ret(retval);
}

DatumPtr Kernel::excList(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = List::alloc();
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumPtr value = h.datumAtIndex(i);
    retval->append(value);
  }
  return h.ret(retval);
}

DatumPtr Kernel::excSentence(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = List::alloc();
  for (int i = 0; i < node.astnodeValue()->countOfChildren(); ++i) {
    DatumPtr value = h.datumAtIndex(i);
    if (value.isList()) {
      ListIterator iter = value.listValue()->newIterator();
      while (iter.elementExists()) {
        DatumPtr element = iter.element();
        retval->append(element);
      }
    } else {
      retval->append(value);
    }
  }
  return h.ret(retval);
}

DatumPtr Kernel::excFput(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(0);
  DatumPtr list = h.validatedDatumAtIndex(1, [&thing](DatumPtr candidate) {
          if (candidate.isWord()) return thing.isWord();
    return candidate.isList() || candidate.isWord();
  });
  if (list.isList()) {
    DatumPtr retval = list.listValue()->fput(thing);
    return h.ret(retval);
  }
  QString retval = thing.wordValue()->rawValue();
  retval.append(list.wordValue()->rawValue());
  return h.ret(retval);
}

DatumPtr Kernel::excLput(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(0);
  DatumPtr list = h.validatedDatumAtIndex(1, [&thing](DatumPtr candidate) {
    if (candidate.isWord())
      return thing.isWord();
    return candidate.isList();
  });
  if (list.isList()) {
    List *retval = List::alloc();
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

DatumPtr Kernel::excArray(DatumPtr node) {
  ProcedureHelper h(this, node);
  int origin = 1;
  int size = h.integerAtIndex(0);
  if (h.countOfChildren() > 1) {
    origin = h.integerAtIndex(1);
  }
  Array *retval = Array::alloc(origin, size);
  for (int i = 0; i < size; ++i) {
    retval->append(DatumPtr(List::alloc()));
  }
  return h.ret(DatumPtr(retval));
}

DatumPtr Kernel::excListtoarray(DatumPtr node) {
  ProcedureHelper h(this, node);
  int origin = 1;
  DatumPtr source = h.listAtIndex(0);
  if (h.countOfChildren() > 1) {
    origin = h.integerAtIndex(1);
  }
  return h.ret(Array::alloc(origin, source.listValue()));
}

DatumPtr Kernel::excArraytolist(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr source = h.arrayAtIndex(0);
  List *retval = List::alloc(source.arrayValue());
  return h.ret(retval);
}

// SELECTORS

DatumPtr Kernel::excFirst(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr value = h.validatedDatumAtIndex(
      0, [](DatumPtr candidate) { return candidate.datumValue()->size() >= 1; });
  return h.ret(value.datumValue()->first());
}

DatumPtr Kernel::excFirsts(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = List::alloc();
  h.validatedListAtIndex(0, [retval](DatumPtr candidate) {
    ListIterator iter = candidate.listValue()->newIterator();
    while (iter.elementExists()) {
      DatumPtr item = iter.element();
      if (item.datumValue()->size() < 1)
        return false;
      retval->append(item.datumValue()->first());
    }
    return true;
  });
  return h.ret(retval);
}

DatumPtr Kernel::excLast(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr value = h.validatedDatumAtIndex(
      0, [](DatumPtr candidate) { return candidate.datumValue()->size() > 0; });
  return h.ret(value.datumValue()->last());
}

DatumPtr Kernel::excButfirst(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr value = h.validatedDatumAtIndex(
      0, [](DatumPtr candidate) { return candidate.datumValue()->size() > 0; });
  return h.ret(value.datumValue()->butfirst());
}

DatumPtr Kernel::excButfirsts(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = List::alloc();
  h.validatedListAtIndex(0, [retval](DatumPtr candidate) {
    ListIterator iter = candidate.listValue()->newIterator();
    while (iter.elementExists()) {
      DatumPtr item = iter.element();
      if (item.datumValue()->size() < 1)
        return false;
      retval->append(item.datumValue()->butfirst());
    }
    return true;
  });
  return h.ret(retval);
}

DatumPtr Kernel::excButlast(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr value = h.validatedDatumAtIndex(
      0, [](DatumPtr candidate) { return candidate.datumValue()->size() > 0; });
  return h.ret(value.datumValue()->butlast());
}

DatumPtr Kernel::excItem(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(1);
  int index = h.validatedIntegerAtIndex(0, [&thing](int candidate) {
    return thing.datumValue()->isIndexInRange((int)candidate);
  });

  return h.ret(thing.datumValue()->datumAtIndex((int)index));
}

// MUTATORS

DatumPtr Kernel::excSetitem(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr array = h.validatedDatumAtIndex(1, [](DatumPtr candidate) {
    return candidate.isList() || candidate.isArray();
  });
  int index = h.validatedIntegerAtIndex(0, [&array](int candidate) {
    return array.datumValue()->isIndexInRange(candidate);
  });
  DatumPtr thing = h.validatedDatumAtIndex(2, [&array, this](DatumPtr candidate) {
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

DatumPtr Kernel::excDotSetfirst(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr array = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    if (!candidate.isList() && !candidate.isArray())
      return false;
    return candidate.datumValue()->size() > 0;
  });
  DatumPtr thing = h.datumAtIndex(1);
  array.datumValue()->setFirstItem(thing);
  return nothing;
}

DatumPtr Kernel::excDotSetbf(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(1);
  DatumPtr array = h.validatedDatumAtIndex(0, [&thing](DatumPtr candidate) {
    if (!candidate.isList() && !candidate.isArray())
      return false;
    if (candidate.datumValue()->size() == 0)
      return false;
    return candidate.isa() == thing.isa();
  });
  array.datumValue()->setButfirstItem(thing);
  return nothing;
}

DatumPtr Kernel::excDotSetitem(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr array = h.validatedDatumAtIndex(1, [](DatumPtr candidate) {
    return candidate.isList() || candidate.isArray();
  });
  int index = (int)h.validatedIntegerAtIndex(0, [&array](int candidate) {
    return array.datumValue()->isIndexInRange(candidate);
  });
  DatumPtr thing = h.datumAtIndex(2);
  array.datumValue()->setItem(index, thing);
  return nothing;
}

// PREDICATES

DatumPtr Kernel::excWordp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr src = h.datumAtIndex(0);
  return h.ret(src.isWord());
}

DatumPtr Kernel::excListp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr src = h.datumAtIndex(0);
  return h.ret(src.isList());
}

DatumPtr Kernel::excArrayp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr src = h.datumAtIndex(0);
  return h.ret(src.isArray());
}

DatumPtr Kernel::excEmptyp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr src = h.datumAtIndex(0);
  return h.ret(src.datumValue()->size() == 0);
}

DatumPtr Kernel::excEqualp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr a = h.datumAtIndex(0);
  DatumPtr b = h.datumAtIndex(1);
  return h.ret(a.isEqual(b, varCASEIGNOREDP()));
}

DatumPtr Kernel::excNotequal(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr a = h.datumAtIndex(0);
  DatumPtr b = h.datumAtIndex(1);
  return h.ret(!a.isEqual(b, varCASEIGNOREDP()));
}

// TODO case-sensitivity
DatumPtr Kernel::excBeforep(DatumPtr node) {
  ProcedureHelper h(this, node);
  const QString &a = h.wordAtIndex(0).wordValue()->printValue();
  const QString &b = h.wordAtIndex(1).wordValue()->printValue();
  return h.ret(a < b);
}

DatumPtr Kernel::excDotEq(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr a = h.datumAtIndex(0);
  DatumPtr b = h.datumAtIndex(1);
  return h.ret(a.isDotEqual(b));
}

DatumPtr Kernel::excMemberp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr container = h.validatedDatumAtIndex(1, [](DatumPtr candidate) {
    return candidate.isList() || candidate.isWord();
  });
  DatumPtr thing = h.validatedDatumAtIndex(0, [&container](DatumPtr candidate) {
    if (container.isWord())
      return candidate.isWord();
    return true;
  });
  if (container.isWord() && (thing.wordValue()->size() != 1))
    return h.ret(false);
  return h.ret(container.datumValue()->isMember(thing, varCASEIGNOREDP()));
}

DatumPtr Kernel::excSubstringp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(0);
  DatumPtr container = h.datumAtIndex(1);
  if (!container.isWord() || !thing.isWord())
    return h.ret(false);
  return h.ret(container.datumValue()->isMember(thing, varCASEIGNOREDP()));
}

DatumPtr Kernel::excNumberp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(0);
  if (!thing.isWord())
    return h.ret(false);
  thing.wordValue()->numberValue();
  return h.ret(thing.wordValue()->didNumberConversionSucceed());
}

DatumPtr Kernel::excVbarredp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    if (!candidate.isWord())
      return false;
    return candidate.wordValue()->size() == 1;
  });
  QChar c = thing.wordValue()->rawValue()[0];
  return h.ret(c != rawToChar(c));
}

// QUERIES

DatumPtr Kernel::excCount(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(0);
  int count = thing.datumValue()->size();
  return h.ret(count);
}

DatumPtr Kernel::excAscii(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr chr = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    return candidate.isWord() && candidate.wordValue()->size() == 1;
  });
  QChar c = chr.printValue()[0];
  int asc = c.unicode();
  return h.ret(asc);
}

DatumPtr Kernel::excRawascii(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr chr = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    return candidate.isWord() && candidate.wordValue()->size() == 1;
  });
  QChar c = chr.wordValue()->rawValue()[0];
  int asc = c.unicode();
  return h.ret(asc);
}

DatumPtr Kernel::excChar(DatumPtr node) {
  ProcedureHelper h(this, node);
  int n = h.validatedIntegerAtIndex(0, [](int candidate) {
    return (candidate >= 0) && (candidate <= USHRT_MAX);
  });
  return h.ret(QString(QChar((ushort)n)));
}

DatumPtr Kernel::excMember(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr container = h.datumAtIndex(1);
  DatumPtr thing = h.validatedDatumAtIndex(0, [&container](DatumPtr candidate) {
    return container.isArray() || container.isList() || candidate.isWord();
  });
  return h.ret(container.datumValue()->fromMember(thing, varCASEIGNOREDP()));
}

DatumPtr Kernel::excLowercase(DatumPtr node) {
  ProcedureHelper h(this, node);
  const QString &phrase = h.wordAtIndex(0).wordValue()->printValue();
  QString retval = phrase.toLower();
  return h.ret(retval);
}

DatumPtr Kernel::excUppercase(DatumPtr node) {
  ProcedureHelper h(this, node);
  const QString &phrase = h.wordAtIndex(0).wordValue()->printValue();
  QString retval = phrase.toUpper();
  return h.ret(retval);
}

DatumPtr Kernel::excStandout(DatumPtr node) {
  ProcedureHelper h(this, node);
  const QString &phrase = h.wordAtIndex(0).wordValue()->printValue();
  QString t = mainController()->addStandoutToString(phrase);
  return h.ret(t);
}

DatumPtr Kernel::excParse(DatumPtr node) {
  ProcedureHelper h(this, node);
  Parser p(this);
  DatumPtr word = h.validatedDatumAtIndex(
      0, [](DatumPtr candidate) { return candidate.isWord(); });
  QString text = word.wordValue()->rawValue();
  QTextStream src(&text, QIODevice::ReadOnly);
  TextStream srcStream(&src);

  // TODO: what happens if the source has multiple lines?
  return h.ret(srcStream.readlistWithPrompt("", false));
}

DatumPtr Kernel::excRunparse(DatumPtr node) {
  ProcedureHelper h(this, node);
  Parser p(this);
  DatumPtr wordOrList = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    return candidate.isWord() || candidate.isList();
  });
  return h.ret(p.runparse(wordOrList));
}
