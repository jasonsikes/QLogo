#ifndef DATUM_TYPES_H
#define DATUM_TYPES_H

#include "datum_ptr.h"
#include <QString>
#include <QList>

class ListIterator;
class VisitedSet;

/// Convert "raw" encoding to Char encoding.
QChar rawToChar(const QChar &src);

/// Convert Char encoding to "raw" encoding.
QChar charToRaw(const QChar &src);

/// Convert a string from "raw" encoding to Char. In place.
void rawToChar(QString &src);



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
    mutable QString rawString;
    mutable QString keyString;
    mutable QString printableString;
    mutable double number;
    mutable bool boolean;
    bool sourceIsNumber;

    void genRawString() const;
    void genPrintString() const;
    void genKeyString() const;

  public:
    /// @brief Set to true if the word was created with vertical bars as delimiters.
    /// Words created this way will not be separated during parsing or runparsing.
    bool isForeverSpecial;

    /// @brief True if a number was calculated/given AND the number is valid
    /// @note Read this AFTER calling numberValue()
    mutable bool numberIsValid = false;

    /// @brief True if the word is either "true" or "false".
    /// @note Read this AFTER calling boolValue()
    mutable bool boolIsValid = false;

    /// @brief Create a Word object that is invalid.
    Word();

    /// @brief Create a Word object with a string.
    ///
    /// @param other the string value of this word
    /// @param aIsForeverSpecial if set to 'true' characters defined with vertical bars will
    /// not be treated as token delimiters during parsing.
    Word(const QString &other, bool aIsForeverSpecial = false);

    /// @brief Create a Word object with a number.
    Word(double other);

    ~Word();

    /// @brief returns the number representation of the Word, if possible.
    /// @note check numberIsValid to determine validity AFTER calling this. It may seem counterintuitive,
    /// but it's because that is the procedure of the underlying Qt toolkit.
    double numberValue(void) const;

    /// @brief returns the boolean representation of the Word, if possible.
    /// @note check boolIsValid to determine validity AFTER calling this. It may seem counterintuitive,
    /// but it's because that is the procedure of the underlying Qt toolkit.
    bool boolValue(void) const;

    virtual QString toString( Datum::ToStringFlags flags = Datum::ToStringFlags_None, int printDepthLimit = -1, int printWidthLimit = -1, VisitedSet *visited = nullptr) const override;


    /// @brief Return true iff this word was created with a number.
    ///
    /// @return True iff this word was created with a number.
    bool isSourceNumber() const
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

    virtual QString toString( Datum::ToStringFlags flags = Datum::ToStringFlags_None, int printDepthLimit = -1, int printWidthLimit = -1, VisitedSet *visited = nullptr) const override;

    /// @brief The starting index of this Array.
    int origin = 1;
};

/// @brief The general container for data. The QLogo List is implemented as a linked list.
///
/// @details The List class is a linked list of DatumPtrs. The head of the list must contain a
/// DatumPtr to a Word, List, or Array. The tail of the list is a DatumPtr which must point to
/// a List or EmptyList.
class List : public Datum
{
    friend class ListIterator;
    friend class Compiler;

  public:
    /// @brief The head of the list, also called the 'element'.
    ///
    /// @details The head of the list.
    DatumPtr head;

    /// @brief The remainder of the list after the head. Must be either List or EmptyList.
    DatumPtr tail;

    /// @brief The time, as returned by QDateTime::currentMSecsSinceEpoch().
    ///
    /// @details This is used to determine if the list needs to be reparsed.
    /// Set when the most recent ASTList is generated from this list. Reset this
    /// to zero when the list is modified to trigger reparsing, if needed.
    ///
    /// @todo It's difficult to know when the list is modified. We should consider
    /// removing this.
    qint64 astParseTimeStamp;

    /// @brief Create a new list by attaching item as the head of srcList.
    ///
    /// @param item The item to add to the head of the list.
    /// @param srcList The list to copy from, this will become the tail of the new list.
    List(const DatumPtr &item, List *srcList);

    /// @brief Destructor.
    virtual ~List();

    virtual QString toString( ToStringFlags flags = ToStringFlags_None, int printDepthLimit = -1, int printWidthLimit = -1, VisitedSet *visited = nullptr) const override;

    /// @brief Empty the List
    void clear();

    /// @brief Returns the count of elements in the List.
    ///
    /// @details This should only be used when you need a specfic number since it traverses
    /// the list. Consider using isEmpty().
    ///
    /// @return The count of elements in the List.
    int count() const;

    /// @brief Returns the element pointed to by anIndex.
    ///
    /// @param anIndex The index of the element to return.
    /// @return The element at the given index, starting at 1. Ensure that the index is
    /// within the bounds of the list. Triggers an error if the index is out of bounds.
    DatumPtr itemAtIndex(int anIndex) const;

    /// @brief Returns true if this is an empty list.
    ///
    /// @return True if this is an empty list, false otherwise.
    bool isEmpty() const;

    /// @brief Replaces everything but the first item in the List with aValue.
    ///
    /// @param aValue The value to replace the first item with.
    void setButfirstItem(const DatumPtr &aValue);

    /// @brief Create a new ListIterator pointing to the head of the List.
    /// @return A new ListIterator pointing to the head of the List.
    ListIterator newIterator() const;
};

/// @brief A singleton class that represents an empty list.
///
/// @details EmptyList is an immutable singleton. There can only be one instance
/// of EmptyList in the entire program. All empty lists should reference this
/// single instance. The list cannot be modified after creation.
class EmptyList : public List
{
  private:
    /// @brief Private constructor to enforce singleton pattern.
    EmptyList();

    /// @brief Deleted copy constructor to prevent copying.
    EmptyList(const EmptyList &) = delete;

    /// @brief Deleted assignment operator to prevent assignment.
    EmptyList &operator=(const EmptyList &) = delete;

    /// @brief Deleted move constructor to prevent moving.
    EmptyList(EmptyList &&) = delete;

    /// @brief Deleted move assignment operator to prevent move assignment.
    EmptyList &operator=(EmptyList &&) = delete;

    /// @brief Static instance pointer.
    static EmptyList *instance_;

    
    void clear();
    void setButfirstItem(const DatumPtr &aValue);

  public:
    /// @brief Get the singleton instance of EmptyList.
    ///
    /// @return A pointer to the singleton EmptyList instance.
    static EmptyList *instance();

    virtual QString toString( ToStringFlags flags = ToStringFlags_None, int printDepthLimit = -1, int printWidthLimit = -1, VisitedSet *visited = nullptr) const override;
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
    const List *iterator;

  public:
    /// @brief Create an empty ListIterator.
    ListIterator();

    /// @brief Create a new ListIterator pointing to the head of the List.
    ListIterator(const DatumPtr &aList);

    /// @brief Return the element at the current location. Advance Iterator to the next location.
    ///
    /// @return The element at the current location.
    DatumPtr element();

    /// @brief Returns true if pointer references a valid element.
    bool elementExists() const;
};


/// @brief A class that allows quickly building a list.
///
/// @details This class is used to build a list quickly by appending elements to the end of the list.
/// @note This class should only be used internally within the qlogo binary. It should not be available to the user.
class ListBuilder
{
  private:
    DatumPtr finishedList_;

public:
    List *firstNode;
    List *lastNode;

    ListBuilder() : firstNode(EmptyList::instance()), lastNode(EmptyList::instance()), finishedList_(EmptyList::instance())
    {}

    /// @brief Append an element to the end of the list.
    /// @param element The element to append to the end of the list.
    void append(DatumPtr element)
    {
        List *newList = new List(element, EmptyList::instance());
        if (firstNode == EmptyList::instance())
        {
            firstNode = newList;
            lastNode = newList;
            finishedList_ = DatumPtr(firstNode);
        } else {
            lastNode->tail = DatumPtr(newList);
            lastNode = newList;
        }
    }

    /// @brief Return the finished list.
    /// @return The finished list.
    DatumPtr finishedList() const
    {
        return finishedList_;
    }
};

#endif // DATUM_TYPES_H