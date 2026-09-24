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
#include <QThread>
#include <clocale>
#include <cstdio>
#include <cstring>
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
    if (!cursesActive_ || atEnd())
        return false;

    nodelay(stdscr, TRUE);
    noecho();
    wint_t ch = 0;
    const int rc = get_wch(&ch);
    echo();
    nodelay(stdscr, FALSE);
    if (rc == ERR)
        return false;
    unget_wch(ch);
    return true;
}

QString LogoInterfaceCurses::inputRawlineWithPrompt(const QString &prompt)
{
    if (atEnd())
        return {};

    printToConsole(prompt);

    wint_t buffer[65536];
    const int rc = getn_wstr(buffer, 65535);
    if (rc == ERR)
    {
        inputAtEof_ = true;
        return {};
    }

    std::u32string u32;
    for (int i = 0; i < 65535 && buffer[i] != 0; ++i)
        u32.push_back(static_cast<char32_t>(buffer[i]));
    return QString::fromStdU32String(u32);
}

DatumPtr LogoInterfaceCurses::readchar()
{
    if (!cursesActive_ || atEnd())
        return nothing();

    refresh();
    noecho();
    wint_t ch = 0;
    const int rc = get_wch(&ch);
    echo();
    if (rc == ERR)
    {
        inputAtEof_ = true;
        return nothing();
    }
    if (rc == KEY_CODE_YES)
        return nothing();
    char32_t ucs4 = static_cast<char32_t>(ch);
    return DatumPtr(QString::fromUcs4(&ucs4, 1));
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
    QThread::msleep(msecs);
}
