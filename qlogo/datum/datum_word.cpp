
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

#include "datum.h"
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
    const ushort rawToAsciiMap[] = {2,  58, 3,  32, 4,  9,  5,  10, 6,  40,  11, 63,  14, 43, 15, 126,
                                    16, 41, 17, 91, 18, 93, 19, 45, 20, 42,  21, 47,  22, 61, 23, 60,
                                    24, 62, 25, 34, 26, 92, 28, 59, 29, 124, 30, 123, 31, 125};
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
    const ushort asciiToRawMap[] = {126, 15, 125, 31, 124, 29, 123, 30, 93, 18, 92, 26, 91, 17, 63, 11,
                                    62,  24, 61,  22, 60,  23, 59,  28, 58, 2,  47, 21, 45, 19, 43, 14,
                                    42,  20, 41,  16, 40,  6,  34,  25, 32, 3,  10, 5,  9,  4,  0,  0};
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

void Word::genRawString()
{
    if (rawString.isNull())
    {
        Q_ASSERT(numberIsValid);
        rawString.setNum(number);
    }
}

void Word::genPrintString()
{
    if (printableString.isNull())
    {
        genRawString();
        printableString = rawString;
        rawToChar(printableString);
    }
}

void Word::genKeyString()
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

double Word::numberValue()
{
    if (!numberIsValid)
    {
        genPrintString();
        number = printableString.toDouble(&numberIsValid);
    }
    return number;
}

bool Word::boolValue()
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

QString Word::toString( ToStringFlags flags, int printDepthLimit, int printWidthLimit, VisitedSet *visited)
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
            if ((s == '\\') || (s == '|') || (s == '\n'))
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
