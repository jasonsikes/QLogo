
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

volatile SignalsEnum_t LogoInterface::lastSignal = noSignal;

/// @brief Handles a signal.
/// @param sig The signal to handle.
/// The function sets lastSignal to the most recent signal that was received from the operating system.
/// The LogoInterface class can query the last signal and take appropriate action.
static void handle_signal(int sig)
{
    switch (sig)
    {
    case SIGINT:
        LogoInterface::lastSignal = toplevelSignal; // Ctrl+C
        break;
    case SIGTSTP:
        LogoInterface::lastSignal = pauseSignal; // Ctrl+Z
        break;
    case SIGQUIT:
        LogoInterface::lastSignal = systemSignal; // Ctrl+[backslash]
        break;
    default:
        qWarning() << "Not expecting signal: " << sig;
    }
}

#ifdef _WIN32

void LogoInterface::initSignals()
{
    // TODO: I need to find out how to handle keyboard interrupts in Windows
}

void LogoInterface::restoreSignals()
{
}

#else

void LogoInterface::initSignals()
{
    signal(SIGINT, handle_signal);  // TOPLEVEL
    signal(SIGTSTP, handle_signal); // PAUSE
    signal(SIGQUIT, handle_signal); // SYSTEM
}

void LogoInterface::restoreSignals()
{
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
}

#endif

LogoInterface::LogoInterface(QObject *parent)
    : inStream(stdin, QIODevice::ReadOnly),
      outStream(stdout, QIODevice::WriteOnly)
{
    dribbleStream = nullptr;
    Config::get().setMainLogoInterface(this);
}

LogoInterface::~LogoInterface()
{
    setDribble("");
    Config::get().setMainLogoInterface(nullptr);
}

void LogoInterface::printToConsole(const QString &s)
{
    outStream << s;
    if (dribbleStream)
        *dribbleStream << s;
}

bool LogoInterface::atEnd()
{
    return inStream.atEnd();
}

bool LogoInterface::keyQueueHasChars()
{
    return !inStream.atEnd();
}

// This is READRAWLINE
QString LogoInterface::inputRawlineWithPrompt(const QString &prompt)
{
    QString retval;
    if (!inStream.atEnd())
    {
        printToConsole(prompt);
        outStream.flush();
        retval = inStream.readLine();
        if (dribbleStream)
            *dribbleStream << retval << '\n';
    }
    return retval;
}

// This is READCHAR
DatumPtr LogoInterface::readchar()
{
    QChar c;
    outStream.flush();
    if (inStream.atEnd())
        return nothing();
    inStream >> c;
    QString retval = c;
    DatumPtr retvalP = DatumPtr(retval);
    return retvalP;
}

bool LogoInterface::setDribble(const QString &filePath)
{
    if (filePath == "")
    {
        if (dribbleStream)
        {
            QIODevice *file = dribbleStream->device();
            dribbleStream->flush();
            delete dribbleStream;
            file->close();
            delete file;
        }
        dribbleStream = nullptr;
        return true;
    }
    auto *file = new QFile(filePath);
    if (!file->open(QIODevice::Append))
        return false;

    dribbleStream = new QTextStream(file);
    return true;
}

bool LogoInterface::isDribbling()
{
    return dribbleStream != nullptr;
}

SignalsEnum_t LogoInterface::latestSignal()
{
    SignalsEnum_t retval = lastSignal;
    lastSignal = noSignal;
    return retval;
}


void LogoInterface::closeInterface()
{
    setDribble("");
}
