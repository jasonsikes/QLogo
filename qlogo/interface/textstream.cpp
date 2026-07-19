
//===-- qlogo/RawStream.cpp - RawStream class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the RawStream class, which is responsible
/// for reading text from any kind of text stream.
///
//===----------------------------------------------------------------------===//

#include "interface/textstream.h"
#include "interface/logointerface.h"
#include "interface/streammanager.h"
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

/*************************************************************/
/************************* RawStream *************************/
/*************************************************************/

RawStream::RawStream(QTextStream *aStream) : stream_(aStream)
{
}

QIODevice *RawStream::device() const
{
    return stream_->device();
}

QString *RawStream::string() const
{
    return stream_->string();
}

bool RawStream::seek(qint64 loc)
{
    return stream_->seek(loc);
}

qint64 RawStream::pos() const
{
    return stream_->pos();
}

bool RawStream::atEnd() const
{
    return stream_->atEnd();
}

void RawStream::flush()
{
    stream_->flush();
}

void RawStream::lprint(const QString &text)
{
    if (stream_ == nullptr)
    {
        StreamManager::get().writeTerminal(text);
    }
    else
    {
        *stream_ << text;
        if (stream_->status() != QTextStream::Ok)
            throw FCError::fileSystem();
    }
}

DatumPtr RawStream::readChar()
{
    if (stream_ == nullptr)
    {
        return StreamManager::get().readTerminalChar();
    }

    if (stream_->atEnd())
        return emptyList();
    QString line = stream_->read(1);
    if (stream_->status() != QTextStream::Ok)
        throw FCError::fileSystem();
    return DatumPtr(line);
}




/*************************************************************/
/************************ WordReader *************************/
/*************************************************************/

WordReader::WordReader(RawStream *rawStream) : rawStream_(rawStream)
{
}

DatumPtr WordReader::readWordWithPrompt(const QString &prompt)
{
    lineHistory_.clear();
    QString retval = "";
    bool isVbarred = false;
    bool isEscaped = false;

    DatumPtr line = readRawlineWithPrompt(prompt);
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
            line = readRawlineWithPrompt("\\ ");
            continue;
        }
        if (isVbarred)
        {
            retval.push_back(charToRaw('\n'));
            line = readRawlineWithPrompt("| ");
            continue;
        }
        if (lastNonSpaceChar(t) == '~')
        {
            retval.push_back('\n');
            line = readRawlineWithPrompt("~ ");
            continue;
        }

        // If (after all the work) the string we generated is the same as the rawline
        // we started with, return the original rawline.
        if (line.wordValue()->toString(Datum::ToStringFlags_Raw) == retval)
            return line;
        return DatumPtr(retval);
    }; // forever
}


QList<DatumPtr> WordReader::lineHistory() const
{
    return lineHistory_;
}

DatumPtr WordReader::readRawlineWithPrompt(const QString &prompt)
{
    QString retval;
    if (rawStream_->stream_ == nullptr)
    {
        retval = StreamManager::get().readTerminalRawline(prompt);
        if (retval.isNull())
            return nothing();
    }
    else
    {
        if (rawStream_->stream_->atEnd())
        {
            return nothing();
        }
        retval = rawStream_->stream_->readLine();
        if (rawStream_->stream_->status() != QTextStream::Ok)
            throw FCError::fileSystem();
    }
    DatumPtr retvalPtr(retval);
    lineHistory_.append(retvalPtr);
    return retvalPtr;
}


/*************************************************************/
/************************ ListReader *************************/
/*************************************************************/

ListReader::ListReader(WordReader *wordReader) : wordReader_(wordReader)
{
}

DatumPtr ListReader::readListWithPrompt(const QString &prompt, bool shouldRemoveComments)
{

    lineHistory_.clear();
    return parseListWithPrompt(prompt, true, false, shouldRemoveComments);
}

QList<DatumPtr> ListReader::lineHistory() const
{
    return lineHistory_;
}

DatumPtr ListReader::parseListWithPrompt(const QString &prompt,
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
        bool isCurrentWordVbarred = false;

        // Process all characters in the current source word
        DatumPtr earlyReturn = processCharacterLoop(builder, currentWord, isCurrentWordVbarred,
                    isBaseLevel, makeArray, shouldRemoveComments);
        if (!earlyReturn.isNothing())
            return earlyReturn;

        // End of current source word. Add the last word to the list.
        addCurrentWordToBuilder(builder, currentWord, isCurrentWordVbarred);

        // Try to finalize or get more input
        DatumPtr result;
        if (!finalizeResult(builder, isBaseLevel, makeArray, result))
            return result;
        // If finalizeResult returns true, it means more input was read and we should continue
    } // /forever
}

bool ListReader::initializeBaseLevelReading(const QString &prompt)
{
    DatumPtr lineP = wordReader_->readWordWithPrompt(prompt);
    lineHistory_.append(wordReader_->lineHistory());
    if (lineP.isNothing())
        return false;
    listSourceWord_ = lineP.wordValue()->toString(Datum::ToStringFlags_Raw);
    listSourceWordIter_ = listSourceWord_.begin();
    return true;
}

DatumPtr ListReader::processCharacterLoop(ListBuilder &builder, QString &currentWord, bool &isCurrentWordVbarred,
                                          bool isBaseLevel, bool makeArray, bool shouldRemoveComments)
{
    bool isVbarred = false;

    while (listSourceWordIter_ != listSourceWord_.end())
    {
        ushort c = listSourceWordIter_->unicode();
        ++listSourceWordIter_;

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
            DatumPtr returnValue = handleDelimiterResult(result, builder);
            if (!returnValue.isNothing())
                return returnValue;
        }
        else
        {
            currentWord.push_back(c);
        }
    }

    return nothing(); // Continue processing
}

void ListReader::addCurrentWordToBuilder(ListBuilder &builder, QString &currentWord, bool isCurrentWordVbarred)
{
    if (currentWord.size() > 0)
    {
        builder.append(DatumPtr(currentWord, isCurrentWordVbarred));
        currentWord = "";
    }
}

bool ListReader::finalizeResult(ListBuilder &builder, bool isBaseLevel, bool makeArray, DatumPtr &result)
{
    if (isBaseLevel)
    {
        result = builder.finishedList();
        return false; // Don't continue, we have a result
    }

    // Get some more source material if we can
    DatumPtr lineP;
    if (makeArray)
        lineP = wordReader_->readWordWithPrompt("{ ");
    else
        lineP = wordReader_->readWordWithPrompt("[ ");
    lineHistory_.append(wordReader_->lineHistory());

    if (!lineP.isNothing())
    {
        listSourceWord_ = lineP.wordValue()->toString(Datum::ToStringFlags_Raw);
        listSourceWordIter_ = listSourceWord_.begin();
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

bool ListReader::processVbarredCharacter(ushort c, bool &isVbarred, bool &isCurrentWordVbarred, QString &currentWord)
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

bool ListReader::processTildeContinuation()
{
    QString::iterator lookAhead = listSourceWordIter_;
    while (lookAhead != listSourceWord_.end() && *lookAhead == ' ')
        ++lookAhead;
    if (lookAhead != listSourceWord_.end() && *lookAhead == '\n')
    {
        ++lookAhead;
        listSourceWordIter_ = lookAhead;
        return true; // Handled, continue
    }
    return false; // Not a continuation, continue normal processing
}

bool ListReader::processComments(ushort c, bool shouldRemoveComments)
{
    if (!shouldRemoveComments)
        return false;

    // Check for ; comment or #! comment
    if (c == ';' || (c == '#' && listSourceWordIter_ != listSourceWord_.end() &&
                     listSourceWordIter_->unicode() == '!'))
    {
        // Skip to end of line
        while ((listSourceWordIter_ != listSourceWord_.end()) && (*listSourceWordIter_ != '\n'))
            ++listSourceWordIter_;
        // Consume the eol
        if (listSourceWordIter_ != listSourceWord_.end())
            ++listSourceWordIter_;
        return true; // Handled, continue
    }
    return false; // Not a comment, continue normal processing
}


ListReader::DelimiterResult ListReader::processDelimiter(ushort c, ListBuilder &builder, QString &currentWord,
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
        builder.append(parseListWithPrompt("", false, false, shouldRemoveComments));
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
        builder.append(parseListWithPrompt("", false, true, shouldRemoveComments));
        return DelimiterResult::AppendSubarray;
    default:
        // Space or tab - just continue
        return DelimiterResult::Continue;
    }
}

int ListReader::processArrayOrigin()
{
    int origin = 1;
    if (listSourceWordIter_ != listSourceWord_.end() && *listSourceWordIter_ == '@')
    {
        QString originStr = "";
        ++listSourceWordIter_;
        while (listSourceWordIter_ != listSourceWord_.end() && (*listSourceWordIter_ >= '0') &&
               (*listSourceWordIter_ <= '9'))
        {
            originStr += *listSourceWordIter_;
            ++listSourceWordIter_;
        }
        bool ok;
        origin = originStr.toInt(&ok);
        if (!ok)
            origin = 1;
    }
    return origin;
}

DatumPtr ListReader::handleDelimiterResult(DelimiterResult result, ListBuilder &builder)
{
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
        return nothing(); // Continue processing
    }
    Q_ASSERT(false);
    return nothing(); // Should never reach here
}

/*************************************************************/
/************************ TextStream *************************/
/*************************************************************/

TextStream::TextStream(QTextStream *aStream) : rawStream_(aStream), wordReader_(&rawStream_), listReader_(&wordReader_)
{
}

DatumPtr TextStream::readListWithPrompt(const QString &prompt, bool shouldRemoveComments)
{
    return listReader_.readListWithPrompt(prompt, shouldRemoveComments);
}

DatumPtr TextStream::readWordWithPrompt(const QString &prompt)
{
    return wordReader_.readWordWithPrompt(prompt);
}

DatumPtr TextStream::readRawlineWithPrompt(const QString &prompt)
{
    return wordReader_.readRawlineWithPrompt(prompt);
}

QList<DatumPtr> TextStream::listReaderLineHistory() const
{
    return listReader_.lineHistory();
}

QList<DatumPtr> TextStream::wordReaderLineHistory() const
{
    return wordReader_.lineHistory();
}

DatumPtr TextStream::readChar()
{
    return rawStream_.readChar();
}

void TextStream::lprint(const QString &text)
{
    rawStream_.lprint(text);
}

QIODevice *TextStream::device() const
{
    return rawStream_.device();
}

QString *TextStream::string() const
{
    return rawStream_.string();
}

bool TextStream::seek(qint64 loc)
{
    return rawStream_.seek(loc);
}

qint64 TextStream::pos() const
{
    return rawStream_.pos();
}

bool TextStream::atEnd() const
{
    return rawStream_.atEnd();
}

void TextStream::flush()
{
    rawStream_.flush();
}