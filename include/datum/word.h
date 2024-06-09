#ifndef DATUM_WORD_H
#define DATUM_WORD_H

//===-- qlogo/datum/word.h - Word class definition -------*- C++ -*-===//
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


#include "datum/datum.h"
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

    /// returns the length of the string in characters.
    int size();

    /// Return true iff this word was created with a number.
    bool isSourceNumber() { return sourceIsNumber; }

};


#endif // DATUM_WORD_H
