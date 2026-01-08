
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
#include <qdatetime.h>
#include <qdebug.h>

extern const QString &specialChars();

void Runparser::runparseSpecialchars()
{
    QString retval = *runparseCIter;
    ++runparseCIter;
    if (runparseCIter != runparseCEnd)
    {
        QChar c = *runparseCIter;
        // there are some cases where special chars are combined
        if (((retval == "<") && (c == '=')) || ((retval == "<") && (c == '>')) || ((retval == ">") && (c == '=')))
        {
            retval += c;
            ++runparseCIter;
        }
    }
    runparseBuilder->append(DatumPtr(retval));
}

void Runparser::runparseString()
{
    QString retval = "";

    if (*runparseCIter == '?')
    {
        retval = "?";
        ++runparseCIter;
        DatumPtr number = runparseNumber();
        if (!number.isNothing())
        {
            runparseBuilder->append(DatumPtr(QString("(")));
            runparseBuilder->append(DatumPtr(QString("?")));
            runparseBuilder->append(number);
            runparseBuilder->append(DatumPtr(QString(")")));
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
    runparseBuilder->append(DatumPtr(QString("0")));
    runparseBuilder->append(DatumPtr(QString("--")));
    // discard the minus
    ++runparseCIter;
}

DatumPtr Runparser::runparseNumber()
{
    if (runparseCIter == runparseCEnd)
        return nothing();
    QString::iterator iter = runparseCIter;
    QString result = "";
    bool hasDigit = false;
    QChar c = *iter;
    if (c == '-')
    {
        result = "-";
        ++iter;
    }

    if (iter == runparseCEnd)
        return nothing();
    c = *iter;
    while (c.isDigit())
    {
        result += c;
        ++iter;
        if (iter == runparseCEnd)
            goto numberSuccessful;
        c = *iter;
        hasDigit = true;
    }
    if (c == '.')
    {
        result += c;
        ++iter;
        if ((iter == runparseCEnd) && hasDigit)
            goto numberSuccessful;
        c = *iter;
    }
    while (c.isDigit())
    {
        result += c;
        ++iter;
        if (iter == runparseCEnd)
            goto numberSuccessful;
        c = *iter;
        hasDigit = true;
    }

    if (!hasDigit)
        return nothing();
    hasDigit = false;
    if ((c == 'e') || (c == 'E'))
    {
        result += c;
        ++iter;
        if (iter == runparseCEnd)
            return nothing();
        c = *iter;
    }
    else
    {
        goto numberSuccessful;
    }

    if ((c == '+') || (c == '-'))
    {
        result += c;
        ++iter;
        if (iter == runparseCEnd)
            return nothing();
        c = *iter;
    }
    while (c.isDigit())
    {
        result += c;
        ++iter;
        hasDigit = true;
        if (iter == runparseCEnd)
            goto numberSuccessful;
        c = *iter;
    }

    if (!hasDigit)
        return nothing();

    // at this point we have successfully parsed a complete number. However, if
    // there are more characters that aren't special characters, then we don't
    // have a complete number.
    if (!specialChars().contains(c))
        return nothing();

numberSuccessful:
    double value = result.toDouble();
    runparseCIter = iter;
    return DatumPtr(value);
}

void Runparser::runparseQuotedWord()
{
    QString retval = "";
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
                    if ((c == '-') && (runparseCIter == oldWord.begin()) && (oldWord != "-"))
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
