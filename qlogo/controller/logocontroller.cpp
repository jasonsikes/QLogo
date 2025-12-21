
//===-- qlogo/logocontroller.h - LogoController class definition -------*- C++ -*-===//
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
/// This file contains the definition of the LogoController class, which is responsible for
/// handling user interaction through standard input and output with no special control characters.
///
//===----------------------------------------------------------------------===//

#include "controller/logocontroller.h"
#include "kernel.h"
#include <QApplication>
#include <QIODevice>
#include <signal.h>
#include <QFile>
/// @brief The most recent signal that was received.
/// The value of this variable is set by the handle_signal function. When the latestSignal
/// method is called, the value is reset to noSignal.
SignalsEnum_t lastSignal = noSignal;

#ifdef _WIN32

static void initSignals()
{
    // TODO: I need to find out how to handle keyboard interrupts in Windows
}

static void restoreSignals()
{
}

#else

/// @brief Handles a signal.
/// @param sig The signal to handle.
/// The function sets lastSignal to the most recent signal that was received from the operating system.
/// The LogoController class can query the last signal and take appropriate action.
static void handle_signal(int sig)
{
    switch (sig)
    {
    case SIGINT:
        lastSignal = toplevelSignal; // Ctrl+C
        break;
    case SIGTSTP:
        lastSignal = pauseSignal; // Ctrl+Z
        break;
    case SIGQUIT:
        lastSignal = systemSignal; // Ctrl+[backslash]
        break;
    default:
        qWarning() << "Not expecting signal: " << sig;
    }
}

/// @brief Initializes the signal handler.
static void initSignals()
{
    signal(SIGINT, handle_signal);  // TOPLEVEL
    signal(SIGTSTP, handle_signal); // PAUSE
    signal(SIGQUIT, handle_signal); // SYSTEM
}

/// @brief Restores the default signal handlers.
static void restoreSignals()
{
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
}

#endif

LogoController::LogoController(QObject *parent)
{
    dribbleStream = nullptr;
    Config::get().setMainLogoController(this);
    kernel = new Kernel;

    inStream = new QTextStream(stdin, QIODevice::ReadOnly);
    outStream = new QTextStream(stdout, QIODevice::WriteOnly);
}

LogoController::~LogoController()
{
    setDribble("");
    delete inStream;
    delete outStream;
    delete kernel;
    Config::get().setMainLogoController(nullptr);
}

void LogoController::printToConsole(const QString &s)
{
    *outStream << s;
    if (dribbleStream)
        *dribbleStream << s;
}

bool LogoController::atEnd()
{
    return inStream->atEnd();
}

bool LogoController::keyQueueHasChars()
{
    return !inStream->atEnd();
}

// This is READRAWLINE
QString LogoController::inputRawlineWithPrompt(const QString &prompt)
{
    QString retval;
    if (!inStream->atEnd())
    {
        printToConsole(prompt);
        outStream->flush();
        retval = inStream->readLine();
        if (dribbleStream)
            *dribbleStream << retval << '\n';
    }
    return retval;
}

// This is READCHAR
DatumPtr LogoController::readchar()
{
    QChar c;
    outStream->flush();
    if (inStream->atEnd())
        return nothing;
    *inStream >> c;
    QString retval = c;
    DatumPtr retvalP = DatumPtr(retval);
    return retvalP;
}

void LogoController::mwait(unsigned long msecs)
{
    outStream->flush();
    QThread::msleep(msecs);
}

bool LogoController::setDribble(const QString &filePath)
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
    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::Append))
        return false;

    dribbleStream = new QTextStream(file);
    return true;
}

bool LogoController::isDribbling()
{
    return dribbleStream != nullptr;
}

SignalsEnum_t LogoController::latestSignal()
{
    SignalsEnum_t retval = lastSignal;
    lastSignal = noSignal;
    return retval;
}

int LogoController::run(void)
{
    initialize();

    initSignals();

    Config::get().mainKernel()->readEvalPrintLoop(false);

    restoreSignals();

    return 0;
}

void LogoController::systemStop()
{
    QApplication::quit();
}
