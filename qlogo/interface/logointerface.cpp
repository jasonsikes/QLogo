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
#include <QThread>
#include <cerrno>
#include <cstdio>
#include <csignal>

#ifdef _WIN32
#include <io.h>
#else
#include <ctime>
#include <poll.h>
#include <unistd.h>
#endif

volatile sig_atomic_t LogoInterface::lastSignal_ = noSignal;

/// @brief Handles a signal.
/// @param sig The signal to handle.
/// Stores lastSignal_. SYSTEM outranks TOPLEVEL, which outranks PAUSE. The
/// handler only writes a sig_atomic_t; it must not allocate, print, or throw.
static void handle_signal(int sig)
{
    sig_atomic_t incoming = noSignal;
    switch (sig)
    {
    case SIGINT:
        incoming = toplevelSignal;
        break;
#ifndef _WIN32
    case SIGTSTP:
        incoming = pauseSignal;
        break;
    case SIGQUIT:
        incoming = systemSignal;
        break;
#endif
    default:
        return;
    }
    const sig_atomic_t current = LogoInterface::lastSignal_;
    if (current == noSignal || incoming < current)
        LogoInterface::lastSignal_ = incoming;
}

#ifndef _WIN32
static void setSignalHandler(int sig, void (*handler)(int))
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGQUIT);
    sigaddset(&sa.sa_mask, SIGTSTP);
    sa.sa_flags = 0; // no SA_RESTART: blocking reads must fail with EINTR
    sa.sa_handler = handler;
    sigaction(sig, &sa, nullptr);
}
#endif

void LogoInterface::initSignals()
{
#ifdef _WIN32
    signal(SIGINT, handle_signal);
#else
    setSignalHandler(SIGINT, handle_signal);
    setSignalHandler(SIGTSTP, handle_signal);
    setSignalHandler(SIGQUIT, handle_signal);
#endif
}

void LogoInterface::restoreSignals()
{
#ifdef _WIN32
    signal(SIGINT, SIG_DFL);
#else
    setSignalHandler(SIGINT, SIG_DFL);
    setSignalHandler(SIGTSTP, SIG_DFL);
    setSignalHandler(SIGQUIT, SIG_DFL);
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

void LogoInterface::retryAfterInterrupt()
{
    DatumPtr result;
    if (Kernel::get().consumePendingSignal(&result) && result.isErr())
        throw result.errValue();
}

bool LogoInterface::waitForInteractiveInput()
{
#ifdef _WIN32
    return true;
#else
    for (;;)
    {
        DatumPtr result;
        if (Kernel::get().consumePendingSignal(&result) && result.isErr())
            throw result.errValue();
        pollfd pfd{};
        pfd.fd = stdinFd();
        pfd.events = POLLIN;
        errno = 0;
        const int prc = poll(&pfd, 1, 50);
        if (prc < 0)
        {
            if (errno == EINTR)
                retryAfterInterrupt();
            continue;
        }
        if (prc == 0)
            continue;
        if (pfd.revents & POLLIN)
            return true;
        return false;
    }
#endif
}

#ifdef _WIN32
static bool stdinIsInteractive()
{
    return _isatty(stdinFd()) != 0;
}
#else
static bool stdinIsInteractive()
{
    return isatty(stdinFd()) != 0;
}
#endif

// This is READRAWLINE
QString LogoInterface::inputRawlineWithPrompt(const QString &prompt)
{
    for (;;)
    {
        if (atEnd())
            return {};

        // feof() is only set after a failed read. For redirected stdin (console tests),
        // peek first so we do not print a trailing prompt after the last script line.
        // Interactive ttys still print the prompt before blocking on input.
        const bool interactive = stdinIsInteractive();
        if (!interactive)
        {
            for (;;)
            {
                errno = 0;
                const int ch = fgetc(stdin);
                if (ch != EOF)
                {
                    ungetc(ch, stdin);
                    break;
                }
                if (errno == EINTR)
                {
                    clearerr(stdin);
                    continue;
                }
                inputAtEof_ = true;
                return {};
            }
        }

        printToConsole(prompt);

        if (interactive && !waitForInteractiveInput())
        {
            inputAtEof_ = true;
            return {};
        }

        char buffer[65536];
        errno = 0;
        if (fgets(buffer, sizeof(buffer), stdin))
        {
            QString retval = QString::fromLocal8Bit(buffer);
            if (retval.endsWith(QLatin1Char('\n')))
                retval.chop(1);
            if (retval.endsWith(QLatin1Char('\r')))
                retval.chop(1);
            return retval;
        }
        if (errno == EINTR)
        {
            clearerr(stdin);
            if (interactive)
                retryAfterInterrupt();
            continue;
        }
        inputAtEof_ = true;
        return {};
    }
}

// This is READCHAR
DatumPtr LogoInterface::readchar()
{
    fflush(stdout);
    for (;;)
    {
        if (atEnd())
            return nothing();

        if (stdinIsInteractive() && !waitForInteractiveInput())
        {
            inputAtEof_ = true;
            return nothing();
        }

        errno = 0;
        const int ch = fgetc(stdin);
        if (ch != EOF)
            return DatumPtr(QString(QChar(ch)));
        if (errno == EINTR)
        {
            clearerr(stdin);
            if (stdinIsInteractive())
                retryAfterInterrupt();
            continue;
        }
        inputAtEof_ = true;
        return nothing();
    }
}

SignalsEnum_t LogoInterface::latestSignal()
{
#ifndef _WIN32
    sigset_t block;
    sigset_t old;
    sigemptyset(&block);
    sigaddset(&block, SIGINT);
    sigaddset(&block, SIGQUIT);
    sigaddset(&block, SIGTSTP);
    sigprocmask(SIG_BLOCK, &block, &old);
#endif
    const SignalsEnum_t retval = static_cast<SignalsEnum_t>(lastSignal_);
    lastSignal_ = noSignal;
#ifndef _WIN32
    sigprocmask(SIG_SETMASK, &old, nullptr);
#endif
    return retval;
}


void LogoInterface::closeInterface()
{
}

void LogoInterface::mwait(unsigned long msecs)
{
    fflush(stdout);
    if (msecs == 0)
        return;
#ifdef _WIN32
    QThread::msleep(msecs);
#else
    unsigned long remaining = msecs;
    while (remaining > 0)
    {
        const unsigned long sliceMs = remaining < 50UL ? remaining : 50UL;
        timespec req{};
        req.tv_sec = static_cast<time_t>(sliceMs / 1000);
        req.tv_nsec = static_cast<long>((sliceMs % 1000) * 1000000UL);
        nanosleep(&req, nullptr);
        remaining -= sliceMs;
        DatumPtr result;
        if (Kernel::get().consumePendingSignal(&result) && result.isErr())
            throw result.errValue();
    }
#endif
}
