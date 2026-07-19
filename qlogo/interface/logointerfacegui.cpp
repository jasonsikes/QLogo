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
#include "interface/pipeio.h"
#include "interface/streammanager.h"
#include <QCoreApplication>
#include <QByteArray>
#include <QDataStream>
#include <QMatrix4x4>
#include <QMessageBox>
#include <QTransform>

qint64 StdoutMessageWriter::write(const QByteArray &buffer)
{
    return pipeWrite(stdoutFd(), buffer.constData(), buffer.size());
}

#define message(X) (MessageTemplate<StdoutMessageWriter>(X))

LogoInterfaceGUI::LogoInterfaceGUI(QObject *parent) : LogoInterface(parent)
{
    // That dreaded \r\n <-> \n problem (no-op on Unix)
setStdioBinaryMode();
}

void LogoInterfaceGUI::closeInterface()
{
    message(W_CLOSE_PIPE);

    messageQueue_.stopQueue();

    StreamManager::get().noDribble();
}

LogoInterfaceGUI::~LogoInterfaceGUI() = default;

void LogoInterfaceGUI::initialize()
{
    messageQueue_.startQueue();

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

    QByteArray buffer = messageQueue_.getMessage();
    QDataStream bufferStream(&buffer, QIODevice::ReadOnly);

    bufferStream >> header;

    switch (header)
    {
    case W_ZERO:
        qInfo() << "ZERO!";
        break;
    case W_INITIALIZE:
    {
        bufferStream >> allFontNames_ >> textFontName_ >> textFontSize_;
        labelFontName_ = textFontName_;
        labelFontSize_ = textFontSize_;
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
        bufferStream >> rawLine_;
        break;
    case C_CONSOLE_CHAR_READ:
        bufferStream >> rawChar_;
        break;
    case W_FILE_DIALOG_GET_PATH:
        bufferStream >> filePath_;
        break;
    case C_CONSOLE_END_EDIT_TEXT:
        bufferStream >> editorText_;
        break;
    case C_CONSOLE_TEXT_CURSOR_POS:
        bufferStream >> cursorRow_ >> cursorCol_;
        break;
    case C_CANVAS_GET_IMAGE:
        bufferStream >> canvasImage_;
        break;
    case C_CANVAS_GET_SVG:
        bufferStream >> canvasSvg_;
        break;
    case C_CANVAS_MOUSE_BUTTON_DOWN:
        bufferStream >> clickPos_ >> lastButtonpressID_;
        isMouseButtonDown_ = true;
        break;
    case C_CANVAS_MOUSE_BUTTON_UP:
        isMouseButtonDown_ = false;
        break;
    case C_CANVAS_MOUSE_MOVED:
        bufferStream >> mousePos_;
        break;
    default:
        // This should never happen. If it does, then there is a mess-up in the protocol.
        qInfo() << "I don't know how I got " << header;
        break;
    }
    return header;
}

void LogoInterfaceGUI::processInputMessageQueue()
{
    while (messageQueue_.isMessageAvailable())
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

    StreamManager::get().teeToDribble(s);
}

QString LogoInterfaceGUI::addStandoutToString(const QString &src)
{
    QString retval = Config::get().escapeString_ + src + Config::get().escapeString_;
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
    row = cursorRow_;
    col = cursorCol_;
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
    cursorModeIsOverwrite_ = isOverwriteMode;
    message(C_CONSOLE_SET_CURSOR_MODE) << isOverwriteMode;
}

bool LogoInterfaceGUI::cursorOverwriteMode() const
{
    return cursorModeIsOverwrite_;
}

QString LogoInterfaceGUI::editText(const QString &startText)
{
    message(C_CONSOLE_BEGIN_EDIT_TEXT) << startText;

    waitForMessage(C_CONSOLE_END_EDIT_TEXT);

    return editorText_;
}

void LogoInterfaceGUI::setTextFontName(const QString &aFontName)
{
    if (textFontName_ == aFontName)
        return;
    // TODO: Validate font name
    textFontName_ = aFontName;
    message(C_CONSOLE_SET_FONT_NAME) << textFontName_;
}

void LogoInterfaceGUI::setTextFontSize(double aSize)
{
    if (textFontSize_ == aSize)
        return;
    textFontSize_ = aSize;
    message(C_CONSOLE_SET_FONT_SIZE) << textFontSize_;
}

double LogoInterfaceGUI::getTextFontSize() const
{
    return textFontSize_;
}

QString LogoInterfaceGUI::getTextFontName() const
{
    return textFontName_;
}

QString LogoInterfaceGUI::inputRawlineWithPrompt(const QString &prompt)
{
    StreamManager::get().teeToDribble(prompt);

    message(C_CONSOLE_REQUEST_LINE) << prompt;
    waitForMessage(C_CONSOLE_RAWLINE_READ);

    return rawLine_;
}

DatumPtr LogoInterfaceGUI::readchar()
{
    message(C_CONSOLE_REQUEST_CHAR);

    waitForMessage(C_CONSOLE_CHAR_READ);

    return DatumPtr(rawChar_);
}

QString LogoInterfaceGUI::fileDialogModal()
{
    message(W_FILE_DIALOG_GET_PATH);

    waitForMessage(W_FILE_DIALOG_GET_PATH);

    return filePath_;
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
    screenMode_ = newMode;
    message(W_SET_SCREENMODE) << newMode;
}

ScreenModeEnum LogoInterfaceGUI::getScreenMode() const
{
    return screenMode_;
}

void LogoInterfaceGUI::setIsCanvasBounded(bool aIsBounded)
{
    if (canvasIsBounded_ == aIsBounded)
        return;
    canvasIsBounded_ = aIsBounded;
    message(C_CANVAS_SET_IS_BOUNDED) << aIsBounded;
}

bool LogoInterfaceGUI::isCanvasBounded() const
{
    return canvasIsBounded_;
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
    if (aName == labelFontName_)
        return;
    labelFontName_ = aName;
    message(C_CANVAS_SET_FONT_NAME) << aName;
}

void LogoInterfaceGUI::setLabelFontSize(double aSize)
{
    if (aSize == labelFontSize_)
        return;
    labelFontSize_ = aSize;
    message(C_CANVAS_SET_FONT_SIZE) << (qreal)labelFontSize_;
}

QString LogoInterfaceGUI::getLabelFontName() const
{
    return labelFontName_;
}

double LogoInterfaceGUI::getLabelFontSize() const
{
    return labelFontSize_;
}

void LogoInterfaceGUI::setCanvasBackgroundColor(const QColor &aColor)
{
    currentBackgroundColor_ = aColor;
    message(C_CANVAS_SET_BACKGROUND_COLOR) << aColor;
}

void LogoInterfaceGUI::setCanvasForegroundColor(const QColor &aColor)
{
    if (currentForegroundColor_ != aColor)
    {
        currentForegroundColor_ = aColor;
        message(C_CANVAS_SET_FOREGROUND_COLOR) << aColor;
    }
}

void LogoInterfaceGUI::setCanvasBackgroundImage(const QImage &anImage)
{
    message(C_CANVAS_SET_BACKGROUND_IMAGE) << anImage;
}

const QColor LogoInterfaceGUI::getCanvasBackgroundColor() const
{
    return currentBackgroundColor_;
}

void LogoInterfaceGUI::clearCanvas()
{
    message(C_CANVAS_CLEAR_SCREEN);
}

QImage LogoInterfaceGUI::getCanvasImage()
{
    message(C_CANVAS_GET_IMAGE);

    waitForMessage(C_CANVAS_GET_IMAGE);

    return canvasImage_;
}

QByteArray LogoInterfaceGUI::getSvgImage()
{
    message(C_CANVAS_GET_SVG);

    waitForMessage(C_CANVAS_GET_SVG);

    return canvasSvg_;
}

bool LogoInterfaceGUI::getIsMouseButtonDown()
{
    processInputMessageQueue();
    return isMouseButtonDown_;
}

QVector2D LogoInterfaceGUI::lastMouseclickPosition()
{
    processInputMessageQueue();
    return clickPos_;
}

int LogoInterfaceGUI::getAndResetButtonID()
{
    processInputMessageQueue();
    int retval = lastButtonpressID_;
    lastButtonpressID_ = 0;
    return retval;
}

QVector2D LogoInterfaceGUI::mousePosition()
{
    processInputMessageQueue();
    return mousePos_;
}

void LogoInterfaceGUI::setBounds(double x, double y)
{
    if ((xbound_ == x) && (ybound_ == y))
        return;
    xbound_ = x;
    ybound_ = y;
    message(C_CANVAS_SETBOUNDS) << (qreal)xbound_ << (qreal)ybound_;
}

void LogoInterfaceGUI::setPensize(qreal aSize)
{
    if (aSize == penSize_)
        return;
    message(C_CANVAS_SET_PENSIZE) << (qreal)aSize;
    penSize_ = aSize;
}
