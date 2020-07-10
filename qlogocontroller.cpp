#include "qlogocontroller.h"
#include <QMessageBox>
#include <QByteArray>
#include <QDataStream>
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


QLogoController::~QLogoController()
{
    setDribble("");

}

/* a message has three parts:
 * 1. A quint detailing how many bytes are in the remainder of the message (datalen).
 * 2. An enum describing the type of data (header).
 * 3. The data (varies).
 */
message_t QLogoController::getMessage()
{
    qint64 datalen;
    qint64 dataread;
    message_t header;
    do {
        dataread = read(STDIN_FILENO, &datalen, sizeof(qint64));
        if (dataread == 0) {
            QThread::msleep(100);
        }
    } while(dataread == 0);
    Q_ASSERT(dataread == sizeof(qint64));
    QByteArray buffer;
    buffer.resize(datalen);
    dataread = read(STDIN_FILENO, buffer.data(), datalen);
    Q_ASSERT(dataread == datalen);
    QDataStream bufferStream(&buffer, QIODevice::ReadOnly);

    bufferStream >> header;

    switch (header) {
    case W_ZERO:
        qDebug() <<"ZERO!";
        break;
    case C_CONSOLE_RAWLINE_READ:
        bufferStream >> rawLine;
        break;
    case C_CONSOLE_CHAR_READ:
        bufferStream >> rawChar;
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

void QLogoController::drawLine(const QVector4D &start, const QVector4D &end, const QColor &color)
{
  QVector3D s = start.toVector3DAffine();
  QVector3D e = end.toVector3DAffine();

  sendMessage([&](QDataStream *out) {
    *out << (message_t)C_CANVAS_DRAW_LINE
         << s
         << e
         << color;
  });
}

void QLogoController::clearScreen()
{
    sendMessage([&](QDataStream *out) {
      *out << (message_t)C_CANVAS_CLEAR_SCREEN;
    });
}
