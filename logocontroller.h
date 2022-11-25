
//===-- qlogo/logocontroller.h - LogoController class definition -------*- C++
//-*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
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
/// This file contains the declaration of the LogoController class, which is the
/// superclass for QLogoController.
///
//===----------------------------------------------------------------------===//

#ifndef LOGOCONTROLLER_H
#define LOGOCONTROLLER_H

#include "constants.h"
#include "datum.h"
#include <QColor>
#include <QImage>
#include <QObject>
#include <QThread>
#include <QVector2D>
#include <QFont>
#include "error.h"

class Kernel;
class QTextStream;

enum ScreenModeEnum {
  initScreenMode,
  textScreenMode,
  fullScreenMode,
  splitScreenMode
};


class LogoController : public QObject {
  Q_OBJECT

    virtual void processInputMessageQueue() {}

public:
  LogoController(QObject *parent = 0);
  ~LogoController();

  /// Returns the most recent interrupt signal that was received. Resets the signal.
  SignalsEnum_t latestSignal();

  virtual void initialize() {}
  virtual DatumP readRawlineWithPrompt(const QString &);
  virtual DatumP readchar();
  virtual bool atEnd();
  virtual void printToConsole(const QString &);
  int run(void);
  virtual void systemStop(void);
  virtual void mwait(unsigned long);
  virtual const QString editText(const QString ) { Error::noGraphics(); return QString(""); }

  virtual void drawLine(const QVector3D &, const QVector3D &, const QColor &, const QColor &) { Error::noGraphics(); }
  virtual void drawPolygon(const QList<QVector3D> &, const QList<QColor> &) { Error::noGraphics(); }
  virtual void clearScreen(void) { Error::noGraphics(); }
  virtual void clearScreenText(void) { Error::noGraphics(); }
  virtual void drawLabel(const QString &, const QVector3D &, const QColor &) { Error::noGraphics(); }
  QString addStandoutToString(const QString &src);
  virtual bool keyQueueHasChars();
  bool setDribble(const QString &filePath);
  bool isDribbling();
  virtual void setBounds(double x, double y) { Error::noGraphics(); }
  virtual double boundX() { Error::noGraphics(); return 0; }
  virtual double boundY() { Error::noGraphics(); return 0; }
  virtual void setCanvasBackgroundColor(QColor) { Error::noGraphics(); }
  virtual QColor getCanvasBackgroundColor(void) { Error::noGraphics(); return QColor(); }
  virtual QImage getCanvasImage() { Error::noGraphics(); return QImage(); }

  virtual bool getIsMouseButtonDown() { Error::noGraphics(); return false; }
  int getButton() { Error::noGraphics();  return 0; }
  virtual QVector2D lastMouseclickPosition() { Error::noGraphics(); return clickPos; }
  QVector2D mousePosition() { Error::noGraphics(); return mousePos; }

  void setTextCursorPos(int, int) { Error::noGraphics(); }
  void getTextCursorPos(int &, int &) { Error::noGraphics(); }
  void setTextColor(const QColor &, const QColor &) { Error::noGraphics(); }
  virtual void setTextFontSize(double) { Error::noGraphics(); }
  virtual double getTextFontSize() { Error::noGraphics(); return 12; }
  virtual const QString getTextFontName() { Error::noGraphics();  return "Courier New"; }
  virtual void setTextFontName(const QString) { Error::noGraphics(); }
  virtual const QStringList getAllFontNames() { Error::noGraphics(); return QStringList(); }
  void setCursorOverwriteMode(bool) { Error::noGraphics(); }

  virtual void setLabelFontSize(double) { Error::noGraphics(); }
  virtual double getLabelFontSize() { Error::noGraphics(); return 12; }
  virtual const QString getLabelFontName() { Error::noGraphics(); return QString(); }
  virtual void setLabelFontName(const QString &) { Error::noGraphics(); }

  void beginInputHistory() {}
  DatumP inputHistory() { return nothing; }

  virtual void setTurtlePos(const QMatrix4x4 &) { Error::noGraphics(); }
  virtual void setTurtleIsVisible(bool) { Error::noGraphics(); }
  void setPenmode(PenModeEnum) { Error::noGraphics(); }
  void setScreenMode(ScreenModeEnum) { Error::noGraphics(); }
  ScreenModeEnum getScreenMode() { Error::noGraphics(); return textScreenMode; }

  virtual void setPensize(double) { Error::noGraphics(); }
  virtual bool isPenSizeValid(double) { Error::noGraphics(); return false; }
  void setIsCanvasBounded(bool) { Error::noGraphics(); }
  void setSplitterSizeRatios(float, float) { Error::noGraphics(); }

  bool eventQueueIsEmpty() { Error::noGraphics(); return true; }
  char nextQueueEvent() { Error::noGraphics(); return 'x'; }

  Kernel *kernel;

protected:
  qreal boundsX = 150;
  qreal boundsY = 150;

  QVector2D mousePos = QVector2D(0,0);
  QVector2D clickPos = QVector2D(0,0);
  int lastButtonpressID = 0;

  QTextStream *readStream;
  QTextStream *writeStream;

  QTextStream *dribbleStream;

    QTextStream *inStream;
    QTextStream *outStream;

};

LogoController *mainController();

#endif // LOGOCONTROLLER_H
