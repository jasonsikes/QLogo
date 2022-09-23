
//===-- qlogo/logo_controller.cpp - Controller class implementation -------*-
// C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// QLogo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with QLogo.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the Controller class, which
/// provides the I/O interfaces.
///
//===----------------------------------------------------------------------===//

#include "controller.h"

#include <QDebug>

#include <signal.h>

#include "kernel.h"

// For rand()
#include <stdlib.h>

#include <iostream>

#include <QFile>
#include <QTextStream>
#include <QApplication>

SignalsEnum_t lastSignal = noSignal;

#ifdef _WIN32

static void initSignals()
{
    //TODO: I need to find out how to handle keyboard interrupts in Windows
}

static void restoreSignals()
{

}

#else

// Not a part of Controller because we are handling interrupts
static void handle_signal(int sig)
{
    switch(sig) {
    case SIGINT:
        lastSignal = toplevelSignal;
    break;
    case SIGTSTP:
        lastSignal = pauseSignal;
    break;
    case SIGQUIT:
        lastSignal = systemSignal;
        break;
    default:
        qDebug() <<"Not expecting signal: " <<sig;
    }
}

static void initSignals()
{
    signal(SIGINT, handle_signal);  // TOPLEVEL
    signal(SIGTSTP, handle_signal); // PAUSE
    signal(SIGQUIT, handle_signal); // SYSTEM
}

static void restoreSignals()
{
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
}

#endif

Controller *_maincontroller = NULL;
qreal initialBoundXY = 150;

Controller *mainController() {
  Q_ASSERT(_maincontroller != NULL);
  return _maincontroller;
}

Controller::Controller(QObject *parent) : QObject(parent) {
    Q_ASSERT(_maincontroller == NULL);
    readStream = NULL;
    writeStream = NULL;
    dribbleStream = NULL;
    _maincontroller = this;
    kernel = new Kernel;
}

Controller::~Controller() {
    delete kernel;
    _maincontroller = NULL;
}

bool Controller::setDribble(const QString &filePath) {
  if (filePath == "") {
    if (dribbleStream) {
      QIODevice *file = dribbleStream->device();
      dribbleStream->flush();
      delete dribbleStream;
      file->close();
      delete file;
    }
    dribbleStream = NULL;
    return true;
  }
  QFile *file = new QFile(filePath);
  if (!file->open(QIODevice::Append))
    return false;

  dribbleStream = new QTextStream(file);
  return true;
}

bool Controller::isDribbling() { return dribbleStream != NULL; }

QString Controller::addStandoutToString(const QString &src) {
  QString retval = escapeString + src + escapeString;
  return retval;
}


SignalsEnum_t Controller::latestSignal()
{
    SignalsEnum_t retval = lastSignal;
    lastSignal = noSignal;
    return retval;
}


int Controller::run(void) {
  kernel->initLibrary();
  initialize();

  initSignals();

  bool shouldContinue = true;
  while (shouldContinue) {
    shouldContinue = kernel->getLineAndRunIt();
  }

  restoreSignals();

  return 0;
}

void Controller::systemStop()
{
    QApplication::quit();
}

