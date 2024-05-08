#include "controller/logocontrollergui.h"
#include "datum/word.h"
#include <QMatrix4x4>
#include <QMessageBox>
#include <QByteArray>
#include <QDataStream>
#include <QApplication>
#include <unistd.h>
#include "stringconstants.h"
#ifdef _WIN32
// For setmode(..., O_BINARY)
#include <fcntl.h>
#endif

// Wrapper function for sending data to the GUI
void sendMessage(std::function<void (QDataStream*)> func)
{
    qint64 datawritten;
    QByteArray buffer; // TODO: Can this be static?
    QDataStream bufferStream(&buffer, QIODevice::WriteOnly);
    func(&bufferStream);
    qint64 datalen = buffer.size();
    buffer.prepend((const char *)&datalen, sizeof(qint64));
    datawritten = write(STDOUT_FILENO, buffer.constData(), buffer.size());
    Q_ASSERT(datawritten == buffer.size());
}


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
    sendMessage([&](QDataStream *out) {
      *out << (message_t)W_CLOSE_PIPE;
    });

    messageQueue.stopQueue();

    qDebug() <<"We are done";
    setDribble("");
    QApplication::quit();
}


LogoControllerGUI::~LogoControllerGUI()
{

}

void LogoControllerGUI::initialize()
{
    messageQueue.startQueue();

    sendMessage([&](QDataStream *out) {
      *out << (message_t)W_INITIALIZE;
    });
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

    switch (header) {
    case W_ZERO:
        qDebug() <<"ZERO!";
        break;
    case W_INITIALIZE:
    {
        bufferStream >> allFontNames
                     >> textFontName
                     >> textFontSize
                     >> minPensize
                     >> maxPensize
                     >> xbound
                     >> ybound
                     >> currentBackgroundColor
                ;
        labelFontName = textFontName;
        break;
    }
    case S_SYSTEM:
        Error::throwError(DatumPtr(k.system()), nothing);
        break;
    case S_TOPLEVEL:
        qDebug() <<"TOPLEVEL triggered";
        Error::throwError(DatumPtr(k.toplevel()), nothing);
        break;
    case S_PAUSE:
        Error::throwError(DatumPtr(k.pause()), nothing);
        break;
    case C_CONSOLE_RAWLINE_READ:
        bufferStream >> rawLine;
        break;
    case C_CONSOLE_CHAR_READ:
        bufferStream >> rawChar;
        break;
    case C_CONSOLE_END_EDIT_TEXT:
        bufferStream >> editorText;
        break;
    case C_CONSOLE_TEXT_CURSOR_POS:
        bufferStream >> cursorRow
                     >> cursorCol;
        break;
    case C_CANVAS_GET_IMAGE:
        bufferStream >> canvasImage;
        break;
    case C_CANVAS_MOUSE_BUTTON_DOWN:
        bufferStream >> clickPos
                     >> lastButtonpressID;
        isMouseButtonDown = true;
        break;
    case C_CANVAS_MOUSE_BUTTON_UP:
        isMouseButtonDown = false;
        break;
    case C_CANVAS_MOUSE_MOVED:
        bufferStream >> mousePos;
        break;
    default:
        qDebug() <<"I don't know how I got " << header;
        break;
    }
    return header;
}


void LogoControllerGUI::processInputMessageQueue()
{
    while (messageQueue.isMessageAvailable()) {
        getMessage();
    }
}


void LogoControllerGUI::waitForMessage(message_t expectedType)
{
    message_t type;
    do {
        type = getMessage();
    } while (type != expectedType);
}


void LogoControllerGUI::printToConsole(const QString &s)
{
    sendMessage([&](QDataStream *out) {
        *out << (message_t)C_CONSOLE_PRINT_STRING << s;
    });

    if (dribbleStream)
        *dribbleStream << s;
}


QString LogoControllerGUI::addStandoutToString(const QString src) {
  QString retval = escapeString + src + escapeString;
  return retval;
}


void LogoControllerGUI::clearScreenText()
{
    sendMessage([&](QDataStream *out) {
        *out << (message_t)C_CANVAS_CLEAR_SCREEN_TEXT;
    });
}

void LogoControllerGUI::getTextCursorPos(int &row, int &col)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CONSOLE_TEXT_CURSOR_POS;
    });

    waitForMessage(C_CONSOLE_TEXT_CURSOR_POS);
    row = cursorRow;
    col = cursorCol;
}


void LogoControllerGUI::setTextCursorPos(int row, int col)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CONSOLE_SET_TEXT_CURSOR_POS
           << row
           << col;
    });
}


void LogoControllerGUI::setTextColor(const QColor foregroundColor, const QColor backgroundColor)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CONSOLE_SET_TEXT_COLOR
           << foregroundColor
           << backgroundColor;
    });
}

void LogoControllerGUI::setCursorOverwriteMode(bool isOverwriteMode)
{
    cursoreModeIsOverwrite = isOverwriteMode;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CONSOLE_SET_CURSOR_MODE
           << isOverwriteMode;
    });
}

bool LogoControllerGUI::cursorOverwriteMode()
{
    return cursoreModeIsOverwrite;
}


const QString LogoControllerGUI::editText(const QString startText)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CONSOLE_BEGIN_EDIT_TEXT << startText;
    });

    waitForMessage(C_CONSOLE_END_EDIT_TEXT);

    return editorText;
}

void LogoControllerGUI::setTextFontName(const QString aFontName)
{
    if (textFontName == aFontName)
        return;
    // TODO: Validate font name
    textFontName = aFontName;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CONSOLE_SET_FONT_NAME << textFontName;
    });
}

void LogoControllerGUI::setTextFontSize(double aSize)
{
    if (textFontSize == aSize)
        return;
    textFontSize = aSize;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CONSOLE_SET_FONT_SIZE << textFontSize;
    });
}

double LogoControllerGUI::getTextFontSize()
{
    return textFontSize;
}

const QString LogoControllerGUI::getTextFontName()
{
    return textFontName;
}


QString LogoControllerGUI::inputRawlineWithPrompt(const QString prompt)
{
    if (dribbleStream)
      *dribbleStream << prompt;

  sendMessage([&](QDataStream *out) {
    *out << (message_t)C_CONSOLE_REQUEST_LINE
         << prompt;
  });
  waitForMessage(C_CONSOLE_RAWLINE_READ);

  return rawLine;
}


DatumPtr LogoControllerGUI::readchar()
{
  sendMessage([&](QDataStream *out) {
    *out << (message_t)C_CONSOLE_REQUEST_CHAR;
  });

  waitForMessage(C_CONSOLE_CHAR_READ);

  return DatumPtr(rawChar);
}

void LogoControllerGUI::setTurtlePos(const QMatrix4x4 &newTurtlePos)
{
  sendMessage([&](QDataStream *out) {
    *out << (message_t)C_CANVAS_UPDATE_TURTLE_POS << newTurtlePos;
  });
}

void LogoControllerGUI::setPenmode(PenModeEnum aMode)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_PENMODE << aMode;
    });
}


void LogoControllerGUI::setScreenMode(ScreenModeEnum newMode)
{
    screenMode = newMode;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)W_SET_SCREENMODE << newMode;
    });
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
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_IS_BOUNDED << aIsBounded;
    });
}

bool LogoControllerGUI::isCanvasBounded()
{
    return canvasIsBounded;
}

void LogoControllerGUI::setTurtleIsVisible(bool isVisible)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_TURTLE_IS_VISIBLE << isVisible;
    });
}

void LogoControllerGUI::drawLine(const QVector3D &start, const QVector3D &end, const QColor &startColor, const QColor &endColor)
{
  sendMessage([&](QDataStream *out) {
    *out << (message_t)C_CANVAS_DRAW_LINE
         << start
         << end
         << startColor
         << endColor;
  });
}

void LogoControllerGUI::drawPolygon(const QList<QVector3D> &points, const QList<QColor> &colors)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_DRAW_POLYGON
           << points
           << colors;
    });
}

void LogoControllerGUI::drawLabel(const QString &aString, const QVector3D &aPosition, const QColor &aColor)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_DRAW_LABEL
           << aString
           << aPosition
           << aColor;
    });
}

void LogoControllerGUI::setLabelFontName(const QString &aName)
{
    if (aName == labelFontName)
        return;
    labelFontName = aName;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_FONT_NAME
           << aName;
    });
}

void LogoControllerGUI::setLabelFontSize(double aSize)
{
    if (aSize == labelFontSize)
        return;
    labelFontSize = aSize;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_FONT_SIZE
           << labelFontSize;
    });
}

const QString LogoControllerGUI::getLabelFontName()
{
    return labelFontName;
}

double LogoControllerGUI::getLabelFontSize()
{
    return labelFontSize;
}

void LogoControllerGUI::setCanvasBackgroundColor(QColor aColor)
{
    currentBackgroundColor = aColor;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_BACKGROUND_COLOR
           << aColor;
    });
}

QColor LogoControllerGUI::getCanvasBackgroundColor(void)
{
    return currentBackgroundColor;
}

void LogoControllerGUI::clearScreen()
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_CLEAR_SCREEN;
    });
}

QImage LogoControllerGUI::getCanvasImage()
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_GET_IMAGE;
    });

    waitForMessage(C_CANVAS_GET_IMAGE);

    return canvasImage;
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
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SETBOUNDS
           << xbound
           << ybound;
    });

}

void LogoControllerGUI::setPensize(double aSize)
{
    if (aSize == penSize)
        return;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_PENSIZE
           << aSize;
    });
    penSize = aSize;
}

void LogoControllerGUI::mwait(unsigned long msecs) {
  QThread::msleep(msecs);
}

