#ifndef CONTROLLER_H
#define CONTROLLER_H

//===-- qlogo/qlogo_controller.h - Controller class definition -------*- C++
//-*-===//
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
/// This file contains the declaration of the Controller class, which is the
/// main event dispatcher between the Kernel thread and the UI thread.
///
//===----------------------------------------------------------------------===//

class MainWindow;
class EditorWindow;

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include "canvas.h"
#include "datum.h"
#include <QFontDatabase>
#include <QKeyEvent>

// This is the chr code to begin and end escape sequences
const ushort htmlEscapeChar = 27;

extern qreal initialBoundX;
extern qreal initialBoundY;

const char characterEvent = 'c';
const char mouseEvent = 'm';
const char systemEvent = 's';   // window close
const char pauseEvent = 'p';    // ctrl-W
const char toplevelEvent = 't'; // ctrl-Q

extern const QString escapeChar;
extern const QString pauseString;
extern const QString toplevelString;
extern const QString systemString;

#define C_CLEAR_TEXT 'E'
#define C_SET_CURSOR_POS 'A'
#define C_SET_TEXT_COLOR 'C'
#define C_SET_TEXT_SIZE 'T'
#define C_SET_FONT 'F'
#define C_STANDOUT 'S'
#define C_DELIM ';'

class Kernel;
class QTextStream;

enum ScreenModeEnum {
  initScreenMode,
  textScreenMode,
  fullScreenMode,
  splitScreenMode
};

class Controller : public QThread {
  Q_OBJECT

public:
  Controller(QObject *parent = 0);
  ~Controller();
  //void setMainWindow(MainWindow *w);
  const QString *editText(QString *text);
  void halt() {}
  bool atEnd();
  bool keyQueueHasChars();
  DatumP readrawlineWithPrompt(const QString &prompt);
  DatumP readchar();
  void setTextCursorPos(int row, int col);
  void getTextCursorPos(int &row, int &col);
  void setTextColor(const QColor &foreground, const QColor &background);
  void printToConsole(const QString &s);
  void receiveString(const QString &line);
  bool setDribble(const QString &filePath);
  bool isDribbling();
  void drawLabel(const QString &aText, const QVector4D &aLocation,
                 const QColor &aColor, const QFont &aFont);
  void drawLine(const QVector4D &vertexA, const QVector4D &vertexB,
                const QColor &color);
  void drawPolygon(const QList<QVector4D> &vertices,
                   const QList<QColor> &colors);
  void setBounds(qreal x, qreal y);
  void getBounds(qreal &x, qreal &y);
  void clearScreen();
  void clearScreenText();
  void setTextSize(double newSize);
  double getTextSize();
  void setFontName(const QString &aName);
  const QString getFontName();
  QStringList getAllFontNames();
  void updateCanvas(void);
  void mwait(unsigned long msecs);
  void setCanvasBackgroundColor(const QColor &c);
  const QColor &getCanvasBackgroundColor(void);
  QImage getCanvasImage(void);
  void setIsCanvasBounded(bool aIsCanvasBounded);
  void setScreenMode(ScreenModeEnum newMode);
  ScreenModeEnum getScreenMode();
  void setSplitterSizeRatios(float canvasRatio, float consoleRatio);
  void setCursorOverwriteMode(bool shouldOverwrite);

  void setPenmode(PenModeEnum newMode);

  void setPensize(double aSize);
  bool isPenSizeValid(GLfloat aSize);

  QVector2D mousePos;
  QVector2D clickPos;

  bool getIsMouseButtonDown();
  int getButton();
  void setButton(int aButton);
  void setIsMouseButtonDown(bool aIsMouseButtonDown);
  QString addStandoutToString(const QString &src);

  void clearEventQueue();
  bool eventQueueIsEmpty();
  char nextQueueEvent();
  void addEventToQueue(char eventChar);
  void shutdownEvent();

public slots:

  // Canvas
  void getCanvasImageSlot(QImage *anImage);
  void updateCanvasSlot();
  void drawLineSlot(const QVector4D &vertexA, const QVector4D &vertexB,
                    const QColor &color);
  void addLabelSlot(const QString &aText, const QVector4D &aLocation,
                    const QColor &aColor, const QFont &aFont);
  void addPolygonSlot(const QList<QVector4D> &vertices,
                      const QList<QColor> &colors);
  void setBoundsSlot(qreal x, qreal y);
  void clearScreenSlot();
  void setCanvasBackgroundColorSlot(const QColor &c);
  void setPenmodeSlot(PenModeEnum newMode);
  void setPensizeSlot(double aSize);
  void setIsCanvasBoundedSlot(bool aIsCanvasBounded);

  // MainWindow
  void setSplitterSizesSlot(float canvasRatio, float consoleRatio);
  void splitterMoved(int, int);

  // EditorWindow
  void openEditorWindowSlot(QString *text);
  void
  editingHasEndedSlot(const QString *text); // NULL means text was "reverted"

  // Console
  void printToScreenSlot(const QString &text);
  void requestCharacterSlot();
  void requestLineWithPromptSlot(const QString &line);
  void getTextCursorPosSlot(int &row, int &col);
  void setCursorOverwriteModeSlot(bool isOverwrite);

signals:

  // Canvas
  void getCanvasImageSignal(QImage *anImage);
  void updateCanvasSignal();
  void drawLineSignal(const QVector4D &vertexA, const QVector4D &vertexB,
                      const QColor &color);
  void addLabelSignal(const QString &aText, const QVector4D &aLocation,
                      const QColor &aColor, const QFont &aFont);
  void addPolygonSignal(const QList<QVector4D> &vertices,
                        const QList<QColor> &colors);
  void setBoundsSignal(qreal x, qreal y);
  void clearScreenSignal();
  void setCanvasBackgroundColorSignal(const QColor &c);
  void setPenmodeSignal(PenModeEnum newMode);
  void setPensizeSignal(double aSize);
  void setIsCanvasBoundedSignal(bool aIsCanvasBounded);

  // MainWindow
  void setSplitterSizesSignal(float canvasRatio, float consoleRatio);
  void openEditorWindowSignal(QString *text);

  // Console
  void printToScreenSignal(const QString &text);
  void requestCharacterSignal(void);
  void requestLineWithPromptSignal(const QString &line);
  void getTextCursorPosSignal(int &row, int &col);
  void setCursorOverwriteModeSignal(bool isOverwrite);

protected:
  QString currentPenMode = "paint";
  GLfloat currentPenSize;
  qreal boundX;
  qreal boundY;
  QColor currentBackgroundColor = QColor("black");
  bool hasCanvasShown = false;
  void introduceCanvasIfItHasntBeenAlready();

  ScreenModeEnum screenMode = initScreenMode;

  MainWindow *mainWindow;
  EditorWindow *editWindow = NULL;
  const QString *editorText;

  Kernel *kernel;

  QString uiInputText;
  QMutex uiInputTextMutex;

  double currentTextSize;
  QString currentFontName;

  void run() Q_DECL_OVERRIDE;

  QMutex threadMutex;
  QWaitCondition condition;

  bool isMouseButtonDown = false;
  int button = 0;

  QList<char> eventQueue;
  QMutex eventQueueMutex;
  bool eventQueueEmpty = true;
  bool shouldQueueEvents = true;

  DatumP interceptInputInterrupt(DatumP message);

  QTextStream *dribbleStream;
};

Controller *mainController();

#endif // CONTROLLER_H
