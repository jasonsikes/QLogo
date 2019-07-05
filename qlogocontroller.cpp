#include "qlogocontroller.h"
#include <QMessageBox>

QLogoController::QLogoController(QObject *parent) : Controller(parent)
{
    guiIn.open(stdin, QFile::ReadOnly);
    guiOut.open(stdout, QFile::WriteOnly);
    guiInstream.setDevice(&guiIn);
}


QLogoController::~QLogoController()
{
    setDribble("");

}


message_t QLogoController::getMessage()
{
    message_t retval;

    guiInstream >> retval;

    switch (retval) {
    case C_CONSOLE_RAWLINE_READ:
        guiInstream >> rawLine;
        break;
    default:
        break;
    }
    return retval;
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
        QByteArray buffer;
        QDataStream out(&buffer, QIODevice::WriteOnly);
        out << (message_t)C_CONSOLE_PRINT_STRING << s;
        guiOut.write(buffer.constData(), buffer.size());

      if (dribbleStream)
        *dribbleStream << s;
    } else {
      *writeStream << s;
    }
}

// TODO: I believe this is only called if the input readStream is NULL
DatumP QLogoController::readRawlineWithPrompt(const QString &prompt)
{
    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);
    out << (message_t)C_CONSOLE_PRINT_STRING << prompt;
    out << (message_t)C_CONSOLE_REQUEST_LINE;
    guiOut.write(buffer.constData(), buffer.size());
    guiOut.flush();
  if (dribbleStream)
    *dribbleStream << prompt;

  waitForMessage(C_CONSOLE_RAWLINE_READ);

  return DatumP(new Word(rawLine));
}
