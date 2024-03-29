//===-- qlogo/datum_array.cpp - Array class implementation -------*-
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
/// This file contains the implementation of the Array class.
/// An array may contain words, lists or arrays.
/// Is is implemented using QVector.
///
//===----------------------------------------------------------------------===//

#include "datum_array.h"
#include "datum_datump.h"
#include "datum_list.h"
#include "datum_iterator.h"
#include <qdebug.h>
#include "stringconstants.h"

static DatumPool<Array> pool(5);

QList<void *> aryVisited;
QList<void *> otherAryVisited;

Array::Array(int aOrigin, int aSize) {
  origin = aOrigin;
  array.reserve(aSize);
  for (int i = 0; i < aSize; ++i) {
    array.append(DatumPtr(List::alloc()));
  }
}


void Array::clear()
{
  origin = 1;
  array.clear();
}

Array * Array::alloc(int aOrigin, int aSize)
{
  Array * retval = (Array *) pool.alloc();
  retval->array.reserve(aSize);
  retval->origin = aOrigin;
  return retval;
}

Array * Array::alloc(int aOrigin, List *source)
{
  Array * retval = alloc(aOrigin, source->size());

  auto iter = source->newIterator();

  while (iter.elementExists()) {
    retval->append(iter.element());
  }
  return retval;
}

Array::~Array() {}

void Array::addToPool()
{
  clear();
  pool.dealloc(this);
}

Datum::DatumType Array::isa() { return Datum::arrayType; }

QString Array::name() {
  return k.array();
}

QString Array::printValue(bool fullPrintp, int printDepthLimit,
                          int printWidthLimit) {
  QString retval = "";
  auto iter = array.begin();
  if (iter == array.end()) {
    return retval;
  }
  if ((printDepthLimit == 0) || (printWidthLimit == 0)) {
    return "...";
  }
  int printWidth = printWidthLimit - 1;
  retval = iter->showValue(fullPrintp, printDepthLimit - 1, printWidthLimit);
  while (++iter != array.end()) {
    retval.append(' ');
    if (printWidth == 0) {
      retval.append("...");
      break;
    }
    retval.append(
        iter->showValue(fullPrintp, printDepthLimit - 1, printWidthLimit));
    --printWidth;
  }
  return retval;
}

QString Array::showValue(bool fullPrintp, int printDepthLimit,
                         int printWidthLimit) {
  if (!aryVisited.contains(this)) {
    aryVisited.push_back(this);
    QString retval = "{";
    retval.append(printValue(fullPrintp, printDepthLimit, printWidthLimit));
    retval.append('}');
    aryVisited.removeOne(this);
    return retval;
  }
  return "...";
}

bool Array::isEqual(DatumPtr other, bool) {
  Array *o = other.arrayValue();
  return this == o;
}

int Array::size() { return array.size(); }

void Array::append(DatumPtr value) { array.append(value); }

bool Array::isIndexInRange(int anIndex) {
  int index = anIndex - origin;
  return ((index >= 0) && (index < array.size()));
}

void Array::setItem(int anIndex, DatumPtr aValue) {
  int index = anIndex - origin;
  array[index] = aValue;
}

void Array::setButfirstItem(DatumPtr aValue) {
  Q_ASSERT(array.size() > 0);
  auto estart = array.begin();
  ++estart;
  array.erase(estart, array.end());
  array.reserve(aValue.arrayValue()->size() + 1);
  array.append(aValue.arrayValue()->array);
}

void Array::setFirstItem(DatumPtr aValue) { array[0] = aValue; }

bool Array::containsDatum(DatumPtr aDatum, bool ignoreCase) {
  for (int i = 0; i < array.size(); ++i) {
    DatumPtr e = array[i];
    if (e == aDatum)
      return true;
    if (e.datumValue()->containsDatum(aDatum, ignoreCase))
      return true;
  }
  return false;
}

bool Array::isMember(DatumPtr aDatum, bool ignoreCase) {
  for (int i = 0; i < array.size(); ++i) {
    if (array[i].isEqual(aDatum, ignoreCase))
      return true;
  }
  return false;
}

DatumPtr Array::fromMember(DatumPtr aDatum, bool ignoreCase) {
  for (int i = 0; i < array.size(); ++i) {
    if (array[i].isEqual(aDatum, ignoreCase)) {
      Array *retval = new Array(origin, 0);
      retval->array.reserve(array.size() - i);
      while (i < array.size()) {
        retval->append(array[i]);
        ++i;
      }
      return DatumPtr(retval);
    }
  }
  return DatumPtr(new Array(origin, 0));
}

DatumPtr Array::datumAtIndex(int anIndex) {
  int index = anIndex - origin;
  Q_ASSERT((index >= 0) && (index < array.size()));
  return array[index];
}

DatumPtr Array::first() { return DatumPtr(origin); }

DatumPtr Array::last() {
  Q_ASSERT(array.size() > 0);
  return array[array.size() - 1];
}

DatumPtr Array::butfirst() {
  Array *retval = new Array(origin, 0);
  retval->array = array.mid(1, array.size() - 1);
  return DatumPtr(retval);
}

DatumPtr Array::butlast() {
  Array *retval = new Array(origin, 0);
  retval->array = array.mid(0, array.size() - 1);
  return DatumPtr(retval);
}

ArrayIterator Array::newIterator() { return ArrayIterator(&array); }

