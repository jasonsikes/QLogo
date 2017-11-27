
//===-- qlogo/qlogo_controller.cpp - Controller class implementation -------*-
// C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// QLogo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with QLogo.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the Controller class, which is the
/// main event dispatcher between the Kernel thread and the UI thread.
///
//===----------------------------------------------------------------------===//

#include "qlogo_controller.h"

#include "editorwindow.h"
#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QTextStream>
#include <QVector4D>
#include <QVector>
#include <QtGui/QOpenGLFunctions>

#include "canvas.h"
#include "console.h"
#include "error.h"
#include "kernel.h"

// For rand()
#include <stdlib.h>

Controller *_maincontroller = NULL;
qreal initialBoundX = 350;
qreal initialBoundY = 150;

const QString escapeChar = QChar(htmlEscapeChar);
const double startingTextSize = 10;
const QString startingFont = "Courier New";

Controller *mainController() {
  Q_ASSERT(_maincontroller != NULL);
  return _maincontroller;
}

Controller::Controller(QObject *parent) : QThread(parent) {
  Q_ASSERT(_maincontroller == NULL);
  _maincontroller = this;
  dribbleStream = NULL;
  boundX = initialBoundX;
  boundY = initialBoundY;

  kernel = new Kernel;

  qRegisterMetaType<QList<QVector4D>>("QList<QVector4D>");
  qRegisterMetaType<QList<QVector4D>>("QList<QColor>");
  qRegisterMetaType<PenModeEnum>("PenModeEnum");

  connect(this, SIGNAL(getCanvasImageSignal(QImage *)), this,
          SLOT(getCanvasImageSlot(QImage *)), Qt::BlockingQueuedConnection);
  connect(this, SIGNAL(setSplitterSizesSignal(float, float)), this,
          SLOT(setSplitterSizesSlot(float, float)), Qt::QueuedConnection);
  connect(this, SIGNAL(openEditorWindowSignal(QString *)), this,
          SLOT(openEditorWindowSlot(QString *)), Qt::BlockingQueuedConnection);
  connect(this, SIGNAL(addLabelSignal(QString, QVector4D, QColor, QFont)), this,
          SLOT(addLabelSlot(QString, QVector4D, QColor, QFont)),
          Qt::QueuedConnection);
  connect(this, SIGNAL(drawLineSignal(QVector4D, QVector4D, QColor)), this,
          SLOT(drawLineSlot(QVector4D, QVector4D, QColor)),
          Qt::QueuedConnection);
  connect(this, SIGNAL(addPolygonSignal(QList<QVector4D>, QList<QColor>)), this,
          SLOT(addPolygonSlot(QList<QVector4D>, QList<QColor>)),
          Qt::QueuedConnection);
  connect(this, SIGNAL(setBoundsSignal(qreal, qreal)), this,
          SLOT(setBoundsSlot(qreal, qreal)), Qt::QueuedConnection);
  connect(this, SIGNAL(clearScreenSignal()), this, SLOT(clearScreenSlot()),
          Qt::QueuedConnection);
  connect(this, SIGNAL(setCanvasBackgroundColorSignal(QColor)), this,
          SLOT(setCanvasBackgroundColorSlot(QColor)), Qt::QueuedConnection);
  connect(this, SIGNAL(setPenmodeSignal(PenModeEnum)), this,
          SLOT(setPenmodeSlot(PenModeEnum)), Qt::QueuedConnection);
  connect(this, SIGNAL(setPensizeSignal(double)), this,
          SLOT(setPensizeSlot(double)), Qt::QueuedConnection);
  connect(this, SIGNAL(setIsCanvasBoundedSignal(bool)), this,
          SLOT(setIsCanvasBoundedSlot(bool)), Qt::QueuedConnection);
  connect(this, SIGNAL(updateCanvasSignal()), this, SLOT(updateCanvasSlot()),
          Qt::QueuedConnection);
  connect(this, SIGNAL(setCursorOverwriteModeSignal(bool)), this,
          SLOT(setCursorOverwriteModeSlot(bool)), Qt::QueuedConnection);

  connect(this, SIGNAL(printToScreenSignal(QString)), this,
          SLOT(printToScreenSlot(QString)), Qt::QueuedConnection);
  connect(this, SIGNAL(getTextCursorPosSignal(int &, int &)), this,
          SLOT(getTextCursorPosSlot(int &, int &)),
          Qt::BlockingQueuedConnection);

  connect(this, SIGNAL(requestCharacterSignal()), this,
          SLOT(requestCharacterSlot()), Qt::QueuedConnection);
  connect(this, SIGNAL(requestLineWithPromptSignal(QString)), this,
          SLOT(requestLineWithPromptSlot(QString)), Qt::QueuedConnection);

  setTextSize(startingTextSize);
  setFontName(startingFont);
  setTextColor(QColor("black"), QColor("white"));
}

Controller::~Controller() {
  setDribble("");
  delete kernel;
  _maincontroller = NULL;
}

void Controller::setMainWindow(MainWindow *w) { mainWindow = w; }

bool Controller::setDribble(const QString &filePath) {
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

bool Controller::isDribbling() { return dribbleStream != NULL; }

void Controller::setScreenMode(ScreenModeEnum newMode) {
  screenMode = newMode;
  switch (screenMode) {
  case textScreenMode:
    setSplitterSizeRatios(0, 1);
    break;
  case fullScreenMode:
    setSplitterSizeRatios(.85f, .15f);
    break;
  case splitScreenMode:
    setSplitterSizeRatios(.5, .5);
    break;
  default:
    break;
  }
}

ScreenModeEnum Controller::getScreenMode() { return screenMode; }

const QString *Controller::editText(QString *text) {
  openEditorWindowSignal(text);

  threadMutex.lock();
  condition.wait(&threadMutex);
  threadMutex.unlock();

  return editorText;
}

void Controller::drawLabel(const QString &aText, const QVector4D &aLocation,
                           const QColor &aColor, const QFont &aFont) {
  addLabelSignal(aText, aLocation, aColor, aFont);
}

void Controller::addLabelSlot(const QString &aText, const QVector4D &aLocation,
                              const QColor &aColor, const QFont &aFont) {
  introduceCanvasIfItHasntBeenAlready();
  mainWindow->mainCanvas()->addLabel(aText, aLocation, aColor, aFont);
}

void Controller::drawLine(const QVector4D &vertexA, const QVector4D &vertexB,
                          const QColor &color) {
  drawLineSignal(vertexA, vertexB, color);
}

void Controller::drawLineSlot(const QVector4D &vertexA,
                              const QVector4D &vertexB, const QColor &color) {
  introduceCanvasIfItHasntBeenAlready();
  mainWindow->mainCanvas()->addLine(vertexA, vertexB, color);
}

void Controller::drawPolygon(const QList<QVector4D> &vertices,
                             const QList<QColor> &colors) {
  addPolygonSignal(vertices, colors);
}

void Controller::addPolygonSlot(const QList<QVector4D> &vertices,
                                const QList<QColor> &colors) {
  introduceCanvasIfItHasntBeenAlready();
  mainWindow->mainCanvas()->addPolygon(vertices, colors);
}

void Controller::setBounds(qreal x, qreal y) {
  boundX = x;
  boundY = y;
  setBoundsSignal(x, y);
}

void Controller::setBoundsSlot(qreal x, qreal y) {
  mainWindow->mainCanvas()->setBounds(x, y);
}

void Controller::getBounds(qreal &x, qreal &y) {
  x = boundX;
  y = boundY;
}

QImage Controller::getCanvasImage() {
  QImage retval;
  getCanvasImageSignal(&retval);
  return retval;
}

void Controller::getCanvasImageSlot(QImage *anImage) {
  *anImage = mainWindow->mainCanvas()->getImage();
}

void Controller::clearScreen() { clearScreenSignal(); }

void Controller::clearScreenSlot() { mainWindow->mainCanvas()->clearScreen(); }

void Controller::setCanvasBackgroundColor(const QColor &c) {
  setCanvasBackgroundColorSignal(c);
  currentBackgroundColor = c;
}

void Controller::setCanvasBackgroundColorSlot(const QColor &c) {
  mainWindow->mainCanvas()->setBackgroundColor(c);
}

void Controller::updateCanvas() { updateCanvasSignal(); }

void Controller::updateCanvasSlot() { mainWindow->mainCanvas()->update(); }

const QColor &Controller::getCanvasBackgroundColor(void) {
  return currentBackgroundColor;
}

QString Controller::addStandoutToString(const QString &src) {
  return QString(escapeChar + C_STANDOUT + escapeChar + src + escapeChar +
                 C_STANDOUT + escapeChar);
}

void Controller::printToConsole(const QString &s) {
  if (dribbleStream)
    *dribbleStream << s;
  printToScreenSignal(s);
}

void Controller::setTextCursorPos(int row, int col) {
  mainController()->printToConsole(escapeChar + C_SET_CURSOR_POS +
                                   QString::number(row) + C_DELIM +
                                   QString::number(col) + escapeChar);
}

void Controller::setTextSize(double newSize) {
  currentTextSize = newSize;
  mainController()->printToConsole(escapeChar + C_SET_TEXT_SIZE +
                                   QString::number(newSize) + escapeChar);
}

void Controller::setFontName(const QString &aName) {
  currentFontName = aName;
  mainController()->printToConsole(escapeChar + C_SET_FONT + aName +
                                   escapeChar);
}

const QString Controller::getFontName() { return currentFontName; }

QStringList Controller::getAllFontNames() {
  QFontDatabase fdb;
  return fdb.families();
}

double Controller::getTextSize() { return currentTextSize; }

void Controller::getTextCursorPos(int &row, int &col) {
  getTextCursorPosSignal(row, col);
}

void Controller::setTextColor(const QColor &foreground,
                              const QColor &background) {
  mainController()->printToConsole(
      escapeChar + C_SET_TEXT_COLOR + foreground.name(QColor::HexArgb) +
      C_DELIM + background.name(QColor::HexArgb) + escapeChar);
}

void Controller::getTextCursorPosSlot(int &row, int &col) {
  mainWindow->mainConsole()->getCursorPos(row, col);
}

void Controller::printToScreenSlot(const QString &text) {
  mainWindow->mainConsole()->printString(text);
}

void Controller::requestCharacterSlot() {
  mainWindow->mainConsole()->requestCharacter();
}

void Controller::requestLineWithPromptSlot(const QString &line) {
  mainWindow->mainConsole()->requestLineWithPrompt(line);
}

void Controller::openEditorWindowSlot(QString *text) {
  if (editWindow == NULL) {
    editWindow = new EditorWindow;

    connect(editWindow, SIGNAL(editingHasEndedSignal(const QString *)), this,
            SLOT(editingHasEndedSlot(const QString *)));
  }

  editWindow->setTextFormat(mainWindow->mainConsole()->textFormat);
  editWindow->setContents(text);
  editWindow->show();
  editWindow->activateWindow();
  editWindow->setFocus();
}

void Controller::editingHasEndedSlot(const QString *text) {
  editorText = text;

  threadMutex.lock();
  condition.wakeOne();
  threadMutex.unlock();
}

bool Controller::atEnd() {
  // never done
  return false;
}

void Controller::clearScreenText() {
  mainController()->printToConsole(escapeChar + C_CLEAR_TEXT + escapeChar);
}

bool Controller::keyQueueHasChars() { return mainWindow->consoleHasChars(); }

void Controller::setSplitterSizeRatios(float canvasRatio, float consoleRatio) {
  setSplitterSizesSignal(canvasRatio, consoleRatio);
}

void Controller::setSplitterSizesSlot(float canvasRatio, float consoleRatio) {
  mainWindow->setSplitterSizeRatios(canvasRatio, consoleRatio);
  if (canvasRatio > 0)
    hasCanvasShown = true;
}

void Controller::splitterMoved(int, int) { hasCanvasShown = true; }

void Controller::introduceCanvasIfItHasntBeenAlready() {
  if (!hasCanvasShown) {
    setSplitterSizesSlot(.7f, .3f);
  }
}

void Controller::beginInputHistory() { history = DatumP(new List); }

DatumP Controller::inputHistory() { return history; }

// This is READRAWLINE
DatumP Controller::readrawlineWithPrompt(const QString &prompt) {
  shouldQueueEvents = false;
  requestLineWithPromptSignal(prompt);
  if (dribbleStream)
    *dribbleStream << prompt;
  threadMutex.lock();
  condition.wait(&threadMutex);
  threadMutex.unlock();

  uiInputTextMutex.lock();
  DatumP retval = DatumP(new Word(uiInputText));
  uiInputTextMutex.unlock();

  shouldQueueEvents = true;
  QString s = retval.wordValue()->rawValue();
  if (s.size() == 1) {
    ushort u = s[0].unicode();
    if (u == toplevelCode) {
      return toplevelTokenP;
    }
    if (u == pauseCode) {
      return pauseTokenP;
    }
  }
  history.listValue()->append(retval);
  return retval;
}

// This is READCHAR
DatumP Controller::readchar() {
  //    if (readStream == NULL) {
  shouldQueueEvents = false;
  requestCharacterSignal();
  threadMutex.lock();
  condition.wait(&threadMutex);
  threadMutex.unlock();

  uiInputTextMutex.lock();
  DatumP retval = DatumP(new Word(uiInputText));
  uiInputTextMutex.unlock();
  shouldQueueEvents = true;
  QString s = retval.wordValue()->rawValue();
  if (s.size() == 1) {
    ushort u = s[0].unicode();
    if (u == toplevelCode) {
      return toplevelTokenP;
    }
    if (u == pauseCode) {
      return pauseTokenP;
    }
  }
  return retval;
}

void Controller::run() {
  beginInputHistory();
  kernel->initLibrary();
  bool shouldContinue = true;
  while (shouldContinue) {

    shouldContinue = kernel->getLineAndRunIt();
  }
}

void Controller::mwait(unsigned long msecs) { QThread::msleep(msecs); }

void Controller::receiveString(const QString &s) {
  uiInputTextMutex.lock();
  uiInputText = s;
  uiInputTextMutex.unlock();

  if (dribbleStream)
    *dribbleStream << s << "\n";

  // Wake up the thread since it was likely sleeping
  threadMutex.lock();
  condition.wakeOne();
  threadMutex.unlock();
}

void Controller::setPenmode(PenModeEnum newMode) { setPenmodeSignal(newMode); }

void Controller::setPenmodeSlot(PenModeEnum newMode) {
  mainWindow->mainCanvas()->setPenmode(newMode);
}

void Controller::setCursorOverwriteMode(bool shouldOverwrite) {
  setCursorOverwriteModeSignal(shouldOverwrite);
}

void Controller::setCursorOverwriteModeSlot(bool isOverwrite) {
  mainWindow->mainConsole()->setOverwriteMode(isOverwrite);
}

void Controller::setPensize(double aSize) {
  setPensizeSignal(aSize);
  currentPenSize = aSize;
}

void Controller::setPensizeSlot(double aSize) {
  mainWindow->mainCanvas()->setPensize(aSize);
}

bool Controller::isPenSizeValid(GLfloat aSize) {
  return mainWindow->mainCanvas()->isPenSizeValid(aSize);
}

void Controller::setIsCanvasBounded(bool aIsCanvasBounded) {
  setIsCanvasBoundedSignal(aIsCanvasBounded);
}

void Controller::setIsCanvasBoundedSlot(bool aIsCanvasBounded) {
  mainWindow->mainCanvas()->setIsBounded(aIsCanvasBounded);
}

bool Controller::getIsMouseButtonDown() { return isMouseButtonDown; }

void Controller::setIsMouseButtonDown(bool aIsMouseButtonDown) {
  isMouseButtonDown = aIsMouseButtonDown;
  if (isMouseButtonDown && shouldQueueEvents) {
    addEventToQueue(mouseEvent);
  }
}

int Controller::getButton() {
  int retval = button;
  button = 0;
  return retval;
}

void Controller::setButton(int aButton) { button = aButton; }

bool Controller::eventQueueIsEmpty() { return eventQueueEmpty; }

void Controller::clearEventQueue() {
  QMutexLocker locker(&eventQueueMutex);
  eventQueue.clear();
  eventQueueEmpty = true;
}

void Controller::addEventToQueue(char eventChar) {
  QMutexLocker locker(&eventQueueMutex);
  eventQueue.push_back(eventChar);
  eventQueueEmpty = false;
}

char Controller::nextQueueEvent() {
  char retval;
  QMutexLocker locker(&eventQueueMutex);
  if (eventQueue.isEmpty()) {
    retval = ' ';
  } else {
    retval = eventQueue.front();
    eventQueue.pop_front();
  }
  eventQueueEmpty = eventQueue.isEmpty();
  return retval;
}
