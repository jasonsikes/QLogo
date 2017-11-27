
//===-- qlogo/test_controller.cpp - Controller class implementation -------*-
// C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
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
/// This file contains the implementation of the Controller class, which is a
/// stand-in replacement controller class for testing.
///
//===----------------------------------------------------------------------===//

#include "test_controller.h"

#include <QDebug>

#include "kernel.h"

// For rand()
#include <stdlib.h>

#include <iostream>

#include <QFile>
#include <QTextStream>

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
  setDribble("");
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
  QString retval = QString("<b>") + src + "</b>";
  return retval;
}

void Controller::printToConsole(const QString &s) {
  if (writeStream == NULL) {
    *outStream << s;
    if (dribbleStream)
      *dribbleStream << s;
  } else {
    *writeStream << s;
  }
}

bool Controller::atEnd() { return inStream->atEnd(); }

bool Controller::keyQueueHasChars() { return !inStream->atEnd(); }

// This is READRAWLINE
DatumP Controller::readrawlineWithPrompt(const QString &) {
  QTextStream *stream = (readStream == NULL) ? inStream : readStream;
  if (stream->atEnd())
    return nothing;
  QString inputText = stream->readLine();
  DatumP retval = DatumP(new Word(inputText));

  return retval;
}

// This is READCHAR
DatumP Controller::readchar() {
  QChar c;
  QTextStream *stream = (readStream == NULL) ? inStream : readStream;
  if (stream->atEnd())
    return nothing;
  *stream >> c;
  DatumP retval = DatumP(new Word(c));
  return retval;
}

QString Controller::run(const QString &aInput) {
  QString input = aInput;
  QString output = "";

  inStream = new QTextStream(&input, QIODevice::ReadOnly);
  outStream = new QTextStream(&output, QIODevice::WriteOnly);

  bool shouldContinue = true;
  while (shouldContinue) {
    shouldContinue = kernel->getLineAndRunIt();
  }

  delete inStream;
  delete outStream;

  return output;
}

void Controller::mwait(unsigned long msecs) {
  outStream->flush();
  QThread::msleep(msecs);
}
