
//===-- runparser.cpp - Parser class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Runparser class, which is
/// responsible for parsing a List into a "runparsed" list.
///
//===----------------------------------------------------------------------===//

#include "runparser.h"
#include "QtCore/qiodevice.h"
#include "controller/logocontroller.h"
#include "controller/textstream.h"
#include "datum_types.h"
#include "op_strings.h"
#include <qdatetime.h>
#include <qdebug.h>

extern const QString &specialChars();

using namespace StringConstants;

void Runparser::runparseSpecialchars()
{
    QString retval = *runparseCIter;
    ++runparseCIter;
    if (runparseCIter != runparseCEnd)
    {
        QChar c = *runparseCIter;
        // there are some cases where special chars are combined
        if (((retval == opLessThan()) && (c == '=')) || ((retval == opLessThan()) && (c == '>')) || ((retval == opGreaterThan()) && (c == '=')))
        {
            retval += c;
            ++runparseCIter;
        }
    }
    runparseBuilder->append(DatumPtr(retval));
}

void Runparser::runparseString()
{
    QString retval;

    if (*runparseCIter == '?')
    {
        retval = opQuestion();
        ++runparseCIter;
        DatumPtr number = runparseNumber();
        if (!number.isNothing())
        {
            runparseBuilder->append(DatumPtr(opOpenParen()));
            runparseBuilder->append(DatumPtr(opQuestion()));
            runparseBuilder->append(number);
            runparseBuilder->append(DatumPtr(opCloseParen()));
            return;
        }
    }

    while ((runparseCIter != runparseCEnd) && (!specialChars().contains(*runparseCIter)))
    {
        retval += *runparseCIter;
        ++runparseCIter;
    }
    runparseBuilder->append(DatumPtr(retval, isRunparseSourceSpecial));
}

void Runparser::runparseMinus()
{
    QString::iterator nextCharIter = runparseCIter;
    ++nextCharIter;
    if (nextCharIter == runparseCEnd)
    {
        runparseSpecialchars();
        return;
    }

    DatumPtr number = runparseNumber();
    if (!number.isNothing())
    {
        runparseBuilder->append(number);
        return;
    }

    // This is a minus function
    runparseBuilder->append(DatumPtr(opNumberZero()));
    runparseBuilder->append(DatumPtr(opDoubleMinus()));
    // discard the minus
    ++runparseCIter;
}

/// @brief Parse digits from the current iterator position.
/// @param iter Reference to the iterator (will be advanced).
/// @param end End iterator.
/// @param result String to append digits to.
/// @return True if at least one digit was parsed.
static bool parseDigits(QString::iterator &iter, const QString::iterator &end, QString &result)
{
    bool hasDigit = false;
    while (iter != end && (*iter).isDigit())
    {
        result += *iter;
        ++iter;
        hasDigit = true;
    }
    return hasDigit;
}

/// @brief Parse the integer part of a number (digits before decimal point).
/// @param iter Reference to the iterator (will be advanced).
/// @param end End iterator.
/// @param result String to append digits to.
/// @return True if at least one digit was parsed.
static bool parseIntegerPart(QString::iterator &iter, const QString::iterator &end, QString &result)
{
    return parseDigits(iter, end, result);
}

/// @brief Parse the decimal part of a number (digits after decimal point).
/// @param iter Reference to the iterator (will be advanced).
/// @param end End iterator.
/// @param result String to append digits to.
/// @return True if at least one digit was parsed.
static bool parseDecimalPart(QString::iterator &iter, const QString::iterator &end, QString &result)
{
    if (iter == end || *iter != '.')
        return false;

    result += '.';
    ++iter;
    return parseDigits(iter, end, result);
}

/// @brief Parse the exponent part of a number (e/E followed by optional sign and digits).
/// @param iter Reference to the iterator (will be advanced).
/// @param end End iterator.
/// @param result String to append exponent to.
/// @return True if a valid exponent was parsed.
static bool parseExponent(QString::iterator &iter, const QString::iterator &end, QString &result)
{
    if (iter == end)
        return false;

    QChar c = *iter;
    if (c != 'e' && c != 'E')
        return false;

    result += c;
    ++iter;

    if (iter == end)
        return false;

    // Optional sign
    c = *iter;
    if (c == '+' || c == '-')
    {
        result += c;
        ++iter;
        if (iter == end)
            return false;
    }

    // Must have at least one digit
    return parseDigits(iter, end, result);
}

DatumPtr Runparser::runparseNumber()
{
    if (runparseCIter == runparseCEnd)
        return nothing();

    QString::iterator iter = runparseCIter;
    QString result;
    bool hasIntegerPart = false;
    bool hasDecimalPart = false;

    // Parse optional minus sign
    QChar c = *iter;
    if (c == '-')
    {
        result = opMinus();
        ++iter;
        if (iter == runparseCEnd)
            return nothing();
    }

    // Parse integer part (digits before decimal point)
    hasIntegerPart = parseIntegerPart(iter, runparseCEnd, result);

    // Parse decimal part (optional)
    if (iter != runparseCEnd && *iter == '.')
    {
        bool hadDigitsBeforeDecimal = hasIntegerPart;
        hasDecimalPart = parseDecimalPart(iter, runparseCEnd, result);

        // Edge case: ".5" is valid (0.5), but "." alone is not
        // Edge case: "1.2.3" should fail - if we see another '.' after parsing decimal, fail
        if (!hadDigitsBeforeDecimal && !hasDecimalPart)
        {
            // We saw "." but no digits after it, and no digits before it
            return nothing();
        }

        // Check for multiple decimal points
        if (iter != runparseCEnd && *iter == '.')
        {
            return nothing();
        }
    }

    // Must have at least integer part or decimal part
    if (!hasIntegerPart && !hasDecimalPart)
        return nothing();

    // Parse exponent (optional)
    if (iter != runparseCEnd && (*iter == 'e' || *iter == 'E'))
    {
        if (!parseExponent(iter, runparseCEnd, result))
        {
            // Invalid exponent (e.g., "1e" or "1e+")
            return nothing();
        }
    }

    // Check that the next character (if any) is a special character
    // This ensures we've parsed a complete number token
    if (iter != runparseCEnd && !specialChars().contains(*iter))
    {
        return nothing();
    }

    // Successfully parsed a number
    double value = result.toDouble();
    runparseCIter = iter;
    return DatumPtr(value);
}

void Runparser::runparseQuotedWord()
{
    QString retval;
    while ((runparseCIter != runparseCEnd) && (*runparseCIter != '(') && (*runparseCIter != ')'))
    {
        retval += *runparseCIter;
        ++runparseCIter;
    }
    runparseBuilder->append(DatumPtr(retval, isRunparseSourceSpecial));
}

DatumPtr Runparser::doRunparse(DatumPtr src)
{
    // Runparse operates on a list. If the source is a word, then parse it into a list
    // first.
    if (src.isWord())
    {
        QString text = src.wordValue()->toString(Datum::ToStringFlags_Raw);
        QTextStream srcStream(&text, QIODevice::ReadOnly);
        TextStream stream(&srcStream);
        src = stream.readlistWithPrompt("", false);
    }

    if (src.isNothing())
    {
        return emptyList();
    }

    ListBuilder builder;
    runparseBuilder = &builder;

    ListIterator iter = src.listValue()->newIterator();

    while (iter.elementExists())
    {
        DatumPtr element = iter.element();
        if (element.isWord())
        {
            QString oldWord = element.wordValue()->toString(Datum::ToStringFlags_Raw);
            isRunparseSourceSpecial = element.wordValue()->isForeverSpecial;

            runparseCIter = oldWord.begin();
            runparseCEnd = oldWord.end();
            while (runparseCIter != runparseCEnd)
            {
                QChar c = *runparseCIter;
                if (specialChars().contains(c))
                {
                    if ((c == '-') && (runparseCIter == oldWord.begin()) && (oldWord != opMinus()))
                        runparseMinus();
                    else
                        runparseSpecialchars();
                    continue;
                }
                if (c == '"')
                {
                    runparseQuotedWord();
                    continue;
                }

                DatumPtr number = runparseNumber();
                if (number.isNothing())
                {
                    runparseString();
                }
                else
                {
                    runparseBuilder->append(number);
                }
            } // while (cIter != oldWord.end())
        }
        else
        {
            // Do not parse arrays or sublists. Just append them as is.
            runparseBuilder->append(element);
        }
    }
    return runparseBuilder->finishedList();
}

/// @brief Parse a QLogo word or list into a list of tokens.
/// @param src A QLogo word or list to parse.
/// @returns A list of tokens.
///
/// @note This function is a wrapper that temporarily creates a `Runparser`,
/// calls its `doRunparse()` method, and returns the result.
DatumPtr runparse(const DatumPtr &src)
{
    Runparser rp;
    return rp.doRunparse(src);
}
