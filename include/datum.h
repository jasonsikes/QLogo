#ifndef DATUM_H
#define DATUM_H

//===-- qlogo/datum.h - Datum class definition -------*- C++ -*-===//
//
// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Datum class, which is the base class
/// for all the data classes used by QLogo.
///
//===----------------------------------------------------------------------===//

#include "QDebug"
#include <QChar>
#include <QStack>

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

/// @brief The unit of data for QLogo. The base class for Word, List, Array, ASTNode, etc.
class Datum
{
    friend class ListIterator;
    friend class DatumPtr;
    int retainCount;

    /// @brief If set to 'true', DatumPtr will send qDebug message when this is deleted.
    bool alertOnDelete = false;

  public:
    /// @brief Value returned by isa().
    enum DatumType
    {
        noType,
        wordType,
        listType,
        arrayType,
        astnodeType,
        procedureType,
        errorType
    };

    /// @brief Constructs a Datum
    ///
    /// @details The Datum class is the superclass for all data. The Datum superclass
    /// maintains retain counts (retain counts are manipulated by the DatumPtr class).
    Datum();

    /// @brief Destructor.
    virtual ~Datum();

    /// @brief Assignment operator.
    ///
    /// @details This is a virtual assignment operator. It is implemented by subclasses.
    /// @param other The Datum to assign to this.
    /// @return A reference to this.
    virtual Datum &operator=(const Datum &);

    /// @brief Return type of this object.
    virtual DatumType isa();

    /// @brief Return a string suitable for the PRINT command
    ///
    /// @param fullPrintp if true print with backslashes and vertical bars
    /// @param printDepthLimit limit the depth of sublists for readability.
    /// printDepthLimit = 1 means don't show sublists.
    /// printDepthLimit = 2 means show sublists, but don't show sublist's sublist.
    /// printDepthLimit = 0 means show '...' instead of this list.
    /// printDepthLimit = -1 means show all sublists.
    /// @param printWidthLimit limit the length of a string or list for readability.
    /// @return A string suitable for the PRINT command
    virtual QString printValue(bool = false, int printDepthLimit = -1, int printWidthLimit = -1);

    /// @brief Return a string suitable for the SHOW command
    ///
    /// @param fullPrintp if true print with backslashes and vertical bars
    /// @param printDepthLimit limit the depth of sublists for readability.
    /// printDepthLimit = 1 means don't show sublists.
    /// printDepthLimit = 2 means show sublists, but don't show sublist's sublist.
    /// printDepthLimit = 0 means show '...' instead of this list.
    /// printDepthLimit = -1 means show all sublists.
    /// @param printWidthLimit limit the length of a string or list for readability.
    /// @return A string suitable for the SHOW command
    virtual QString showValue(bool = false, int printDepthLimit = -1, int printWidthLimit = -1);
};

/// @brief A smart pointer to a Datum.
///
/// @details This class is a smart pointer to a Datum. It incorporates convenience
/// methods, reference-counting, and automatic destruction of the referred datum.
class DatumPtr
{
  protected:
    Datum *d;

    void destroy();

  public:
    /// Copy constructor. Increases retain count of the referred object.
    DatumPtr(const DatumPtr &other) noexcept;

    /// Default constructor. Points to notADatum (like NULL)
    DatumPtr();

    /// @brief Creates a pointer to a Datum object. Begins reference counting.
    ///
    /// Creates a pointer to the referenced object and increases its retain count.
    /// The referred object will be destroyed when the last object referring to it
    /// is destroyed.
    DatumPtr(Datum *);

    /// @brief Destructor.
    ///
    /// Decreases the retain count of the referred object. If this is the last
    /// pointer to the referred object (if its retain count reaches zero) the
    /// object is destroyed.
    ~DatumPtr();

    /// @brief Convenience constructor for "true" and "false".
    ///
    /// @param b The boolean value to create the DatumPtr for.
    /// @return A new DatumPtr pointing to a new Word containing the boolean value.
    explicit DatumPtr(bool b);

    /// @brief Convenience constructor for numbers.
    ///
    /// @param n The number to create the DatumPtr for.
    /// @return A new DatumPtr pointing to a new Word containing the number.
    explicit DatumPtr(double n);

    /// @brief Convenience constructor for integers.
    ///
    /// @param n The integer to create the DatumPtr for.
    /// @return A new DatumPtr pointing to a new Word containing the integer.
    explicit DatumPtr(int n);

    /// @brief Convenience constructor for strings.
    ///
    /// @param n The string to create the DatumPtr for.
    /// @param isVBarred Whether the string is created with vertical bars.
    /// @return A new DatumPtr pointing to a new Word containing the string.
    explicit DatumPtr(QString n, bool isVBarred = false);

    /// @brief Convenience constructor for const char strings.
    ///
    /// @param n The string to create the DatumPtr for.
    /// @return A new DatumPtr pointing to a new Word containing the string.
    explicit DatumPtr(const char *n);

    /// @brief Returns a pointer to the referred Datum or any of Datum's subclasses.
    ///
    /// @return A pointer to the referred Datum or any of Datum's subclasses.
    Datum *datumValue()
    {
        return d;
    }

    /// @brief Returns a pointer to the referred Datum as a Word.
    ///
    /// @return A pointer to the referred Datum as a Word.
    Word *wordValue();

    /// @brief Returns a pointer to the referred Datum as a List.
    ///
    /// @return A pointer to the referred Datum as a List.
    List *listValue();

    /// @brief Returns a pointer to the referred Datum as a Procedure.
    ///
    /// @return A pointer to the referred Datum as a Procedure.
    Procedure *procedureValue();

    /// @brief Returns a pointer to the referred Datum as an ASTNode.
    ///
    /// @return A pointer to the referred Datum as an ASTNode.
    ASTNode *astnodeValue();

    /// @brief Returns a pointer to the referred Datum as an Array.
    ///
    /// @return A pointer to the referred Datum as an Array.
    Array *arrayValue();

    /// @brief Returns a pointer to the referred Datum as an Error.
    ///
    /// @return A pointer to the referred Datum as an Error.
    Error *errorValue();

    /// @brief Returns true if the referred Datum is a Word, false otherwise.
    ///
    /// @return True if the referred Datum is a Word, false otherwise.
    bool isWord();

    /// @brief Returns true if the referred Datum is a List, false otherwise.
    ///
    /// @return True if the referred Datum is a List, false otherwise.
    bool isList();

    /// @brief Returns true if the referred Datum is an ASTNode, false otherwise.
    ///
    /// @return True if the referred Datum is an ASTNode, false otherwise.
    bool isASTNode();

    /// @brief Returns true if the referred Datum is an Array, false otherwise.
    ///
    /// @return True if the referred Datum is an Array, false otherwise.
    bool isArray();

    /// @brief Returns true if the referred Datum is an Error, false otherwise.
    ///
    /// @return True if the referred Datum is an Error, false otherwise.
    bool isError();

    /// @brief Returns true if the referred Datum is a notADatum, false otherwise.
    ///
    /// @return True if the referred Datum is a notADatum, false otherwise.
    bool isNothing();

    /// @brief Reassign the pointer to refer to the other object.
    ///
    /// @param other The DatumPtr to assign to this.
    /// @return A reference to this.
    DatumPtr &operator=(const DatumPtr &other) noexcept;

    /// @brief Reassign the pointer to refer to the other object.
    ///
    /// @param other The DatumPtr to assign to this.
    /// @return A reference to this.
    DatumPtr &operator=(DatumPtr *other) noexcept;

    /// @brief Return true if and only if other points to the same object as this.
    ///
    /// @param other The DatumPtr to compare to this.
    /// @return True if and only if other points to the same object as this.
    bool operator==(DatumPtr *other);

    /// @brief Return true if and only if other points to the same object as this.
    ///
    /// @param other The DatumPtr to compare to this.
    /// @return True if and only if other points to the same object as this.
    bool operator==(const DatumPtr &other);

    /// @brief Return true if and only if other does not point to the same object as this.
    ///
    /// @param other The DatumPtr to compare to this.
    /// @return True if and only if other does not point to the same object as this.
    bool operator!=(DatumPtr *other);

    /// @brief Return true if and only if other does not point to the same object as this.
    ///
    /// @param other The DatumPtr to compare to this.
    /// @return True if and only if other does not point to the same object as this.
    bool operator!=(const DatumPtr &other);

    /// @brief Return a string suitable for the PRINT command.
    ///
    /// @param fullPrintp if true print with backslashes and vertical bars
    /// @param printDepthLimit limit the depth of sublists for readability.
    /// @param printWidthLimit limit the length of a string or list for readability.
    /// @return A string suitable for the PRINT command
    QString printValue(bool fullPrintp = false, int printDepthLimit = -1, int printWidthLimit = -1);

    /// @brief Return a string suitable for the SHOW command.
    ///
    /// @param fullPrintp if true print with backslashes and vertical bars
    /// @param printDepthLimit limit the depth of sublists for readability.
    /// @param printWidthLimit limit the length of a string or list for readability.
    /// @return A string suitable for the SHOW command
    QString showValue(bool fullPrintp = false, int printDepthLimit = -1, int printWidthLimit = -1);

    /// @brief returns a DatumType enumerated value which is the DatumType of the referenced object.
    ///
    /// @return A DatumType enumerated value which is the DatumType of the referenced object.
    Datum::DatumType isa();

    /// @brief Set a mark on the datum so that debug message will print when datum is
    /// destroyed.
    ///
    /// @details This is used to help debug memory leaks. The MARK command is used to set
    /// a mark on the datum so that a debug message will be printed when the datum is destroyed.
    void alertOnDelete()
    {
        qDebug() << "MARKED: " << d << " " << d->showValue();
        d->alertOnDelete = true;
    }
};

Q_DECLARE_TYPEINFO(DatumPtr, Q_RELOCATABLE_TYPE);

/// @brief The basic unit of data in QLogo. A Word is a string or number.
///
/// A word can be a string or number. String operations can be used on numbers.
///
/// e.g. "FIRST 23 + 34" outputs "5"
///
/// Words that are initially defined as strings may be parsed as numbers.
///
/// e.g. "SUM WORD 3 4 2" outputs "36".
class Word : public Datum
{
  protected:
    QString rawString;
    QString keyString;
    QString printableString;
    double number;
    bool sourceIsNumber;

    void genRawString();
    void genPrintString();
    void genKeyString();

  public:
    /// @brief Set to true if the word was created with vertical bars as delimiters.
    /// Words created this way will not be separated during parsing or runparsing.
    bool isForeverSpecial;

    /// @brief Create a Word object that is invalid.
    Word();

    /// @brief Create a Word object with a string.
    ///
    /// @param other the string value of this word
    /// @param aIsForeverSpecial if set to 'true' characters defined with vertical bars will
    /// not be treated as token delimiters during parsing.
    Word(const QString other, bool aIsForeverSpecial = false);

    /// @brief Create a Word object with a number.
    Word(double other);

    /// @brief returns the number representation of the Word, if possible. Otherwise,
    /// returns NaN.
    double numberValue(void);

    /// @brief returns the DatumType of this object.
    DatumType isa();

    /// @brief convert encoded chars to their printable equivalents.
    ///
    /// @param fullPrintp if true print with backslashes and vertical bars
    /// @param printDepthLimit if zero print ellipsis.
    /// @param printWidthLimit limit the length of a string or list for readability.
    /// @return A string suitable for the PRINT command
    QString printValue(bool fullPrintp = false, int printDepthLimit = -1, int printWidthLimit = -1);

    /// @brief convert encoded chars to their printable equivalents for use in showValue().
    ///
    /// @param fullPrintp if true print with backslashes and vertical bars
    /// @param printDepthLimit if zero print ellipsis.
    /// @param printWidthLimit limit the length of a string or list for readability.
    /// @return A string suitable for the SHOW command
    QString showValue(bool fullPrintp = false, int printDepthLimit = -1, int printWidthLimit = -1);

    /// @brief Returns a string that can be used as a key for an assiciative container.
    ///
    /// @return A string that can be used as the name of a procedure, variable, or property list.
    QString keyValue();

    /// @brief Returns the string with the special character encoding intact.
    ///
    /// @return The string with the special character encoding intact.
    QString rawValue();

    /// @brief Return true iff this word was created with a number.
    ///
    /// @return True iff this word was created with a number.
    bool isSourceNumber()
    {
        return sourceIsNumber;
    }
};

/// @brief The container that allows efficient read and write access to its elements.
struct Array : public Datum
{
    /// @brief The container that stores the elements of the Array.
    QList<DatumPtr> array;

    /// @brief Create an Array containing aSize empty List with starting index at aOrigin.
    ///
    /// @param aOrigin The starting index of the Array.
    /// @param aSize The number of elements in the Array.
    Array(int aOrigin = 1, int aSize = 0);

    /// @brief Create an Array containing items copied from source with index starting at aOrigin.
    ///
    /// @param aOrigin The starting index of the Array.
    /// @param source The source list to copy from.
    Array(int aOrigin, List *source);

    /// @brief Destructor.
    ~Array();

    /// @brief returns the DatumType of this object.
    ///
    /// @return The DatumType of this object.
    DatumType isa();

    /// @brief Return a string suitable for the PRINT command
    ///
    /// @param fullPrintp if true print with backslashes and vertical bars
    /// @param printDepthLimit if zero print ellipsis.
    /// @param printWidthLimit limit the length of the array for readability.
    /// @return A string suitable for the PRINT command
    QString printValue(bool fullPrintp = false, int printDepthLimit = -1, int printWidthLimit = -1);

    /// @brief Return a string suitable for the SHOW command
    ///
    /// @param fullPrintp if true print with backslashes and vertical bars
    /// @param printDepthLimit if zero print ellipsis.
    /// @param printWidthLimit limit the length of the array for readability.
    /// @return A string suitable for the SHOW command
    QString showValue(bool fullPrintp = false, int printDepthLimit = -1, int printWidthLimit = -1);

    /// @brief The starting index of this Array.
    int origin = 1;
};

/// @brief The general container for data. The QLogo List is implemented as a linked list.
///
/// @details The List class is a linked list of DatumPtrs. The head of the list is the first
/// element and the tail is the remainder of the list after the head. The lastNode is only
/// used during list initialization and should not be accessed by the user.
class List : public Datum
{
    friend class ListIterator;
    friend class Parser; // Parser needs access to astList

    /// @brief If this list is parsed into an AST, this is the list of ASTNode roots.
    QList<DatumPtr> astList;

  public:
    /// @brief The head of the list, also called the 'element'.
    ///
    /// @details The head of the list. Must not be nothing
    DatumPtr head;

    /// @brief The remainder of the list after the head. Must be either a list or nothing.
    DatumPtr tail;

    /// @brief The last node of the list.
    ///
    /// @details Only use as a shortcut during list initialization. Should not be directly
    /// accessible to user. It should not be used after list has been made available to the
    /// user.
    DatumPtr lastNode;

    /// @brief The time, as returned by QDateTime::currentMSecsSinceEpoch().
    ///
    /// @details This is used to determine if the list needs to be reparsed.
    /// Set when the most recent ASTList is generated from this list. Reset this
    /// to zero when the list is modified to trigger reparsing, if needed.
    qint64 astParseTimeStamp;

    /// @brief Create an empty List.
    List();

    /// @brief Create a new list populated with elements of Array.
    ///
    /// @param source The source array to copy from.
    List(Array *source);

    /// @brief Create a new list by attaching item as the head of srcList.
    ///
    /// @param item The item to add to the head of the list.
    /// @param srcList The list to copy from, this will become the tail of the new list.
    List(DatumPtr item, List *srcList);

    /// @brief Destructor.
    ~List();

    /// @brief returns the DatumType of this object.
    ///
    /// @return The DatumType of this object.
    DatumType isa();

    /// @brief Return a string suitable for the PRINT command
    ///
    /// @param fullPrintp if true print with backslashes and vertical bars
    /// @param printDepthLimit if zero print ellipsis.
    /// @param printWidthLimit limit the length of the array for readability.
    /// @return A string suitable for the PRINT command
    QString printValue(bool fullPrintp = false, int printDepthLimit = -1, int printWidthLimit = -1);

    /// @brief Return a string suitable for the SHOW command
    ///
    /// @param fullPrintp if true print with backslashes and vertical bars
    /// @param printDepthLimit if zero print ellipsis.
    /// @param printWidthLimit limit the length of the array for readability.
    /// @return A string suitable for the SHOW command
    QString showValue(bool fullPrintp = false, int printDepthLimit = -1, int printWidthLimit = -1);

    /// @brief Empty the List
    void clear();

    /// @brief Add an element to the end of the list.
    /// @details DO NOT USE after the List has been modified by any other method. This is
    /// only to be used during initial contruction of the list. It should not be available
    /// to the user.
    void append(DatumPtr element);

    /// @brief Returns the count of elements in the List.
    ///
    /// @details This should only be used when you need a specfic number since it traverses
    /// the list. Consider using isEmpty().
    ///
    /// @return The count of elements in the List.
    int count();

    /// @brief Returns the element pointed to by anIndex.
    ///
    /// @param anIndex The index of the element to return.
    /// @return The element at the given index, starting at 1. Ensure that the index is
    /// within the bounds of the list. Triggers an error if the index is out of bounds.
    DatumPtr itemAtIndex(int anIndex);

    /// @brief Returns true if this is an empty list.
    ///
    /// @return True if this is an empty list, false otherwise.
    bool isEmpty();

    /// @brief Replaces everything but the first item in the List with aValue.
    ///
    /// @param aValue The value to replace the first item with.
    void setButfirstItem(DatumPtr aValue);

    /// @brief Create a new ListIterator pointing to the head of the List.
    /// @return A new ListIterator pointing to the head of the List.
    ListIterator newIterator();
};

/// @brief A class that simplifies iterating through a list.
///
/// @details This class is used to iterate through a list. Unlike the c++ STL iterator,
/// this class is very simple. There are only two methods: element() and elementExists().
/// The element() method returns the current element and advances the iterator to the
/// next element. The elementExists() method returns true if the iterator is pointing to
/// a valid element.
class ListIterator
{
  protected:
    DatumPtr ptr;

  public:
    /// @brief Create an empty ListIterator.
    ListIterator();

    /// @brief Create a new ListIterator pointing to the head of the List.
    ListIterator(DatumPtr aList);

    /// @brief Return the element at the current location. Advance Iterator to the next location.
    ///
    /// @return The element at the current location.
    DatumPtr element();

    /// @brief Returns true if pointer references a valid element.
    bool elementExists();
};

/// @brief A datum that has no value.
extern Datum notADatum;

/// @brief A pointer to notADatum, like NULL.
extern DatumPtr nothing;

#endif // DATUM_H
