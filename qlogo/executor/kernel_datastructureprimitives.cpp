
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


/***DOC WORD
WORD word1 word2
(WORD word1 word2 word3 ...)

    outputs a word formed by concatenating its inputs.

COD***/

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

DatumPtr Kernel::excList(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = List::alloc();
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


/***DOC FPUT
FPUT thing list

    outputs a list equal to its second input with one extra member,
    the first input, at the beginning.  If the second input is a word,
    then the first input must be a one-letter word, and FPUT is
    equivalent to WORD.

COD***/

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


/***DOC LPUT
LPUT thing list

    outputs a list equal to its second input with one extra member,
    the first input, at the end.  If the second input is a word,
    then the first input must be a one-letter word, and LPUT is
    equivalent to WORD with its inputs in the other order.

COD***/

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


/***DOC LISTTOARRAY
LISTTOARRAY list
(LISTTOARRAY list origin)

    outputs an array of the same size as the input list, whose members
    are the members of the input list.

COD***/

DatumPtr Kernel::excListtoarray(DatumPtr node) {
  ProcedureHelper h(this, node);
  int origin = 1;
  DatumPtr source = h.listAtIndex(0);
  if (h.countOfChildren() > 1) {
    origin = h.integerAtIndex(1);
  }
  return h.ret(Array::alloc(origin, source.listValue()));
}


/***DOC ARRAYTOLIST
ARRAYTOLIST array

    outputs a list whose members are the members of the input array.
    The first member of the output is the first member of the array,
    regardless of the array's origin.

COD***/

DatumPtr Kernel::excArraytolist(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr source = h.arrayAtIndex(0);
  List *retval = List::alloc(source.arrayValue());
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

DatumPtr Kernel::excFirst(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr value = h.validatedDatumAtIndex(
      0, [](DatumPtr candidate) { return candidate.datumValue()->size() >= 1; });
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


/***DOC LAST
LAST wordorlist

    if the input is a word, outputs the last character of the word.
    If the input is a list, outputs the last member of the list.

COD***/

DatumPtr Kernel::excLast(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr value = h.validatedDatumAtIndex(
      0, [](DatumPtr candidate) { return candidate.datumValue()->size() > 0; });
  return h.ret(value.datumValue()->last());
}


/***DOC BUTFIRST BF
BUTFIRST wordorlist
BF wordorlist

    if the input is a word, outputs a word containing all but the first
    character of the input.  If the input is a list, outputs a list
    containing all but the first member of the input.

COD***/

DatumPtr Kernel::excButfirst(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr value = h.validatedDatumAtIndex(
      0, [](DatumPtr candidate) { return candidate.datumValue()->size() > 0; });
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


/***DOC BUTLAST BL
BUTLAST wordorlist
BL wordorlist

    if the input is a word, outputs a word containing all but the last
    character of the input.  If the input is a list, outputs a list
    containing all but the last member of the input.

COD***/

DatumPtr Kernel::excButlast(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr value = h.validatedDatumAtIndex(
      0, [](DatumPtr candidate) { return candidate.datumValue()->size() > 0; });
  return h.ret(value.datumValue()->butlast());
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

DatumPtr Kernel::excItem(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(1);
  int index = h.validatedIntegerAtIndex(0, [&thing](int candidate) {
    return thing.datumValue()->isIndexInRange((int)candidate);
  });

  return h.ret(thing.datumValue()->datumAtIndex((int)index));
}

// MUTATORS


/***DOC SETITEM
SETITEM index array value

    command.  Replaces the "index"th member of "array" with the new
    "value".  Ensures that the resulting array is not circular, i.e.,
    "value" may not be a list or array that contains "array".

COD***/

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


/***DOC .SETFIRST
.SETFIRST list value

    command.  Changes the first member of "list" to be "value".

    WARNING:  Primitives whose names start with a period are DANGEROUS.
    Their use by non-experts is not recommended.  The use of .SETFIRST can
    lead to circular list structures, which will get some Logo primitives
    into infinite loops, and to unexpected changes to other data
    structures that share storage with the list being modified.

COD***/

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


/***DOC .SETITEM
.SETITEM index array value

    command.  Changes the "index"th member of "array" to be "value",
    like SETITEM, but without checking for circularity.

    WARNING: Primitives whose names start with a period are DANGEROUS.
    Their use by non-experts is not recommended.  The use of .SETITEM
    can lead to circular arrays, which will get some Logo primitives into
    infinite loops.

COD***/

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


/***DOC WORDP WORD?
WORDP thing
WORD? thing

    outputs TRUE if the input is a word, FALSE otherwise.

COD***/

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

DatumPtr Kernel::excEmptyp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr src = h.datumAtIndex(0);
  return h.ret(src.datumValue()->size() == 0);
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

// TODO: should this be 'excNotequalp'?
DatumPtr Kernel::excNotequal(DatumPtr node) {
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

// TODO case-sensitivity
DatumPtr Kernel::excBeforep(DatumPtr node) {
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

DatumPtr Kernel::excNumberp(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(0);
  if (!thing.isWord())
    return h.ret(false);
  thing.wordValue()->numberValue();
  return h.ret(thing.wordValue()->didNumberConversionSucceed());
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

DatumPtr Kernel::excCount(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr thing = h.datumAtIndex(0);
  int count = thing.datumValue()->size();
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

DatumPtr Kernel::excAscii(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr chr = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    return candidate.isWord() && candidate.wordValue()->size() == 1;
  });
  QChar c = chr.printValue()[0];
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

DatumPtr Kernel::excRunparse(DatumPtr node) {
  ProcedureHelper h(this, node);
  Parser p;
  DatumPtr wordOrList = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    return candidate.isWord() || candidate.isList();
  });
  return h.ret(p.runparse(wordOrList));
}
