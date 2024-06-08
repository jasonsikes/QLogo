#ifndef DATUM_ARRAY_H
#define DATUM_ARRAY_H

//===-- qlogo/datum/array.h - Array class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Array class. The array is a
/// collection of data the can be accessed by index.
///
//===----------------------------------------------------------------------===//



#include "datum/datum.h"

/// The container that allows efficient read and write access to its elements.
struct Array : public Datum {
    QList<DatumPtr> array;

    /// Create an Array containing aSize empty List with starting index at aOrigin.
    Array(int aOrigin = 1, int aSize = 0);

    /// Create an Array containing items copied from source with index starting at aOrigin.
    Array(int aOrigin, List *source);


    ~Array();
    DatumType isa();

    QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                       int printWidthLimit = -1);
    QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                      int printWidthLimit = -1);

    /// Returns true if items in other Array are equal to this Array's items.
    bool isEqual(DatumPtr other, bool ignoreCase);

    /// The starting index of this Array.
    int origin = 1;

    /// Returns the number of items in this Array.
    int size();

    /// Empty the array of its contents and set the origin
    void clear();

    /// Returns true if anIndex can point to a valid element in the Array.
    bool isIndexInRange(int anIndex);

    /// Returns the first element of the Array.
    DatumPtr first();

    /// Returns a new Array which is everything but the first element of this Array.
    DatumPtr butfirst();

    /// Returns the last element of the Array.
    DatumPtr last(void);

    /// Add an element to the end of this Array.
    void append(DatumPtr value);

    ArrayIterator newIterator();
};


#endif // DATUM_ARRAY_H
