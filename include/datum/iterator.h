#ifndef DATUM_ITERATOR_H
#define DATUM_ITERATOR_H

//===-- qlogo/datum/iterator.h - ListIterator class definition ------*- C++ -*-===//
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
/// This file contains the declaration of the ListIterator class, which is a
/// simple iterator for lists.
///
//===----------------------------------------------------------------------===//



#include "datum/datump.h"


class ListIterator {
protected:
    DatumPtr ptr;

public:
    ListIterator();

    /// Create a new ListIterator pointing to the head of the List.
    ListIterator(DatumPtr aList);

    /// Return the element at the current location. Advance Iterator to the next location.
    DatumPtr element();

    /// Returns true if pointer references a valid element.
    bool elementExists();
};

#endif // DATUM_ITERATOR_H
