
//===-- qlogo/datum_iterator.cpp - Iterator class and subclasses implementations -------*-
// C++ -*-===//
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
/// This file contains the implementation of the Iterator class and subclasses.
/// Iterators are used internally so they are rather minimal.
///
//===----------------------------------------------------------------------===//

#include "datum_iterator.h"
#include "datum_list.h"
#include "datum_word.h"

#include <qdebug.h>

/******************************************
 *
 * Iterator
 *
 ******************************************/

DatumP Iterator::element() {
  Q_ASSERT(false);
  return DatumP();
}

bool Iterator::elementExists() {
  Q_ASSERT(false);
  return false;
}

/******************************************
 *
 * ListIterator
 *
 ******************************************/

ListIterator::ListIterator() {}

ListIterator::ListIterator(DatumP head) {
    ptr = head;
}

DatumP ListIterator::element() {
    DatumP retval = ptr.listNodeValue()->item;
    ptr = ptr.listNodeValue()->next;
    return retval;
}

bool ListIterator::elementExists() { return (ptr != nothing); }

/******************************************
 *
 * ArrayIterator
 *
 ******************************************/

ArrayIterator::ArrayIterator() {}

ArrayIterator::ArrayIterator(QVector<DatumP> *aArray) {
  arrayIter = aArray->begin();
  end = aArray->end();
}

DatumP ArrayIterator::element() { return *arrayIter++; }

bool ArrayIterator::elementExists() { return (arrayIter != end); }
