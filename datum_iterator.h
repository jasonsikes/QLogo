#ifndef DATUM_ITERATOR_H
#define DATUM_ITERATOR_H

//===-- qlogo/datum_iterator.h - Iterator class definition ------*- C++ -*-===//
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
/// This file contains the declaration of the Iterator class and its subclasses,
/// WordIterator, ListIterator, and ArrayIterator, which are simple iterators
/// for their respective Datum subclasses.
///
//===----------------------------------------------------------------------===//



#include "datum_datump.h"
#include <QVector>


/// A very simple iterator. Base class does nothing. Meant to be subclassed.
class Iterator {
public:
    virtual DatumP element(); /// Returns current DatumP in collection, advances pointer.
    virtual bool elementExists(); /// Returns true if not at end.
};

class ListIterator : public Iterator {
protected:
    DatumP ptr;

public:
    ListIterator();

    /// Create a new ListIterator pointing to the head of the List.
    ListIterator(DatumP head);

    /// Return the element at the current location. Advance Iterator to the next location.
    DatumP element();

    /// Returns true if pointer references a valid element.
    bool elementExists();
};

/// Iterator for an Array.
class ArrayIterator : public Iterator {
protected:
    QVector<DatumP>::iterator arrayIter;
    QVector<DatumP>::iterator end;

public:
    ArrayIterator();

    /// Create a new ArrayIterator pointing to the first element of the Array.
    ArrayIterator(QVector<DatumP> *aArray);

    /// Return the element at the current index. Advance the index pointer.
    DatumP element();

    /// Returns true if the index points to an element in the Array.
    bool elementExists();
};


#endif // DATUM_ITERATOR_H
