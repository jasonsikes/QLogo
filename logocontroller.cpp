#include "logocontroller.h"
#include <QIODevice>

LogoController::LogoController(QObject *parent): Controller(parent)
{
    inStream = new QTextStream(stdin, QIODevice::ReadOnly);
    outStream = new QTextStream(stdout, QIODevice::WriteOnly);

}


LogoController::~LogoController()
{
    setDribble("");
    delete inStream;
    delete outStream;
}


void LogoController::printToConsole(const QString &s) {
  if (writeStream == NULL) {
    *outStream << s;
    if (dribbleStream)
      *dribbleStream << s;
  } else {
    *writeStream << s;
  }
}

bool LogoController::atEnd() { return inStream->atEnd(); }

bool LogoController::keyQueueHasChars() { return !inStream->atEnd(); }

// This is READRAWLINE
DatumP LogoController::readRawlineWithPrompt(const QString &prompt) {
  QTextStream *stream = (readStream == NULL) ? inStream : readStream;
  if (stream->atEnd())
    return nothing;
  printToConsole(prompt);
  outStream->flush();
  QString inputText = stream->readLine();
  if (dribbleStream)
      *dribbleStream << inputText <<"\n";
  DatumP retval = DatumP(new Word(inputText));

  return retval;
}

// This is READCHAR
DatumP LogoController::readchar() {
  QChar c;
  outStream->flush();
  QTextStream *stream = (readStream == NULL) ? inStream : readStream;
  if (stream->atEnd())
    return nothing;
  *stream >> c;
  DatumP retval = DatumP(new Word(c));
  return retval;
}

void LogoController::mwait(unsigned long msecs) {
  outStream->flush();
  QThread::msleep(msecs);
}
