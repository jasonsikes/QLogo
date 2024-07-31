
//===-- qlogo/mainwindow.cpp - MainWindow class implementation --*- C++ -*-===//
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
/// This file contains the implementation of the MainWindow class, which is the
/// main window portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include "gui/mainwindow.h"
#include "gui/canvas.h"
#include "gui/editorwindow.h"
#include "sharedconstants.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDir>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QMessageBox>
#include <QScrollBar>
#include <QThread>
#include <QTimer>
#include <QFileDialog>

/// This function is a wrapper around QProcess::write() to send a message to the
/// QLogo process. It takes a function that writes the message to a QDataStream
/// object and sends it to the QLogo process.
///
/// @param func A function that writes the message to a QDataStream.
void MainWindow::sendMessage(std::function<void(QDataStream *)> func)
{
    qint64 datawritten;
    QByteArray buffer;
    QDataStream bufferStream(&buffer, QIODevice::WriteOnly);
    func(&bufferStream);
    qint64 datalen = buffer.size();
    buffer.prepend((const char *)&datalen, sizeof(qint64));
    datawritten = logoProcess->write(buffer);
    Q_ASSERT(datawritten == buffer.size());
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

QString MainWindow::findQlogoExe()
{
    // Windows executables have 'exe' extension.
#ifdef WIN32
    QString filename("qlogo.exe");
#else
    QString filename("qlogo");
#endif

    // Build a list of candidate locations to try.
    QStringList candidates;

    // The qlogo directory relative to wherever the app binary is.
    candidates << QCoreApplication::applicationDirPath() + QDir::separator() + ".." + QDir::separator() + "qlogo" +
                      QDir::separator() + filename;
    // The same directory as the app binary.
    candidates << QCoreApplication::applicationDirPath() + QDir::separator() + filename;

    for (auto &c : candidates)
    {
        // qDebug() << "Checking: " << c;
        if (QFileInfo::exists(c))
            return c;
    }

    // TODO: How do we handle this gracefully?
    return QString();
}

int MainWindow::startLogo()
{
    QString command = findQlogoExe();

    QStringList arguments;
    arguments << "--QLogoGUI";

    logoProcess = new QProcess(this);

    // TODO: maybe call setWorkingDirectory()

    connect(logoProcess, &QProcess::started, this, &MainWindow::processStarted);

    connect(
        logoProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::processFinished);

    connect(logoProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::readStandardOutput);

    connect(logoProcess, &QProcess::readyReadStandardError, this, &MainWindow::readStandardError);

    connect(logoProcess, &QProcess::errorOccurred, this, &MainWindow::errorOccurred);

    connect(ui->mainConsole, &Console::sendRawlineSignal, this, &MainWindow::sendRawlineSlot);

    connect(ui->mainConsole, &Console::sendCharSignal, this, &MainWindow::sendCharSlot);

    connect(ui->splitter, &QSplitter::splitterMoved, this, &MainWindow::splitterHasMovedSlot);

    connect(ui->mainCanvas, &Canvas::sendMouseclickedSignal, this, &MainWindow::mouseclickedSlot);

    connect(ui->mainCanvas, &Canvas::sendMousemovedSignal, this, &MainWindow::mousemovedSlot);

    connect(ui->mainCanvas, &Canvas::sendMouseReleasedSignal, this, &MainWindow::mousereleasedSlot);

    logoProcess->start(command, arguments);
    return 0;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qint64 pid = logoProcess->processId();
    // Tell the process to die, then ignore.
    // Because when the process dies another signal will be sent to close the application.
    if (pid > 0)
    {
        sendMessage([&](QDataStream *out) { *out << (message_t)S_SYSTEM; });
        logoProcess->closeWriteChannel();

        event->ignore();
    }
    else
    {
        event->accept();
    }
}

void MainWindow::initialize()
{
    QFont defaultFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->mainConsole->setTextFontSize(defaultFont.pointSizeF());
    ui->mainConsole->setTextFontName(defaultFont.family());
    ui->mainCanvas->setLabelFontSize(defaultFont.pointSizeF());
    ui->mainCanvas->setLabelFontName(defaultFont.family());
    setSplitterforMode(initScreenMode);

    sendMessage([&](QDataStream *out) {
        *out << (message_t)W_INITIALIZE << QFontDatabase::families() << defaultFont.family()
             << (double)defaultFont.pointSizeF();
    });
}

void MainWindow::fileDialogModal()
{
    QString startingDir = QDir::homePath();
    QString filePath = QFileDialog::getOpenFileName(this, tr("Choose file"), startingDir);
    sendMessage([&](QDataStream *out) {
        *out << (message_t)W_FILE_DIALOG_GET_PATH
             << filePath;
    });
}

void MainWindow::openEditorWindow(const QString startingText)
{
    if (editWindow == NULL)
    {
        editWindow = new EditorWindow;

        connect(editWindow, SIGNAL(editingHasEndedSignal(QString)), this, SLOT(editingHasEndedSlot(QString)));
    }

    editWindow->setTextFormat(ui->mainConsole->getFont());
    editWindow->setContents(startingText);
    editWindow->show();
    editWindow->activateWindow();
    editWindow->setFocus();
}

void MainWindow::editingHasEndedSlot(QString text)
{
    sendMessage([&](QDataStream *out) { *out << (message_t)C_CONSOLE_END_EDIT_TEXT << text; });
}

void MainWindow::introduceCanvas()
{
    if (hasShownCanvas)
        return;
    hasShownCanvas = true;
    setSplitterforMode(splitScreenMode);
}

void MainWindow::processStarted()
{
    qDebug() << "ProcessStarted()";
}

void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit)
    {
        QApplication::exit(0);
    }
    else
    {
        qDebug() << "processFinished()" << exitCode << exitStatus;
    }
}

// TODO: rename this. It sounds confusing.
void MainWindow::readStandardOutput()
{
    int readResult;

    // Continue to read data as long as data is available to read. Act on the data
    // as soon as it is available and read.
    do
    {
        qint64 datalen;
        message_t header;
        QByteArray buffer;
        QDataStream inDataStream;
        readResult = logoProcess->read((char *)&datalen, sizeof(qint64));
        if (readResult == 0)
            break;
        Q_ASSERT(readResult == sizeof(qint64));

        buffer.resize(datalen);
        readResult = logoProcess->read(buffer.data(), datalen);
        Q_ASSERT(readResult == (int)datalen);
        QDataStream *dataStream = new QDataStream(&buffer, QIODevice::ReadOnly);

        *dataStream >> header;
        switch (header)
        {
        case W_ZERO:
            // This only exists to help catch errors.
            qDebug() << "Zero!";
            break;
        case W_INITIALIZE:
        {
            initialize();
            break;
        }
        case W_CLOSE_PIPE:
        {
            logoProcess->closeWriteChannel();
            break;
        }
        case W_SET_SCREENMODE:
        {
            ScreenModeEnum newMode;
            *dataStream >> newMode;
            setSplitterforMode(newMode);
            break;
        }
        case W_FILE_DIALOG_GET_PATH:
        {
            fileDialogModal();
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
            qreal aSize;
            *dataStream >> aSize;
            ui->mainConsole->setTextFontSize(aSize);
            break;
        }
        case C_CONSOLE_REQUEST_LINE:
        {
            QString prompt;
            *dataStream >> prompt;
            beginReadRawlineWithPrompt(prompt);
            break;
        }
        case C_CONSOLE_REQUEST_CHAR:
            beginReadChar();
            break;
        case C_CONSOLE_BEGIN_EDIT_TEXT:
        {
            QString startingText;
            *dataStream >> startingText;
            openEditorWindow(startingText);
            break;
        }
        case C_CONSOLE_TEXT_CURSOR_POS:
        {
            sendConsoleCursorPosition();
            break;
        }
        case C_CONSOLE_SET_TEXT_CURSOR_POS:
        {
            int row, col;
            *dataStream >> row >> col;
            ui->mainConsole->setTextCursorPosition(row, col);
            break;
        }
        case C_CONSOLE_SET_CURSOR_MODE:
        {
            bool mode;
            *dataStream >> mode;
            ui->mainConsole->setOverwriteMode(mode);
            break;
        }
        case C_CONSOLE_SET_TEXT_COLOR:
        {
            QColor foreground;
            QColor background;
            *dataStream >> foreground >> background;
            ui->mainConsole->setTextFontColor(foreground, background);
            break;
        }
        case C_CONSOLE_CLEAR_SCREEN_TEXT:
            ui->mainConsole->setPlainText("");
            break;
        case C_CANVAS_UPDATE_TURTLE_POS:
        {
            QTransform matrix;
            *dataStream >> matrix;
            ui->mainCanvas->setTurtleMatrix(matrix);
            introduceCanvas();
            break;
        }
        case C_CANVAS_SET_TURTLE_IS_VISIBLE:
        {
            bool isVisible;
            *dataStream >> isVisible;
            ui->mainCanvas->setTurtleIsVisible(isVisible);
            introduceCanvas();
            break;
        }
        case C_CANVAS_EMIT_VERTEX:
        {
            ui->mainCanvas->emitVertex();
            introduceCanvas();
            break;
        }
        case C_CANVAS_SET_FOREGROUND_COLOR:
        {
            QColor color;
            *dataStream >> color;
            ui->mainCanvas->setForegroundColor(color);
            introduceCanvas();
            break;
        }
        case C_CANVAS_SET_BACKGROUND_COLOR:
        {
            QColor color;
            *dataStream >> color;
            ui->mainCanvas->setBackgroundColor(color);
            introduceCanvas();
            break;
        }
        case C_CANVAS_SET_BACKGROUND_IMAGE:
        {
            QImage image;
            *dataStream >> image;
            ui->mainCanvas->setBackgroundImage(image);
            introduceCanvas();
            break;
        }
        case C_CANVAS_BEGIN_POLYGON:
        {
            QColor color;
            *dataStream >> color;
            ui->mainCanvas->beginPolygon(color);
            break;
        }
        case C_CANVAS_END_POLYGON:
        {
            ui->mainCanvas->endPolygon();
            break;
        }
        case C_CANVAS_CLEAR_SCREEN:
            ui->mainCanvas->clearScreen();
            introduceCanvas();
            break;
        case C_CANVAS_SETBOUNDS:
        {
            qreal x, y;
            *dataStream >> x >> y;
            ui->mainCanvas->setBounds(x, y);
            break;
        }
        case C_CANVAS_SET_IS_BOUNDED:
        {
            bool isBounded;
            *dataStream >> isBounded;
            ui->mainCanvas->setIsBounded(isBounded);
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
            qreal aSize;
            *dataStream >> aSize;
            ui->mainCanvas->setLabelFontSize(aSize);
            break;
        }
        case C_CANVAS_DRAW_LABEL:
        {
            QString aString;
            *dataStream >> aString;
            ui->mainCanvas->addLabel(aString);
            introduceCanvas();
            break;
        }
        case C_CANVAS_DRAW_ARC:
        {
            qreal angle;
            qreal radius;
            *dataStream >> angle >> radius;
            ui->mainCanvas->addArc(angle, radius);
            introduceCanvas();
            break;
        }
        case C_CANVAS_SET_PENSIZE:
        {
            qreal newSize;
            *dataStream >> newSize;
            ui->mainCanvas->setPensize(newSize);
            break;
        }
        case C_CANVAS_SET_PENMODE:
        {
            PenModeEnum newMode;
            *dataStream >> newMode;
            ui->mainCanvas->setPenmode(newMode);
            break;
        }
        case C_CANVAS_SET_PENUPDOWN:
        {
            bool penIsDown;
            *dataStream >> penIsDown;
            ui->mainCanvas->setPenIsDown(penIsDown);
            break;
        }
        case C_CANVAS_GET_IMAGE:
        {
            sendCanvasImage();
            break;
        }
        case C_CANVAS_GET_SVG:
        {
            sendCanvasSvg();
            break;
        }
        default:
            qDebug() << "was not expecting" << header;
            break;
        }
        delete dataStream;
    } while (1);
}

void MainWindow::setSplitterforMode(ScreenModeEnum mode)
{
    float canvasSize, consoleSize;
    switch (mode)
    {
    case initScreenMode:
        canvasSize = Config::get().initScreenSize;
        break;
    case textScreenMode:
        canvasSize = Config::get().textScreenSize;
        break;
    case fullScreenMode:
        canvasSize = Config::get().fullScreenSize;
        break;
    case splitScreenMode:
        canvasSize = Config::get().splitScreenSize;
        break;
    }
    QList<int> sizes = ui->splitter->sizes();
    float splitterSize = sizes[0] + sizes[1];
    canvasSize = canvasSize * splitterSize;
    consoleSize = splitterSize - canvasSize;
    ui->splitter->setSizes(QList<int>() << (int)canvasSize << (int)consoleSize);
}

void MainWindow::readStandardError()
{
    QByteArray ary = logoProcess->readAllStandardError();
    qDebug() << "stderr: " << QString(ary);
}

void MainWindow::errorOccurred(QProcess::ProcessError error)
{
    qDebug() << "Error occurred" << error;
}

void MainWindow::beginReadRawlineWithPrompt(const QString prompt)
{
    windowMode = windowMode_waitForRawline;
    ui->mainConsole->requestRawlineWithPrompt(prompt);
}

void MainWindow::beginReadChar()
{
    windowMode = windowMode_waitForChar;
    ui->mainConsole->requestChar();
}

void MainWindow::mouseclickedSlot(QPointF position, int buttonID)
{
    sendMessage([&](QDataStream *out) { *out << (message_t)C_CANVAS_MOUSE_BUTTON_DOWN << position << buttonID; });
}

void MainWindow::mousemovedSlot(QPointF position)
{
    sendMessage([&](QDataStream *out) { *out << (message_t)C_CANVAS_MOUSE_MOVED << position; });
}

void MainWindow::mousereleasedSlot()
{
    sendMessage([&](QDataStream *out) { *out << (message_t)C_CANVAS_MOUSE_BUTTON_UP; });
}

void MainWindow::sendCharSlot(QChar c)
{
    sendMessage([&](QDataStream *out) { *out << (message_t)C_CONSOLE_CHAR_READ << c; });
}

void MainWindow::sendRawlineSlot(const QString &line)
{
    sendMessage([&](QDataStream *out) { *out << (message_t)C_CONSOLE_RAWLINE_READ << line; });
}

void MainWindow::sendConsoleCursorPosition()
{
    int row = 0;
    int col = 0;
    ui->mainConsole->getCursorPos(row, col);
    sendMessage([&](QDataStream *out) { *out << (message_t)C_CONSOLE_TEXT_CURSOR_POS << row << col; });
}

void MainWindow::sendCanvasImage()
{
    QImage image(ui->mainCanvas->getImage());
    sendMessage([&](QDataStream *out) { *out << (message_t)C_CANVAS_GET_IMAGE << image; });
}

void MainWindow::sendCanvasSvg()
{
    QByteArray svg = ui->mainCanvas->getSvg();
    sendMessage([&](QDataStream *out) { *out << (message_t)C_CANVAS_GET_SVG << svg; });
}

void MainWindow::splitterHasMovedSlot(int, int)
{
    hasShownCanvas = true;
}
