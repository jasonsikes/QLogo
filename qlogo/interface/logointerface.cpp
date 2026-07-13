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
#include "workspace/kernel.h"
#include <QCoreApplication>
#include <QFile>
#include <QIODevice>
#include <cstdio>
#include <csignal>

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
    dribbleStream_ = nullptr;
    Config::get().setMainLogoInterface(this);
}

LogoInterface::~LogoInterface()
{
    setDribble("");
    Config::get().setMainLogoInterface(nullptr);
}

void LogoInterface::printToConsole(const QString &s)
{
    const QByteArray utf8 = s.toUtf8();
    fwrite(utf8.constData(), 1, static_cast<size_t>(utf8.size()), stdout);
    fflush(stdout);
    if (dribbleStream_)
        *dribbleStream_ << s;
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

    if (dribbleStream_)
        *dribbleStream_ << retval << '\n';
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

bool LogoInterface::setDribble(const QString &filePath)
{
    if (filePath == "")
    {
        if (dribbleStream_)
        {
            QIODevice *file = dribbleStream_->device();
            dribbleStream_->flush();
            delete dribbleStream_;
            file->close();
            delete file;
        }
        dribbleStream_ = nullptr;
        return true;
    }
    auto *file = new QFile(filePath);
    if (!file->open(QIODevice::Append))
        return false;

    dribbleStream_ = new QTextStream(file);
    return true;
}

bool LogoInterface::isDribbling()
{
    return dribbleStream_ != nullptr;
}

SignalsEnum_t LogoInterface::latestSignal()
{
    SignalsEnum_t retval = lastSignal_;
    lastSignal_ = noSignal;
    return retval;
}


void LogoInterface::closeInterface()
{
    setDribble("");
}
