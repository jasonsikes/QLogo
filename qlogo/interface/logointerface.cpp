
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
#include <QApplication>
#include <QFile>
#include <QIODevice>
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
    : inStream_(stdin, QIODevice::ReadOnly),
      outStream_(stdout, QIODevice::WriteOnly)
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
    outStream_ << s;
    if (dribbleStream_)
        *dribbleStream_ << s;
}

bool LogoInterface::atEnd()
{
    return inStream_.atEnd();
}

bool LogoInterface::keyQueueHasChars()
{
    return !inStream_.atEnd();
}

// This is READRAWLINE
QString LogoInterface::inputRawlineWithPrompt(const QString &prompt)
{
    QString retval;
    if (!inStream_.atEnd())
    {
        printToConsole(prompt);
        outStream_.flush();
        retval = inStream_.readLine();
        if (dribbleStream_)
            *dribbleStream_ << retval << '\n';
    }
    return retval;
}

// This is READCHAR
DatumPtr LogoInterface::readchar()
{
    QChar c;
    outStream_.flush();
    if (inStream_.atEnd())
        return nothing();
    inStream_ >> c;
    QString retval = c;
    DatumPtr retvalP = DatumPtr(retval);
    return retvalP;
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
