#ifndef DATUM_H
#define DATUM_H

//===-- qlogo/datum.h - Datum class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Datum class, which is the base class
/// for all the data classes used by QLogo.
///
//===----------------------------------------------------------------------===//

#include <QChar>
#include <QStack>
#include "QDebug"

#ifndef dv
#define dv(x) qDebug()<<#x<<'='<<x
#endif

class ASTNode;
class Word;
class List;
class Array;
class Error;
class DatumPtr;
class Procedure;

class ListIterator;

class Kernel;
typedef DatumPtr (Kernel::*KernelMethod)(DatumPtr);

/// Convert "raw" encoding to Char encoding.
QChar rawToChar(const QChar &src);

/// Convert Char encoding to "raw" encoding.
QChar charToRaw(const QChar &src);

/// Convert a string from "raw" encoding to Char. In place.
void rawToChar(QString &src);

/// \brief Return a list of two words for the NODES command.
///
/// Returns a list of two words (numbers). The first represents the number of Datums
/// currently in use. The second shows the maximum number of Datums in use at any one
/// time since the last invocation of this function.
///
/// Each Word, List, and Array is a node. So, for example, an array of two words is three
/// nodes (1 Array + 2 Words). Furthermore, if the list is "RUN" (e.g. "RUN [forward 100]")
/// then ASTNodes will be created, adding to the number of nodes.
DatumPtr nodes();

/// The unit of data for QLogo. The base class for Word, List, Array, ASTNode, etc.
class Datum {
  friend class ListIterator;
  friend class DatumPtr;
  int retainCount;

  // If set to 'true', DatumPtr will send qDebug message when this is deleted.
  bool alertOnDelete = false;

public:
  /// Value returned by isa().
  enum DatumType {
    noType,
    wordType,
    listType,
    arrayType,
    astnodeType,
    procedureType,
    errorType
  };

  /// \brief Constructs a Datum
  ///
  /// The Datum class is the superclass for all data.
  /// The Datum superclass maintains retain counts (manipulated by the DatumPtr class).
  Datum();

  virtual ~Datum();

  Datum &operator=(const Datum &);

  /// Return type of this object.
  virtual DatumType isa();

  /// \brief Return a string suitable for the PRINT command
  ///
  /// \param fullPrintp if true print with backslashes and vertical bars
  /// \param printDepthLimit limit the depth of sublists for readability.
  /// printDepthLimit = 1 means don't show sublists.
  /// printDepthLimit = 2 means show sublists, but don't show sublist's sublist.
  /// printDepthLimit = 0 means show '...' instead of this list.
  /// printDepthLimit = -1 means show all sublists.
  /// \param printWidthLimit limit the length of a string or list for readability.
  virtual QString printValue(
      bool = false, int printDepthLimit = -1,
      int printWidthLimit = -1);

  /// \brief Return a string suitable for the SHOW command
  ///
  /// \param fullPrintp if true print with backslashes and vertical bars
  /// \param printDepthLimit limit the depth of sublists for readability.
  /// printDepthLimit = 1 means don't show sublists.
  /// printDepthLimit = 2 means show sublists, but don't show sublist's sublist.
  /// printDepthLimit = 0 means show '...' instead of this list.
  /// printDepthLimit = -1 means show all sublists.
  /// \param printWidthLimit limit the length of a string or list for readability.
  virtual QString showValue(
      bool = false, int printDepthLimit = -1,
      int printWidthLimit = -1);
};


/// A pointer to a Datum. Incorporates convenience methods, reference-counting, and automatic destruction.
class DatumPtr {
protected:

    Datum *d;

    void destroy();

public:

    /// Copy constructor. Increases retain count of the referred object.
    DatumPtr(const DatumPtr &other) noexcept;

    /// Default constructor. Points to notADatum (like NULL)
    DatumPtr();

    /// \brief Creates a pointer to a Datum object. Begins reference counting.
    ///
    /// Creates a pointer to the referenced object and increases its retain count.
    /// The referred object will be destroyed when the last object referring to it
    /// is destroyed.
    DatumPtr(Datum *);

    /// Convenience constructor for "true" and "false".
    explicit DatumPtr(bool b);

    /// Convenience constructor for numbers.
    explicit DatumPtr(double n);

    /// Convenience constructor for integers.
    explicit DatumPtr(int n);

    /// Convenience constructor for strings.
    explicit DatumPtr(QString n, bool isVBarred = false);

    /// Convenience constructor for const char strings
    explicit DatumPtr(const char* n);


    /// \brief Destructor.
    ///
    /// Decreases the retain count of the referred object. If this is the last
    /// pointer to the referred object (if its retain count reaches zero) the
    /// object is destroyed.
    ~DatumPtr();

    /// Returns a pointer to the referred Datum or any of Datum's subclasses.
    Datum *datumValue() { return d; }

    /// Returns a pointer to the referred Datum as a Word.
    Word *wordValue();

    /// Returns a pointer to the referred Datum as a List.
    List *listValue();

    /// Returns a pointer to the referred Datum as a Procedure.
    Procedure *procedureValue();

    /// Returns a pointer to the referred Datum as an ASTNode.
    ASTNode *astnodeValue();

    /// Returns a pointer to the referred Datum as an Array.
    Array *arrayValue();

    /// Returns a pointer to the referred Datum as an Error.
    Error *errorValue();

    /// Returns true if the referred Datum is a Word, false otherwise.
    bool isWord();

    /// Returns true if the referred Datum is a List, false otherwise.
    bool isList();

    /// Returns true if the referred Datum is an ASTNode, false otherwise.
    bool isASTNode();

    /// Returns true if the referred Datum is an Array, false otherwise.
    bool isArray();

    /// Returns true if the referred Datum is an Error, false otherwise.
    bool isError();

    /// Returns true if the referred Datum is a notADatum, false otherwise.
    bool isNothing();

    /// Reassign the pointer to refer to the other object.
    DatumPtr &operator=(const DatumPtr &other) noexcept;

    /// Reassign the pointer to refer to the other object.
    DatumPtr &operator=(DatumPtr *other) noexcept;

    /// Return true if and only if other points to the same object as this.
    bool operator==(DatumPtr *other);

    /// Return true if and only if other points to the same object as this.
    bool operator==(const DatumPtr &other);

    /// Return true if and only if other does not point to the same object as this.
    bool operator!=(DatumPtr *other);

    /// Return true if and only if other does not point to the same object as this.
    bool operator!=(const DatumPtr &other);

    /// Return a string suitable for the PRINT command
    QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                       int printWidthLimit = -1);

    /// Return a string suitable for the SHOW command
    QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                      int printWidthLimit = -1);

    /// returns a DatumType enumerated value which is the DatumType of the referenced object.
    Datum::DatumType isa();

    /// Set a mark on the datum so that debug message will print when datum is
    /// destroyed.
    void alertOnDelete() {
        qDebug() <<"MARKED: " <<d <<" " <<d->showValue();
        d->alertOnDelete = true;
    }
};

// If/when List is implemented using QList, this will increase efficiency.
// Since we're using linked lists, this is a noop for now.
Q_DECLARE_TYPEINFO(DatumPtr, Q_RELOCATABLE_TYPE);


/// \brief The basic unit of data in QLogo. A Word is a string or number.
///
/// A word can be a string or number. String operations can be used on numbers.
///
/// e.g. "FIRST 23 + 34" outputs "5"
///
/// Words that are initially defined as strings may be parsed as numbers.
///
/// e.g. "SUM WORD 3 4 2" outputs "36".
class Word : public Datum {
protected:

    QString rawString;
    QString keyString;
    QString printableString;
    void genRawString();
    void genPrintString();
    void genKeyString();
    double number;
    bool keyStringIsValid;
    bool printableStringIsValid;
    bool sourceIsNumber;

public:

    /// Set to true if the word was created with vertical bars as delimiters.
    /// Words created this way will not be separated during parsing or runparsing.
    bool isForeverSpecial;

    /// Create a Word object that is invalid.
    Word();

    /// Create a Word object with a string.
    ///
    /// \param other the string value of this word
    /// \param aIsForeverSpecial characters defined with vertical bars will retain their special use.
    Word(const QString other, bool aIsForeverSpecial = false);

    /// Create a Word object with a number.
    Word(double other);

    /// returns the number representation of the Word, if possible. Otherwise,
    /// returns NaN.
    double numberValue(void);

    DatumType isa();

    // print() and show() will convert encoded chars to their displayable
    // equivalents
    QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                       int printWidthLimit = -1);
    QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                      int printWidthLimit = -1);

    /// Returns a string that can be used as the name of a procedure, variable, or property list.
    QString keyValue(); // for use as key for procedure names and variable names

    /// Returns the string with the special character encoding intact.
    QString rawValue();

    /// Return true iff this word was created with a number.
    bool isSourceNumber() { return sourceIsNumber; }

};




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

    /// The starting index of this Array.
    int origin = 1;
};


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

    /// Empty the List
    void clear();

    /// Add an element to the end of the list.
    /// DO NOT USE after the List has been modified by any other method.
    void append(DatumPtr element);

    /// Returns the count of elements in the List. This should only be used when
    /// you need a specfic number. Consider using isEmpty() instead.
    int count();

    /// Returns the element pointed to by anIndex.
    DatumPtr itemAtIndex(int anIndex);

    /// Returns true if this is an empty list.
    bool isEmpty();

    /// Replaces everything but the first item in the List with aValue.
    void setButfirstItem(DatumPtr aValue);

    ListIterator newIterator(void);
};


/// A class that simplifies iterating through a list.
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



/// A datum that has no value.
extern Datum notADatum;

/// A pointer to notADatum, like NULL.
extern DatumPtr nothing;


#endif // DATUM_H
