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
/// standard I/O.
///
/// Standard I/O text is passed through the controller class.
///
/// The class provides an interface that allows text to be read in ways that
/// the Logo language expects: READRAWLINE, READWORD, and READLIST.
///
/// Other operations simply pass through to the underlying QTextStream or
/// controller class.
///
//===----------------------------------------------------------------------===//

#include "datum_datump.h"

#include <QTextStream>

class TextStream {
    // Keep the most recent history in case it's needed as source material.
    // (This is a List of RawLines.)
     DatumPtr recentLineHistory;

    // Clear the recent Line History.
     void clearLineHistory();

    // The stream source/destination. If NULL then use standard Input and Output.
    QTextStream *stream;

    // The work of List/Array reading is done here. Will call itself to process
    // sublists and subarrays.
    DatumPtr tokenizeListWithPrompt(const QString &prompt, bool isBaseLevel, bool makeArray,
                                    bool shouldRemoveComments);

    // The current source word for string parsing.
    QString listSourceWord;
    QString::iterator listSourceWordIter;

public:

    /// Create a TextStream object using a QTextStream as source/destination.
    /// Use NULL for the standardIO streams.
    TextStream(QTextStream *aStream);

    /// Returns the exact string of characters as they appear
    /// in the line, with no special meaning for backslash, vertical bar,
    /// tilde, or any other formatting characters.
    DatumPtr readrawlineWithPrompt(const QString &prompt,
                                   bool shouldSavePreviousLines = false);

    /// Returns a line read as a word. Backslashes, vertical bars, and tilde
    /// characters are processed.
    DatumPtr readwordWithPrompt(const QString &prompt,
                                bool shouldSavePreviousLines = false);

    /// Reads a line as a list.
    DatumPtr readlistWithPrompt(const QString &prompt,
                                bool shouldRemoveComments,
                                bool shouldSavePreviousLines = false);

    /// Read a single character. No formatting is applied.
    DatumPtr readChar();

    /// Return the contents of the current line history.
    DatumPtr recentHistory();

    /// Pass-through to the underlying seek()
    bool seek(qint64 loc);

    /// Pass-through to the underlying pos()
    qint64 pos();

    /// Pass-through to the underlying atEnd()
    bool atEnd();

    /// Pass-through to the underlying flush()
    void flush();

    /// Print a QString to the device
    void lprint(QString text);

    /// Return the underlying device (NULL if there isn't a device).
    QIODevice* device();

    /// Return the underlying string buffer (NULL if there isn't one).
    QString* string();
};



#endif // READER_H
