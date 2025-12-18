
//===-- qlogo/datum_word.cpp - Word class implementation --*- C++ -*-===//
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
/// This file contains the implementation of the Word class, which is the basic
/// unit of data in QLogo. A word may be a string or a number.
///
//===----------------------------------------------------------------------===//

#include "datum_types.h"
#include <QObject>
#include <qdebug.h>

/// @brief Convert a raw character to a printable character.
/// @param src The raw character to convert.
/// @return The printable character.
/// @details This function converts a raw character to a printable character.
/// If the raw character is printable (value is >= 32), it is returned unchanged.
/// Otherwise, a mapping is used to convert the raw character to a printable character.
QChar rawToChar(const QChar &src)
{
    const ushort rawToAsciiMap[] = {3,  32, // ' ' (space)
                                    4,  9,  // \t (tab)
                                    5,  10, // \n (newline)
                                    6,  40, // ( (left parenthesis)
                                    11, 63, // ? (question mark)
                                    14, 43, // + (plus)
                                    15, 126,// ~ (tilde)
                                    16, 41, // ) (right parenthesis)
                                    17, 91, // [ (left bracket)
                                    18, 93, // ] (right bracket)
                                    19, 45, // - (minus)
                                    20, 42, // * (asterisk)
                                    21, 47, // / (slash)
                                    22, 61, // = (equals)
                                    23, 60, // < (less than)
                                    24, 62, // > (greater than)
                                    25, 34, // " (quote)
                                    26, 92, // \ (backslash)
                                    27, 58, // : (colon)
                                    28, 59, // ; (semicolon)
                                    29, 124,// | (vertical bar)
                                    30, 123,// { (left brace)
                                    31, 125 // } (right brace)
                                };
    ushort v = src.unicode();
    if (v >= 32)
        return src;
    for (const ushort *i = rawToAsciiMap; *i <= v; i += 2)
    {
        if (*i == v)
        {
            return QChar(*(i + 1));
        }
    }
    return src;
}

/// @brief Convert a string of raw characters to a string of printable characters.
/// @param src The string of raw characters to convert, in place.
/// @details This function converts a string of raw characters to a string of printable characters.
/// It iterates through each character in the string and calls rawToChar() to convert each character.
/// If all the characters are already printable, the QString subscript operator is never used,
/// and thus QString's copy-on-write is not triggered.
void rawToChar(QString &src)
{
    for (int i = 0; i < src.size(); ++i)
    {
        QChar s = src[i];
        QChar d = rawToChar(s);
        if (s != d)
            src[i] = d;
    }
}

/// @brief Convert a printable character to a raw character.
/// @param src The printable character to convert.
/// @return The raw character, or the original character if it does not map to a raw character.
/// @details This function converts a printable character to a raw character.
/// If the printable character does not map to a raw character, it is returned unchanged.
/// Otherwise, a mapping is used to convert the printable character to a raw character.
QChar charToRaw(const QChar &src)
{
    const ushort asciiToRawMap[] = {126, 15, // ~ (tilde)
                                    125, 31, // } (right brace)
                                    124, 29, // | (vertical bar)
                                    123, 30, // { (left brace)
                                    93, 18,  // ] (right bracket)
                                    92, 26,  // \ (backslash)
                                    91, 17,  // [ (left bracket)
                                    63, 11,  // ? (question mark)
                                    62, 24,  // > (greater than)
                                    61, 22,  // = (equals)
                                    60, 23,  // < (less than)
                                    59, 28,  // ; (semicolon)
                                    58, 27,  // : (colon)
                                    47, 21,  // / (slash)
                                    45, 19,  // - (minus)
                                    43, 14,  // + (plus)
                                    42, 20,  // * (asterisk)
                                    41, 16,  // ) (right parenthesis)
                                    40,  6,  // ( (left parenthesis)
                                    34,  25, // " (quote)
                                    32,  3,  // ' ' (space)
                                    10,  5,  // \n (newline)
                                    9,  4,   // \t (tab)
                                    0,  0};
    ushort v = src.unicode();
    for (const ushort *i = asciiToRawMap; *i >= v; i += 2)
    {
        if (*i == v)
        {
            return QChar(*(i + 1));
        }
    }
    return src;
}

Word::Word()
{
    isa = Datum::typeWord;
    number = nan("");
    rawString = QString();
    printableString = QString();
    keyString = QString();
    sourceIsNumber = false;
    //qDebug() <<this << " new++ word";
}

Word::Word(const QString other, bool aIsForeverSpecial)
{
    isa = Datum::typeWord;
    number = nan("");
    isForeverSpecial = aIsForeverSpecial;
    rawString = other;
    printableString = QString();
    keyString = QString();
    sourceIsNumber = false;
    //qDebug() <<this << " new++ word: " <<other;
}

Word::Word(double other)
{
    isa = Datum::typeWord;
    numberIsValid = !std::isnan(other);
    number = other;
    rawString = QString();
    printableString = QString();
    keyString = QString();
    sourceIsNumber = true;
    //qDebug() <<this << " new++ word: " <<other;
}

Word::~Word()
{
    //qDebug() <<this << " --del word";
}

void Word::genRawString() const
{
    if (rawString.isNull())
    {
        Q_ASSERT(numberIsValid);
        rawString.setNum(number);
    }
}

void Word::genPrintString() const
{
    if (printableString.isNull())
    {
        genRawString();
        printableString = rawString;
        rawToChar(printableString);
    }
}

void Word::genKeyString() const
{
    if (keyString.isNull())
    {
        genPrintString();
        keyString = printableString;
        for (int i = 0; i < keyString.size(); ++i)
        {
            QChar s = printableString[i];
            QChar d = s.toUpper();
            if (s != d)
                keyString[i] = d;
        }
    }
}

double Word::numberValue() const
{
    if (!numberIsValid)
    {
        genPrintString();
        number = printableString.toDouble(&numberIsValid);
    }
    return number;
}

bool Word::boolValue() const
{
    if (!boolIsValid)
    {
        genKeyString();
        if ((keyString == "TRUE") || (keyString == "FALSE"))
        {
            boolIsValid = true;
            boolean = (keyString == "TRUE");
        }
    }
    return boolean;
}

QString Word::toString( ToStringFlags flags, int printDepthLimit, int printWidthLimit, VisitedSet *visited) const
{
    if (flags & Datum::ToStringFlags_Key)
    {
        genKeyString();
        return keyString;
    }
    if (flags & Datum::ToStringFlags_Raw)
    {
        genRawString();
        return rawString;
    }

    if (printDepthLimit == 0)
    {
        return "...";
    }

    genPrintString();
    bool fullPrintp = (flags & (Datum::ToStringFlags_FullPrint | Datum::ToStringFlags_Source)) != 0;
    QString srcString = (fullPrintp) ? rawString : printableString;

    if ((printWidthLimit >= 0) && (printWidthLimit <= 10))
    {
        printWidthLimit = 10;
    }
    if ((printWidthLimit > 0) && (srcString.size() > printWidthLimit))
    {
        srcString = srcString.left(printWidthLimit) + "...";
    }

    if ( ! fullPrintp)
    {
        return srcString;
    }

    QString retval = (flags & Datum::ToStringFlags_Source) != 0 ? "\"" : "";

    if (srcString.size() == 0)
    {
        return retval + "||";
    }

    bool shouldShowBars = false;
    for (auto c : srcString)
    {
        QChar t = rawToChar(c);
        if (t != c)
        {
            shouldShowBars = true;
            break;
        }
    }
    if (shouldShowBars)
    {
        retval.append('|');
        for (auto c : srcString)
        {
            QChar s = rawToChar(c);
            if ((s == '\\') || (s == '|'))
            {
                retval.append('\\');
            }
            retval.append(s);
        }
        retval.append('|');
        return retval;
    }
    for (auto c : srcString)
    {
        QChar r = charToRaw(c);
        QChar s = rawToChar(r);
        if (r != s)
        {
            retval.append('\\');
        }
        retval.append(s);
    }
    return retval;
}
