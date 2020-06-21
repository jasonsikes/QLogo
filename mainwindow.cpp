
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
#include "constants.h"
#include <QDebug>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTimer>
#include <QMessageBox>
#include <QDir>


// Wrapper function for sending data to the logo interpreter
void MainWindow::sendMessage(std::function<void (QDataStream*)> func)
{
    QByteArray buffer;
    QDataStream bufferStream(&buffer, QIODevice::WriteOnly);
    func(&bufferStream);
    quint32 datalen = buffer.size();
    logoProcess->write((const char*)&datalen, sizeof(quint32));
    logoProcess->write(buffer);
}


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
  QString command = QCoreApplication::applicationDirPath().append("/logo");
  QStringList arguments;
  arguments << "--QLogoGUI";

  logoProcess = new QProcess(this);

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

  connect(ui->mainConsole, &Console::sendCharSignal,
          this, &MainWindow::sendCharSlot);

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

// TODO: rename this. It sounds confusing.
void MainWindow::readStandardOutput()
{
    int readResult;
    do {
        quint32 datalen;
        message_t header;
        QByteArray buffer;
        QDataStream inDataStream;
        readResult = logoProcess->read((char*)&datalen, sizeof(quint32));
        if (readResult < 1) // TODO: We need better checks for input throughout this function.
            break;
        buffer.resize(datalen);
        logoProcess->read(buffer.data(), datalen);
        QDataStream *dataStream = new QDataStream(&buffer, QIODevice::ReadOnly);

        *dataStream >> header;
        switch(header)
        {
        case W_ZERO:
            qDebug() <<"Zero!";
            break;
        case C_CONSOLE_PRINT_STRING:
        {
            QString text;
            *dataStream >> text;
            ui->mainConsole->printString(text);
            break;
        }
        case C_CONSOLE_REQUEST_LINE:
            beginReadRawline();
            break;
        case C_CONSOLE_REQUEST_CHAR:
            beginReadChar();
            break;
        default:
            qDebug() <<"was not expecting" <<header;
            break;

        }
        delete dataStream;
    } while (1);
}


void MainWindow::readStandardError()
{
    QByteArray ary = logoProcess->readAllStandardError();
    qDebug() <<"stderr: " <<ary;
//  QMessageBox msgBox;
//  msgBox.setText(ary);
//  msgBox.exec();
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


void MainWindow::beginReadChar()
{
    windowMode = windowMode_waitForChar;
    ui->mainConsole->requestChar();
}


void MainWindow::sendCharSlot(QChar c)
{
    sendMessage([&](QDataStream *out) {
        *out << (message_t)C_CONSOLE_CHAR_READ << c;
    });
}


void MainWindow::sendRawlineSlot(const QString &line)
{
    sendMessage([&](QDataStream *out) {
        *out << (message_t)C_CONSOLE_RAWLINE_READ << line;
    });
}
