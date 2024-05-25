
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

#include "datum/iterator.h"
#include "datum/list.h"

#include <qdebug.h>

/******************************************
 *
 * Iterator
 *
 ******************************************/

DatumPtr Iterator::element() {
  Q_ASSERT(false);
  return DatumPtr();
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

ListIterator::ListIterator(DatumPtr aList) {
    ptr = aList;
}

DatumPtr ListIterator::element() {
    DatumPtr retval = ptr.listValue()->head;
    ptr = ptr.listValue()->tail;
    return retval;
}

bool ListIterator::elementExists()
{
    return (( ! ptr.isNothing()) && ( ! (ptr.listValue()->head).isNothing()));
}

/******************************************
 *
 * ArrayIterator
 *
 ******************************************/

ArrayIterator::ArrayIterator() {}

ArrayIterator::ArrayIterator(QList<DatumPtr> *aArray) {
  arrayIter = aArray->begin();
  end = aArray->end();
}

DatumPtr ArrayIterator::element() { return *arrayIter++; }

bool ArrayIterator::elementExists() { return (arrayIter != end); }
