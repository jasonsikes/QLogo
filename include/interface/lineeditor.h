#ifndef LINEEDITOR_H
#define LINEEDITOR_H

//===-- qlogo/lineeditor.h - LineEditor class definition -------*- C++ -*-===//
//
// Copyright 2017-2026 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the LineEditor class, which holds the
/// buffer, cursor, prompt origin, and session history for a console prompt.
/// It does not draw or read the terminal; LogoInterfaceCurses owns that.
///
//===----------------------------------------------------------------------===//

#include <QString>
#include <QStringList>

/// @brief In-memory state for one editable console prompt line.
///
/// The cursor is a character index into @c text(), not a screen column.
/// History follows Psychi's console: a scratch slot at the end while a prompt
/// is active, and Up/Down replace the buffer from earlier entries.
class LineEditor
{
    QString buffer_;
    int cursor_ = 0;
    int startY_ = 0;
    int startX_ = 0;
    QStringList history_;
    int historyIndex_ = 0;
    bool promptActive_ = false;

    void clampCursor();
    void saveScratchIfEdited();
    void loadHistoryIndex(int newIndex);

  public:
    /// @brief Start a new prompt at the given screen origin.
    /// @param startY The row of the first character after the prompt.
    /// @param startX The column of the first character after the prompt.
    /// @details Clears the buffer. Reuses an unfinished scratch history slot
    /// when the previous prompt did not call finish() (interrupt / EOF).
    void beginPrompt(int startY, int startX);

    /// @brief Update the prompt origin (wrap or scroll moved the start cell).
    /// @param startY The row of the first character of the edit buffer.
    /// @param startX The column of the first character of the edit buffer.
    void setOrigin(int startY, int startX);

    /// @brief Row of the first character of the edit buffer.
    int originRow() const
    {
        return startY_;
    }

    /// @brief Column of the first character of the edit buffer.
    int originCol() const
    {
        return startX_;
    }

    /// @brief The current line text.
    const QString &text() const
    {
        return buffer_;
    }

    /// @brief Character index of the cursor in text().
    int cursor() const
    {
        return cursor_;
    }

    /// @brief Move the cursor, clamped to [0, text().size()].
    /// @param index The desired character index.
    void setCursor(int index);

    /// @brief Move the cursor one Unicode character left, if possible.
    void moveLeft();

    /// @brief Move the cursor one Unicode character right, if possible.
    void moveRight();

    /// @brief Move the cursor to the start of the line.
    void moveHome();

    /// @brief Move the cursor to the end of the line.
    void moveEnd();

    /// @brief Delete the Unicode character to the left of the cursor (Backspace).
    void eraseLeft();

    /// @brief Delete the Unicode character at the cursor (Delete).
    void eraseAtCursor();

    /// @brief Delete from the cursor to the end of the line (Ctrl-K).
    void killToEnd();

    /// @brief Delete the entire line and move the cursor home (Ctrl-U).
    void killLine();

    /// @brief Delete the word to the left of the cursor (Ctrl-W).
    void killWord();

    /// @brief Insert @p ch at the cursor and advance the cursor.
    /// @param ch A Unicode code point.
    void insert(char32_t ch);

    /// @brief Insert @p text at the cursor and advance the cursor.
    /// @param text The text to insert.
    void insert(const QString &text);

    /// @brief Commit the current line to history and return it.
    /// @return The accepted line (may be empty, but not null).
    QString finish();

    /// @brief Replace the buffer with the previous history entry.
    /// @return True if the buffer changed.
    bool recallOlder();

    /// @brief Replace the buffer with the next history entry (toward scratch).
    /// @return True if the buffer changed.
    bool recallNewer();
};

#endif // LINEEDITOR_H
