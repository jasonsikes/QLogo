#ifndef READER_H
#define READER_H

//===-- qlogo/reader.h - Reader class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the TextStream, which is
/// responsible for reading and writing text through a text stream or through
/// standard I/O. It provides tokenization of text into a list of tokens.
///
/// Other operations simply pass through to the underlying QTextStream.
///
//===----------------------------------------------------------------------===//

#include "datum_types.h"
#include <QTextStream>
#include <QList>


class RawStream
{
    public:

    // The stream source/destination. If nullptr then use standard Input and Output.
    QTextStream *stream_;

    /// @brief Create a TextStream object using a QTextStream as source/destination.
    /// @param aStream The QTextStream to use. Use nullptr for the standardIO streams.
    RawStream(QTextStream *aStream);

    /// @brief Return the underlying device (nullptr if there isn't a device).
    /// @return The QIODevice.
    QIODevice *device() const;

    /// @brief Return the underlying string buffer (nullptr if there isn't one).
    /// @return The string buffer.
    QString *string() const;

    /// @brief Pass-through to the underlying QTextStream::seek()
    /// @param loc The position to seek to.
    /// @return True if the seek was successful, false otherwise.
    bool seek(qint64 loc);

    /// @brief Pass-through to the underlying QTextStream::pos()
    /// @return The current position in the stream.
    qint64 pos() const;

    /// @brief Pass-through to the underlying QTextStream::atEnd()
    /// @return True if the stream is at the end, false otherwise.
    bool atEnd() const;

    /// @brief Pass-through to the underlying QTextStream::flush()
    void flush();

    /// @brief Print a QString to the device
    /// @param text The text to print.
    void lprint(const QString &text);

    /// @brief Returns the exact string of characters as they appear
    /// in the line.
    /// @param prompt The prompt to display to the user.
    /// @return The line as a Word object, or nothing if no input is available.
    DatumPtr readRawlineWithPrompt(const QString &prompt);

    /// @brief Read a single character. No formatting is applied.
    /// @return The character as a Character object, or an empty List object if no input is available.
    DatumPtr readChar();
};


/// @brief A reader for words from a TextStream.
class WordReader
{
    RawStream *rawStream_;
    QList<DatumPtr> lineHistory_;

    public:
    /// @brief Create a WordReader object using a RawStream as source.
    /// @param textStream The TextStream to use.
    WordReader(RawStream *rawStream);

    /// @brief Read a word from the RawStream.
    /// @param prompt The prompt to display to the user.
    /// @return The word as a Word object, or nothing if no input is available.
    DatumPtr readWordWithPrompt(const QString &prompt);

    /// @brief Return a QList of rawlines that have been read to create the most recent word.
    /// @return The line history as a List of RawLines.
    /// @details Each call to readWordWithPrompt generates a new list of rawlines. They are
    /// discarded with each call to readWordWithPrompt. Call this function after the call to readWordWithPrompt
    /// to get the rawlines that were used to create the most recent word.
    QList<DatumPtr> lineHistory() const;

    /// @brief Read a rawline with a prompt from the RawStream.
    /// @param prompt The prompt to display to the user.
    /// @return The rawline as a Word object, or nothing if no input is available.
    DatumPtr readRawlineWithPrompt(const QString &prompt);
};

class ListReader
{
    WordReader *wordReader_;
    QList<DatumPtr> lineHistory_;

    // The current source word for string parsing.
    QString listSourceWord_;
    QString::iterator listSourceWordIter_;
    
    enum class DelimiterResult { Continue, ReturnList, ReturnArray, AppendSublist, AppendSubarray };
    DelimiterResult processDelimiter(ushort c, ListBuilder &builder, QString &currentWord, bool &isCurrentWordVbarred,
                                     bool isBaseLevel, bool makeArray, bool shouldRemoveComments);

    DatumPtr handleDelimiterResult(DelimiterResult result, ListBuilder &builder);

    /// @brief Parse a list with a prompt from the WordReader.
    /// @param prompt The prompt to display to the user.
    /// @param isBaseLevel If true, the list is a base level list.
    /// @param makeArray If true, return the list as an array.
    /// @param shouldRemoveComments If true, remove QLogo-formatted comments from the list.
    /// @return The list as a List object, or nothing if no input is available.
    DatumPtr parseListWithPrompt(const QString &prompt, bool isBaseLevel, bool makeArray, bool shouldRemoveComments);

    bool initializeBaseLevelReading(const QString &prompt);

    DatumPtr processCharacterLoop(ListBuilder &builder, QString &currentWord, bool &isCurrentWordVbarred,
    bool isBaseLevel, bool makeArray, bool shouldRemoveComments);

    void addCurrentWordToBuilder(ListBuilder &builder, QString &currentWord, bool isCurrentWordVbarred);

    bool finalizeResult(ListBuilder &builder, bool isBaseLevel, bool makeArray, DatumPtr &result);

    bool processVbarredCharacter(ushort c, bool &isVbarred, bool &isCurrentWordVbarred, QString &currentWord);

    bool processTildeContinuation();

    bool processComments(ushort c, bool shouldRemoveComments);

    int processArrayOrigin();

public:
    /// @brief Create a ListReader object using a WordReader as source.
    /// @param wordReader The WordReader to use.
    ListReader(WordReader *wordReader);

    /// @brief Reads a line as a list.
    /// @param prompt The prompt to display to the user.
    /// @param shouldRemoveComments If true, remove QLogo-formatted comments from the list.
    /// @return The line as a List object, or nothing if no input is available.
    DatumPtr readListWithPrompt(const QString &prompt, bool shouldRemoveComments);

    /// @brief Return a QList of rawlines that have been read to create the most recent list.
    /// @return The line history as a List of RawLines.
    /// @details Each call to readListWithPrompt generates a new list of rawlines. They are
    /// discarded with each call to readListWithPrompt. Call this function after the call to readListWithPrompt
    /// to get the rawlines that were used to create the most recent list.
    QList<DatumPtr> lineHistory() const;
};

class TextStream
{
    RawStream rawStream_;
    WordReader wordReader_;
    ListReader listReader_;

    public:
    TextStream(QTextStream *aStream);

    DatumPtr readListWithPrompt(const QString &prompt, bool shouldRemoveComments);
    QList<DatumPtr> listReaderLineHistory() const;

    DatumPtr readWordWithPrompt(const QString &prompt);
    QList<DatumPtr> wordReaderLineHistory() const;

    DatumPtr readRawlineWithPrompt(const QString &prompt);

    DatumPtr readChar();

    void lprint(const QString &text);

    QIODevice *device() const;

    QString *string() const;

    bool seek(qint64 loc);

    qint64 pos() const;

    bool atEnd() const;

    void flush();
};



#endif // READER_H
