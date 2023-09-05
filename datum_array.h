#ifndef DATUM_ARRAY_H
#define DATUM_ARRAY_H

//===-- qlogo/datum_array.h - Array class definition -------*- C++ -*-===//
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



#include "datum.h"
#include <QVector>

/// The container that allows efficient read and write access to its elements.
class Array : public Datum {
    friend class ArrayIterator;

protected:
    QVector<DatumP> array;

public:

    /// Create an Array containing aSize empty List with starting index at aOrigin.
    Array(int aOrigin, int aSize);

    /// Create an Array containing items copied from source with index starting at aOrigin.
    Array(int aOrigin, List *source);
    ~Array();
    DatumType isa();
    QString name();
    QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                       int printWidthLimit = -1);
    QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                      int printWidthLimit = -1);

    /// Returns true if items in other Array are equal to this Array's items.
    bool isEqual(DatumP other, bool ignoreCase);

    /// The starting index of this Array.
    int origin = 1;

    /// Returns the number of items in this Array.
    int size();

    /// Returns the item pointed to by anIndex.
    DatumP datumAtIndex(int anIndex);

    /// Returns true if anIndex can point to a valid element in the Array.
    bool isIndexInRange(int anIndex);

    /// Replace item at anIndex with aValue.
    void setItem(int anIndex, DatumP aValue);

    /// Replace the first item in the Array with aValue.
    void setFirstItem(DatumP aValue);

    /// Replace all but the first item in the Array with elements from aValue Array.
    void setButfirstItem(DatumP aValue);

    /// Recursively searches Array for aDatum. Returns true if found.
    bool containsDatum(DatumP aDatum, bool ignoreCase);

    /// Returns true if aDatum is a member of Array.
    bool isMember(DatumP aDatum, bool ignoreCase);

    /// Returns a new Array beginning with the first occurrence of aDatum to the end of the Array.
    DatumP fromMember(DatumP aDatum, bool ignoreCase);

    /// Returns the first element of the Array.
    DatumP first();

    /// Returns a new Array which is everything but the first element of this Array.
    DatumP butfirst();

    /// Returns the last element of the Array.
    DatumP last(void);

    /// Returns a new Array which is everything but the last element of this Array.
    DatumP butlast(void);

    /// Add an element to the end of this Array.
    void append(DatumP value);

    ArrayIterator newIterator();
};



#endif // DATUM_ARRAY_H
