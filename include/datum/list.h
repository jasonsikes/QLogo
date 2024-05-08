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
#include <QList>

/// An element of a List.
class ListNode : public Datum {
    void addToPool();

public:
    DatumType isa() { return listNodeType; }

    /// The item at this position in the list.
    DatumPtr item;

    /// A pointer to the next ListNode.
    DatumPtr next;

    static ListNode * alloc();
};

/// The container for data. The QLogo List is implemented as a linked list.
class List : public Datum {
    friend class ListIterator;
    friend class Parser; // Parser needs access to astList and astParseTimeStamp

protected:
    DatumPtr head;
    DatumPtr lastNode;
    int listSize;
    QList<DatumPtr> astList;
    qint64 astParseTimeStamp;
    void setListSize();

    void addToPool();

public:

    /// Create an empty List. Don't use this. It is slow.
    /// Use alloc() instead.
    List();

    /// Create a new list populated with elements of Array.
    static List * alloc(Array *source);

    /// Create a new list populated with elements of another List.
    static List * alloc(List *source);

    /// Allocate an empty List.
    static List * alloc();

    ~List();
    DatumType isa();
    QString name();
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

    /// Returns the count of elements in the List.
    int size() { return listSize; }

    /// Returns the last elements of the List.
    DatumPtr last();

    /// Creates a new List using all but the last element of this List.
    DatumPtr butlast(void);

    /// Adds an element to the head of this List.
    void prepend(DatumPtr element);

    /// Creates a new List by adding an element to the head of this List.
    DatumPtr fput(DatumPtr item);

    /// Returns the element pointed to by anIndex.
    DatumPtr datumAtIndex(int anIndex);

    /// Returns true if anIndex is between 1 and the count of elements in the List.
    bool isIndexInRange(int anIndex);

    /// Replaces the item pointed to by anIndex with aValue.
    void setItem(int anIndex, DatumPtr aValue);

    /// Replaces the first item in the List with aValue.
    void setFirstItem(DatumPtr aValue);

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
