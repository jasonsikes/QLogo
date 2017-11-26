
//===-- qlogo/test_controller.h - Controller class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Controller class, which is a
/// stand-in replacement controller class for testing.
///
//===----------------------------------------------------------------------===//

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "datum.h"
#include "turtle.h"
#include <QColor>
#include <QImage>
#include <QObject>
#include <QThread>
#include <QVector2D>

class Kernel;
class QTextStream;

extern qreal initialBoundXY;

const char characterEvent = 'c';
const char mouseEvent = 'm';
const char pauseEvent = 'p';    // ctrl-W
const char toplevelEvent = 't'; // ctrl-Q

enum ScreenModeEnum {
  initScreenMode,
  textScreenMode,
  fullScreenMode,
  splitScreenMode
};

class Controller : public QObject {
  Q_OBJECT

  QTextStream *readStream;
  QTextStream *writeStream;

public:
  Controller(QObject *parent = 0);
  ~Controller();
  DatumP readrawlineWithPrompt(const QString &);
  DatumP readchar();
  bool atEnd();
  void printToConsole(const QString &s);
  QString run(const QString &aInput);
  void mwait(unsigned long msecs);
  const QString *editText(const QString *) { return NULL; }

  QVector2D mousePos;
  QVector2D clickPos;

  void drawLine(const QVector4D &, const QVector4D &, const QColor &) {}
  void drawPolygon(const QList<QVector4D> &, const QList<QColor> &) {}
  void updateCanvas(void) {}
  void clearScreen(void) {}
  void clearScreenText(void) {}
  void drawLabel(const QString &, const QVector4D &, const QColor &,
                 const QFont &) {}
  QString addStandoutToString(const QString &src);
  bool keyQueueHasChars();
  bool setDribble(const QString &filePath);
  bool isDribbling();
  void setScrunch(double, double) {}
  void getScrunch(double &, double &) {}
  void setBounds(qreal x, qreal y) {
    boundsX = x;
    boundsY = y;
  }
  void getBounds(qreal &x, qreal &y) {
    x = boundsX;
    y = boundsY;
  }
  void setCanvasBackgroundColor(QColor) {}
  QColor getCanvasBackgroundColor(void) { return QColor(); }
  QImage getCanvasImage() { return QImage(); }
  bool getIsMouseButtonDown() { return false; }
  int getButton() { return 0; }
  void setTextCursorPos(int, int) {}
  void getTextCursorPos(int &, int &) {}
  void setTextColor(const QColor &, const QColor &) {}
  void setTextSize(int) {}
  double getTextSize() { return 12; }
  QString getFontName() { return "Courier New"; }
  void setFontName(QString) {}
  QStringList getAllFontNames() { return QStringList(); }
  void setCursorOverwriteMode(bool) {}

  void beginInputHistory() {}
  DatumP inputHistory() { return nothing; }

  void setPenmode(PenModeEnum) {}
  void setScreenMode(ScreenModeEnum) {}
  ScreenModeEnum getScreenMode() { return textScreenMode; }

  void setPensize(double) {}
  bool isPenSizeValid(double) { return true; }
  void setIsCanvasBounded(bool) {}
  void setSplitterSizeRatios(float, float) {}

  bool eventQueueIsEmpty() { return true; }
  char nextQueueEvent() { return 'x'; }

  Kernel *kernel;

protected:
  qreal boundsX = 150;
  qreal boundsY = 150;

  QTextStream *inStream;
  QTextStream *outStream;
  QTextStream *dribbleStream;
};

Controller *mainController();

#endif // CONTROLLER_H
