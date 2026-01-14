
//===-- qlogo/textstream.cpp - TextStream class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the TextStream class, which is responsible
/// for reading text from any kind of text stream.
///
//===----------------------------------------------------------------------===//

#include "controller/textstream.h"
#include "controller/logocontroller.h"
#include "datum_types.h"

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
    recentLineHistory = emptyList();
}

bool TextStream::initializeBaseLevelReading(const QString &prompt)
{
    DatumPtr lineP = readwordWithPrompt(prompt, true);
    if (lineP.isNothing())
        return false;
    listSourceWord = lineP.wordValue()->toString(Datum::ToStringFlags_Raw);
    listSourceWordIter = listSourceWord.begin();
    return true;
}

bool TextStream::processVbarredCharacter(ushort c, bool &isVbarred, bool &isCurrentWordVbarred, QString &currentWord)
{
    if (isVbarred)
    {
        if (c == '|')
        {
            isVbarred = false;
            return true; // Continue processing
        }
        currentWord.push_back(charToRaw(c));
        return true; // Continue processing
    }
    if (c == '|')
    {
        isVbarred = true;
        isCurrentWordVbarred = true;
        return true; // Continue processing
    }
    return false; // Not a vbarred character, continue normal processing
}

bool TextStream::processTildeContinuation()
{
    QString::iterator lookAhead = listSourceWordIter;
    while (lookAhead != listSourceWord.end() && *lookAhead == ' ')
        ++lookAhead;
    if (lookAhead != listSourceWord.end() && *lookAhead == '\n')
    {
        ++lookAhead;
        listSourceWordIter = lookAhead;
        return true; // Handled, continue
    }
    return false; // Not a continuation, continue normal processing
}

bool TextStream::processComments(ushort c, bool shouldRemoveComments)
{
    if (!shouldRemoveComments)
        return false;

    // Check for ; comment or #! comment
    if (c == ';' || (c == '#' && listSourceWordIter != listSourceWord.end() &&
                     listSourceWordIter->unicode() == '!'))
    {
        // Skip to end of line
        while ((listSourceWordIter != listSourceWord.end()) && (*listSourceWordIter != '\n'))
            ++listSourceWordIter;
        // Consume the eol
        if (listSourceWordIter != listSourceWord.end())
            ++listSourceWordIter;
        return true; // Handled, continue
    }
    return false; // Not a comment, continue normal processing
}

TextStream::DelimiterResult TextStream::processDelimiter(ushort c, ListBuilder &builder, QString &currentWord,
                                                          bool &isCurrentWordVbarred, bool isBaseLevel, bool makeArray,
                                                          bool shouldRemoveComments)
{
    // Add current word to builder if it exists
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
        return DelimiterResult::AppendSublist;
    case ']':
        if (isBaseLevel || makeArray)
        {
            throw FCError::unexpectedCloseSquare();
        }
        return DelimiterResult::ReturnList;
    case '}':
    {
        if (isBaseLevel || !makeArray)
        {
            throw FCError::unexpectedCloseBrace();
        }
        return DelimiterResult::ReturnArray;
    }
    case '{':
        builder.append(tokenizeListWithPrompt("", false, true, shouldRemoveComments));
        return DelimiterResult::AppendSubarray;
    default:
        // Space or tab - just continue
        return DelimiterResult::Continue;
    }
}

int TextStream::processArrayOrigin()
{
    int origin = 1;
    if (listSourceWordIter != listSourceWord.end() && *listSourceWordIter == '@')
    {
        QString originStr = "";
        ++listSourceWordIter;
        while (listSourceWordIter != listSourceWord.end() && (*listSourceWordIter >= '0') &&
               (*listSourceWordIter <= '9'))
        {
            originStr += *listSourceWordIter;
            ++listSourceWordIter;
        }
        origin = originStr.toInt();
    }
    return origin;
}

bool TextStream::finalizeResult(ListBuilder &builder, bool isBaseLevel, bool makeArray, DatumPtr &result)
{
    if (isBaseLevel)
    {
        result = builder.finishedList();
        return false; // Don't continue, we have a result
    }

    // Get some more source material if we can
    DatumPtr lineP;
    if (makeArray)
        lineP = readwordWithPrompt("{ ", true);
    else
        lineP = readwordWithPrompt("[ ", true);

    if (!lineP.isNothing())
    {
        listSourceWord = lineP.wordValue()->toString(Datum::ToStringFlags_Raw);
        listSourceWordIter = listSourceWord.begin();
        return true; // Continue processing
    }

    // We have exhausted our source. Return what we have.
    if (makeArray)
    {
        auto *ary = new Array(1, builder.finishedList().listValue());
        result = {ary};
    }
    else
    {
        result = builder.finishedList();
    }
    return false; // Don't continue, we have a result
}

DatumPtr TextStream::tokenizeListWithPrompt(const QString &prompt,
                                            bool isBaseLevel,
                                            bool makeArray,
                                            bool shouldRemoveComments)
{
    if (isBaseLevel)
    {
        if (!initializeBaseLevelReading(prompt))
            return nothing();
    }

    ListBuilder builder;
    QString currentWord = "";

    forever
    {
        bool isVbarred = false;
        bool isCurrentWordVbarred = false;

        while (listSourceWordIter != listSourceWord.end())
        {
            ushort c = listSourceWordIter->unicode();
            ++listSourceWordIter;

            // Process vbarred characters
            if (processVbarredCharacter(c, isVbarred, isCurrentWordVbarred, currentWord))
                continue;

            // Process tilde continuation
            if (c == '~' && processTildeContinuation())
                continue;

            // Process comments
            if (processComments(c, shouldRemoveComments))
                continue;

            // Process delimiters
            if ((c == ' ') || (c == '\t') || (c == '[') || (c == ']') || (c == '{') || (c == '}'))
            {
                DelimiterResult result = processDelimiter(c, builder, currentWord, isCurrentWordVbarred,
                                                          isBaseLevel, makeArray, shouldRemoveComments);
                switch (result)
                {
                case DelimiterResult::ReturnList:
                    return builder.finishedList();
                case DelimiterResult::ReturnArray:
                {
                    int origin = processArrayOrigin();
                    auto *ary = new Array(origin, builder.finishedList().listValue());
                    return {ary};
                }
                case DelimiterResult::AppendSublist:
                case DelimiterResult::AppendSubarray:
                case DelimiterResult::Continue:
                    break;
                }
            }
            else
            {
                currentWord.push_back(c);
            }
        }

        // End of current source word. Add the last word to the list.
        if (currentWord.size() > 0)
        {
            builder.append(DatumPtr(currentWord, isCurrentWordVbarred));
            currentWord = "";
        }

        // Try to finalize or get more input
        DatumPtr result;
        if (!finalizeResult(builder, isBaseLevel, makeArray, result))
            return result;
        // If finalizeResult returns true, it means more input was read and we should continue
    } // /forever
}

DatumPtr TextStream::readrawlineWithPrompt(const QString &prompt, bool shouldSavePreviousLines)
{
    QString retval;
    if (stream == nullptr)
    {
        retval = Config::get().mainController()->inputRawlineWithPrompt(prompt);
        if (retval.isNull())
            return nothing();
    }
    else
    {
        if (stream->atEnd())
        {
            return nothing();
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
    // recentLineHistory.listValue()->append(retvalPtr);

    return retvalPtr;
}

DatumPtr TextStream::readwordWithPrompt(const QString &prompt, bool shouldSavePreviousLines)
{
    QString retval = "";
    bool isVbarred = false;
    bool isEscaped = false;

    DatumPtr line = readrawlineWithPrompt(prompt, shouldSavePreviousLines);
    if (line.isNothing())
        return nothing();

    forever
    {
        if (line.isNothing())
            return DatumPtr(retval);

        const QString &t = line.wordValue()->toString(Datum::ToStringFlags_Raw);
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
        if (line.wordValue()->toString(Datum::ToStringFlags_Raw) == retval)
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
        return emptyList();
    QString line = stream->read(1);
    if (stream->status() != QTextStream::Ok)
        throw FCError::fileSystem();
    return DatumPtr(line);
}

DatumPtr TextStream::recentHistory() const
{
    return recentLineHistory;
}

bool TextStream::seek(qint64 loc)
{
    return stream->seek(loc);
}

qint64 TextStream::pos() const
{
    return stream->pos();
}

bool TextStream::atEnd() const
{
    return stream->atEnd();
}

void TextStream::flush()
{
    stream->flush();
}

void TextStream::lprint(const QString &text)
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

QIODevice *TextStream::device() const
{
    return stream->device();
}

QString *TextStream::string() const
{
    return stream->string();
}
