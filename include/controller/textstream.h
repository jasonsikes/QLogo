#ifndef READER_H
#define READER_H

//===-- qlogo/reader.h - Reader class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the TextStream, which is
/// responsible for reading and writing text through a text stream or through
/// standard I/O. It provides tokenization of text into a list of tokens.
///
/// Other operations simply pass through to the underlying QTextStream.
///
//===----------------------------------------------------------------------===//

#include "datum.h"
#include <QTextStream>

class TextStream
{
    // Keep the most recent history in case it's needed as source material.
    // (This is a List of RawLines.)
    DatumPtr recentLineHistory;

    // Clear the recent Line History.
    void clearLineHistory();

    // The stream source/destination. If NULL then use standard Input and Output.
    QTextStream *stream;

    // The work of List/Array reading is done here. Will call itself to process
    // sublists and subarrays.
    DatumPtr tokenizeListWithPrompt(const QString &prompt, bool isBaseLevel, bool makeArray, bool shouldRemoveComments);

    // The current source word for string parsing.
    QString listSourceWord;
    QString::iterator listSourceWordIter;

  public:
    /// @brief Create a TextStream object using a QTextStream as source/destination.
    /// @param aStream The QTextStream to use. Use NULL for the standardIO streams.
    TextStream(QTextStream *aStream);

    /// @brief Returns the exact string of characters as they appear
    /// in the line.
    /// @param prompt The prompt to display to the user.
    /// @param shouldSavePreviousLines If true, do not delete the previous line(s) from the line history.
    /// @return The line as a Word object, or nothing if no input is available.
    DatumPtr readrawlineWithPrompt(const QString &prompt, bool shouldSavePreviousLines = false);

    /// @brief Returns a line read as a word. Backslashes, vertical bars, and tilde
    /// characters are processed.
    /// @param prompt The prompt to display to the user.
    /// @param shouldSavePreviousLines If true, do not delete the previous line(s) from the line history.
    /// @return The line as a Word object, or nothing if no input is available.
    DatumPtr readwordWithPrompt(const QString &prompt, bool shouldSavePreviousLines = false);

    /// @brief Reads a line as a list.
    /// @param prompt The prompt to display to the user.
    /// @param shouldRemoveComments If true, remove QLogo-formatted comments from the list.
    /// @param shouldSavePreviousLines If true, do not delete the previous line(s) from the line history.
    /// @return The line as a List object, or nothing if no input is available.
    DatumPtr readlistWithPrompt(const QString &prompt, bool shouldRemoveComments, bool shouldSavePreviousLines = false);

    /// @brief Read a single character. No formatting is applied.
    /// @return The character as a Character object, or an empty List object if no input is available.
    DatumPtr readChar();

    /// @brief Return the contents of the current line history.
    /// @return The line history as a List of RawLines.
    DatumPtr recentHistory();

    /// @brief Pass-through to the underlying QTextStream::seek()
    /// @param loc The position to seek to.
    /// @return True if the seek was successful, false otherwise.
    bool seek(qint64 loc);

    /// @brief Pass-through to the underlying QTextStream::pos()
    /// @return The current position in the stream.
    qint64 pos();

    /// @brief Pass-through to the underlying QTextStream::atEnd()
    /// @return True if the stream is at the end, false otherwise.
    bool atEnd();

    /// @brief Pass-through to the underlying QTextStream::flush()
    void flush();

    /// @brief Print a QString to the device
    /// @param text The text to print.
    void lprint(QString text);

    /// @brief Return the underlying device (NULL if there isn't a device).
    /// @return The QIODevice.
    QIODevice *device();

    /// @brief Return the underlying string buffer (NULL if there isn't one).
    /// @return The string buffer.
    QString *string();
};

#endif // READER_H
