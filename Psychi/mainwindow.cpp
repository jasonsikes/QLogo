
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
#include <QFileDialog>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <iostream>

// External declaration of logging flag from qlogo_main.cpp
extern bool logging;

/// @brief a pointer to the qlogo process.
static QProcess *logoProcess;

QProcess *ProcessMessageWriter::process = nullptr;

/// @brief Get the name of a message type for logging
static QString getMessageTypeName(message_t type)
{
    switch (type)
    {
    case W_ZERO:
        return "W_ZERO";
    case W_INITIALIZE:
        return "W_INITIALIZE";
    case W_CLOSE_PIPE:
        return "W_CLOSE_PIPE";
    case W_SET_SCREENMODE:
        return "W_SET_SCREENMODE";
    case W_FILE_DIALOG_GET_PATH:
        return "W_FILE_DIALOG_GET_PATH";
    case S_SYSTEM:
        return "S_SYSTEM";
    case S_TOPLEVEL:
        return "S_TOPLEVEL";
    case S_PAUSE:
        return "S_PAUSE";
    case C_CONSOLE_PRINT_STRING:
        return "C_CONSOLE_PRINT_STRING";
    case C_CONSOLE_REQUEST_LINE:
        return "C_CONSOLE_REQUEST_LINE";
    case C_CONSOLE_REQUEST_CHAR:
        return "C_CONSOLE_REQUEST_CHAR";
    case C_CONSOLE_RAWLINE_READ:
        return "C_CONSOLE_RAWLINE_READ";
    case C_CONSOLE_CHAR_READ:
        return "C_CONSOLE_CHAR_READ";
    case C_CONSOLE_SET_FONT_NAME:
        return "C_CONSOLE_SET_FONT_NAME";
    case C_CONSOLE_SET_FONT_SIZE:
        return "C_CONSOLE_SET_FONT_SIZE";
    case C_CONSOLE_BEGIN_EDIT_TEXT:
        return "C_CONSOLE_BEGIN_EDIT_TEXT";
    case C_CONSOLE_END_EDIT_TEXT:
        return "C_CONSOLE_END_EDIT_TEXT";
    case C_CONSOLE_TEXT_CURSOR_POS:
        return "C_CONSOLE_TEXT_CURSOR_POS";
    case C_CONSOLE_SET_TEXT_CURSOR_POS:
        return "C_CONSOLE_SET_TEXT_CURSOR_POS";
    case C_CONSOLE_SET_CURSOR_MODE:
        return "C_CONSOLE_SET_CURSOR_MODE";
    case C_CONSOLE_SET_TEXT_COLOR:
        return "C_CONSOLE_SET_TEXT_COLOR";
    case C_CONSOLE_CLEAR_SCREEN_TEXT:
        return "C_CONSOLE_CLEAR_SCREEN_TEXT";
    case C_CANVAS_UPDATE_TURTLE_POS:
        return "C_CANVAS_UPDATE_TURTLE_POS";
    case C_CANVAS_EMIT_VERTEX:
        return "C_CANVAS_EMIT_VERTEX";
    case C_CANVAS_SET_FOREGROUND_COLOR:
        return "C_CANVAS_SET_FOREGROUND_COLOR";
    case C_CANVAS_SET_BACKGROUND_COLOR:
        return "C_CANVAS_SET_BACKGROUND_COLOR";
    case C_CANVAS_SET_BACKGROUND_IMAGE:
        return "C_CANVAS_SET_BACKGROUND_IMAGE";
    case C_CANVAS_BEGIN_POLYGON:
        return "C_CANVAS_BEGIN_POLYGON";
    case C_CANVAS_END_POLYGON:
        return "C_CANVAS_END_POLYGON";
    case C_CANVAS_SET_TURTLE_IS_VISIBLE:
        return "C_CANVAS_SET_TURTLE_IS_VISIBLE";
    case C_CANVAS_DRAW_LABEL:
        return "C_CANVAS_DRAW_LABEL";
    case C_CANVAS_DRAW_ARC:
        return "C_CANVAS_DRAW_ARC";
    case C_CANVAS_CLEAR_SCREEN:
        return "C_CANVAS_CLEAR_SCREEN";
    case C_CANVAS_SETBOUNDS:
        return "C_CANVAS_SETBOUNDS";
    case C_CANVAS_SET_IS_BOUNDED:
        return "C_CANVAS_SET_IS_BOUNDED";
    case C_CANVAS_SET_PENSIZE:
        return "C_CANVAS_SET_PENSIZE";
    case C_CANVAS_SET_PENUPDOWN:
        return "C_CANVAS_SET_PENUPDOWN";
    case C_CANVAS_SET_FONT_NAME:
        return "C_CANVAS_SET_FONT_NAME";
    case C_CANVAS_SET_FONT_SIZE:
        return "C_CANVAS_SET_FONT_SIZE";
    case C_CANVAS_GET_IMAGE:
        return "C_CANVAS_GET_IMAGE";
    case C_CANVAS_GET_SVG:
        return "C_CANVAS_GET_SVG";
    case C_CANVAS_MOUSE_BUTTON_DOWN:
        return "C_CANVAS_MOUSE_BUTTON_DOWN";
    case C_CANVAS_MOUSE_MOVED:
        return "C_CANVAS_MOUSE_MOVED";
    case C_CANVAS_MOUSE_BUTTON_UP:
        return "C_CANVAS_MOUSE_BUTTON_UP";
    case C_CANVAS_SET_PENMODE:
        return "C_CANVAS_SET_PENMODE";
    default:
        return QString("UNKNOWN(%1)").arg(type);
    }
}

/// @brief Escape a string for YAML output
static QString yamlEscape(const QString &str)
{
    QString result;
    result.reserve(str.length() + 10);
    for (QChar c : str)
    {
        if (c == '\n')
            result += "\\n";
        else if (c == '\r')
            result += "\\r";
        else if (c == '\t')
            result += "\\t";
        else if (c == '"')
            result += "\\\"";
        else if (c == '\\')
            result += "\\\\";
        else if (c.unicode() < 32 || c.unicode() > 126)
            result += QString("\\u%1").arg(c.unicode(), 4, 16, QChar('0'));
        else
            result += c;
    }
    return result;
}

/// @brief Serialize message data to YAML format
static QString serializeMessageData(message_t type, const QByteArray &dataBuffer)
{
    QString result;
    QTextStream yaml(&result);

    bool hasData = false;
    yaml << "    data:";

    // If buffer is empty, output null
    if (dataBuffer.isEmpty())
    {
        yaml << " null\n";
        return result;
    }

    QDataStream readStream(dataBuffer);

    switch (type)
    {
    case W_INITIALIZE:
    {
        // Can be empty (request) or QStringList, QString, double (response)
        if (!readStream.atEnd() && dataBuffer.size() >= static_cast<int>(sizeof(qint32)))
        {
            yaml << "\n";
            int fontCount;
            readStream >> fontCount;
            QStringList fontNames;
            for (int i = 0; i < fontCount && !readStream.atEnd(); ++i)
            {
                QString font;
                readStream >> font;
                fontNames << font;
            }
            QString textFontName;
            double textFontSize;
            if (!readStream.atEnd())
            {
                readStream >> textFontName >> textFontSize;

                // Output as list format matching test_pipe.py expectations
                yaml << "      - " << fontCount << "\n";
                for (const QString &font : fontNames)
                {
                    yaml << "      - \"" << yamlEscape(font) << "\"\n";
                }
                yaml << "      - \"" << yamlEscape(textFontName) << "\"\n";
                yaml << "      - " << textFontSize << "\n";
                hasData = true;
            }
        }
        break;
    }
    case W_SET_SCREENMODE:
    {
        ScreenModeEnum mode;
        readStream >> mode;
        yaml << " " << (int)mode << "\n";
        hasData = true;
        break;
    }
    case W_FILE_DIALOG_GET_PATH:
    {
        QString path;
        readStream >> path;
        yaml << " \"" << yamlEscape(path) << "\"\n";
        hasData = true;
        break;
    }
    case C_CONSOLE_PRINT_STRING:
    {
        QString text;
        readStream >> text;
        yaml << " \"" << yamlEscape(text) << "\"\n";
        hasData = true;
        break;
    }
    case C_CONSOLE_SET_FONT_NAME:
    {
        QString name;
        readStream >> name;
        yaml << " \"" << yamlEscape(name) << "\"\n";
        hasData = true;
        break;
    }
    case C_CONSOLE_SET_FONT_SIZE:
    {
        qreal size;
        readStream >> size;
        yaml << " " << size << "\n";
        hasData = true;
        break;
    }
    case C_CONSOLE_REQUEST_LINE:
    {
        QString prompt;
        readStream >> prompt;
        yaml << "\n";
        yaml << "      prompt: \"" << yamlEscape(prompt) << "\"\n";
        hasData = true;
        break;
    }
    case C_CONSOLE_BEGIN_EDIT_TEXT:
    {
        QString text;
        readStream >> text;
        yaml << " \"" << yamlEscape(text) << "\"\n";
        hasData = true;
        break;
    }
    case C_CONSOLE_END_EDIT_TEXT:
    {
        QString text;
        readStream >> text;
        yaml << " \"" << yamlEscape(text) << "\"\n";
        hasData = true;
        break;
    }
    case C_CONSOLE_TEXT_CURSOR_POS:
    {
        int row, col;
        readStream >> row >> col;
        yaml << "\n";
        yaml << "      row: " << row << "\n";
        yaml << "      col: " << col << "\n";
        hasData = true;
        break;
    }
    case C_CONSOLE_SET_TEXT_CURSOR_POS:
    {
        int row, col;
        readStream >> row >> col;
        yaml << "\n";
        yaml << "      - " << row << "\n";
        yaml << "      - " << col << "\n";
        hasData = true;
        break;
    }
    case C_CONSOLE_SET_CURSOR_MODE:
    {
        bool mode;
        readStream >> mode;
        yaml << " " << (mode ? "true" : "false") << "\n";
        hasData = true;
        break;
    }
    case C_CONSOLE_SET_TEXT_COLOR:
    {
        QColor foreground, background;
        readStream >> foreground >> background;
        yaml << "\n";
        yaml << "      foreground:\n";
        yaml << "        r: " << foreground.red() << "\n";
        yaml << "        g: " << foreground.green() << "\n";
        yaml << "        b: " << foreground.blue() << "\n";
        yaml << "        a: " << foreground.alpha() << "\n";
        yaml << "      background:\n";
        yaml << "        r: " << background.red() << "\n";
        yaml << "        g: " << background.green() << "\n";
        yaml << "        b: " << background.blue() << "\n";
        yaml << "        a: " << background.alpha() << "\n";
        hasData = true;
        break;
    }
    case C_CONSOLE_RAWLINE_READ:
    {
        QString line;
        readStream >> line;
        yaml << " \"" << yamlEscape(line) << "\"\n";
        hasData = true;
        break;
    }
    case C_CONSOLE_CHAR_READ:
    {
        QChar c;
        readStream >> c;
        yaml << " \"" << yamlEscape(QString(c)) << "\"\n";
        hasData = true;
        break;
    }
    case C_CANVAS_UPDATE_TURTLE_POS:
    {
        QTransform matrix;
        readStream >> matrix;
        // Transform is complex, just output a placeholder
        yaml << " <QTransform>\n";
        hasData = true;
        break;
    }
    case C_CANVAS_SET_TURTLE_IS_VISIBLE:
    {
        bool visible;
        readStream >> visible;
        yaml << " " << (visible ? "true" : "false") << "\n";
        hasData = true;
        break;
    }
    case C_CANVAS_SET_FOREGROUND_COLOR:
    case C_CANVAS_SET_BACKGROUND_COLOR:
    case C_CANVAS_BEGIN_POLYGON:
    {
        QColor color;
        readStream >> color;
        yaml << "\n";
        yaml << "      r: " << color.red() << "\n";
        yaml << "      g: " << color.green() << "\n";
        yaml << "      b: " << color.blue() << "\n";
        yaml << "      a: " << color.alpha() << "\n";
        hasData = true;
        break;
    }
    case C_CANVAS_SET_BACKGROUND_IMAGE:
    {
        QImage image;
        readStream >> image;
        yaml << " <QImage>\n";
        hasData = true;
        break;
    }
    case C_CANVAS_SETBOUNDS:
    {
        qreal x, y;
        readStream >> x >> y;
        yaml << "\n";
        yaml << "      - " << x << "\n";
        yaml << "      - " << y << "\n";
        hasData = true;
        break;
    }
    case C_CANVAS_SET_IS_BOUNDED:
    {
        bool bounded;
        readStream >> bounded;
        yaml << " " << (bounded ? "true" : "false") << "\n";
        hasData = true;
        break;
    }
    case C_CANVAS_SET_FONT_NAME:
    {
        QString name;
        readStream >> name;
        yaml << " \"" << yamlEscape(name) << "\"\n";
        hasData = true;
        break;
    }
    case C_CANVAS_SET_FONT_SIZE:
    {
        qreal size;
        readStream >> size;
        yaml << " " << size << "\n";
        hasData = true;
        break;
    }
    case C_CANVAS_DRAW_LABEL:
    {
        QString label;
        readStream >> label;
        yaml << " \"" << yamlEscape(label) << "\"\n";
        hasData = true;
        break;
    }
    case C_CANVAS_DRAW_ARC:
    {
        qreal angle, radius;
        readStream >> angle >> radius;
        yaml << "\n";
        yaml << "      - " << angle << "\n";
        yaml << "      - " << radius << "\n";
        hasData = true;
        break;
    }
    case C_CANVAS_SET_PENSIZE:
    {
        qreal size;
        readStream >> size;
        yaml << " " << size << "\n";
        hasData = true;
        break;
    }
    case C_CANVAS_SET_PENUPDOWN:
    {
        bool down;
        readStream >> down;
        yaml << " " << (down ? "true" : "false") << "\n";
        hasData = true;
        break;
    }
    case C_CANVAS_SET_PENMODE:
    {
        PenModeEnum mode;
        readStream >> mode;
        yaml << " " << (int)mode << "\n";
        hasData = true;
        break;
    }
    case C_CANVAS_GET_IMAGE:
    {
        QImage image;
        readStream >> image;
        yaml << " <QImage>\n";
        hasData = true;
        break;
    }
    case C_CANVAS_GET_SVG:
    {
        QByteArray svg;
        readStream >> svg;
        yaml << " <QByteArray>\n";
        hasData = true;
        break;
    }
    case C_CANVAS_MOUSE_BUTTON_DOWN:
    {
        QPointF point;
        int button;
        readStream >> point >> button;
        yaml << "\n";
        yaml << "      point:\n";
        yaml << "        x: " << point.x() << "\n";
        yaml << "        y: " << point.y() << "\n";
        yaml << "      button: " << button << "\n";
        hasData = true;
        break;
    }
    case C_CANVAS_MOUSE_MOVED:
    {
        QPointF point;
        readStream >> point;
        yaml << "\n";
        yaml << "      x: " << point.x() << "\n";
        yaml << "      y: " << point.y() << "\n";
        hasData = true;
        break;
    }
    default:
        // No data or unknown message type
        break;
    }

    if (!hasData)
    {
        yaml << " null\n";
    }

    return result;
}

qint64 ProcessMessageWriter::write(const QByteArray &buffer)
{
    if (logging)
    {
        // Extract message type and data from buffer
        // Buffer format: [length (8 bytes)][header (1 byte)][data...]
        if (buffer.size() >= static_cast<int>(sizeof(qint64) + sizeof(message_t)))
        {
            QByteArray messageData = buffer.mid(sizeof(qint64)); // Skip length prefix
            QDataStream stream(messageData);
            message_t header;
            stream >> header;

            // Extract data portion (after header)
            QByteArray dataPortion = messageData.mid(sizeof(message_t));

            QString msgTypeName = getMessageTypeName(header);

            std::cout << "send:\n";
            std::cout << "  message: " << msgTypeName.toStdString() << "\n";

            // Serialize the data
            QString yamlData = serializeMessageData(header, dataPortion);
            std::cout << yamlData.toStdString();
        }
    }

    return process->write(buffer);
}

#define message(X) (MessageTemplate<ProcessMessageWriter>(X))

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
    return {};
}

int MainWindow::startLogo()
{
    QString command = findQlogoExe();

    QStringList arguments;
    arguments << "--Psychi";

    logoProcess = new QProcess(this);
    ProcessMessageWriter::process = logoProcess;

    connect(
        logoProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::processFinished);

    connect(logoProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::readStandardOutput);

    connect(logoProcess, &QProcess::readyReadStandardError, this, &MainWindow::readStandardError);

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
        message(S_SYSTEM);
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

    message(W_INITIALIZE) << QFontDatabase::families() << defaultFont.family() << (double)defaultFont.pointSizeF();
}

void MainWindow::fileDialogModal()
{
    QString startingDir = QDir::homePath();
    QString filePath = QFileDialog::getOpenFileName(this, tr("Choose file"), startingDir);
    message(W_FILE_DIALOG_GET_PATH) << filePath;
}

void MainWindow::openEditorWindow(const QString &startingText)
{
    if (editWindow == nullptr)
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

void MainWindow::editingHasEndedSlot(const QString &text)
{
    message(C_CONSOLE_END_EDIT_TEXT) << text;
}

void MainWindow::introduceCanvas()
{
    if (hasShownCanvas)
        return;
    hasShownCanvas = true;
    setSplitterforMode(splitScreenMode);
}

void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus != QProcess::NormalExit)
    {
        QMessageBox msgBox;
        msgBox.setText(tr("qlogo has reached an unstable state and will be terminated."));
        msgBox.exec();
    }
    QApplication::exit(0);
}

void MainWindow::readStandardOutput()
{
    qint64 datalen;

    forever
    {
        // If a message is complete then it was already sent,
        // and we can start a new one.
        if (readBuffer.size() == readBufferLen)
        {
            int readResult = logoProcess->read(reinterpret_cast<char *>(&datalen), sizeof(qint64));
            if (readResult != sizeof(qint64))
                return;
            readBufferLen = datalen;
            readBuffer = logoProcess->read(readBufferLen);
        }
        else
        {
            // We are appending the incoming message to the buffer.
            qint64 remain = readBufferLen - readBuffer.size();
            QByteArray post = logoProcess->read(remain);
            Q_ASSERT(!post.isEmpty());
            readBuffer.append(post);
        }

        // If we don't have all of the message yet, keep what we have,
        // and wait for the next signal to come back later.
        if (readBuffer.size() < readBufferLen)
            return;

        // We do have a complete message.
        processReadBuffer();
    }
}

void MainWindow::processReadBuffer()
{
    QDataStream dataStream = QDataStream(readBuffer);
    message_t header;

    dataStream >> header;

    // Log received message if logging is enabled
    if (logging)
    {
        QString msgTypeName = getMessageTypeName(header);

        std::cout << "expect:\n";
        std::cout << "  message: " << msgTypeName.toStdString() << "\n";

        // Extract data portion (after header, which is 1 byte)
        QByteArray dataPortion = readBuffer.mid(sizeof(message_t));

        QString yamlData = serializeMessageData(header, dataPortion);
        std::cout << yamlData.toStdString();
    }

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
        dataStream >> newMode;
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
        dataStream >> text;
        ui->mainConsole->printString(text);
        break;
    }
    case C_CONSOLE_SET_FONT_NAME:
    {
        QString name;
        dataStream >> name;
        ui->mainConsole->setTextFontName(name);
        break;
    }
    case C_CONSOLE_SET_FONT_SIZE:
    {
        qreal aSize;
        dataStream >> aSize;
        ui->mainConsole->setTextFontSize(aSize);
        break;
    }
    case C_CONSOLE_REQUEST_LINE:
    {
        QString prompt;
        dataStream >> prompt;
        beginReadRawlineWithPrompt(prompt);
        break;
    }
    case C_CONSOLE_REQUEST_CHAR:
        beginReadChar();
        break;
    case C_CONSOLE_BEGIN_EDIT_TEXT:
    {
        QString startingText;
        dataStream >> startingText;
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
        dataStream >> row >> col;
        ui->mainConsole->setTextCursorPosition(row, col);
        break;
    }
    case C_CONSOLE_SET_CURSOR_MODE:
    {
        bool mode;
        dataStream >> mode;
        ui->mainConsole->setOverwriteMode(mode);
        break;
    }
    case C_CONSOLE_SET_TEXT_COLOR:
    {
        QColor foreground;
        QColor background;
        dataStream >> foreground >> background;
        ui->mainConsole->setTextFontColor(foreground, background);
        break;
    }
    case C_CONSOLE_CLEAR_SCREEN_TEXT:
        ui->mainConsole->setPlainText("");
        break;
    case C_CANVAS_UPDATE_TURTLE_POS:
    {
        QTransform matrix;
        dataStream >> matrix;
        ui->mainCanvas->setTurtleMatrix(matrix);
        introduceCanvas();
        break;
    }
    case C_CANVAS_SET_TURTLE_IS_VISIBLE:
    {
        bool isVisible;
        dataStream >> isVisible;
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
        dataStream >> color;
        ui->mainCanvas->setForegroundColor(color);
        introduceCanvas();
        break;
    }
    case C_CANVAS_SET_BACKGROUND_COLOR:
    {
        QColor color;
        dataStream >> color;
        ui->mainCanvas->setBackgroundColor(color);
        introduceCanvas();
        break;
    }
    case C_CANVAS_SET_BACKGROUND_IMAGE:
    {
        QImage image;
        dataStream >> image;
        ui->mainCanvas->setBackgroundImage(image);
        introduceCanvas();
        break;
    }
    case C_CANVAS_BEGIN_POLYGON:
    {
        QColor color;
        dataStream >> color;
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
        dataStream >> x >> y;
        ui->mainCanvas->setBounds(x, y);
        break;
    }
    case C_CANVAS_SET_IS_BOUNDED:
    {
        bool isBounded;
        dataStream >> isBounded;
        ui->mainCanvas->setIsBounded(isBounded);
        break;
    }
    case C_CANVAS_SET_FONT_NAME:
    {
        QString name;
        dataStream >> name;
        ui->mainCanvas->setLabelFontName(name);
        break;
    }
    case C_CANVAS_SET_FONT_SIZE:
    {
        qreal aSize;
        dataStream >> aSize;
        ui->mainCanvas->setLabelFontSize(aSize);
        break;
    }
    case C_CANVAS_DRAW_LABEL:
    {
        QString aString;
        dataStream >> aString;
        ui->mainCanvas->addLabel(aString);
        introduceCanvas();
        break;
    }
    case C_CANVAS_DRAW_ARC:
    {
        qreal angle;
        qreal radius;
        dataStream >> angle >> radius;
        ui->mainCanvas->addArc(angle, radius);
        introduceCanvas();
        break;
    }
    case C_CANVAS_SET_PENSIZE:
    {
        qreal newSize;
        dataStream >> newSize;
        ui->mainCanvas->setPensize(newSize);
        break;
    }
    case C_CANVAS_SET_PENMODE:
    {
        PenModeEnum newMode;
        dataStream >> newMode;
        ui->mainCanvas->setPenmode(newMode);
        break;
    }
    case C_CANVAS_SET_PENUPDOWN:
    {
        bool penIsDown;
        dataStream >> penIsDown;
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
    message(C_CANVAS_MOUSE_BUTTON_DOWN) << position << buttonID;
}

void MainWindow::mousemovedSlot(QPointF position)
{
    message(C_CANVAS_MOUSE_MOVED) << position;
}

void MainWindow::mousereleasedSlot()
{
    message(C_CANVAS_MOUSE_BUTTON_UP);
}

void MainWindow::sendCharSlot(QChar c)
{
    message(C_CONSOLE_CHAR_READ) << c;
}

void MainWindow::sendRawlineSlot(const QString &line)
{
    message(C_CONSOLE_RAWLINE_READ) << line;
}

void MainWindow::sendConsoleCursorPosition()
{
    int row = 0;
    int col = 0;
    ui->mainConsole->getCursorPos(row, col);
    message(C_CONSOLE_TEXT_CURSOR_POS) << row << col;
}

void MainWindow::sendCanvasImage()
{
    QImage image(ui->mainCanvas->getImage());
    message(C_CANVAS_GET_IMAGE) << image;
}

void MainWindow::sendCanvasSvg()
{
    QByteArray svg = ui->mainCanvas->getSvg();
    message(C_CANVAS_GET_SVG) << svg;
}

void MainWindow::splitterHasMovedSlot(int, int)
{
    hasShownCanvas = true;
}
