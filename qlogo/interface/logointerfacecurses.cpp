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
#include <cerrno>
#include <clocale>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>

#ifndef NCURSES_WIDECHAR
#define NCURSES_WIDECHAR 1
#endif
#include <ncurses.h>

namespace
{
quint64 packPairKey(int fg, int bg)
{
    return (static_cast<quint64>(static_cast<quint32>(fg)) << 32) | static_cast<quint32>(bg);
}

int unpackFg(quint64 key)
{
    return static_cast<int>(static_cast<qint32>(key >> 32));
}

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

/// keypad on and noecho for one prompt so the line editor can paint the
/// buffer. Restores the idle settings from initialize() (echo, keypad off)
/// on return, EOF, or an interrupt THROW.
class DrawLineMode
{
    bool active_;
    bool prevKeypad_;

  public:
    explicit DrawLineMode(bool cursesActive) : active_(cursesActive), prevKeypad_(false)
    {
        if (!active_)
            return;
        prevKeypad_ = is_keypad(stdscr);
        keypad(stdscr, TRUE);
        noecho();
        nodelay(stdscr, FALSE);
    }

    ~DrawLineMode()
    {
        if (!active_)
            return;
        keypad(stdscr, prevKeypad_ ? TRUE : FALSE);
        echo();
    }

    DrawLineMode(const DrawLineMode &) = delete;
    DrawLineMode &operator=(const DrawLineMode &) = delete;
};

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

void LogoInterfaceCurses::initialize()
{
    if (cursesActive_)
        return;

    if (!stdioIsInteractive())
    {
        fprintf(stderr, "qlogo: --tui requires an interactive terminal\n");
        Config::get().setMainLogoInterface(nullptr);
        exit(1);
    }

    setlocale(LC_CTYPE, "");

    if (initscr() == nullptr)
    {
        fprintf(stderr, "qlogo: failed to initialize ncurses\n");
        Config::get().setMainLogoInterface(nullptr);
        exit(1);
    }

    cursesActive_ = true;
    cbreak();
    echo();
    keypad(stdscr, FALSE);
    scrollok(stdscr, TRUE);
    idlok(stdscr, TRUE);
    if (has_colors())
    {
        start_color();
        use_default_colors();
    }
    refresh();
}

void LogoInterfaceCurses::closeInterface()
{
    if (!cursesActive_)
        return;
    endwin();
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

bool LogoInterfaceCurses::keyQueueHasChars()
{
    if (!unread_.isEmpty())
        return true;
    if (!cursesActive_ || atEnd())
        return false;

    nodelay(stdscr, TRUE);
    noecho();
    for (;;)
    {
        wint_t ch = 0;
        const int rc = get_wch(&ch);
        if (rc == ERR)
        {
            echo();
            nodelay(stdscr, FALSE);
            return false;
        }
        if (rc != KEY_CODE_YES)
        {
            unread_.append(static_cast<char32_t>(ch));
            echo();
            nodelay(stdscr, FALSE);
            return true;
        }
    }
}

bool LogoInterfaceCurses::lineQueueHasLine()
{
    if (!cursesActive_ || atEnd())
        return false;

    nodelay(stdscr, TRUE);
    noecho();
    for (;;)
    {
        wint_t ch = 0;
        const int rc = get_wch(&ch);
        if (rc == ERR)
            break;
        if (rc == KEY_CODE_YES)
            continue;
        unread_.append(static_cast<char32_t>(ch));
        if (ch == L'\n' || ch == L'\r')
            break;
    }
    echo();
    nodelay(stdscr, FALSE);

    for (char32_t ch : unread_)
    {
        if (ch == U'\n' || ch == U'\r')
            return true;
    }
    return false;
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

QString LogoInterfaceCurses::inputRawlineWithPrompt(const QString &prompt)
{
    DrawLineMode drawMode(cursesActive_);
    if (atEnd())
        return {};

    printToConsole(prompt);

    int startY = 0;
    int startX = 0;
    getyx(stdscr, startY, startX);
    lineEditor_.beginPrompt(startY, startX);
    editDrawEndY_ = lineEditor_.originRow();

    while (!unread_.isEmpty())
    {
        const char32_t ch = unread_.takeFirst();
        if (ch == U'\n' || ch == U'\r')
            return lineEditor_.finish();
        lineEditor_.insert(ch);
    }

    redrawEditLine();

    auto acceptLine = [this]() {
        lineEditor_.setCursor(lineEditor_.text().size());
        redrawEditLine();
        paintWString(QStringLiteral("\n"));
        refresh();
        return lineEditor_.finish();
    };

    for (;;)
    {
        if (atEnd())
            return {};

        wint_t ch = 0;
        errno = 0;
        const int rc = get_wch(&ch);
        if (rc == ERR)
        {
            if (errno == EINTR)
            {
                retryAfterInterrupt();
                redrawEditLine();
                continue;
            }
            inputAtEof_ = true;
            return {};
        }
        if (rc == KEY_CODE_YES)
        {
            if (ch == KEY_ENTER)
                return acceptLine();
            if (ch == KEY_LEFT)
            {
                lineEditor_.moveLeft();
                redrawEditLine();
                continue;
            }
            if (ch == KEY_RIGHT)
            {
                lineEditor_.moveRight();
                redrawEditLine();
                continue;
            }
            if (ch == KEY_HOME)
            {
                lineEditor_.moveHome();
                redrawEditLine();
                continue;
            }
            if (ch == KEY_END)
            {
                lineEditor_.moveEnd();
                redrawEditLine();
                continue;
            }
            if (ch == KEY_UP)
            {
                if (lineEditor_.recallOlder())
                    redrawEditLine();
                continue;
            }
            if (ch == KEY_DOWN)
            {
                if (lineEditor_.recallNewer())
                    redrawEditLine();
                continue;
            }
            if (ch == KEY_BACKSPACE)
            {
                lineEditor_.eraseLeft();
                redrawEditLine();
                continue;
            }
            if (ch == KEY_DC)
            {
                lineEditor_.eraseAtCursor();
                redrawEditLine();
                continue;
            }
            if (ch == KEY_RESIZE)
            {
                redrawEditLine();
                continue;
            }
            continue;
        }
        if (ch == L'\n' || ch == L'\r')
            return acceptLine();
        const char32_t ucs4 = static_cast<char32_t>(ch);
        if (ucs4 == 8U || ucs4 == 127U)
        {
            lineEditor_.eraseLeft();
            redrawEditLine();
            continue;
        }
        if (ucs4 == 1U)
        {
            lineEditor_.moveHome();
            redrawEditLine();
            continue;
        }
        if (ucs4 == 5U)
        {
            lineEditor_.moveEnd();
            redrawEditLine();
            continue;
        }
        if (ucs4 == 11U)
        {
            lineEditor_.killToEnd();
            redrawEditLine();
            continue;
        }
        if (ucs4 == 21U)
        {
            lineEditor_.killLine();
            redrawEditLine();
            continue;
        }
        if (ucs4 == 23U)
        {
            lineEditor_.killWord();
            redrawEditLine();
            continue;
        }
        if (ucs4 < 32U)
            continue;
        lineEditor_.insert(ucs4);
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
            const char32_t ucs4 = unread_.takeFirst();
            return DatumPtr(QString::fromUcs4(&ucs4, 1));
        }

        refresh();
        noecho();
        wint_t ch = 0;
        errno = 0;
        const int rc = get_wch(&ch);
        echo();
        if (rc != ERR)
        {
            if (rc == KEY_CODE_YES)
                return nothing();
            char32_t ucs4 = static_cast<char32_t>(ch);
            return DatumPtr(QString::fromUcs4(&ucs4, 1));
        }
        if (errno == EINTR)
        {
            retryAfterInterrupt();
            continue;
        }
        inputAtEof_ = true;
        return nothing();
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

void LogoInterfaceCurses::initColorPair(short pair, int fg, int bg)
{
#if defined(NCURSES_EXT_COLORS) && NCURSES_EXT_COLORS
    init_extended_pair(pair, fg, bg);
#else
    init_pair(pair, static_cast<short>(fg), static_cast<short>(bg));
#endif
}

short LogoInterfaceCurses::ensurePair(int fg, int bg)
{
    const quint64 key = packPairKey(fg, bg);
    const auto it = pairByFgBg_.constFind(key);
    if (it != pairByFgBg_.cend())
        return it.value();

    if (nextPair_ >= COLOR_PAIRS)
    {
        nextPair_ = 1;
        pairByFgBg_.clear();
    }
    const short pair = static_cast<short>(nextPair_++);
    initColorPair(pair, fg, bg);
    pairByFgBg_.insert(key, pair);
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
    QHash<quint64, short> rebound;
    rebound.reserve(pairByFgBg_.size());
    for (auto it = pairByFgBg_.cbegin(); it != pairByFgBg_.cend(); ++it)
    {
        const short pair = it.value();
        const int fg = unpackFg(it.key());
        initColorPair(pair, fg, newBg);
        rebound.insert(packPairKey(fg, newBg), pair);
    }
    pairByFgBg_ = rebound;
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
    const short newPair = ensurePair(-1, newBg);

    for (int y = 0; y < maxy; ++y)
    {
        for (int x = 0; x < maxx; ++x)
        {
            cchar_t cc;
            std::memset(&cc, 0, sizeof(cc));
            if (mvwin_wch(stdscr, y, x, &cc) == ERR)
                continue;
            attr_t attrs = 0;
            short pair = 0;
            wchar_t wch[CCHARW_MAX + 1] = {};
            if (getcchar(&cc, wch, &attrs, &pair, nullptr) == ERR)
                continue;
            if (pair != 0)
                continue;
            if (setcchar(&cc, wch, attrs, newPair, nullptr) == ERR)
                continue;
            mvwadd_wch(stdscr, y, x, &cc);
        }
    }
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
            recolorDefaultCells(bgNum_);
            bkgdset(COLOR_PAIR(ensurePair(-1, bgNum_)) | static_cast<chtype>(' '));
        }
    }

    if (has_colors())
    {
        currentPair_ = ensurePair(fgNum_, bgNum_);
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
