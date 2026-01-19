//===-- qlogo/logointerfacegui.h - class definition -------*- C++ -*-===//
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
/// This file contains the definition of the LogoInterfaceGUI class, which is
/// responsible for handling user input and output through Psychi, the QLogo-GUI
/// terminal application. In addition to text input and output, the GUI interface
/// also receives mouse and keyboard events from the GUI, and provides a way to
/// communicate Turtle movements and drawing commands to the QLogo canvas.
///
//===----------------------------------------------------------------------===//

#include "interface/logointerfacegui.h"
#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QMatrix4x4>
#include <QMessageBox>
#include <QTransform>

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

#define message(X) (MessageTemplate<StdoutMessageWriter>(X))

LogoInterfaceGUI::LogoInterfaceGUI(QObject *parent) : LogoInterface(parent)
{
#ifdef _WIN32
    // That dreaded \r\n <-> \n problem
    setmode(STDOUT_FILENO, O_BINARY);
    setmode(STDIN_FILENO, O_BINARY);
#endif
}

void LogoInterfaceGUI::closeInterface()
{
    message(W_CLOSE_PIPE);

    messageQueue.stopQueue();

    setDribble("");
}

LogoInterfaceGUI::~LogoInterfaceGUI() = default;

void LogoInterfaceGUI::initialize()
{
    messageQueue.startQueue();

    message(W_INITIALIZE);
    waitForMessage(W_INITIALIZE);
}

/* a message has three parts:
 * 1. datalen: A quint detailing how many bytes are in the remainder of the message.
 * 2. header:  An enum describing the type of data.
 * 3. The data (varies, may be empty).
 */
message_t LogoInterfaceGUI::getMessage()
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
        // This should never happen. If it does, then there is a mess-up in the protocol.
        qDebug() << "I don't know how I got " << header;
        break;
    }
    return header;
}

void LogoInterfaceGUI::processInputMessageQueue()
{
    while (messageQueue.isMessageAvailable())
    {
        getMessage();
    }
}

void LogoInterfaceGUI::waitForMessage(message_t expectedType)
{
    message_t type;
    do
    {
        type = getMessage();
    } while (type != expectedType);
}

void LogoInterfaceGUI::printToConsole(const QString &s)
{
    message(C_CONSOLE_PRINT_STRING) << s;

    if (dribbleStream)
        *dribbleStream << s;
}

QString LogoInterfaceGUI::addStandoutToString(const QString &src)
{
    QString retval = Config::get().escapeString + src + Config::get().escapeString;
    return retval;
}

void LogoInterfaceGUI::clearScreenText()
{
    message(C_CONSOLE_CLEAR_SCREEN_TEXT);
}

void LogoInterfaceGUI::getTextCursorPos(int &row, int &col)
{
    message(C_CONSOLE_TEXT_CURSOR_POS);

    waitForMessage(C_CONSOLE_TEXT_CURSOR_POS);
    row = cursorRow;
    col = cursorCol;
}

void LogoInterfaceGUI::setTextCursorPos(int row, int col)
{
    message(C_CONSOLE_SET_TEXT_CURSOR_POS) << row << col;
}

void LogoInterfaceGUI::setTextColor(const QColor &foregroundColor, const QColor &backgroundColor)
{
    message(C_CONSOLE_SET_TEXT_COLOR) << foregroundColor << backgroundColor;
}

void LogoInterfaceGUI::setCursorOverwriteMode(bool isOverwriteMode)
{
    cursorModeIsOverwrite = isOverwriteMode;
    message(C_CONSOLE_SET_CURSOR_MODE) << isOverwriteMode;
}

bool LogoInterfaceGUI::cursorOverwriteMode() const
{
    return cursorModeIsOverwrite;
}

QString LogoInterfaceGUI::editText(const QString &startText)
{
    message(C_CONSOLE_BEGIN_EDIT_TEXT) << startText;

    waitForMessage(C_CONSOLE_END_EDIT_TEXT);

    return editorText;
}

void LogoInterfaceGUI::setTextFontName(const QString &aFontName)
{
    if (textFontName == aFontName)
        return;
    // TODO: Validate font name
    textFontName = aFontName;
    message(C_CONSOLE_SET_FONT_NAME) << textFontName;
}

void LogoInterfaceGUI::setTextFontSize(double aSize)
{
    if (textFontSize == aSize)
        return;
    textFontSize = aSize;
    message(C_CONSOLE_SET_FONT_SIZE) << textFontSize;
}

double LogoInterfaceGUI::getTextFontSize() const
{
    return textFontSize;
}

QString LogoInterfaceGUI::getTextFontName() const
{
    return textFontName;
}

QString LogoInterfaceGUI::inputRawlineWithPrompt(const QString &prompt)
{
    if (dribbleStream)
        *dribbleStream << prompt;

    message(C_CONSOLE_REQUEST_LINE) << prompt;
    waitForMessage(C_CONSOLE_RAWLINE_READ);

    return rawLine;
}

DatumPtr LogoInterfaceGUI::readchar()
{
    message(C_CONSOLE_REQUEST_CHAR);

    waitForMessage(C_CONSOLE_CHAR_READ);

    return DatumPtr(rawChar);
}

QString LogoInterfaceGUI::fileDialogModal()
{
    message(W_FILE_DIALOG_GET_PATH);

    waitForMessage(W_FILE_DIALOG_GET_PATH);

    return filePath;
}

void LogoInterfaceGUI::setTurtlePos(QTransform *newTurtlePosPtr)
{
    message(C_CANVAS_UPDATE_TURTLE_POS) << *newTurtlePosPtr;
}

void LogoInterfaceGUI::setPenmode(PenModeEnum aMode)
{
    message(C_CANVAS_SET_PENMODE) << aMode;
}

void LogoInterfaceGUI::setScreenMode(ScreenModeEnum newMode)
{
    screenMode = newMode;
    message(W_SET_SCREENMODE) << newMode;
}

ScreenModeEnum LogoInterfaceGUI::getScreenMode() const
{
    return screenMode;
}

void LogoInterfaceGUI::setIsCanvasBounded(bool aIsBounded)
{
    if (canvasIsBounded == aIsBounded)
        return;
    canvasIsBounded = aIsBounded;
    message(C_CANVAS_SET_IS_BOUNDED) << aIsBounded;
}

bool LogoInterfaceGUI::isCanvasBounded() const
{
    return canvasIsBounded;
}

void LogoInterfaceGUI::setTurtleIsVisible(int isVisible)
{
    message(C_CANVAS_SET_TURTLE_IS_VISIBLE) << (bool)isVisible;
}

void LogoInterfaceGUI::setPenIsDown(bool penIsDown)
{
    message(C_CANVAS_SET_PENUPDOWN) << penIsDown;
}

void LogoInterfaceGUI::emitVertex()
{
    message(C_CANVAS_EMIT_VERTEX);
}

void LogoInterfaceGUI::beginPolygon(const QColor &color)
{
    message(C_CANVAS_BEGIN_POLYGON) << color;
}

void LogoInterfaceGUI::endPolygon()
{
    message(C_CANVAS_END_POLYGON);
}

void LogoInterfaceGUI::drawLabel(const QString &aString)
{
    message(C_CANVAS_DRAW_LABEL) << aString;
}

void LogoInterfaceGUI::drawArc(double angle, double radius)
{
    message(C_CANVAS_DRAW_ARC) << (qreal)angle << (qreal)radius;
}

void LogoInterfaceGUI::setLabelFontName(const QString &aName)
{
    if (aName == labelFontName)
        return;
    labelFontName = aName;
    message(C_CANVAS_SET_FONT_NAME) << aName;
}

void LogoInterfaceGUI::setLabelFontSize(double aSize)
{
    if (aSize == labelFontSize)
        return;
    labelFontSize = aSize;
    message(C_CANVAS_SET_FONT_SIZE) << (qreal)labelFontSize;
}

QString LogoInterfaceGUI::getLabelFontName() const
{
    return labelFontName;
}

double LogoInterfaceGUI::getLabelFontSize() const
{
    return labelFontSize;
}

void LogoInterfaceGUI::setCanvasBackgroundColor(const QColor &aColor)
{
    currentBackgroundColor = aColor;
    message(C_CANVAS_SET_BACKGROUND_COLOR) << aColor;
}

void LogoInterfaceGUI::setCanvasForegroundColor(const QColor &aColor)
{
    if (currentForegroundColor != aColor)
    {
        currentForegroundColor = aColor;
        message(C_CANVAS_SET_FOREGROUND_COLOR) << aColor;
    }
}

void LogoInterfaceGUI::setCanvasBackgroundImage(const QImage &anImage)
{
    message(C_CANVAS_SET_BACKGROUND_IMAGE) << anImage;
}

const QColor LogoInterfaceGUI::getCanvasBackgroundColor() const
{
    return currentBackgroundColor;
}

void LogoInterfaceGUI::clearCanvas()
{
    message(C_CANVAS_CLEAR_SCREEN);
}

QImage LogoInterfaceGUI::getCanvasImage()
{
    message(C_CANVAS_GET_IMAGE);

    waitForMessage(C_CANVAS_GET_IMAGE);

    return canvasImage;
}

QByteArray LogoInterfaceGUI::getSvgImage()
{
    message(C_CANVAS_GET_SVG);

    waitForMessage(C_CANVAS_GET_SVG);

    return canvasSvg;
}

bool LogoInterfaceGUI::getIsMouseButtonDown()
{
    processInputMessageQueue();
    return isMouseButtonDown;
}

QVector2D LogoInterfaceGUI::lastMouseclickPosition()
{
    processInputMessageQueue();
    return clickPos;
}

int LogoInterfaceGUI::getAndResetButtonID()
{
    processInputMessageQueue();
    int retval = lastButtonpressID;
    lastButtonpressID = 0;
    return retval;
}

QVector2D LogoInterfaceGUI::mousePosition()
{
    processInputMessageQueue();
    return mousePos;
}

void LogoInterfaceGUI::setBounds(double x, double y)
{
    if ((xbound == x) && (ybound == y))
        return;
    xbound = x;
    ybound = y;
    message(C_CANVAS_SETBOUNDS) << (qreal)xbound << (qreal)ybound;
}

void LogoInterfaceGUI::setPensize(qreal aSize)
{
    if (aSize == penSize)
        return;
    message(C_CANVAS_SET_PENSIZE) << (qreal)aSize;
    penSize = aSize;
}
