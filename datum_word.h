#ifndef DATUM_WORD_H
#define DATUM_WORD_H

#include "datum.h"
#include <QString>

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

    friend class WordIterator;

    // TODO: the nomenclature assumes words are mutable. They are not.
    enum DatumWordDirtyFlag { noValue, stringIsDirty, numberIsDirty, allClean };
    DatumWordDirtyFlag dirtyFlag;

    QString rawString;
    QString keyString;
    QString printableString;
    double number;
    bool numberConversionSucceeded;

public:

    /// Set to true if the word was created with vertical bars as delimiters.
    /// Words created this way will not be separated during parsing or runparsing.
    bool isForeverSpecial = false;

    /// Create a Word object with a string.
    ///
    /// \param other the string value of this word
    /// \param aIsForeverSpecial characters defined with vertical bars will retain their special use.
    /// \param canBeDestroyed Allow this object to be destroyed if its retain count falls below 1.
    Word(const QString other, bool aIsForeverSpecial = false,
         bool canBeDestroyed = true);

    /// Create a Word object with a number.
    Word(double other);

    /// Create a Word object with an empty string.
    Word();
    ~Word();

    /// returns the number representation of the Word. Use didNumberConversionSucceed() to check.
    double numberValue(void);

    DatumType isa();
    QString name();

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

    /// Returns true if the value pointed to by other is equal to this Word's value.
    /// \param other the value to be tested against.
    /// \param ignoreCase if true use case-insensitive compare.
    bool isEqual(DatumP other, bool ignoreCase);

    /// Returns the first character of the string value.
    DatumP first(void);

    /// Returns all but the first character of the string value
    DatumP butfirst(void);

    /// Returns the last character of the string value
    DatumP last(void);

    /// Returns all but the last character of the string value
    DatumP butlast(void);

    /// Returns true if the last call to numberValue() returned a valid number.
    bool didNumberConversionSucceed();

    /// returns the length of the string in characters.
    int size();

    /// Returns the character at position indicated by anIndex.
    DatumP datumAtIndex(int anIndex);

    /// Retruns true if anIndex is between 1 and the length of the string.
    bool isIndexInRange(int anIndex);

    /// Returns true if aDatum is a substring.
    bool containsDatum(DatumP aDatum, bool ignoreCase);

    /// Returns true if aDatum is a substring.
    bool isMember(DatumP aDatum, bool ignoreCase);

    /// Returns a substring starting at the first occurrence of aDatum to the end of the string.
    DatumP fromMember(DatumP aDatum, bool ignoreCase);

    Iterator newIterator(void);
};


extern Word trueWord;
extern Word falseWord;


#endif // DATUM_WORD_H
