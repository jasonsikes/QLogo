
//===-- qlogo/logointerfacecurses.h - LogoInterfaceCurses class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the LogoInterfaceCurses class, which
/// handles user interaction through an ncursesw text user interface.
///
//===----------------------------------------------------------------------===//

#ifndef LOGOINTERFACECURSES_H
#define LOGOINTERFACECURSES_H

#include "interface/lineeditor.h"
#include "interface/logointerface.h"
#include <QColor>
#include <QHash>
#include <QList>

/// @brief The LogoInterfaceCurses class is a LogoInterface that talks to the
/// terminal through ncursesw.
///
/// It implements the Logo text console: printing, line input, SETTEXTCOLOR,
/// CLEARTEXT, SETCURSOR/CURSOR, KEYP, LINEP, and STANDOUT. Turtle graphics remain
/// unavailable.
class LogoInterfaceCurses : public LogoInterface
{
    bool cursesActive_ = false;
    QColor foregroundColor_;
    QColor backgroundColor_;
    int fgNum_ = -1;
    int bgNum_ = -1;
    short currentPair_ = 0;
    int nextPair_ = 1;
    QHash<quint64, short> pairByFgBg_;
    bool standoutOn_ = false;
    QList<char32_t> unread_;
    LineEditor lineEditor_;
    int editDrawEndY_ = 0;

    int colorNumber(const QColor &color) const;
    short ensurePair(int fg, int bg);
    void initColorPair(short pair, int fg, int bg);
    void applyCurrentPair();
    void applyStandoutAttr();
    void rebindPairsToBackground(int newBg);
    void recolorDefaultCells(int newBg);
    void redrawEditLine();

  public:
    /// @brief Constructor for the LogoInterfaceCurses class.
    /// @param parent The parent Qt object.
    LogoInterfaceCurses(QObject *parent = nullptr);

    /// @brief Destructor. Restores the terminal if ncurses is still active.
    ~LogoInterfaceCurses() override;

    /// @brief Starts ncurses and color support.
    void initialize() override;

    /// @brief Tears down ncurses and restores the terminal.
    void closeInterface() override;

    /// @brief Prints a string to the ncurses screen.
    /// @param text The text to print.
    void printToConsole(const QString &text) override;

    /// @brief Prompts for a line of input and returns it.
    /// @param prompt The prompt to display to the user, if any.
    /// @return The line entered by the user, or a null QString on EOF.
    QString inputRawlineWithPrompt(const QString &prompt) override;

    /// @brief Reads a single character from the terminal.
    /// @return The character that was read, or nothing on EOF.
    DatumPtr readchar() override;

    /// @brief True if a character is waiting on the terminal (KEYP / KEY?).
    bool keyQueueHasChars() override;

    /// @brief True if a complete line is waiting on the terminal (LINEP / LINE?).
    bool lineQueueHasLine() override;

    /// @brief Wraps a string in STANDOUT markers (ESC). Printing toggles A_REVERSE.
    /// @param src The string to mark.
    /// @return The marked string.
    QString addStandoutToString(const QString &src) override;

    /// @brief CLEARTEXT / CT: erase the screen and home the cursor.
    void clearScreenText() override;

    /// @brief CURSOR: current text cursor as row and column.
    /// @param row The row (0 is the top).
    /// @param col The column (0 is the left).
    void getTextCursorPos(int &row, int &col) override;

    /// @brief SETCURSOR: move the text cursor. Logo order is [column row].
    /// @param row The row (0 is the top).
    /// @param col The column (0 is the left).
    void setTextCursorPos(int row, int col) override;

    /// @brief Sets the text color.
    /// @param foregroundColor The color of future printed text.
    /// @param backgroundColor The background color. An invalid color leaves
    /// the existing background unchanged. A valid color also updates the
    /// background of text already on the screen.
    void setTextColor(const QColor &foregroundColor, const QColor &backgroundColor) override;

    /// @brief Flushes the ncurses screen, then sleeps.
    /// @param msecs Milliseconds to wait.
    void mwait(unsigned long msecs) override;
};

#endif // LOGOINTERFACECURSES_H
