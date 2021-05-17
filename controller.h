
//===-- qlogo/logo_controller.h - Controller class definition -------*- C++
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
/// superclass for both LogoController and QLogoController.
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
#include <QFont>

class Kernel;
class QTextStream;

extern qreal initialBoundXY;

const char characterEvent = 'c';
const char mouseEvent = 'm';
const char pauseEvent = 'p';    // ctrl-W
const char toplevelEvent = 't'; // ctrl-Q
const char systemEvent = 's';   // window close

enum ScreenModeEnum {
  initScreenMode,
  textScreenMode,
  fullScreenMode,
  splitScreenMode
};

class Controller : public QObject {
  Q_OBJECT

public:
  Controller(QObject *parent = 0);
  ~Controller();

  // TODO: Lots of these virtual functions should just throw errors since they should only be used in a GUI
  virtual void initialize() {}
  virtual DatumP readRawlineWithPrompt(const QString &) { return nothing; }
  virtual DatumP readchar() { return nothing; }
  virtual bool atEnd() { return true; }
  virtual void printToConsole(const QString &) {}
  int run(void);
  virtual void mwait(unsigned long) {}
  const QString *editText(const QString *) { return NULL; }

  QVector2D mousePos;
  QVector2D clickPos;

  virtual void drawLine(const QVector3D &, const QVector3D &, const QColor &) {}
  virtual void drawPolygon(const QList<QVector3D> &, const QList<QColor> &) {}
  void updateCanvas(void) {}
  virtual void clearScreen(void) {}
  void clearScreenText(void) {}
  virtual void drawLabel(const QString &, const QVector3D &, const QColor &,
                         const QFont &) {}
  QString addStandoutToString(const QString &src);
  virtual bool keyQueueHasChars() { return false; }
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
  virtual void setCanvasBackgroundColor(QColor) {}
  QColor getCanvasBackgroundColor(void) { return QColor(); }
  QImage getCanvasImage() { return QImage(); }
  bool getIsMouseButtonDown() { return false; }
  int getButton() { return 0; }
  void setTextCursorPos(int, int) {}
  void getTextCursorPos(int &, int &) {}
  void setTextColor(const QColor &, const QColor &) {}
  virtual void setTextSize(double) {}
  virtual double getTextSize() { return 12; }
  virtual QString getFontName() { return "Courier New"; }
  virtual void setFontName(QString) {}
  virtual const QStringList getAllFontNames() { return QStringList(); }
  void setCursorOverwriteMode(bool) {}

  virtual void setLabelSize(double) {}
  virtual double getLabelSize() { return 12; }
  virtual QFont getLabelFont() { return QFont(); }
  virtual void setLabelFontName(const QString &) {}

  void beginInputHistory() {}
  DatumP inputHistory() { return nothing; }

  virtual void setTurtlePos(const QMatrix4x4 &) {}

  void setPenmode(PenModeEnum) {}
  void setScreenMode(ScreenModeEnum) {}
  ScreenModeEnum getScreenMode() { return textScreenMode; }

  virtual void setPensize(double) {}
  virtual bool isPenSizeValid(double) { return false; }
  void setIsCanvasBounded(bool) {}
  void setSplitterSizeRatios(float, float) {}

  bool eventQueueIsEmpty() { return true; }
  char nextQueueEvent() { return 'x'; }

  Kernel *kernel;

protected:
  qreal boundsX = 150;
  qreal boundsY = 150;

  QTextStream *readStream;
  QTextStream *writeStream;

  QTextStream *dribbleStream;
};

Controller *mainController();

#endif // CONTROLLER_H
