#ifndef DATUM_WORD_H
#define DATUM_WORD_H

//===-- qlogo/datum_word.h - Word class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Word class, which is a basic unit
/// of data in QLogo.
///
//===----------------------------------------------------------------------===//


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

    QString rawString;
    QString keyString;
    QString printableString;
    void genRawString();
    void genPrintString();
    void genKeyString();
    double number;
    bool numberIsValid;
    bool rawStringIsValid;
    bool keyStringIsValid;
    bool printableStringIsValid;
    bool sourceIsNumber;

    void addToPool();

public:

    /// Set to true if the word was created with vertical bars as delimiters.
    /// Words created this way will not be separated during parsing or runparsing.
    bool isForeverSpecial;

    /// Create a Word object that is invalid. Don't use this.
    /// To get a new word, use alloc().
    Word();

    /// Create a Word object with a string.
    ///
    /// \param other the string value of this word
    /// \param aIsForeverSpecial characters defined with vertical bars will retain their special use.
    static Word * alloc(const QString other, bool aIsForeverSpecial = false);

    /// Create a Word object with a number.
    static Word * alloc(double other);

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
    bool isEqual(DatumPtr other, bool ignoreCase);

    /// Returns the first character of the string value.
    DatumPtr first(void);

    /// Returns all but the first character of the string value
    DatumPtr butfirst(void);

    /// Returns the last character of the string value
    DatumPtr last(void);

    /// Returns all but the last character of the string value
    DatumPtr butlast(void);

    /// Returns true if the last call to numberValue() returned a valid number.
    bool didNumberConversionSucceed();

    /// returns the length of the string in characters.
    int size();

    /// Returns the character at position indicated by anIndex.
    DatumPtr datumAtIndex(int anIndex);

    /// Retruns true if anIndex is between 1 and the length of the string.
    bool isIndexInRange(int anIndex);

    /// Returns true if aDatum is a substring.
    bool containsDatum(DatumPtr aDatum, bool ignoreCase);

    /// Returns true if aDatum is a substring.
    bool isMember(DatumPtr aDatum, bool ignoreCase);

    /// Returns a substring starting at the first occurrence of aDatum to the end of the string.
    DatumPtr fromMember(DatumPtr aDatum, bool ignoreCase);
};


#endif // DATUM_WORD_H
