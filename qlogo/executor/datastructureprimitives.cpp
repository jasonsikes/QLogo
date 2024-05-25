
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
#include "datum/word.h"
#include "datum/astnode.h"
#include "datum/array.h"
#include <QTextStream>
#include "runparser.h"
#include "controller/textstream.h"

#include "controller/logocontroller.h"

// CONSTRUCTORS


/***DOC WORD
WORD word1 word2
(WORD word1 word2 word3 ...)

    outputs a word formed by concatenating its inputs.

COD***/
//CMD WORD 0 2 -1
DatumPtr Kernel::excWord(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString retval = "";
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumPtr value = h.wordAtIndex(i);
    retval.append(value.wordValue()->rawValue());
  }
  return h.ret(retval);
}


/***DOC LIST
LIST thing1 thing2
(LIST thing1 thing2 thing3 ...)

    outputs a list whose members are its inputs, which can be any
    Logo datum (word, list, or array).

COD***/
//CMD LIST 0 2 -1
DatumPtr Kernel::excList(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = new List();
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumPtr value = h.datumAtIndex(i);
    retval->append(value);
  }
  return h.ret(retval);
}


/***DOC SENTENCE SE
SENTENCE thing1 thing2
SE thing1 thing2
(SENTENCE thing1 thing2 thing3 ...)
(SE thing1 thing2 thing3 ...)

    outputs a list whose members are its inputs, if those inputs are
    not lists, or the members of its inputs, if those inputs are lists.

COD***/
//CMD SENTENCE 0 2 -1
//CMD SE 0 2 -1
DatumPtr Kernel::excSentence(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = new List();
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


/***DOC FPUT
FPUT thing list

    outputs a list equal to its second input with one extra member,
    the first input, at the beginning.  If the second input is a word,
    then the first input must be a word, and FPUT is equivalent to WORD.

COD***/
//CMD FPUT 2 2 2
DatumPtr Kernel::excFput(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(0);
  DatumPtr list = h.validatedDatumAtIndex(1, [&thing](DatumPtr candidate) {
          if (candidate.isWord()) return thing.isWord();
    return candidate.isList() || candidate.isWord();
  });
  if (list.isList()) {
      return h.ret(new List(thing, list.listValue()));
  }
  QString retval = thing.wordValue()->rawValue();
  retval.append(list.wordValue()->rawValue());
  return h.ret(retval);
}


/***DOC LPUT
LPUT thing list

    outputs a list equal to its second input with one extra member,
    the first input, at the end.  If the second input is a word,
    then the first input must be a one-letter word, and LPUT is
    equivalent to WORD with its inputs in the other order.

COD***/
//CMD LPUT 2 2 2
DatumPtr Kernel::excLput(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(0);
  DatumPtr list = h.validatedDatumAtIndex(1, [&thing](DatumPtr candidate) {
    if (candidate.isWord())
      return thing.isWord();
    return candidate.isList();
  });
  if (list.isList()) {
    List *retval = new List();
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


/***DOC ARRAY
ARRAY size
(ARRAY size origin)

    outputs an array of "size" members (must be a positive integer),
    each of which initially is an empty list.  Array members can be
    selected with ITEM and changed with SETITEM.  The first member of
    the array is member number 1 unless an "origin" input (must be an
    integer) is given, in which case the first member of the array has
    that number as its index.  (Typically 0 is used as the origin if
    anything.)  Arrays are printed by PRINT and friends, and can be
    typed in, inside curly braces; indicate an origin with {a b c}@0.

COD***/
//CMD ARRAY 1 1 2
DatumPtr Kernel::excArray(DatumPtr node) {
  ProcedureHelper h(this, node);
  int origin = 1;
  int size = h.integerAtIndex(0);
  if (h.countOfChildren() > 1) {
    origin = h.integerAtIndex(1);
  }
  Array *retval = new Array(origin, size);
  for (int i = 0; i < size; ++i) {
    retval->append(DatumPtr(new List()));
  }
  return h.ret(DatumPtr(retval));
}


/***DOC LISTTOARRAY
LISTTOARRAY list
(LISTTOARRAY list origin)

    outputs an array of the same size as the input list, whose members
    are the members of the input list.

COD***/
//CMD LISTTOARRAY 1 1 2
DatumPtr Kernel::excListtoarray(DatumPtr node) {
  ProcedureHelper h(this, node);
  int origin = 1;
  DatumPtr source = h.listAtIndex(0);
  if (h.countOfChildren() > 1) {
    origin = h.integerAtIndex(1);
  }
  return h.ret(new Array(origin, source.listValue()));
}


/***DOC ARRAYTOLIST
ARRAYTOLIST array

    outputs a list whose members are the members of the input array.
    The first member of the output is the first member of the array,
    regardless of the array's origin.

COD***/
//CMD ARRAYTOLIST 1 1 1
DatumPtr Kernel::excArraytolist(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr source = h.arrayAtIndex(0);
  List *retval = new List(source.arrayValue());
  return h.ret(retval);
}

// SELECTORS


/***DOC FIRST
FIRST thing

    if the input is a word, outputs the first character of the word.
    If the input is a list, outputs the first member of the list.
    If the input is an array, outputs the origin of the array (that
    is, the INDEX OF the first member of the array).

COD***/
//CMD FIRST 1 1 1
DatumPtr Kernel::excFirst(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr value = h.validatedDatumAtIndex(
      0, [](DatumPtr candidate)
      {
          if (candidate.isWord()) return candidate.wordValue()->rawValue().size() > 0;
          if (candidate.isArray()) return true;
          if (candidate.isList()) return ! candidate.listValue()->isEmpty();
          return false;
      });
  return h.ret(value.datumValue()->first());
}


/***DOC FIRSTS
FIRSTS list

    outputs a list containing the FIRST of each member of the input
    list.  It is an error if any member of the input list is empty.
    (The input itself may be empty, in which case the output is also
    empty.)  This could be written as

        to firsts :list
        output map "first :list
        end

    but is provided as a primitive in order to speed up the iteration
    tools MAP, MAP.SE, and FOREACH.
COD***/
//CMD FIRSTS 1 1 1
DatumPtr Kernel::excFirsts(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = new List();
  h.validatedListAtIndex(0, [retval](DatumPtr candidate) {
    ListIterator iter = candidate.listValue()->newIterator();
    while (iter.elementExists()) {
      DatumPtr item = iter.element();
      if (item.isWord() && item.wordValue()->rawValue().size() < 1) return false;
      if (item.isArray() && item.arrayValue()->array.size() < 1) return false;
      if (item.isList() && item.listValue()->isEmpty()) return false;
      retval->append(item.datumValue()->first());
    }
    return true;
  });
  return h.ret(retval);
}


/***DOC LAST
LAST wordorlist

    if the input is a word, outputs the last character of the word.
    If the input is a list, outputs the last member of the list.

COD***/
//CMD LAST 1 1 1
DatumPtr Kernel::excLast(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr value = h.validatedDatumAtIndex(
      0, [](DatumPtr candidate)
      {
          if (candidate.isWord()) return candidate.wordValue()->rawValue().size() > 0;
          if (candidate.isList()) return ! candidate.listValue()->isEmpty();
          return false;
      });
  return h.ret(value.datumValue()->last());
}


/***DOC BUTFIRST BF
BUTFIRST wordorlist
BF wordorlist

    if the input is a word, outputs a word containing all but the first
    character of the input.  If the input is a list, outputs a list
    containing all but the first member of the input.

COD***/
//CMD BUTFIRST 1 1 1
//CMD BF 1 1 1
DatumPtr Kernel::excButfirst(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr value = h.validatedDatumAtIndex(
      0, [](DatumPtr candidate)
      {
          if (candidate.isWord()) return candidate.wordValue()->rawValue().size() > 0;
          if (candidate.isList()) return ! candidate.listValue()->isEmpty();
          return false;
      });

  return h.ret(value.datumValue()->butfirst());
}


/***DOC BUTFIRSTS BFS
BUTFIRSTS list
BFS list

    outputs a list containing the BUTFIRST of each member of the input
    list.  It is an error if any member of the input list is empty or an
    array.  (The input itself may be empty, in which case the output is
    also empty.)  This could be written as

        to butfirsts :list
        output map "butfirst :list
        end

    but is provided as a primitive in order to speed up the iteration
    tools MAP, MAP.SE, and FOREACH.

COD***/
//CMD BUTFIRSTS 1 1 1
//CMD BFS 1 1 1
DatumPtr Kernel::excButfirsts(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = new List();
  h.validatedListAtIndex(0, [retval](DatumPtr candidate) {
    ListIterator iter = candidate.listValue()->newIterator();
    while (iter.elementExists()) {
      DatumPtr item = iter.element();
      if (item.isWord() && item.wordValue()->rawValue().size() < 1) return false;
      if (item.isArray() && item.arrayValue()->array.size() < 1) return false;
      if (item.isList() && item.listValue()->isEmpty()) return false;
      retval->append(item.datumValue()->butfirst());
    }
    return true;
  });
  return h.ret(retval);
}


/***DOC BUTLAST BL
BUTLAST wordorlist
BL wordorlist

    if the input is a word, outputs a word containing all but the last
    character of the input.  If the input is a list, outputs a list
    containing all but the last member of the input.

COD***/
//CMD BUTLAST 1 1 1
//CMD BL 1 1 1
DatumPtr Kernel::excButlast(DatumPtr node) {
    ProcedureHelper h(this, node);
    DatumPtr value = h.validatedDatumAtIndex(
        0, [](DatumPtr candidate)
        {
            if (candidate.isWord()) return candidate.wordValue()->rawValue().size() > 0;
            if (candidate.isList()) return ! candidate.listValue()->isEmpty();
            return false;
        });

    // value is either a Word or List
    if (value.isWord()) {
        QString source = value.wordValue()->rawValue();
        QString retval = source.left(source.size() - 1);
        return h.ret(retval);
    }

    // Value is a list.
    List *retval = new List();
    List *dest = retval;

    List *src = value.listValue();
    while (src->tail != nothing) {
        dest->head = src->head;
        src = src->tail.listValue();
        if (src->tail == nothing) {
            dest->tail = nothing;
        } else {
            dest->tail = DatumPtr(new List);
            dest = dest->tail.listValue();
        }
    }
    return h.ret(retval);
}


/***DOC ITEM
ITEM index thing

    if the "thing" is a word, outputs the "index"th character of the
    word.  If the "thing" is a list, outputs the "index"th member of
    the list.  If the "thing" is an array, outputs the "index"th
    member of the array.  "Index" starts at 1 for words and lists;
    the starting index of an array is specified when the array is
    created.

COD***/
//CMD ITEM 2 2 2
DatumPtr Kernel::excItem(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(1);
  int index = h.validatedIntegerAtIndex(0, [&thing](int candidate) {
    return thing.datumValue()->isIndexInRange((int)candidate);
  });

  DatumPtr retval;
  if (thing.isArray()) {
      Array *ary = thing.arrayValue();
      retval = ary->array[index - ary->origin];
  } else if (thing.isWord()) {
      retval = DatumPtr(thing.wordValue()->rawValue().mid(index - 1, 1));
  } else if (thing.isList()) {
      retval = thing.listValue()->itemAtIndex((int)index);
  } else {
      Q_ASSERT(false);
  }

  return h.ret(retval);
}

// MUTATORS


/***DOC SETITEM
SETITEM index array value

    command.  Replaces the "index"th member of "array" with the new
    "value".  Ensures that the resulting array is not circular, i.e.,
    "value" may not be a list or array that contains "array".

COD***/
//CMD SETITEM 3 3 3
DatumPtr Kernel::excSetitem(DatumPtr node) {
  ProcedureHelper h(this, node);
  Array *ary = h.arrayAtIndex(1).arrayValue();
  int index = h.validatedIntegerAtIndex(0, [&ary](int candidate) {
    return ary->isIndexInRange(candidate);
  });
  DatumPtr value = h.validatedDatumAtIndex(2, [&ary, this](DatumPtr candidate) {
      if (candidate == ary)
          return false;
      if (candidate.isArray() && candidate.arrayValue()->containsDatum(ary, varCASEIGNOREDP()))
          return false;
      if (candidate.isList() && candidate.listValue()->containsDatum(ary, varCASEIGNOREDP()))
          return false;
      return true;
  });

  index -= ary->origin;
  ary->array[index] = value;
  return nothing;
}


/***DOC .SETFIRST
.SETFIRST list value

    command.  Changes the first member of "list" to be "value".

    WARNING:  Primitives whose names start with a period are DANGEROUS.
    Their use by non-experts is not recommended.  The use of .SETFIRST can
    lead to circular list structures, which will get some Logo primitives
    into infinite loops, and to unexpected changes to other data
    structures that share storage with the list being modified.

COD***/
//CMD .SETFIRST 2 2 2
DatumPtr Kernel::excDotSetfirst(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr list = h.validatedListAtIndex(0, [](DatumPtr candidate) {
    return ! candidate.listValue()->isEmpty();
  });
  DatumPtr value = h.datumAtIndex(1);
  list.listValue()->head = value;
  list.listValue()->astParseTimeStamp = 0;
  return nothing;
}


/***DOC .SETBF
.SETBF list value

    command.  Changes the butfirst of "list" to be "value".

    WARNING: Primitives whose names start with a period are DANGEROUS.
    Their use by non-experts is not recommended.  The use of .SETBF can
    lead to circular list structures, which will get some Logo primitives
    into infinite loops; unexpected changes to other data structures that
    share storage with the list being modified; or to Logo crashes and
    coredumps if the butfirst of a list is not itself a list.

COD***/
//CMD .SETBF 2 2 2
DatumPtr Kernel::excDotSetbf(DatumPtr node) {
    // I'm not sure of the practicality of having list and value being anything
    // other than lists. So they must be lists.
  ProcedureHelper h(this, node);
  DatumPtr list = h.validatedListAtIndex(0, [](DatumPtr candidate) {
      return ! candidate.listValue()->isEmpty();
  });
  DatumPtr value = h.listAtIndex(1);
  list.listValue()->setButfirstItem(value);
  return nothing;
}


/***DOC .SETITEM
.SETITEM index array value

    command.  Changes the "index"th member of "array" to be "value",
    like SETITEM, but without checking for circularity.

    WARNING: Primitives whose names start with a period are DANGEROUS.
    Their use by non-experts is not recommended.  The use of .SETITEM
    can lead to circular arrays, which will get some Logo primitives into
    infinite loops.

COD***/
//CMD .SETITEM 3 3 3
DatumPtr Kernel::excDotSetitem(DatumPtr node) {
  ProcedureHelper h(this, node);
  Array *ary = h.arrayAtIndex(1).arrayValue();
  int index = (int)h.validatedIntegerAtIndex(0, [ary](int candidate) {
    return ary->isIndexInRange(candidate);
  });
  DatumPtr value = h.datumAtIndex(2);

  index -= ary->origin;
  ary->array[index] = value;
  return nothing;
}

// PREDICATES


/***DOC WORDP WORD?
WORDP thing
WORD? thing

    outputs TRUE if the input is a word, FALSE otherwise.

COD***/
//CMD WORDP 1 1 1
//CMD WORD? 1 1 1
DatumPtr Kernel::excWordp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr src = h.datumAtIndex(0);
  return h.ret(src.isWord());
}


/***DOC LISTP LIST?
LISTP thing
LIST? thing

    outputs TRUE if the input is a list, FALSE otherwise.

COD***/
//CMD LISTP 1 1 1
//CMD LIST? 1 1 1
DatumPtr Kernel::excListp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr src = h.datumAtIndex(0);
  return h.ret(src.isList());
}


/***DOC ARRAYP ARRAY?
ARRAYP thing
ARRAY? thing

    outputs TRUE if the input is an array, FALSE otherwise.

COD***/
//CMD ARRAYP 1 1 1
//CMD ARRAY? 1 1 1
DatumPtr Kernel::excArrayp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr src = h.datumAtIndex(0);
  return h.ret(src.isArray());
}


/***DOC EMPTYP EMPTY?
EMPTYP thing
EMPTY? thing

    outputs TRUE if the input is the empty word or the empty list,
    FALSE otherwise.

COD***/
//CMD EMPTYP 1 1 1
//CMD EMPTY? 1 1 1
DatumPtr Kernel::excEmptyp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr src = h.datumAtIndex(0);
  bool retval = false;
  if (src.isWord()) retval = (src.wordValue()->rawValue().size() == 0);
  if (src.isList()) retval = src.listValue()->isEmpty();

  return h.ret(retval);
}


/***DOC EQUALP EQUAL?
EQUALP thing1 thing2
EQUAL? thing1 thing2
thing1 = thing2

    outputs TRUE if the inputs are equal, FALSE otherwise.  Two numbers
    are equal if they have the same numeric value.  Two non-numeric words
    are equal if they contain the same characters in the same order.  If
    there is a variable named CASEIGNOREDP whose value is TRUE, then an
    upper case letter is considered the same as the corresponding lower
    case letter.  (This is the case by default.)  Two lists are equal if
    their members are equal.  An array is only equal to itself; two
    separately created arrays are never equal even if their members are
    equal.  (It is important to be able to know if two expressions have
    the same array as their value because arrays are mutable; if, for
    example, two variables have the same array as their values then
    performing SETITEM on one of them will also change the other.)

COD***/
//CMD EQUALP 2 2 2
//CMD EQUAL? 2 2 2
DatumPtr Kernel::excEqualp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr a = h.datumAtIndex(0);
  DatumPtr b = h.datumAtIndex(1);
  return h.ret(a.isEqual(b, varCASEIGNOREDP()));
}


/***DOC NOTEQUALP NOTEQUAL?
NOTEQUALP thing1 thing2
NOTEQUAL? thing1 thing2
thing1 <> thing2

    outputs FALSE if the inputs are equal, TRUE otherwise.  See EQUALP
    for the meaning of equality for different data types.

COD***/
//CMD NOTEQUALP 2 2 2
//CMD NOTEQUAL? 2 2 2
DatumPtr Kernel::excNotequalp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr a = h.datumAtIndex(0);
  DatumPtr b = h.datumAtIndex(1);
  return h.ret(!a.isEqual(b, varCASEIGNOREDP()));
}


/***DOC BEFOREP BEFORE?
BEFOREP word1 word2
BEFORE? word1 word2

    outputs TRUE if word1 comes before word2 in ASCII collating sequence
    (for words of letters, in alphabetical order).  Case-sensitivity is
    determined by the value of CASEIGNOREDP.  Note that if the inputs are
    numbers, the result may not be the same as with LESSP; for example,
    BEFOREP 3 12 is false because 3 collates after 1.

COD***/
//CMD BEFOREP 2 2 2
//CMD BEFORE? 2 2 2
DatumPtr Kernel::excBeforep(DatumPtr node) {
  // TODO case-sensitivity?
  ProcedureHelper h(this, node);
  const QString &a = h.wordAtIndex(0).wordValue()->printValue();
  const QString &b = h.wordAtIndex(1).wordValue()->printValue();
  return h.ret(a < b);
}


/***DOC .EQ
.EQ thing1 thing2

    outputs TRUE if its two inputs are the same datum, so that applying a
    mutator to one will change the other as well.  Outputs FALSE otherwise,
    even if the inputs are equal in value.
    WARNING: Primitives whose names start with a period are DANGEROUS.
    Their use by non-experts is not recommended.  The use of mutators
    can lead to circular data structures, infinite loops, or Logo crashes.

COD***/
//CMD .EQ 2 2 2
DatumPtr Kernel::excDotEq(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr a = h.datumAtIndex(0);
  DatumPtr b = h.datumAtIndex(1);
  return h.ret(a.isDotEqual(b));
}


/***DOC MEMBERP MEMBER?
MEMBERP thing1 thing2
MEMBER? thing1 thing2

    if "thing2" is a list or an array, outputs TRUE if "thing1" is EQUALP
    to a member of "thing2", FALSE otherwise.  If "thing2" is
    a word, outputs TRUE if "thing1" is a one-character word EQUALP to a
    character of "thing2", FALSE otherwise.

COD***/
//CMD MEMBERP 2 2 2
//CMD MEMBER? 2 2 2
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


/***DOC SUBSTRINGP SUBSTRING?
SUBSTRINGP thing1 thing2
SUBSTRING? thing1 thing2

    if "thing1" or "thing2" is a list or an array, outputs FALSE.  If
    "thing2" is a word, outputs TRUE if "thing1" is EQUALP to a
    substring of "thing2", FALSE otherwise.

COD***/
//CMD SUBSTRINGP 2 2 2
//CMD SUBSTRING? 2 2 2
DatumPtr Kernel::excSubstringp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(0);
  DatumPtr container = h.datumAtIndex(1);
  if (!container.isWord() || !thing.isWord())
    return h.ret(false);
  return h.ret(container.datumValue()->isMember(thing, varCASEIGNOREDP()));
}


/***DOC NUMBERP NUMBER?
NUMBERP thing
NUMBER? thing

    outputs TRUE if the input is a number, FALSE otherwise.

COD***/
//CMD NUMBERP 1 1 1
//CMD NUMBER? 1 1 1
DatumPtr Kernel::excNumberp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(0);
  if (!thing.isWord())
    return h.ret(false);
  return h.ret( ! isnan(thing.wordValue()->numberValue()));
}


/***DOC VBARREDP VBARRED? BACKSLASHEDP BACKSLASHED?
VBARREDP char
VBARRED? char
BACKSLASHEDP char                               (library procedure)
BACKSLASHED? char                               (library procedure)

    outputs TRUE if the input character was originally entered into Logo
    within vertical bars (|) to prevent its usual special syntactic
    meaning, FALSE otherwise.  (Outputs TRUE only if the character is a
    backslashed space, tab, newline, or one of ()[]+-/=*<>":;\~?| )

    The names BACKSLASHEDP and BACKSLASHED? are included in the Logo
    library for backward compatibility with the former names of this
    primitive, although it does *not* output TRUE for characters
    originally entered with backslashes.


COD***/
//CMD VBARREDP 1 1 1
//CMD VBARRED? 1 1 1
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


/***DOC COUNT
COUNT thing

    outputs the number of characters in the input, if the input is a word;
    outputs the number of members in the input, if it is a list
    or an array.  (For an array, this may or may not be the index of the
    last member, depending on the array's origin.)

COD***/
//CMD COUNT 1 1 1
DatumPtr Kernel::excCount(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(0);
  int count = -1;
  if (thing.isWord()) count = thing.wordValue()->rawValue().size();
  if (thing.isArray()) count = thing.arrayValue()->array.size();
  if (thing.isList()) count = thing.listValue()->count();

  Q_ASSERT(count != -1);
  return h.ret(count);
}


/***DOC ASCII
ASCII char

    outputs the integer (between 0 and 65535) that represents the input
    character in Unicode.  Interprets control characters as
    representing vbarred punctuation, and returns the character code
    for the corresponding punctuation character without vertical bars.
    (Compare RAWASCII.)

    Even though QLogo uses Unicode instead of ASCII, the primitives ASCII,
    RAWASCII, and CHAR are maintained for compatibility with UCBLogo and
    because ASCII is a proper subset of Unicode.

COD***/
//CMD ASCII 1 1 1
DatumPtr Kernel::excAscii(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr chr = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    return candidate.isWord() && candidate.wordValue()->size() == 1;
  });
  QChar c = chr.printValue().at(0);
  int asc = c.unicode();
  return h.ret(asc);
}


/***DOC RAWASCII
RAWASCII char

    outputs the integer (between 0 and 65535) that represents the input
    character in Unicode.  Interprets control characters as
    representing themselves.  To find out the Unicode value of an arbitrary
    keystroke, use RAWASCII RC.

    See ASCII for discussion of Unicode characters.

COD***/
//CMD RAWASCII 1 1 1
DatumPtr Kernel::excRawascii(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr chr = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    return candidate.isWord() && candidate.wordValue()->size() == 1;
  });
  QChar c = chr.wordValue()->rawValue()[0];
  int asc = c.unicode();
  return h.ret(asc);
}


/***DOC CHAR
CHAR int

    outputs the character represented in Unicode by the input,
    which must be an integer between 0 and 65535.

    See ASCII for discussion of Unicode characters.

COD***/
//CMD CHAR 1 1 1
DatumPtr Kernel::excChar(DatumPtr node) {
  ProcedureHelper h(this, node);
  int n = h.validatedIntegerAtIndex(0, [](int candidate) {
    return (candidate >= 0) && (candidate <= USHRT_MAX);
  });
  return h.ret(QString(QChar((ushort)n)));
}


/***DOC MEMBER
MEMBER thing1 thing2

    if "thing2" is a word or list and if MEMBERP with these inputs would
    output TRUE, outputs the portion of "thing2" from the first instance
    of "thing1" to the end.  If MEMBERP would output FALSE, outputs the
    empty word or list according to the type of "thing2".  It is an error
    for "thing2" to be an array.

COD***/
//CMD MEMBER 2 2 2
DatumPtr Kernel::excMember(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr container = h.datumAtIndex(1);
  DatumPtr thing = h.validatedDatumAtIndex(0, [&container](DatumPtr candidate) {
    return container.isArray() || container.isList() || candidate.isWord();
  });
  return h.ret(container.datumValue()->fromMember(thing, varCASEIGNOREDP()));
}


/***DOC LOWERCASE
LOWERCASE word

    outputs a copy of the input word, but with all uppercase letters
    changed to the corresponding lowercase letter.

COD***/
//CMD LOWERCASE 1 1 1
DatumPtr Kernel::excLowercase(DatumPtr node) {
  ProcedureHelper h(this, node);
  const QString &phrase = h.wordAtIndex(0).wordValue()->printValue();
  QString retval = phrase.toLower();
  return h.ret(retval);
}


/***DOC UPPERCASE
UPPERCASE word

    outputs a copy of the input word, but with all lowercase letters
    changed to the corresponding uppercase letter.

COD***/
//CMD UPPERCASE 1 1 1
DatumPtr Kernel::excUppercase(DatumPtr node) {
  ProcedureHelper h(this, node);
  const QString &phrase = h.wordAtIndex(0).wordValue()->printValue();
  QString retval = phrase.toUpper();
  return h.ret(retval);
}


/***DOC STANDOUT
STANDOUT thing

    outputs a word that, when printed, will appear like the input but
    displayed in standout mode (reverse video).  The word contains
    magic characters at the beginning and end; in between is the printed
    form (as if displayed using TYPE) of the input.  The output is always
    a word, even if the input is of some other type, but it may include
    spaces and other formatting characters.

COD***/
//CMD STANDOUT 1 1 1
DatumPtr Kernel::excStandout(DatumPtr node) {
  ProcedureHelper h(this, node);
  const QString &phrase = h.wordAtIndex(0).wordValue()->printValue();
  QString t = mainController()->addStandoutToString(phrase);
  return h.ret(t);
}


/***DOC PARSE
PARSE word

    outputs the list that would result if the input word were entered
    in response to a READLIST operation.  That is, PARSE READWORD has
    the same value as READLIST for the same characters read.

COD***/
//CMD PARSE 1 1 1
DatumPtr Kernel::excParse(DatumPtr node) {
  ProcedureHelper h(this, node);
  Parser p;
  DatumPtr word = h.validatedDatumAtIndex(
      0, [](DatumPtr candidate) { return candidate.isWord(); });
  QString text = word.wordValue()->rawValue();
  QTextStream src(&text, QIODevice::ReadOnly);
  TextStream srcStream(&src);

  // TODO: what happens if the source has multiple lines?
  return h.ret(srcStream.readlistWithPrompt("", false));
}


/***DOC RUNPARSE
RUNPARSE wordorlist

    outputs the list that would result if the input word or list were
    entered as an instruction line; characters such as infix operators
    and parentheses are separate members of the output.  Note that
    sublists of a runparsed list are not themselves runparsed.


COD***/
//CMD RUNPARSE 1 1 1
DatumPtr Kernel::excRunparse(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr wordOrList = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    return candidate.isWord() || candidate.isList();
  });
  return h.ret(runparse(wordOrList));
}
