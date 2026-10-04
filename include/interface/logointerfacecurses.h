
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
#include <cwchar>

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

    /// Color pairs. Every pair shares the current background (bgNum_), so the
    /// table is keyed by foreground only. currentPair_ paints text and
    /// bgPair_ (foreground -1) is the window background.
    short currentPair_ = 0;
    short bgPair_ = 0;
    int nextPair_ = 1;
    QHash<int, short> pairByFg_;

    bool standoutOn_ = false;
    QList<char32_t> unread_;
    LineEditor lineEditor_;
    int editDrawEndY_ = 0;

    int colorNumber(const QColor &color) const;

    /// @brief Pair numbers used by this class stay in [1, pairLimit()).
    /// @return The smaller of COLOR_PAIRS and the range COLOR_PAIR() / short
    /// interfaces can carry on every ncurses ABI.
    int pairLimit() const;

    /// @brief Pair for @p fg on the current background, defining it if needed.
    /// @return The pair number, or 0 (terminal default) if none can be allocated.
    short ensurePair(int fg);

    void applyCurrentPair();
    void applyStandoutAttr();

    /// @brief Redefine every allocated pair with @p newBg as its background.
    void rebindPairsToBackground(int newBg);
    void recolorDefaultCells(int newBg);
    void redrawEditLine();

    /// @brief get_wch that classifies ERR by a pending Logo signal rather than
    /// trusting errno, and removes the stale ERR ncurses queues after an
    /// interrupted read so the next call reads the terminal again.
    /// @param ch Receives the character or KEY_* code.
    /// @return OK, KEY_CODE_YES, or ERR. On ERR, errno is EINTR exactly when
    /// the read was interrupted; otherwise it was a nodelay timeout or EOF.
    int readWideChar(wint_t &ch);

    /// @brief Repaint the edit line with the cursor at its end and move to the
    /// next screen line, leaving the text visible (Enter, or before an interrupt).
    void leaveEditLine();

    /// @brief Print @p prompt on a fresh line and point lineEditor_ at the cell
    /// after it. Used to resume an interrupted prompt after PAUSE returns.
    void printPromptAndSetOrigin(const QString &prompt);

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
