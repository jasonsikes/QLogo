
//===-- qlogo/mainwindow.cpp - MainWindow class implementation -------*- C++
//-*-===//
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
#include <QThread>
#include <QFontDatabase>
#include <signal.h>

// Wrapper function for sending data to the logo interpreter
void MainWindow::sendMessage(std::function<void (QDataStream*)> func)
{
    qint64 datawritten;
    QByteArray buffer;
    QDataStream bufferStream(&buffer, QIODevice::WriteOnly);
    func(&bufferStream);
    qint64 datalen = buffer.size();
    buffer.prepend((const char*)&datalen, sizeof(qint64));
    datawritten = logoProcess->write(buffer);
    Q_ASSERT(datawritten == buffer.size());
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

  connect(ui->splitter, &QSplitter::splitterMoved,
          this, &MainWindow::splitterHasMovedSlot);

  logoProcess->start(command, arguments);
  return 0;
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    qint64 pid = logoProcess->processId();
    // Tell the process to die, then ignore.
    // Because when the process dies another signal will be sent to close the application.
    if (pid > 0) {
        kill(pid, SIGINT);
        event->ignore();
    } else {
        event->accept();
    }

}


void MainWindow::initialize()
{
    QList<int> sizes;
    sizes << 0 << 100;
    QFont defaultFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->mainConsole->setTextFontSize(defaultFont.pointSizeF());
    ui->mainConsole->setTextFontName(defaultFont.family());
    ui->mainCanvas->setLabelFontSize(defaultFont.pointSizeF());
    ui->mainCanvas->setLabelFontName(defaultFont.family());
    ui->splitter->setSizes(sizes);

    sendMessage([&](QDataStream *out) {
        *out
        << (message_t)W_INITIALIZE
        << QFontDatabase::families()
        << defaultFont.family()
        << (double)defaultFont.pointSizeF()
        << ui->mainCanvas->minimumPenSize()
        << ui->mainCanvas->maximumPenSize()
        << ui->mainCanvas->xbound()
        << ui->mainCanvas->ybound()
           ;
    });

}

void MainWindow::introduceCanvas() {
    if (hasShownCanvas)
        return;
    hasShownCanvas = true;
    QList<int>sizes;
    sizes << 75 << 25;
    ui->splitter->setSizes(sizes);
}


void MainWindow::processStarted()
{
  qDebug() <<"ProcessStarted()";
}


void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit) {
        QApplication::exit(0);
    } else {
        qDebug() <<"processFinished()" <<exitCode << exitStatus;
    }
}

// TODO: rename this. It sounds confusing.
void MainWindow::readStandardOutput()
{
    int readResult;
    do {
        qint64 datalen;
        message_t header;
        QByteArray buffer;
        QDataStream inDataStream;
             readResult = logoProcess->read((char*)&datalen, sizeof(qint64));
        if (readResult == 0) break;
        Q_ASSERT(readResult == sizeof(qint64));

        buffer.resize(datalen);
        readResult = logoProcess->read(buffer.data(), datalen);
        Q_ASSERT(readResult == (int)datalen);
        QDataStream *dataStream = new QDataStream(&buffer, QIODevice::ReadOnly);

        *dataStream >> header;
        switch(header)
        {
        case W_ZERO:
            // This only exists to help catch errors.
            qDebug() <<"Zero!";
            break;
        case W_INITIALIZE:
        {
            initialize();
            break;
        }
        case C_CONSOLE_PRINT_STRING:
        {
            QString text;
            *dataStream >> text;
            ui->mainConsole->printString(text);
            break;
        }
        case C_CONSOLE_SET_FONT_NAME:
        {
            QString name;
            *dataStream >> name;
            ui->mainConsole->setTextFontName(name);
            break;
        }
        case C_CONSOLE_SET_FONT_SIZE:
        {
            double aSize;
            *dataStream >> aSize;
            ui->mainConsole->setTextFontSize(aSize);
            break;
        }
        case C_CONSOLE_REQUEST_LINE:
            beginReadRawline();
            break;
        case C_CONSOLE_REQUEST_CHAR:
            beginReadChar();
            break;
        case C_CANVAS_UPDATE_TURTLE_POS:
            {
              QMatrix4x4 matrix;
              *dataStream >> matrix;
              ui->mainCanvas->setTurtleMatrix(matrix);
              introduceCanvas();
              break;
            }
          case C_CANVAS_DRAW_LINE:
            {
              QVector3D a, b;
              QColor color;
              *dataStream
                  >> a
                  >> b
                  >> color;
              ui->mainCanvas->addLine(a, b, color);
              introduceCanvas();
              break;
            }
        case C_CANVAS_DRAW_POLYGON:
        {
            QList<QVector3D> points;
            QList<QColor> colors;
            *dataStream
                    >> points
                    >> colors;
            ui->mainCanvas->addPolygon(points, colors);
            introduceCanvas();
            break;
        }
        case C_CANVAS_CLEAR_SCREEN:
            ui->mainCanvas->clearScreen();
            introduceCanvas();
            break;
        case C_CANVAS_SETBOUNDS:
        {
            double x,y;
            *dataStream
                    >> x
                    >> y;
            ui->mainCanvas->setBounds(x, y);
            break;
        }
        case C_CANVAS_SET_FONT_NAME:
        {
            QString name;
            *dataStream >> name;
            ui->mainCanvas->setLabelFontName(name);
            break;
        }
        case C_CANVAS_SET_FONT_SIZE:
        {
            double aSize;
            *dataStream >> aSize;
            ui->mainCanvas->setLabelFontSize(aSize);
            break;
        }
        case C_CANVAS_DRAW_LABEL:
        {
            QString aString;
            QVector3D aPosition;
            QColor aColor;
            *dataStream
                    >> aString
                    >> aPosition
                    >> aColor;
            ui->mainCanvas->addLabel(aString, aPosition, aColor);
            introduceCanvas();
            break;
        }
        case C_CANVAS_SET_BACKGROUND_COLOR:
        {
            QColor aColor;
            *dataStream
                    >> aColor;
            ui->mainCanvas->setBackgroundColor(aColor);
            introduceCanvas();
            break;
        }
        case C_CANVAS_SET_PENSIZE:
        {
            double newSize;
            *dataStream >> newSize;
            ui->mainCanvas->setPensize((GLfloat)newSize);
            break;
        }
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
    qDebug() <<"stderr: " <<QString(ary);
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


void MainWindow::splitterHasMovedSlot(int, int)
{
    hasShownCanvas = true;
}
