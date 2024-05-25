#ifndef DATUM_LIST_H
#define DATUM_LIST_H

//===-- qlogo/datum/list.h - List class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the List class, which is the basic
/// collection class in the Logo language.
///
//===----------------------------------------------------------------------===//


#include "datum/datump.h"

/// The container for data. The QLogo List is implemented as a linked list.
class List : public Datum {
    friend class ListIterator;
    friend class Parser; // Parser needs access to astList

protected:
    QList<DatumPtr> astList;

public:

    /// The head of the list, also called the 'element'.
    DatumPtr head;

    /// The remainder of the list after the head. Can be a list or nothing.
    DatumPtr tail;

    /// The last node of the list. Only use as a shortcut during list
    /// initialization. Should not be directly accessible to user.
    DatumPtr lastNode;

    /// The time, as returned by QDateTime::currentMSecsSinceEpoch(), which is
    /// set when the most recent ASTList is generated from this list. Reset this
    /// to zero when the list is modified to trigger reparsing, if needed.
    /// TODO: reconsider whether this is useful, as it is unfeasable to keep
    /// track of all this list's sublist modifications.
    qint64 astParseTimeStamp;

    /// Create an empty List.
    List();

    /// Create a new list populated with elements of Array.
    List(Array *source);

    /// Create a new list populated with elements of another List.
    // TODO: When do we use this?
    List(List *source);

    /// Create a new list by attaching item as the head of srcList.
    List(DatumPtr item, List *srcList);

    ~List();
    DatumType isa();
    QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                       int printWidthLimit = -1);
    QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                      int printWidthLimit = -1);
    bool isEqual(DatumPtr other, bool ignoreCase);

    /// Return the first item of the List.
    DatumPtr first(void);

    /// Return a new List starting with the second item of the List.
    DatumPtr butfirst(void);

    /// Empty the List
    void clear();

    /// Add an element to the end of the list.
    /// DO NOT USE after the List has been modified by any other method.
    void append(DatumPtr element);

    /// Returns the count of elements in the List. This should only be used when
    /// you need a specfic number. Consider using isEmpty() instead.
    int count();

    /// Returns the last element of the List.
    DatumPtr last();

    /// Returns the element pointed to by anIndex.
    DatumPtr itemAtIndex(int anIndex);

    /// Returns true if this is an empty list.
    bool isEmpty();

    /// Returns true if anIndex is between 1 and the count of elements in the List.
    bool isIndexInRange(int anIndex);

    /// Replaces everything but the first item in the List with aValue.
    void setButfirstItem(DatumPtr aValue);

    /// Recursively searches List for aDatum. Returns true if found.
    bool containsDatum(DatumPtr aDatum, bool ignoreCase);

    /// Returns true if aDatum is a member of this List.
    bool isMember(DatumPtr aDatum, bool ignoreCase);

    /// Non-recursively searches this List for aDatum. Returns a new List starting
    /// from where aDaum was found to the end of this List.
    DatumPtr fromMember(DatumPtr aDatum, bool ignoreCase);

    ListIterator newIterator(void);
};


#endif // DATUM_LIST_H
