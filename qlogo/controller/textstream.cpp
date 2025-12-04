
//===-- qlogo/parser.cpp - Parser class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Reader class, which is responsible
/// for reading text from any kind of text stream.
///
//===----------------------------------------------------------------------===//

#include "controller/textstream.h"
#include "controller/logocontroller.h"
#include "datum.h"

/// @brief Find the last non-space character in a string
/// @param line The string to search
/// @return The last non-space character
char lastNonSpaceChar(const QString &line)
{
    char retval = ' ';
    for (int i = line.length() - 1; i >= 0; --i)
    {
        retval = line[i].toLatin1();
        if (retval != ' ')
            break;
    }
    return retval;
}

TextStream::TextStream(QTextStream *aStream)
{
    stream = aStream;
    clearLineHistory();
}

void TextStream::clearLineHistory()
{
    recentLineHistory = DatumPtr(new List());
}

// TODO: This is huge. Break it up.
DatumPtr TextStream::tokenizeListWithPrompt(const QString &prompt,
                                            bool isBaseLevel,
                                            bool makeArray,
                                            bool shouldRemoveComments)
{
    DatumPtr lineP;

    if (isBaseLevel)
    {
        lineP = readwordWithPrompt(prompt, true);

        if (lineP.isNothing())
            return nothing;

        listSourceWord = lineP.wordValue()->rawValue();
        listSourceWordIter = listSourceWord.begin();
    }
    List *retval = new List();
    ListBuilder builder(retval);
    DatumPtr retvalP(retval);
    QString currentWord = "";

    forever
    {
        bool isVbarred = false;
        bool isCurrentWordVbarred = false;

        while (listSourceWordIter != listSourceWord.end())
        {
            ushort c = listSourceWordIter->unicode();
            ++listSourceWordIter;

            if (isVbarred)
            {
                if (c == '|')
                {
                    isVbarred = false;
                    continue;
                }
                currentWord.push_back(charToRaw(c));
                continue;
            }
            if (c == '|')
            {
                isVbarred = true;
                isCurrentWordVbarred = true;
                continue;
            }

            if (c == '~')
            {
                // If this is the last character of the line then jump to the beginning
                // of the next line
                QString::iterator lookAhead = listSourceWordIter;
                while (*lookAhead == ' ')
                    ++lookAhead;
                if (*lookAhead == '\n')
                {
                    ++lookAhead;
                    listSourceWordIter = lookAhead;
                    continue;
                }
            }
            if (((c == ';') || ((c == '#') && (listSourceWordIter != listSourceWord.end()) &&
                                (listSourceWordIter->unicode() == '!'))) &&
                shouldRemoveComments)
            {
                // We are parsing a comment
                while ((listSourceWordIter != listSourceWord.end()) && (*listSourceWordIter != '\n'))
                    ++listSourceWordIter;
                // Consume the eol
                if (listSourceWordIter != listSourceWord.end())
                    ++listSourceWordIter;
                continue;
            }
            if ((c == '#') && (listSourceWordIter != listSourceWord.end()) && (listSourceWordIter->unicode() == '!') &&
                shouldRemoveComments)
            {
                // This is a comment
                while ((listSourceWordIter != listSourceWord.end()) && (*listSourceWordIter != '\n'))
                    ++listSourceWordIter;
                // Consume the eol
                if (listSourceWordIter != listSourceWord.end())
                    ++listSourceWordIter;
                continue;
            }
            if ((c == ' ') || (c == '\t') || (c == '[') || (c == ']') || (c == '{') || (c == '}'))
            {
                // This is a delimiter
                if (currentWord.size() > 0)
                {
                    builder.append(DatumPtr(currentWord, isCurrentWordVbarred));
                    currentWord = "";
                    isCurrentWordVbarred = false;
                }
                switch (c)
                {
                case '[':
                    builder.append(tokenizeListWithPrompt("", false, false, shouldRemoveComments));
                    break;
                case ']':
                    if ((isBaseLevel) || makeArray)
                    {
                        throw FCError::unexpectedCloseSquare();
                    }
                    return retvalP;
                case '}':
                {
                    if ((isBaseLevel) || !makeArray)
                    {
                        throw FCError::unexpectedCloseBrace();
                    }
                    int origin = 1;
                    // See if array has a custom origin
                    if (*listSourceWordIter == '@')
                    {
                        QString originStr = "";
                        ++listSourceWordIter;
                        while ((*listSourceWordIter >= '0') && (*listSourceWordIter <= '9'))
                        {
                            originStr += *listSourceWordIter;
                            ++listSourceWordIter;
                        }
                        origin = originStr.toInt();
                    }
                    Array *ary = new Array(origin, retval);
                    return DatumPtr(ary);
                }
                case '{':
                    builder.append(tokenizeListWithPrompt("", false, true, shouldRemoveComments));
                    break;
                default:
                    break;
                }
            }
            else
            {
                currentWord.push_back(c);
            }
        }
        // This is the end of the read. Add the last word to the list.
        if (currentWord.size() > 0)
        {
            builder.append(DatumPtr(currentWord, isCurrentWordVbarred));
            currentWord = "";
        }

        // If this is the base-level list then we can just return
        if (isBaseLevel)
            return retvalP;

        // Get some more source material if we can
        if (makeArray)
            lineP = readwordWithPrompt("{ ", true);
        else
            lineP = readwordWithPrompt("[ ", true);
        if ( ! lineP.isNothing())
        {
            listSourceWord = lineP.wordValue()->rawValue();
            listSourceWordIter = listSourceWord.begin();
            continue;
        }
        // We have exhausted our source. Return what we have.
        if (makeArray)
        {
            Array *ary = new Array(1, retval);
            return DatumPtr(ary);
        }
        return retvalP;
    } // /forever
}

DatumPtr TextStream::readrawlineWithPrompt(const QString &prompt, bool shouldSavePreviousLines)
{
    QString retval;
    if (stream == nullptr)
    {
        retval = Config::get().mainController()->inputRawlineWithPrompt(prompt);
        if (retval.isNull())
            return nothing;
    }
    else
    {
        if (stream->atEnd())
        {
            return nothing;
        }
        retval = stream->readLine();
        if (stream->status() != QTextStream::Ok)
            throw FCError::fileSystem();
    }
    DatumPtr retvalPtr(retval);

    if (!shouldSavePreviousLines)
    {
        clearLineHistory();
    }
    // TODO: How to save input when running `TO`?
    //recentLineHistory.listValue()->append(retvalPtr);

    return retvalPtr;
}

DatumPtr TextStream::readwordWithPrompt(const QString &prompt, bool shouldSavePreviousLines)
{
    QString retval = "";
    bool isVbarred = false;
    bool isEscaped = false;

    DatumPtr line = readrawlineWithPrompt(prompt, shouldSavePreviousLines);
    if (line.isNothing())
        return nothing;

    forever
    {
        if (line.isNothing())
            return DatumPtr(retval);

        const QString &t = line.wordValue()->rawValue();
        for (auto c : t)
        {
            if (isEscaped)
            {
                isEscaped = false;
                retval.push_back(charToRaw(c));
                continue;
            }
            if (c == '|')
            {
                isVbarred = !isVbarred;
            }
            if (c == '\\')
            {
                isEscaped = true;
                continue;
            }

            retval.push_back(c);
        } // for (auto c : t)
        // The end of the line
        if (isEscaped)
        {
            isEscaped = false;
            retval.push_back('\n');
            line = readrawlineWithPrompt("\\ ", true);
            continue;
        }
        if (isVbarred)
        {
            retval.push_back(charToRaw('\n'));
            line = readrawlineWithPrompt("| ", true);
            continue;
        }
        if (lastNonSpaceChar(t) == '~')
        {
            retval.push_back('\n');
            line = readrawlineWithPrompt("~ ", true);
            continue;
        }

        // If (after all the work) the string we generated is the same as the rawline
        // we started with, return the original rawline.
        if (line.wordValue()->rawValue() == retval)
            return line;
        return DatumPtr(retval);

    }; // forever
}

DatumPtr TextStream::readlistWithPrompt(const QString &prompt, bool shouldRemoveComments, bool shouldSavePreviousLines)
{

    if (!shouldSavePreviousLines)
        clearLineHistory();
    return tokenizeListWithPrompt(prompt, true, false, shouldRemoveComments);
}

DatumPtr TextStream::readChar()
{
    if (stream == nullptr)
    {
        return Config::get().mainController()->readchar();
    }

    if (stream->atEnd())
        return DatumPtr(new List());
    QString line = stream->read(1);
    if (stream->status() != QTextStream::Ok)
        throw FCError::fileSystem();
    return DatumPtr(line);
}

DatumPtr TextStream::recentHistory()
{
    return recentLineHistory;
}

bool TextStream::seek(qint64 loc)
{
    return stream->seek(loc);
}

qint64 TextStream::pos()
{
    return stream->pos();
}

bool TextStream::atEnd()
{
    return stream->atEnd();
}

void TextStream::flush()
{
    stream->flush();
}

void TextStream::lprint(QString text)
{
    if (stream == nullptr)
    {
        Config::get().mainController()->printToConsole(text);
    }
    else
    {
        *stream << text;
        if (stream->status() != QTextStream::Ok)
            throw FCError::fileSystem();
    }
}

QIODevice *TextStream::device()
{
    return stream->device();
}

QString *TextStream::string()
{
    return stream->string();
}
