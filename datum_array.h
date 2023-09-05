#ifndef DATUM_ARRAY_H
#define DATUM_ARRAY_H

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
