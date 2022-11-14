#include "qlogocontroller.h"
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


QLogoController::QLogoController(QObject *parent) : Controller(parent)
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
 * 1. A quint detailing how many bytes are in the remainder of the message (datalen).
 * 2. An enum describing the type of data (header).
 * 3. The data (varies).
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
    default:
        qDebug() <<"I don't know how I got " << header;
        break;
    }
    return header;
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
DatumP QLogoController::readRawlineWithPrompt(const QString &prompt)
{
  sendMessage([&](QDataStream *out) {
    *out << (message_t)C_CONSOLE_PRINT_STRING << prompt;
  });
  sendMessage([&](QDataStream *out) {
    *out << (message_t)C_CONSOLE_REQUEST_LINE;
  });
  if (dribbleStream)
    *dribbleStream << prompt;

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
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_SET_BACKGROUND_COLOR
           << aColor;
    });
}

void QLogoController::clearScreen()
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_CLEAR_SCREEN;
    });
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

