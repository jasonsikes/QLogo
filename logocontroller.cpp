#include "logocontroller.h"
#include <QIODevice>
#include "kernel.h"
#include <QApplication>
#include <signal.h>

LogoController *_maincontroller = NULL;
qreal initialBoundXY = 150;


SignalsEnum_t lastSignal = noSignal;

#ifdef _WIN32

static void initSignals()
{
    //TODO: I need to find out how to handle keyboard interrupts in Windows
}

static void restoreSignals()
{

}

#else

// Not a part of LogoController because we are handling interrupts
static void handle_signal(int sig)
{
    switch(sig) {
    case SIGINT:
        lastSignal = toplevelSignal;
    break;
    case SIGTSTP:
        lastSignal = pauseSignal;
    break;
    case SIGQUIT:
        lastSignal = systemSignal;
        break;
    default:
        qDebug() <<"Not expecting signal: " <<sig;
    }
}

static void initSignals()
{
    signal(SIGINT, handle_signal);  // TOPLEVEL
    signal(SIGTSTP, handle_signal); // PAUSE
    signal(SIGQUIT, handle_signal); // SYSTEM
}

static void restoreSignals()
{
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
}


#endif



LogoController *mainController() {
  Q_ASSERT(_maincontroller != NULL);
  return _maincontroller;
}

LogoController::LogoController(QObject *parent)
{
    Q_ASSERT(_maincontroller == NULL);
    readStream = NULL;
    writeStream = NULL;
    dribbleStream = NULL;
    _maincontroller = this;
    kernel = new Kernel;

    inStream = new QTextStream(stdin, QIODevice::ReadOnly);
    outStream = new QTextStream(stdout, QIODevice::WriteOnly);

}


LogoController::~LogoController()
{
    setDribble("");
    delete inStream;
    delete outStream;
    delete kernel;
    _maincontroller = NULL;
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
DatumPtr LogoController::readRawlineWithPrompt(const QString prompt) {
  QTextStream *stream = (readStream == NULL) ? inStream : readStream;
  if (stream->atEnd())
    return nothing;
  printToConsole(prompt);
  outStream->flush();
  QString inputText = stream->readLine();
  if (dribbleStream)
      *dribbleStream << inputText <<'\n';

  return DatumPtr(inputText);
}

// This is READCHAR
DatumPtr LogoController::readchar() {
  QChar c;
  outStream->flush();
  QTextStream *stream = (readStream == NULL) ? inStream : readStream;
  if (stream->atEnd())
    return nothing;
  *stream >> c;
  QString retval = c;
  DatumPtr retvalP = DatumPtr(retval);
  return retvalP;
}

void LogoController::mwait(unsigned long msecs) {
  outStream->flush();
  QThread::msleep(msecs);
}

bool LogoController::setDribble(const QString &filePath) {
  if (filePath == "") {
    if (dribbleStream) {
      QIODevice *file = dribbleStream->device();
      dribbleStream->flush();
      delete dribbleStream;
      file->close();
      delete file;
    }
    dribbleStream = NULL;
    return true;
  }
  QFile *file = new QFile(filePath);
  if (!file->open(QIODevice::Append))
    return false;

  dribbleStream = new QTextStream(file);
  return true;
}

bool LogoController::isDribbling() { return dribbleStream != NULL; }

SignalsEnum_t LogoController::latestSignal()
{
    SignalsEnum_t retval = lastSignal;
    lastSignal = noSignal;
    return retval;
}


int LogoController::run(void) {
  kernel->initLibrary();
  initialize();

  initSignals();

  bool shouldContinue = true;
  while (shouldContinue) {
    shouldContinue = kernel->getLineAndRunIt();
  }

  restoreSignals();

  return 0;
}

void LogoController::systemStop()
{
    QApplication::quit();
}

