#ifndef DATUM_ITERATOR_H
#define DATUM_ITERATOR_H


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

/// Iterator for a Word. Each element is a character in the Word.
class WordIterator : public Iterator {
    QString::iterator charIter;
    QString::iterator end;

public:
    WordIterator();

    /// Create a new Iterator pointing to the first char of a Word.
    WordIterator(Word *aWord);

    /// Returns the character at the current index as a Word. Advances the index.
    DatumP element();

    /// Returns true if there is a character at the curent index.
    bool elementExists();
};

#endif // DATUM_ITERATOR_H
