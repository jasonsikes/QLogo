//===-- qlogo/logocontrollergui.h - class definition -------*- C++ -*-===//
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
/// This file contains the definition of the LogoControllerGUI class, which is
/// responsible for handling user input and output through Psychi, the QLogo-GUI
/// terminal application. In addition to text input and output, the GUI controller
/// also receives mouse and keyboard events from the GUI, and provides a way to
/// communicate Turtle movements and drawing commands to the QLogo canvas.
///
//===----------------------------------------------------------------------===//

#include "controller/logocontrollergui.h"
#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QMatrix4x4>
#include <QMessageBox>

#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef _WIN32
// For setmode(..., O_BINARY)
#include <fcntl.h>
#endif

qint64 StdoutMessageWriter::write(const QByteArray &buffer)
{
    return ::write(STDOUT_FILENO, buffer.constData(), buffer.size());
}

using message = MessageTemplate<StdoutMessageWriter>;

LogoControllerGUI::LogoControllerGUI(QObject *parent) : LogoController(parent)
{
#ifdef _WIN32
    // That dreaded \r\n <-> \n problem
    setmode(STDOUT_FILENO, O_BINARY);
    setmode(STDIN_FILENO, O_BINARY);
#endif
}

void LogoControllerGUI::systemStop()
{
    message() << (message_t)W_CLOSE_PIPE;

    messageQueue.stopQueue();

    qDebug() << "We are done";
    setDribble("");
    QApplication::quit();
}

LogoControllerGUI::~LogoControllerGUI()
{
}

void LogoControllerGUI::initialize()
{
    messageQueue.startQueue();

    message() << (message_t)W_INITIALIZE;
    waitForMessage(W_INITIALIZE);
}

/* a message has three parts:
 * 1. datalen: A quint detailing how many bytes are in the remainder of the message.
 * 2. header:  An enum describing the type of data.
 * 3. The data (varies, may be empty).
 */
message_t LogoControllerGUI::getMessage()
{
    message_t header;

    QByteArray buffer = messageQueue.getMessage();
    QDataStream bufferStream(&buffer, QIODevice::ReadOnly);

    bufferStream >> header;

    switch (header)
    {
    case W_ZERO:
        qDebug() << "ZERO!";
        break;
    case W_INITIALIZE:
    {
        bufferStream >> allFontNames >> textFontName >> textFontSize;
        labelFontName = textFontName;
        labelFontSize = textFontSize;
        break;
    }
    case S_SYSTEM:
        throw FCError::custom(DatumPtr(QObject::tr("SYSTEM")));
        break;
    case S_TOPLEVEL:
        throw FCError::custom(DatumPtr(QObject::tr("TOPLEVEL")));
        break;
    case S_PAUSE:
        throw FCError::custom(DatumPtr(QObject::tr("PAUSE")));
        break;
    case C_CONSOLE_RAWLINE_READ:
        bufferStream >> rawLine;
        break;
    case C_CONSOLE_CHAR_READ:
        bufferStream >> rawChar;
        break;
    case W_FILE_DIALOG_GET_PATH:
        bufferStream >> filePath;
        break;
    case C_CONSOLE_END_EDIT_TEXT:
        bufferStream >> editorText;
        break;
    case C_CONSOLE_TEXT_CURSOR_POS:
        bufferStream >> cursorRow >> cursorCol;
        break;
    case C_CANVAS_GET_IMAGE:
        bufferStream >> canvasImage;
        break;
    case C_CANVAS_GET_SVG:
        bufferStream >> canvasSvg;
        break;
    case C_CANVAS_MOUSE_BUTTON_DOWN:
        bufferStream >> clickPos >> lastButtonpressID;
        isMouseButtonDown = true;
        break;
    case C_CANVAS_MOUSE_BUTTON_UP:
        isMouseButtonDown = false;
        break;
    case C_CANVAS_MOUSE_MOVED:
        bufferStream >> mousePos;
        break;
    default:
        qDebug() << "I don't know how I got " << header;
        break;
    }
    return header;
}

void LogoControllerGUI::processInputMessageQueue()
{
    while (messageQueue.isMessageAvailable())
    {
        getMessage();
    }
}

void LogoControllerGUI::waitForMessage(message_t expectedType)
{
    message_t type;
    do
    {
        type = getMessage();
    } while (type != expectedType);
}

void LogoControllerGUI::printToConsole(QString s)
{
    message() << (message_t)C_CONSOLE_PRINT_STRING << s;

    if (dribbleStream)
        *dribbleStream << s;
}

QString LogoControllerGUI::addStandoutToString(QString src)
{
    QString retval = Config::get().escapeString + src + Config::get().escapeString;
    return retval;
}

void LogoControllerGUI::clearScreenText()
{
    message() << (message_t)C_CONSOLE_CLEAR_SCREEN_TEXT;
}

void LogoControllerGUI::getTextCursorPos(int &row, int &col)
{
    message() << (message_t)C_CONSOLE_TEXT_CURSOR_POS;

    waitForMessage(C_CONSOLE_TEXT_CURSOR_POS);
    row = cursorRow;
    col = cursorCol;
}

void LogoControllerGUI::setTextCursorPos(int row, int col)
{
    message() << (message_t)C_CONSOLE_SET_TEXT_CURSOR_POS << row << col;
}

void LogoControllerGUI::setTextColor(const QColor &foregroundColor, const QColor &backgroundColor)
{
    message() << (message_t)C_CONSOLE_SET_TEXT_COLOR << foregroundColor << backgroundColor;
}

void LogoControllerGUI::setCursorOverwriteMode(bool isOverwriteMode)
{
    cursoreModeIsOverwrite = isOverwriteMode;
    message() << (message_t)C_CONSOLE_SET_CURSOR_MODE << isOverwriteMode;
}

bool LogoControllerGUI::cursorOverwriteMode()
{
    return cursoreModeIsOverwrite;
}

QString LogoControllerGUI::editText(QString startText)
{
    message() << (message_t)C_CONSOLE_BEGIN_EDIT_TEXT << startText;

    waitForMessage(C_CONSOLE_END_EDIT_TEXT);

    return editorText;
}

void LogoControllerGUI::setTextFontName(const QString aFontName)
{
    if (textFontName == aFontName)
        return;
    // TODO: Validate font name
    textFontName = aFontName;
    message() << (message_t)C_CONSOLE_SET_FONT_NAME << textFontName;
}

void LogoControllerGUI::setTextFontSize(double aSize)
{
    if (textFontSize == aSize)
        return;
    textFontSize = aSize;
    message() << (message_t)C_CONSOLE_SET_FONT_SIZE << textFontSize;
}

double LogoControllerGUI::getTextFontSize()
{
    return textFontSize;
}

QString LogoControllerGUI::getTextFontName()
{
    return textFontName;
}

QString LogoControllerGUI::inputRawlineWithPrompt(QString prompt)
{
    if (dribbleStream)
        *dribbleStream << prompt;

    message() << (message_t)C_CONSOLE_REQUEST_LINE << prompt;
    waitForMessage(C_CONSOLE_RAWLINE_READ);

    return rawLine;
}

DatumPtr LogoControllerGUI::readchar()
{
    message() << (message_t)C_CONSOLE_REQUEST_CHAR;

    waitForMessage(C_CONSOLE_CHAR_READ);

    return DatumPtr(rawChar);
}

QString LogoControllerGUI::fileDialogModal()
{
    message() << (message_t)W_FILE_DIALOG_GET_PATH;

    waitForMessage(W_FILE_DIALOG_GET_PATH);

    return filePath;
}

void LogoControllerGUI::setTurtlePos(const Transform &newTurtlePos)
{
    message() << (message_t)C_CANVAS_UPDATE_TURTLE_POS << newTurtlePos;
}

void LogoControllerGUI::setPenmode(PenModeEnum aMode)
{
    message() << (message_t)C_CANVAS_SET_PENMODE << aMode;
}

void LogoControllerGUI::setScreenMode(ScreenModeEnum newMode)
{
    screenMode = newMode;
    message() << (message_t)W_SET_SCREENMODE << newMode;
}

ScreenModeEnum LogoControllerGUI::getScreenMode()
{
    return screenMode;
}

void LogoControllerGUI::setIsCanvasBounded(bool aIsBounded)
{
    if (canvasIsBounded == aIsBounded)
        return;
    canvasIsBounded = aIsBounded;
    message() << (message_t)C_CANVAS_SET_IS_BOUNDED << aIsBounded;
}

bool LogoControllerGUI::isCanvasBounded()
{
    return canvasIsBounded;
}

void LogoControllerGUI::setTurtleIsVisible(int isVisible)
{
    message() << (message_t)C_CANVAS_SET_TURTLE_IS_VISIBLE << (bool)isVisible;
}

void LogoControllerGUI::setPenIsDown(bool penIsDown)
{
    message() << (message_t)C_CANVAS_SET_PENUPDOWN << penIsDown;
}

void LogoControllerGUI::emitVertex()
{
    message() << (message_t)C_CANVAS_EMIT_VERTEX;
}

void LogoControllerGUI::beginPolygon(const QColor &color)
{
    message() << (message_t)C_CANVAS_BEGIN_POLYGON << color;
}

void LogoControllerGUI::endPolygon()
{
    message() << (message_t)C_CANVAS_END_POLYGON;
}

void LogoControllerGUI::drawLabel(QString aString)
{
    message() << (message_t)C_CANVAS_DRAW_LABEL << aString;
}

void LogoControllerGUI::drawArc(double angle, double radius)
{
    message() << (message_t)C_CANVAS_DRAW_ARC << (qreal)angle << (qreal)radius;
}

void LogoControllerGUI::setLabelFontName(QString aName)
{
    if (aName == labelFontName)
        return;
    labelFontName = aName;
    message() << (message_t)C_CANVAS_SET_FONT_NAME << aName;
}

void LogoControllerGUI::setLabelFontSize(double aSize)
{
    if (aSize == labelFontSize)
        return;
    labelFontSize = aSize;
    message() << (message_t)C_CANVAS_SET_FONT_SIZE << (qreal)labelFontSize;
}

QString LogoControllerGUI::getLabelFontName()
{
    return labelFontName;
}

double LogoControllerGUI::getLabelFontSize()
{
    return labelFontSize;
}

void LogoControllerGUI::setCanvasBackgroundColor(const QColor &aColor)
{
    currentBackgroundColor = aColor;
    message() << (message_t)C_CANVAS_SET_BACKGROUND_COLOR << aColor;
}

void LogoControllerGUI::setCanvasForegroundColor(const QColor &aColor)
{
    if (currentForegroundColor != aColor)
    {
        currentForegroundColor = aColor;
        message() << (message_t)C_CANVAS_SET_FOREGROUND_COLOR << aColor;
    }
}

void LogoControllerGUI::setCanvasBackgroundImage(QImage anImage)
{
    message() << (message_t)C_CANVAS_SET_BACKGROUND_IMAGE << anImage;
}

const QColor LogoControllerGUI::getCanvasBackgroundColor(void)
{
    return currentBackgroundColor;
}

void LogoControllerGUI::clearCanvas()
{
    message() << (message_t)C_CANVAS_CLEAR_SCREEN;
}

QImage LogoControllerGUI::getCanvasImage()
{
    message() << (message_t)C_CANVAS_GET_IMAGE;

    waitForMessage(C_CANVAS_GET_IMAGE);

    return canvasImage;
}

QByteArray LogoControllerGUI::getSvgImage()
{
    message() << (message_t)C_CANVAS_GET_SVG;

    waitForMessage(C_CANVAS_GET_SVG);

    return canvasSvg;
}

bool LogoControllerGUI::getIsMouseButtonDown()
{
    processInputMessageQueue();
    return isMouseButtonDown;
}

QVector2D LogoControllerGUI::lastMouseclickPosition()
{
    processInputMessageQueue();
    return clickPos;
}

int LogoControllerGUI::getAndResetButtonID()
{
    processInputMessageQueue();
    int retval = lastButtonpressID;
    lastButtonpressID = 0;
    return retval;
}

QVector2D LogoControllerGUI::mousePosition()
{
    processInputMessageQueue();
    return mousePos;
}

void LogoControllerGUI::setBounds(double x, double y)
{
    if ((xbound == x) && (ybound == y))
        return;
    xbound = x;
    ybound = y;
    message() << (message_t)C_CANVAS_SETBOUNDS << (qreal)xbound << (qreal)ybound;
}

void LogoControllerGUI::setPensize(qreal aSize)
{
    if (aSize == penSize)
        return;
    message() << (message_t)C_CANVAS_SET_PENSIZE << (qreal)aSize;
    penSize = aSize;
}

void LogoControllerGUI::mwait(unsigned long msecs)
{
    QThread::msleep(msecs);
}
