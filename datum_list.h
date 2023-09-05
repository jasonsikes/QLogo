#ifndef DATUM_LIST_H
#define DATUM_LIST_H

#include "datum_datump.h"
#include <QList>

/// An element of a List.
class ListNode : public Datum {
public:
    DatumType isa() { return listNodeType; }

    /// The item at this position in the list.
    DatumP item;

    /// A pointer to the next ListNode.
    DatumP next;
};

/// The container for data. The QLogo List is implemented as a linked list.
class List : public Datum {
    friend class ListIterator;
    friend class Parser; // Parser needs access to astList and astParseTimeStamp

protected:
    DatumP head;
    DatumP lastNode;
    int listSize;
    QList<DatumP> astList;
    qint64 astParseTimeStamp;
    void setListSize();

public:

    /// Create an empty List
    List();

    /// Create a new List using the elements of the source List.
    List(List *source);

    /// Create a new List using the elements of the source Array.
    List(Array *source);

    ~List();
    DatumType isa();
    QString name();
    QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                       int printWidthLimit = -1);
    QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                      int printWidthLimit = -1);
    bool isEqual(DatumP other, bool ignoreCase);

    /// Return the first item of the List.
    DatumP first(void);

    /// Return a new List starting with the second item of the List.
    DatumP butfirst(void);

    /// Empty the List
    void clear();

    /// Add an element to the end of the list.
    /// DO NOT USE after the List has been modified by any other method.
    void append(DatumP element);

    /// Returns the count of elements in the List.
    int size() { return listSize; }

    /// Returns the last elements of the List.
    DatumP last();

    /// Creates a new List using all but the last element of this List.
    DatumP butlast(void);

    /// Adds an element to the head of this List.
    void prepend(DatumP element);

    /// Creates a new List by adding an element to the head of this List.
    DatumP fput(DatumP item);

    /// Returns the element pointed to by anIndex.
    DatumP datumAtIndex(int anIndex);

    /// Returns true if anIndex is between 1 and the count of elements in the List.
    bool isIndexInRange(int anIndex);

    /// Replaces the item pointed to by anIndex with aValue.
    void setItem(int anIndex, DatumP aValue);

    /// Replaces the first item in the List with aValue.
    void setFirstItem(DatumP aValue);

    /// Replaces everything but the first item in the List with aValue.
    void setButfirstItem(DatumP aValue);

    /// Recursively searches List for aDatum. Returns true if found.
    bool containsDatum(DatumP aDatum, bool ignoreCase);

    /// Returns true if aDatum is a member of this List.
    bool isMember(DatumP aDatum, bool ignoreCase);

    /// Non-recursively searches this List for aDatum. Returns a new List starting
    /// from where aDaum was found to the end of this List.
    DatumP fromMember(DatumP aDatum, bool ignoreCase);

    ListIterator newIterator(void);
};




#endif // DATUM_LIST_H
