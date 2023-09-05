#include "qlogocontroller.h"
#include "datum_word.h"
#include <QMatrix4x4>
#include <QMessageBox>
#include <QByteArray>
#include <QDataStream>
#include <QApplication>
#include <unistd.h>

#ifdef _WIN32
// For setmode(..., O_BINARY)
#include <fcntl.h>
#endif

// Wrapper function for sending data to the GUI
void sendMessage(std::function<void (QDataStream*)> func)
{
    qint64 datawritten;
    QByteArray buffer;
    QDataStream bufferStream(&buffer, QIODevice::WriteOnly);
    func(&bufferStream);
    qint64 datalen = buffer.size();
    buffer.prepend((const char *)&datalen, sizeof(qint64));
    datawritten = write(STDOUT_FILENO, buffer.constData(), buffer.size());
    Q_ASSERT(datawritten == buffer.size());
}


QLogoController::QLogoController(QObject *parent) : LogoController(parent)
{
#ifdef _WIN32
    // That dreaded \r\n <-> \n problem
    setmode(STDOUT_FILENO, O_BINARY);
    setmode(STDIN_FILENO, O_BINARY);
#endif
}

void QLogoController::systemStop()
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)W_CLOSE_PIPE;
    });

    messageQueue.stopQueue();

    qDebug() <<"We are done";
    setDribble("");
    QApplication::quit();
}


QLogoController::~QLogoController()
{

}

void QLogoController::initialize()
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
message_t QLogoController::getMessage()
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
        Error::throwError(DatumP(new Word("SYSTEM")), nothing);
        break;
    case S_TOPLEVEL:
        qDebug() <<"TOPLEVEL triggered";
        Error::throwError(DatumP(new Word("TOPLEVEL")), nothing);
        break;
    case S_PAUSE:
        Error::throwError(DatumP(new Word("PAUSE")), nothing);
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


void QLogoController::processInputMessageQueue()
{
    while (messageQueue.isMessageAvailable()) {
        getMessage();
    }
}


void QLogoController::waitForMessage(message_t expectedType)
{
    message_t type;
    do {
        type = getMessage();
    } while (type != expectedType);
}


void QLogoController::printToConsole(const QString &s)
{
    if (writeStream == NULL) {
        sendMessage([&](QDataStream *out) {
          *out << (message_t)C_CONSOLE_PRINT_STRING << s;
        });

      if (dribbleStream)
        *dribbleStream << s;
    } else {
      *writeStream << s;
    }
}


QString QLogoController::addStandoutToString(const QString src) {
  QString retval = escapeString + src + escapeString;
  return retval;
}


void QLogoController::clearScreenText()
{
    sendMessage([&](QDataStream *out) {
        *out << (message_t)C_CANVAS_CLEAR_SCREEN_TEXT;
    });
}

void QLogoController::getTextCursorPos(int &row, int &col)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CONSOLE_TEXT_CURSOR_POS;
    });

    waitForMessage(C_CONSOLE_TEXT_CURSOR_POS);
    row = cursorRow;
    col = cursorCol;
}


void QLogoController::setTextCursorPos(int row, int col)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CONSOLE_SET_TEXT_CURSOR_POS
           << row
           << col;
    });
}


void QLogoController::setTextColor(const QColor foregroundColor, const QColor backgroundColor)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CONSOLE_SET_TEXT_COLOR
           << foregroundColor
           << backgroundColor;
    });
}

void QLogoController::setCursorOverwriteMode(bool isOverwriteMode)
{
    cursoreModeIsOverwrite = isOverwriteMode;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CONSOLE_SET_CURSOR_MODE
           << isOverwriteMode;
    });
}

bool QLogoController::cursorOverwriteMode()
{
    return cursoreModeIsOverwrite;
}


const QString QLogoController::editText(const QString startText)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CONSOLE_BEGIN_EDIT_TEXT << startText;
    });

    waitForMessage(C_CONSOLE_END_EDIT_TEXT);

    return editorText;
}

void QLogoController::setTextFontName(const QString aFontName)
{
    if (textFontName == aFontName)
        return;
    // TODO: Validate font name
    textFontName = aFontName;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CONSOLE_SET_FONT_NAME << textFontName;
    });
}

void QLogoController::setTextFontSize(double aSize)
{
    if (textFontSize == aSize)
        return;
    textFontSize = aSize;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CONSOLE_SET_FONT_SIZE << textFontSize;
    });
}

double QLogoController::getTextFontSize()
{
    return textFontSize;
}

const QString QLogoController::getTextFontName()
{
    return textFontName;
}

// TODO: I believe this is only called if the input readStream is NULL
DatumP QLogoController::readRawlineWithPrompt(const QString prompt)
{
    if (dribbleStream)
      *dribbleStream << prompt;

  sendMessage([&](QDataStream *out) {
    *out << (message_t)C_CONSOLE_REQUEST_LINE
         << prompt;
  });
  waitForMessage(C_CONSOLE_RAWLINE_READ);

  return DatumP(new Word(rawLine));
}


DatumP QLogoController::readchar()
{
  sendMessage([&](QDataStream *out) {
    *out << (message_t)C_CONSOLE_REQUEST_CHAR;
  });

  waitForMessage(C_CONSOLE_CHAR_READ);

  return DatumP(new Word(rawChar));
}

void QLogoController::setTurtlePos(const QMatrix4x4 &newTurtlePos)
{
  sendMessage([&](QDataStream *out) {
    *out << (message_t)C_CANVAS_UPDATE_TURTLE_POS << newTurtlePos;
  });
}

void QLogoController::setPenmode(PenModeEnum aMode)
{
    if (aMode == currentPenmode)
        return;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_PENMODE << aMode;
    });
}


void QLogoController::setScreenMode(ScreenModeEnum newMode)
{
    screenMode = newMode;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)W_SET_SCREENMODE << newMode;
    });
}

ScreenModeEnum QLogoController::getScreenMode()
{
    return screenMode;
}

void QLogoController::setIsCanvasBounded(bool aIsBounded)
{
    if (canvasIsBounded == aIsBounded)
        return;
    canvasIsBounded = aIsBounded;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_IS_BOUNDED << aIsBounded;
    });
}

bool QLogoController::isCanvasBounded()
{
    return canvasIsBounded;
}

void QLogoController::setTurtleIsVisible(bool isVisible)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_TURTLE_IS_VISIBLE << isVisible;
    });
}

void QLogoController::drawLine(const QVector3D &start, const QVector3D &end, const QColor &startColor, const QColor &endColor)
{
  sendMessage([&](QDataStream *out) {
    *out << (message_t)C_CANVAS_DRAW_LINE
         << start
         << end
         << startColor
         << endColor;
  });
}

void QLogoController::drawPolygon(const QList<QVector3D> &points, const QList<QColor> &colors)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_DRAW_POLYGON
           << points
           << colors;
    });
}

void QLogoController::drawLabel(const QString &aString, const QVector3D &aPosition, const QColor &aColor)
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_DRAW_LABEL
           << aString
           << aPosition
           << aColor;
    });
}

void QLogoController::setLabelFontName(const QString &aName)
{
    if (aName == labelFontName)
        return;
    labelFontName = aName;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_FONT_NAME
           << aName;
    });
}

void QLogoController::setLabelFontSize(double aSize)
{
    if (aSize == labelFontSize)
        return;
    labelFontSize = aSize;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_FONT_SIZE
           << labelFontSize;
    });
}

const QString QLogoController::getLabelFontName()
{
    return labelFontName;
}

double QLogoController::getLabelFontSize()
{
    return labelFontSize;
}

void QLogoController::setCanvasBackgroundColor(QColor aColor)
{
    currentBackgroundColor = aColor;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_BACKGROUND_COLOR
           << aColor;
    });
}

QColor QLogoController::getCanvasBackgroundColor(void)
{
    return currentBackgroundColor;
}

void QLogoController::clearScreen()
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_CLEAR_SCREEN;
    });
}

QImage QLogoController::getCanvasImage()
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_GET_IMAGE;
    });

    waitForMessage(C_CANVAS_GET_IMAGE);

    return canvasImage;
}

bool QLogoController::getIsMouseButtonDown()
{
    processInputMessageQueue();
    return isMouseButtonDown;
}

QVector2D QLogoController::lastMouseclickPosition()
{
    processInputMessageQueue();
    return clickPos;
}


int QLogoController::getAndResetButtonID()
{
    processInputMessageQueue();
    int retval = lastButtonpressID;
    lastButtonpressID = 0;
    return retval;
}


QVector2D QLogoController::mousePosition()
{
    processInputMessageQueue();
    return mousePos;
}


void QLogoController::setBounds(double x, double y)
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

void QLogoController::setPensize(double aSize)
{
    if (aSize == penSize)
        return;
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_PENSIZE
           << aSize;
    });
    penSize = aSize;
}

void QLogoController::mwait(unsigned long msecs) {
  QThread::msleep(msecs);
}

