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

#include "datum/array.h"
#include "datum/datump.h"
#include "datum/list.h"
#include "datum/iterator.h"
#include <qdebug.h>
#include <QObject>

QList<void *> aryVisited;
QList<void *> otherAryVisited;

Array::Array(int aOrigin, int aSize) {
  origin = aOrigin;
  array.reserve(aSize);
}


void Array::clear()
{
  origin = 1;
  array.clear();
}

Array::Array(int aOrigin, List *source)
{
  auto iter = source->newIterator();
    origin = aOrigin;
  while (iter.elementExists()) {
    append(iter.element());
  }
}

Array::~Array() {}

Datum::DatumType Array::isa() { return Datum::arrayType; }

QString Array::printValue(bool fullPrintp, int printDepthLimit,
                          int printWidthLimit) {
    if (!aryVisited.contains(this)) {
        aryVisited.push_back(this);
        auto iter = array.begin();
        if (iter == array.end()) {
            return "{}";
        }
        if ((printDepthLimit == 0) || (printWidthLimit == 0)) {
            return "{...}";
        }
        int printWidth = printWidthLimit;
        QString retval = "{";
        do {
            if (iter != array.begin())
                retval.append(' ');
            if (printWidth == 0) {
                retval.append("...");
                break;
            }
            retval.append(
        iter->showValue(fullPrintp, printDepthLimit - 1, printWidthLimit));
            --printWidth;
        }while (++iter != array.end());
        retval += "}";
        aryVisited.removeOne(this);
        return retval;
    }
    return "...";
}

QString Array::showValue(bool fullPrintp, int printDepthLimit,
                         int printWidthLimit) {
    return printValue(fullPrintp, printDepthLimit, printWidthLimit);
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

DatumPtr Array::first() { return DatumPtr(origin); }

DatumPtr Array::last() {
  Q_ASSERT(array.size() > 0);
  return array[array.size() - 1];
}

ArrayIterator Array::newIterator() { return ArrayIterator(&array); }

