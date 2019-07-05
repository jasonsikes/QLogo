
//===-- qlogo/mainwindow.cpp - MainWindow class implementation -------*- C++
//-*-===//
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
/// This file contains the implementation of the MainWindow class, which is the
/// main window portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include "mainwindow.h"
#include "canvas.h"
#include "ui_mainwindow.h"
#include "message.h"
#include <QDebug>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTimer>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  // There seems to be a bug involving window Maximize with an OpenGL widget.
  // So disable Maximize.
  setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint |
                 Qt::WindowCloseButtonHint);

  windowMode = windowMode_noWait;
}

void MainWindow::show()
{
  QMainWindow::show();
  ui->mainConsole->setFocus();

  startLogo();
}

MainWindow::~MainWindow()
{
    delete ui;
}


int MainWindow::startLogo()
{
  QString command = "/home/jsikes/Projects/build-logo-Desktop-Debug/logo"; // On my Linux
  //QString command = "/Volumes/jsikes/bin/logo"; // on my Mac
  QStringList arguments;
  arguments << "--QLogoGUI";

  logoProcess = new QProcess(this);

  logoStream.setDevice(logoProcess);

  // TODO: maybe call setWorkingDirectory()

  connect(logoProcess, &QProcess::started,
          this, &MainWindow::processStarted);

  connect(logoProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this, &MainWindow::processFinished);

  connect(logoProcess, &QProcess::readyReadStandardOutput,
          this, &MainWindow::readStandardOutput);

  connect(logoProcess, &QProcess::readyReadStandardError,
          this, &MainWindow::readStandardError);

  connect(logoProcess, &QProcess::errorOccurred,
          this, &MainWindow::errorOccurred);

  connect(ui->mainConsole, &Console::sendRawlineSignal,
          this, &MainWindow::sendRawlineSlot);

  logoProcess->start(command, arguments);
  return 0;
}



void MainWindow::processStarted()
{
  qDebug() <<"ProcessStarted()";
}


void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  qDebug() <<"processFinished()" <<exitCode << exitStatus;

}


void MainWindow::readStandardOutput()
{
    do {
        message_t messageType;
        logoStream >> messageType;
        switch(messageType)
        {
        case C_CONSOLE_PRINT_STRING:
        {
            QString text;
            logoStream >> text;
            ui->mainConsole->printString(text);
            break;
        }
        case C_CONSOLE_REQUEST_LINE:
            beginReadRawline();
            break;
        default:
            break;

        }
    } while ( ! logoStream.atEnd());
}


void MainWindow::readStandardError()
{
  QByteArray ary = logoProcess->readAllStandardError();
  QMessageBox msgBox;
  msgBox.setText(ary);
  msgBox.exec();
}

void MainWindow::errorOccurred(QProcess::ProcessError error)
{
    qDebug() <<"Error occurred" <<error;
}


void MainWindow::beginReadRawline()
{
    windowMode = windowMode_waitForRawline;
    ui->mainConsole->requestRawline();
}


void MainWindow::sendRawlineSlot(const QString &line)
{
    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);
    out << (message_t)C_CONSOLE_RAWLINE_READ << line;
    logoStream << buffer;
}
