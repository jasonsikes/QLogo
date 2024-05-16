
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
/// superclass for LogoControllerGUI.
///
//===----------------------------------------------------------------------===//

#ifndef LOGOCONTROLLER_H
#define LOGOCONTROLLER_H

#include "sharedconstants.h"
#include <QColor>
#include <QImage>
#include <QObject>
#include <QThread>
#include <QVector2D>
#include <QFont>
#include "error.h"

class Kernel;
class QTextStream;


class LogoController : public QObject {
  Q_OBJECT

    virtual void processInputMessageQueue() {}

public:
  LogoController(QObject *parent = 0);
  ~LogoController();

  /// Returns the most recent interrupt signal that was received. Resets the signal.
  SignalsEnum_t latestSignal();

  virtual void initialize() {}
  virtual QString inputRawlineWithPrompt(QString);
  virtual DatumPtr readchar();
  virtual bool atEnd();
  virtual void printToConsole(QString);
  int run(void);
  virtual void systemStop(void);
  virtual void mwait(unsigned long);
  virtual QString editText( QString ) { Error::noGraphics(); return QString(); }

  virtual void setTurtlePos(const QTransform &newTurtlePos) { Error::noGraphics(); }
  virtual void emitVertex() { Error::noGraphics(); }
  virtual void beginPolygon(const QColor &color) { Error::noGraphics(); }
  virtual void endPolygon() { Error::noGraphics(); }
  virtual void clearCanvas(void) { Error::noGraphics(); }
  virtual void drawLabel(QString) { Error::noGraphics(); }
  virtual void drawArc(qreal angle, qreal radius) { Error::noGraphics(); }
  virtual void setLabelFontName(QString) { Error::noGraphics(); }
  virtual QString addStandoutToString(QString src) { return src; };
  virtual bool keyQueueHasChars();
  bool setDribble(QString filePath);
  bool isDribbling();
  virtual void setBounds(double x, double y) { Error::noGraphics(); }
  virtual double boundX() { Error::noGraphics(); return 0; }
  virtual double boundY() { Error::noGraphics(); return 0; }
  virtual void setCanvasForegroundColor(const QColor &color) { Error::noGraphics(); }
  virtual void setCanvasBackgroundColor(const QColor &) { Error::noGraphics(); }
  virtual void setCanvasBackgroundImage(QImage image) { Error::noGraphics(); }
  virtual const QColor getCanvasBackgroundColor(void) { Error::noGraphics(); return QColor(); }
  virtual QImage getCanvasImage() { Error::noGraphics(); return QImage(); }

  virtual bool getIsMouseButtonDown() { Error::noGraphics(); return false; }
  virtual int getAndResetButtonID() { Error::noGraphics();  return 0; }
  virtual QVector2D lastMouseclickPosition() { Error::noGraphics(); return QVector2D(); }
  virtual QVector2D mousePosition() { Error::noGraphics(); return QVector2D(); }

  virtual void clearScreenText() { Error::noGraphics(); }
  virtual void setTextCursorPos(int, int) { Error::noGraphics(); }
  virtual void getTextCursorPos(int &, int &) { Error::noGraphics(); }
  virtual void setTextColor(const QColor&, const QColor&) { Error::noGraphics(); }
  virtual void setTextFontSize(double) { Error::noGraphics(); }
  virtual double getTextFontSize() { Error::noGraphics(); return 12; }
  virtual QString getTextFontName() { Error::noGraphics();  return QString(); }
  virtual void setTextFontName(QString) { Error::noGraphics(); }
  virtual QStringList getAllFontNames() { Error::noGraphics(); return QStringList(); }
  virtual void setCursorOverwriteMode(bool) { Error::noGraphics(); }
  virtual bool cursorOverwriteMode() { Error::noGraphics(); return false; }

  virtual void setLabelFontSize(double) { Error::noGraphics(); }
  virtual double getLabelFontSize() { Error::noGraphics(); return 12; }
  virtual QString getLabelFontName() { Error::noGraphics(); return QString(); }
  virtual void setLabelFontName(QString &) { Error::noGraphics(); }

  virtual void setTurtleIsVisible(bool) { Error::noGraphics(); }
  virtual void setPenmode(PenModeEnum) { Error::noGraphics(); }
  virtual void setPenIsDown(bool) { Error::noGraphics(); }
  virtual void setScreenMode(ScreenModeEnum) { Error::noGraphics(); }
  virtual ScreenModeEnum getScreenMode() { Error::noGraphics(); return textScreenMode; }

  virtual void setPensize(double) { Error::noGraphics(); }
  virtual bool isPenSizeValid(double) { Error::noGraphics(); return false; }
  virtual void setIsCanvasBounded(bool) { Error::noGraphics(); }
  virtual bool isCanvasBounded() { Error::noGraphics(); return false; }
  virtual void setSplitterSizeRatios(float, float) { Error::noGraphics(); }

  Kernel *kernel;

protected:

  QTextStream *dribbleStream;

  QTextStream *inStream;
  QTextStream *outStream;

};

LogoController *mainController();

#endif // LOGOCONTROLLER_H
