//===-- qlogo/logointerface.h - LogoInterface class definition -------*- C++ -*-===//
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
/// This file contains the definition of the LogoInterface class, which is responsible for
/// handling user interaction through standard input and output with no special control characters.
///
//===----------------------------------------------------------------------===//

#include "interface/logointerface.h"
#include "interface/pipeio.h"
#include "workspace/kernel.h"
#include <QCoreApplication>
#include <QFile>
#include <QIODevice>
#include <cstdio>
#include <csignal>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

volatile SignalsEnum_t LogoInterface::lastSignal_ = noSignal;

/// @brief Handles a signal.
/// @param sig The signal to handle.
/// The function sets lastSignal to the most recent signal that was received from the operating system.
/// The LogoInterface class can query the last signal and take appropriate action.
static void handle_signal(int sig)
{
    switch (sig)
    {
    case SIGINT:
        LogoInterface::lastSignal_ = toplevelSignal; // Ctrl+C
        break;
#ifndef _WIN32
    case SIGTSTP:
        LogoInterface::lastSignal_ = pauseSignal; // Ctrl+Z
        break;
    case SIGQUIT:
        LogoInterface::lastSignal_ = systemSignal; // Ctrl+[backslash]
        break;
#endif
    default:
        qWarning() << "Not expecting signal: " << sig;
    }
}

void LogoInterface::initSignals()
{
    signal(SIGINT, handle_signal); // TOPLEVEL
#ifndef _WIN32
    signal(SIGTSTP, handle_signal); // PAUSE
    signal(SIGQUIT, handle_signal); // SYSTEM
#endif
}

void LogoInterface::restoreSignals()
{
    signal(SIGINT, SIG_DFL);
#ifndef _WIN32
    signal(SIGTSTP, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
#endif
}

LogoInterface::LogoInterface(QObject *parent)
{
    Config::get().setMainLogoInterface(this);
}

LogoInterface::~LogoInterface()
{
    Config::get().setMainLogoInterface(nullptr);
}

void LogoInterface::printToConsole(const QString &s)
{
    const QByteArray utf8 = s.toUtf8();
    fwrite(utf8.constData(), 1, static_cast<size_t>(utf8.size()), stdout);
    fflush(stdout);
}

bool LogoInterface::atEnd()
{
    return inputAtEof_ || feof(stdin) != 0;
}

bool LogoInterface::keyQueueHasChars()
{
    return !atEnd();
}

// This is READRAWLINE
QString LogoInterface::inputRawlineWithPrompt(const QString &prompt)
{
    if (atEnd())
        return {};

    // feof() is only set after a failed read. For redirected stdin (console tests),
    // peek first so we do not print a trailing prompt after the last script line.
    // Interactive ttys still print the prompt before blocking on input.
#ifdef _WIN32
    const bool interactive = _isatty(stdinFd()) != 0;
#else
    const bool interactive = isatty(stdinFd()) != 0;
#endif
    if (!interactive)
    {
        const int ch = fgetc(stdin);
        if (ch == EOF)
        {
            inputAtEof_ = true;
            return {};
        }
        ungetc(ch, stdin);
    }

    printToConsole(prompt);

    char buffer[65536];
    if (!fgets(buffer, sizeof(buffer), stdin))
    {
        inputAtEof_ = true;
        return {};
    }

    QString retval = QString::fromLocal8Bit(buffer);
    if (retval.endsWith(QLatin1Char('\n')))
        retval.chop(1);
    if (retval.endsWith(QLatin1Char('\r')))
        retval.chop(1);

    return retval;
}

// This is READCHAR
DatumPtr LogoInterface::readchar()
{
    fflush(stdout);
    if (atEnd())
        return nothing();

    const int ch = fgetc(stdin);
    if (ch == EOF)
    {
        inputAtEof_ = true;
        return nothing();
    }
    return DatumPtr(QString(QChar(ch)));
}

SignalsEnum_t LogoInterface::latestSignal()
{
    SignalsEnum_t retval = lastSignal_;
    lastSignal_ = noSignal;
    return retval;
}


void LogoInterface::closeInterface()
{
}
