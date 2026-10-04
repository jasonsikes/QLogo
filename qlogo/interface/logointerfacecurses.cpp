//===-- qlogo/logointerfacecurses.cpp - LogoInterfaceCurses implementation -------*- C++ -*-===//
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
/// This file contains the ncursesw implementation of LogoInterfaceCurses.
///
//===----------------------------------------------------------------------===//

#include "interface/logointerfacecurses.h"
#include "interface/pipeio.h"
#include <algorithm>
#include <cerrno>
#include <clocale>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <optional>
#include <string>

#ifndef NCURSES_WIDECHAR
#define NCURSES_WIDECHAR 1
#endif
#include <ncurses.h>

namespace
{
/// Upper bound on pair numbers.
constexpr int kPairNumberLimit = 256;

int rgbToAnsi8(const QColor &color)
{
    const int rBit = color.red() >= 128 ? 1 : 0;
    const int gBit = color.green() >= 128 ? 2 : 0;
    const int bBit = color.blue() >= 128 ? 4 : 0;
    return rBit | gBit | bBit;
}

int rgbToXterm256(const QColor &color)
{
    const int r = color.red();
    const int g = color.green();
    const int b = color.blue();
    if (r == g && g == b)
    {
        if (r < 8)
            return 16;
        if (r > 238)
            return 231;
        return 232 + (r - 8) / 10;
    }
    const int ri = (r * 5 + 127) / 255;
    const int gi = (g * 5 + 127) / 255;
    const int bi = (b * 5 + 127) / 255;
    return 16 + 36 * ri + 6 * gi + bi;
}

/// Non-blocking get_wch for the duration of a scope (KEYP / LINEP polling).
/// initialize() leaves stdscr in noecho + keypad mode permanently, so the only
/// per-read setting is nodelay.
//
//  The previous value is restored on exit, which
/// keeps nested reads (PAUSE inside a prompt) in a consistent state.
class ScopedNodelay
{
    bool active_;
    bool prevNodelay_;

  public:
    explicit ScopedNodelay(bool cursesActive) : active_(cursesActive), prevNodelay_(false)
    {
        if (!active_)
            return;
        prevNodelay_ = is_nodelay(stdscr);
        nodelay(stdscr, TRUE);
    }

    ~ScopedNodelay()
    {
        if (!active_)
            return;
        nodelay(stdscr, prevNodelay_ ? TRUE : FALSE);
    }

    ScopedNodelay(const ScopedNodelay &) = delete;
    ScopedNodelay &operator=(const ScopedNodelay &) = delete;
};

/// Marker for the Delete key (KEY_DC) in the typeahead queue. It lies outside
/// Unicode, so it cannot collide with typed text. Raw 127 (DEL) is what most
/// terminals send for the physical Backspace key and must stay distinct from
/// this: the line editor erases forward for KEY_DC and backward for 127.
///
/// READCHAR reports both as 127 via readcharString().
constexpr char32_t kDeleteKey = 0x110000;

/// True if get_wch produced a key for KEYP / LINEP / READCHAR.
/// Motion, history, and resize keys are not characters. Backspace and keypad
/// Enter map to the bytes Logo programs already test for; Delete is queued as
/// kDeleteKey so the line editor can tell it apart from a raw DEL byte.
bool mapGetWchToChar(int rc, wint_t ch, char32_t &out)
{
    if (rc == ERR)
        return false;
    if (rc == KEY_CODE_YES)
    {
        if (ch == KEY_BACKSPACE)
        {
            out = 8U;
            return true;
        }
        if (ch == KEY_DC)
        {
            out = kDeleteKey;
            return true;
        }
        if (ch == KEY_ENTER)
        {
            out = U'\n';
            return true;
        }
        return false;
    }
    out = static_cast<char32_t>(ch);
    return true;
}

/// The one-character string READCHAR returns for a queued key.
QString readcharString(char32_t key)
{
    if (key == kDeleteKey)
        key = 127U;
    return QString::fromUcs4(&key, 1);
}

enum class KeyResult
{
    Continue,
    LineDone,
    EndOfInput
};

/// Apply one key, in the encoding mapGetWchToChar() produces, to the line
/// editor. Used for both typeahead and interactive input so the two cannot
/// drift. CR/LF completes the line; Ctrl-D on an empty line is end of input,
/// otherwise it deletes forward; other control characters are ignored.
KeyResult applyKeyToLineEditor(LineEditor &editor, char32_t ch)
{
    if (ch == U'\n' || ch == U'\r')
        return KeyResult::LineDone;
    if (ch == 4U)
    {
        if (editor.text().isEmpty())
            return KeyResult::EndOfInput;
        editor.eraseAtCursor();
    }
    else if (ch == kDeleteKey)
        editor.eraseAtCursor();
    else if (ch == 8U || ch == 127U)
        editor.eraseLeft();
    else if (ch == 1U)
        editor.moveHome();
    else if (ch == 5U)
        editor.moveEnd();
    else if (ch == 11U)
        editor.killToEnd();
    else if (ch == 21U)
        editor.killLine();
    else if (ch == 23U)
        editor.killWord();
    else if (ch >= 32U)
        editor.insert(ch);
    return KeyResult::Continue;
}

/// Keys that only exist interactively: cursor motion, history, and resize.
/// They never enter the typeahead queue because mapGetWchToChar() drops them.
/// Returns true if @p ch was one of these (the caller redraws).
bool applyMotionKey(LineEditor &editor, wint_t ch)
{
    switch (ch)
    {
    case KEY_LEFT:
        editor.moveLeft();
        return true;
    case KEY_RIGHT:
        editor.moveRight();
        return true;
    case KEY_HOME:
        editor.moveHome();
        return true;
    case KEY_END:
        editor.moveEnd();
        return true;
    case KEY_UP:
        editor.recallOlder();
        return true;
    case KEY_DOWN:
        editor.recallNewer();
        return true;
    case KEY_RESIZE:
        return true;
    default:
        return false;
    }
}

bool unreadHasLineBreak(const QList<char32_t> &unread)
{
    for (char32_t ch : unread)
    {
        if (ch == U'\n' || ch == U'\r')
            return true;
    }
    return false;
}

void paintWString(const QString &text)
{
    if (text.isEmpty())
        return;
    const std::wstring wide = text.toStdWString();
    addwstr(wide.c_str());
}

struct CellPos
{
    int y = 0;
    int x = 0;
};

char32_t nextUcs4(const QString &text, int &i)
{
    const QChar c = text.at(i++);
    if (c.isHighSurrogate() && i < text.size() && text.at(i).isLowSurrogate())
        return QChar::surrogateToUcs4(c, text.at(i++));
    return static_cast<char32_t>(c.unicode());
}

int displayWidth(char32_t cp)
{
    const int w = wcwidth(static_cast<wchar_t>(cp));
    return w < 0 ? 1 : w;
}

void wrapIfNeeded(CellPos &p, int w, int maxx)
{
    if (w > 0 && p.x > 0 && p.x + w > maxx)
    {
        p.y++;
        p.x = 0;
    }
}

void consumeWidth(CellPos &p, int w, int maxx)
{
    if (w <= 0)
        return;
    p.x += w;
    if (p.x >= maxx)
    {
        p.y++;
        p.x = 0;
    }
}
} // namespace

LogoInterfaceCurses::LogoInterfaceCurses(QObject *parent) : LogoInterface(parent)
{
}

LogoInterfaceCurses::~LogoInterfaceCurses()
{
    closeInterface();
}

bool LogoInterfaceCurses::initialize()
{
    if (cursesActive_)
        return true;

    // logo_main.cpp checks this before choosing the TUI; kept as a guard for
    // any other caller, since curses on a pipe would misbehave badly.
    if (!stdioIsInteractive())
    {
        fprintf(stderr, "qlogo: --tui requires an interactive terminal\n");
        return false;
    }

    setlocale(LC_CTYPE, "");

    // initscr() exits the process itself when the terminal cannot be opened;
    // newterm() reports failure instead, so the caller can unwind normally.
    screen_ = newterm(nullptr, stdout, stdin);
    if (screen_ == nullptr)
    {
        const char *term = getenv("TERM");
        fprintf(stderr, "qlogo: cannot start the text interface on terminal type \"%s\"\n", term ? term : "");
        return false;
    }
    def_prog_mode();

    cursesActive_ = true;
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, FALSE);
    scrollok(stdscr, TRUE);
    idlok(stdscr, TRUE);
    if (has_colors())
    {
        start_color();
        use_default_colors();
    }
    refresh();
    return true;
}

void LogoInterfaceCurses::closeInterface()
{
    if (!cursesActive_)
        return;
    endwin();
    delscreen(screen_);
    screen_ = nullptr;
    cursesActive_ = false;
}

void LogoInterfaceCurses::printToConsole(const QString &text)
{
    if (!cursesActive_)
        return;

    const QStringList parts = text.split(Config::get().escapeChar_);
    for (int i = 0; i < parts.size(); ++i)
    {
        if (i != 0)
        {
            standoutOn_ = !standoutOn_;
            applyStandoutAttr();
        }
        const std::wstring wide = parts[i].toStdWString();
        addwstr(wide.c_str());
    }
    refresh();
}

QString LogoInterfaceCurses::addStandoutToString(const QString &src)
{
    return Config::get().escapeString_ + src + Config::get().escapeString_;
}

void LogoInterfaceCurses::clearScreenText()
{
    if (!cursesActive_)
        return;
    erase();
    wmove(stdscr, 0, 0);
    applyCurrentPair();
    applyStandoutAttr();
    refresh();
}

void LogoInterfaceCurses::getTextCursorPos(int &row, int &col)
{
    if (!cursesActive_)
    {
        row = 0;
        col = 0;
        return;
    }
    getyx(stdscr, row, col);
}

void LogoInterfaceCurses::setTextCursorPos(int row, int col)
{
    if (!cursesActive_)
        return;

    int maxy = 0;
    int maxx = 0;
    getmaxyx(stdscr, maxy, maxx);
    if (row < 0)
        row = 0;
    if (col < 0)
        col = 0;
    if (maxy > 0 && row >= maxy)
        row = maxy - 1;
    if (maxx > 0 && col >= maxx)
        col = maxx - 1;
    wmove(stdscr, row, col);
    refresh();
}

int LogoInterfaceCurses::readWideChar(wint_t &ch)
{
    errno = 0;
    const int rc = get_wch(&ch);
    if (rc != ERR)
        return rc;

    if (!LogoInterface::signalPending() && errno != EINTR)
        return ERR;

    // Pop the stale ERR that ncurses leaves queued after an EINTR'd read.
    {
        ScopedNodelay drain(cursesActive_);
        wint_t stale = 0;
        const int staleRc = get_wch(&stale);
        char32_t ucs4 = 0;
        if (staleRc != ERR && mapGetWchToChar(staleRc, stale, ucs4))
            unread_.append(ucs4);
    }

    errno = EINTR; // callers test this; normalize whatever ncurses left behind
    return ERR;
}

bool LogoInterfaceCurses::keyQueueHasChars()
{
    if (!unread_.isEmpty())
        return true;
    if (!cursesActive_ || atEnd())
        return false;

    ScopedNodelay poll(cursesActive_);
    for (;;)
    {
        wint_t ch = 0;
        const int rc = readWideChar(ch);
        if (rc == ERR)
            return false;
        char32_t ucs4 = 0;
        if (!mapGetWchToChar(rc, ch, ucs4))
            continue;
        unread_.append(ucs4);
        return true;
    }
}

bool LogoInterfaceCurses::lineQueueHasLine()
{
    if (!cursesActive_ || atEnd())
        return false;
    if (unreadHasLineBreak(unread_))
        return true;

    ScopedNodelay poll(cursesActive_);
    for (;;)
    {
        wint_t ch = 0;
        const int rc = readWideChar(ch);
        if (rc == ERR)
            break;
        char32_t ucs4 = 0;
        if (!mapGetWchToChar(rc, ch, ucs4))
            continue;
        unread_.append(ucs4);
        if (ucs4 == U'\n' || ucs4 == U'\r')
            break;
    }

    return unreadHasLineBreak(unread_);
}

void LogoInterfaceCurses::redrawEditLine()
{
    if (!cursesActive_)
        return;

    int maxy = 0;
    int maxx = 0;
    getmaxyx(stdscr, maxy, maxx);
    if (maxy < 1)
        maxy = 1;
    if (maxx < 1)
        maxx = 1;

    int originY = lineEditor_.originRow();
    int originX = lineEditor_.originCol();
    if (originY < 0)
        originY = 0;
    if (originX < 0)
        originX = 0;
    if (originY >= maxy)
        originY = maxy - 1;
    if (originX >= maxx)
        originX = 0;
    lineEditor_.setOrigin(originY, originX);

    applyCurrentPair();
    applyStandoutAttr();

    const QString &text = lineEditor_.text();
    const int cursor = lineEditor_.cursor();
    CellPos p{originY, originX};
    CellPos cursorPos{originY, originX};

    auto scrollIfOffScreen = [&](int y) {
        if (y < maxy)
            return 0;
        const int scrolled = y - (maxy - 1);
        wscrl(stdscr, scrolled);
        int newOrigin = lineEditor_.originRow() - scrolled;
        if (newOrigin < 0)
            newOrigin = 0;
        lineEditor_.setOrigin(newOrigin, lineEditor_.originCol());
        editDrawEndY_ -= scrolled;
        return scrolled;
    };

    auto applyScroll = [&](int scrolled) {
        if (scrolled <= 0)
            return;
        p.y -= scrolled;
        if (p.y < 0)
            p.y = 0;
        cursorPos.y -= scrolled;
        if (cursorPos.y < 0)
            cursorPos.y = 0;
    };

    int i = 0;
    while (i < text.size())
    {
        const char32_t cp = nextUcs4(text, i);
        const int w = displayWidth(cp);
        wrapIfNeeded(p, w, maxx);
        applyScroll(scrollIfOffScreen(p.y));
        wmove(stdscr, p.y, p.x);
        const QString piece = QString::fromUcs4(&cp, 1);
        paintWString(piece);
        consumeWidth(p, w, maxx);
        if (i <= cursor)
            cursorPos = p;
    }

    applyScroll(scrollIfOffScreen(p.y));
    wmove(stdscr, p.y, p.x);
    clrtoeol();

    const int lastY = editDrawEndY_;
    for (int y = p.y + 1; y <= lastY && y < maxy; ++y)
    {
        wmove(stdscr, y, 0);
        clrtoeol();
    }
    editDrawEndY_ = p.y;

    applyScroll(scrollIfOffScreen(cursorPos.y));
    if (cursorPos.y < 0)
        cursorPos.y = 0;
    if (cursorPos.y >= maxy)
        cursorPos.y = maxy - 1;
    if (cursorPos.x < 0)
        cursorPos.x = 0;
    if (cursorPos.x >= maxx)
        cursorPos.x = maxx - 1;
    wmove(stdscr, cursorPos.y, cursorPos.x);
    refresh();
}

void LogoInterfaceCurses::leaveEditLine()
{
    lineEditor_.moveEnd();
    redrawEditLine();
    paintWString(QStringLiteral("\n"));
    refresh();
}

void LogoInterfaceCurses::printPromptAndSetOrigin(const QString &prompt)
{
    int y = 0;
    int x = 0;
    getyx(stdscr, y, x);
    if (x != 0)
        paintWString(QStringLiteral("\n"));

    printToConsole(prompt);
    getyx(stdscr, y, x);
    lineEditor_.setOrigin(y, x);
    editDrawEndY_ = y;
}

QString LogoInterfaceCurses::inputRawlineWithPrompt(const QString &prompt)
{
    if (atEnd())
        return {};

    printToConsole(prompt);

    int startY = 0;
    int startX = 0;
    getyx(stdscr, startY, startX);
    lineEditor_.beginPrompt(startY, startX);
    editDrawEndY_ = lineEditor_.originRow();

    auto acceptLine = [this]() {
        leaveEditLine();
        return lineEditor_.finish();
    };

    // Ctrl-D on an empty line. Leave the prompt row and report end of input.
    auto endInput = [this]() {
        paintWString(QStringLiteral("\n"));
        refresh();
        inputAtEof_ = true;
        return QString();
    };

    // Shared dispatch for typeahead and interactive keys. Returns the finished
    // line (or the null EOF string) when the key ends the prompt.
    auto applyKey = [&](char32_t key) -> std::optional<QString> {
        switch (applyKeyToLineEditor(lineEditor_, key))
        {
        case KeyResult::LineDone:
            return acceptLine();
        case KeyResult::EndOfInput:
            return endInput();
        case KeyResult::Continue:
            break;
        }
        return std::nullopt;
    };

    // Typeahead queued by KEYP / LINEP. A line completed here is accepted the
    // same way as one typed interactively, so it is painted and followed by a
    // newline instead of leaving the cursor on the prompt row.
    while (!unread_.isEmpty())
    {
        if (const auto done = applyKey(unread_.takeFirst()))
            return *done;
    }

    redrawEditLine();

    for (;;)
    {
        if (atEnd())
            return {};

        wint_t ch = 0;
        const int rc = readWideChar(ch);
        if (rc == ERR)
        {
            if (errno == EINTR)
            {
                // TOPLEVEL / SYSTEM throw out of retryAfterInterrupt(); PAUSE runs a
                // nested REPL that reuses lineEditor_ and editDrawEndY_. Either way
                // the kernel is about to print, so leave the partial line on screen,
                // move below it, and keep a copy of the editor to come back to.
                const LineEditor saved = lineEditor_;
                leaveEditLine();
                retryAfterInterrupt();

                // Back from PAUSE (or no Logo signal was pending): re-prompt on a
                // fresh line and redraw the saved text with its cursor position.
                lineEditor_ = saved;
                printPromptAndSetOrigin(prompt);
                redrawEditLine();
                continue;
            }
            inputAtEof_ = true;
            return {};
        }
        if (rc == KEY_CODE_YES && applyMotionKey(lineEditor_, ch))
        {
            redrawEditLine();
            continue;
        }

        // Everything else is encoded exactly as it would be in the typeahead
        // queue (Backspace, Delete, keypad Enter, plain characters) and goes
        // through the same editor dispatch. Unmapped function keys are dropped.
        char32_t key = 0;
        if (!mapGetWchToChar(rc, ch, key))
            continue;
        if (const auto done = applyKey(key))
            return *done;
        redrawEditLine();
    }
}

DatumPtr LogoInterfaceCurses::readchar()
{
    if (!cursesActive_)
        return nothing();
    for (;;)
    {
        if (atEnd())
            return nothing();

        if (!unread_.isEmpty())
        {
            return DatumPtr(readcharString(unread_.takeFirst()));
        }

        refresh();
        wint_t ch = 0;
        const int rc = readWideChar(ch);
        if (rc == ERR)
        {
            if (errno == EINTR)
            {
                retryAfterInterrupt();
                continue;
            }
            inputAtEof_ = true;
            return nothing();
        }
        char32_t ucs4 = 0;
        if (!mapGetWchToChar(rc, ch, ucs4))
            continue;
        return DatumPtr(readcharString(ucs4));
    }
}

int LogoInterfaceCurses::colorNumber(const QColor &color) const
{
    if (!color.isValid())
        return -1;
    if (COLORS >= 256)
        return rgbToXterm256(color);
    return rgbToAnsi8(color);
}

int LogoInterfaceCurses::pairLimit() const
{
    return qMin(static_cast<int>(COLOR_PAIRS), kPairNumberLimit);
}

short LogoInterfaceCurses::ensurePair(int fg)
{
    const auto it = pairByFg_.constFind(fg);
    if (it != pairByFg_.cend())
        return it.value();

    const int limit = pairLimit();
    const int usable = limit - 1; // pair 0 is reserved for the terminal default
    if (usable < 1)
        return 0;

    if (pairByFg_.size() >= usable)
    {
        // Every pair number is taken. Forget the mappings except the ones the
        // screen is still drawing with, so the current text and the window
        // background keep their colors. Cells painted earlier with any other
        // pair will take on whatever that pair is redefined to next.
        QHash<int, short> live;
        for (auto p = pairByFg_.cbegin(); p != pairByFg_.cend(); ++p)
        {
            if (p.value() == currentPair_ || p.value() == bgPair_)
                live.insert(p.key(), p.value());
        }
        pairByFg_ = live;
        if (pairByFg_.size() >= usable)
            return 0; // degenerate terminal: nothing free even after the purge
    }

    // Next number not held by a live mapping. Terminates because the table
    // holds fewer than `usable` entries.
    short pair = 0;
    for (;;)
    {
        pair = static_cast<short>(nextPair_);
        nextPair_ = nextPair_ + 1 < limit ? nextPair_ + 1 : 1;
        if (std::find(pairByFg_.cbegin(), pairByFg_.cend(), pair) == pairByFg_.cend())
            break;
    }

    init_pair(pair, static_cast<short>(fg), static_cast<short>(bgNum_));
    pairByFg_.insert(fg, pair);
    return pair;
}

void LogoInterfaceCurses::applyCurrentPair()
{
    if (!cursesActive_ || !has_colors())
        return;
    wcolor_set(stdscr, currentPair_, nullptr);
}

void LogoInterfaceCurses::applyStandoutAttr()
{
    if (!cursesActive_)
        return;
    if (standoutOn_)
        attron(A_REVERSE);
    else
        attroff(A_REVERSE);
}

void LogoInterfaceCurses::rebindPairsToBackground(int newBg)
{
    // Pairs are keyed by foreground, so the table itself does not change;
    // text already on screen picks up the new background through its pair.
    for (auto it = pairByFg_.cbegin(); it != pairByFg_.cend(); ++it)
        init_pair(it.value(), static_cast<short>(it.key()), static_cast<short>(newBg));
}

void LogoInterfaceCurses::recolorDefaultCells(int newBg)
{
    if (!cursesActive_)
        return;

    int y0 = 0;
    int x0 = 0;
    getyx(stdscr, y0, x0);
    int maxy = 0;
    int maxx = 0;
    getmaxyx(stdscr, maxy, maxx);
    Q_ASSERT(newBg == bgNum_);
    const short newPair = ensurePair(-1);

    // Re-adding the bottom-right cell would otherwise scroll the whole screen.
    scrollok(stdscr, FALSE);

    for (int y = 0; y < maxy; ++y)
    {
        int width = 1;
        for (int x = 0; x < maxx; x += width)
        {
            width = 1;
            cchar_t cc;
            std::memset(&cc, 0, sizeof(cc));
            if (mvwin_wch(stdscr, y, x, &cc) == ERR)
                continue;
            attr_t attrs = 0;
            short pair = 0;
            wchar_t wch[CCHARW_MAX + 1] = {};
            if (getcchar(&cc, wch, &attrs, &pair, nullptr) == ERR)
                continue;
            // An empty cell, or one whose first character is non-spacing, has
            // nothing that can be re-added on its own.
            if (wch[0] == 0)
                continue;
            const int glyphWidth = wcwidth(wch[0]);
            if (glyphWidth == 0)
                continue;
            // A double-width glyph reads back identically from both of its
            // cells. Re-add it once, from its first cell, and step over the
            // continuation cell; adding it there again would shift it right
            // and overwrite the character that follows.
            if (glyphWidth > 1)
                width = glyphWidth;
            if (pair != 0)
                continue;
            if (setcchar(&cc, wch, attrs, newPair, nullptr) == ERR)
                continue;
            mvwadd_wch(stdscr, y, x, &cc);
        }
    }

    scrollok(stdscr, TRUE);
    wmove(stdscr, y0, x0);
}

void LogoInterfaceCurses::setTextColor(const QColor &foregroundColor, const QColor &backgroundColor)
{
    if (!cursesActive_)
        return;

    foregroundColor_ = foregroundColor;
    fgNum_ = colorNumber(foregroundColor);

    if (backgroundColor.isValid())
    {
        backgroundColor_ = backgroundColor;
        bgNum_ = colorNumber(backgroundColor);
        if (has_colors())
        {
            rebindPairsToBackground(bgNum_);
            bgPair_ = ensurePair(-1);
            recolorDefaultCells(bgNum_);
            bkgdset(COLOR_PAIR(bgPair_) | static_cast<chtype>(' '));
        }
    }

    if (has_colors())
    {
        currentPair_ = ensurePair(fgNum_);
        applyCurrentPair();
        applyStandoutAttr();
        refresh();
    }
}

void LogoInterfaceCurses::mwait(unsigned long msecs)
{
    if (cursesActive_)
        refresh();
    LogoInterface::mwait(msecs);
}
