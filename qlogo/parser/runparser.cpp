
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

/// @brief Check if a minus sign at the start of a word should be treated as
///        a potential negative number or unary minus operator.
/// @param c The current character (should be '-').
/// @param iter The current iterator position.
/// @param wordBegin The beginning of the word being parsed.
/// @param word The full word being parsed.
/// @return True if we should attempt to parse a negative number or unary minus.
static bool shouldParseMinusAtWordStart(QChar c, QString::iterator iter, QString::iterator wordBegin, const QString &word)
{
    // Only process minus signs
    if (c != '-')
        return false;

    // Only process minus at the start of a word
    if (iter != wordBegin)
        return false;

    // Special case: if the word is exactly "-", treat it as a special character
    // (not as a unary minus operator). This prevents parsing a standalone "-"
    // as "0" followed by "--".
    if (word == opMinus())
        return false;

    return true;
}

/// @brief Attempt to parse a negative number starting from the current position.
/// @return The parsed number if successful, nothing() otherwise.
/// @note This function advances the iterator if a number is successfully parsed.
DatumPtr Runparser::tryParseNegativeNumber()
{
    // Check if there's at least one more character after the minus
    QString::iterator nextCharIter = runparseCIter;
    ++nextCharIter;
    if (nextCharIter == runparseCEnd)
    {
        // Just a minus sign with nothing after it - not a valid number
        return nothing();
    }

    // Try to parse a number (runparseNumber handles the minus sign)
    DatumPtr number = runparseNumber();
    return number;
}

/// @brief Parse a unary minus operator (MINUS function).
/// @details This outputs "0" followed by "--" to represent the binary minus
///          operation with highest precedence. The "--" operator will be handled by the treeifier as
///          subtraction, so "0 -- x" effectively becomes "MINUS x".
/// @note This function advances the iterator past the minus sign.
void Runparser::parseUnaryMinus()
{
    // Output "0" and "--" to represent unary minus
    // The treeifier will interpret this as: 0 - x, which is equivalent to MINUS x
    runparseBuilder->append(DatumPtr(opNumberZero()));
    runparseBuilder->append(DatumPtr(opDoubleMinus()));
    ++runparseCIter;
}

/// @brief Handle a minus sign that appears at the start of a word.
/// @details This function attempts to parse a negative number first. If that fails,
///          it treats the minus as a unary minus operator (MINUS function).
void Runparser::runparseMinus()
{
    // First, try to parse as a negative number
    DatumPtr number = tryParseNegativeNumber();
    if (!number.isNothing())
    {
        // Successfully parsed a negative number
        runparseBuilder->append(number);
        return;
    }

    // Not a negative number, so treat it as a unary minus operator
    parseUnaryMinus();
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
                    // Check if this minus sign at the start of a word should be
                    // treated as a potential negative number or unary minus operator
                    if (shouldParseMinusAtWordStart(c, runparseCIter, oldWord.begin(), oldWord))
                    {
                        runparseMinus();
                    }
                    else
                    {
                        // Treat as a regular special character (e.g., binary minus, standalone "-")
                        runparseSpecialchars();
                    }
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
