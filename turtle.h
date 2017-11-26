#ifndef TURTLE_H
#define TURTLE_H

//===-- qlogo/turtle.h - Turtle class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Turtle class, which maintains
/// the turtle state.
///
//===----------------------------------------------------------------------===//


#include <QColor>
#include <QMatrix4x4>
#include <map>

#include "datum.h"
#include <math.h>

const float startingPensize = 1;

enum PenModeEnum { penModePaint, penModeErase, penModeReverse };

enum TurtleModeEnum { turtleWrap, turtleFence, turtleWindow };

class Turtle {
  QMatrix4x4 matrix;
  void preTurtleMovement();
  void postTurtleMovement();
  void drawTurtleWrap();
  void drawTurtleFence();
  void drawTurtleWindow();
  QColor penColor;
  QVector4D lineStart;
  TurtleModeEnum mode = turtleFence;
  bool isFilling = false;
  QList<QVector4D> fillVertices;
  QList<QColor> fillVertexColors;
  QColor fillColor;
  PenModeEnum penMode = penModePaint;
  double penSize = 1;

  double scrunchX = 1;
  double scrunchY = 1;

  bool isVisible;
  bool penIsDown;

public:
  Turtle();
  ~Turtle();

  const QMatrix4x4 &getMatrix(void) { return matrix; }

  bool isTurtleVisible() { return isVisible; }
  void setIsTurtleVisible(bool aIsVisible) { isVisible = aIsVisible; }

  bool isPenDown() { return penIsDown; }
  void setPenIsDown(bool aIsPenDown) { penIsDown = aIsPenDown; }
  void setPenMode(PenModeEnum aPenMode);
  PenModeEnum getPenMode();

  void rotate(qreal angle, char axis);
  void move(qreal x, qreal y, qreal z);
  void setxyz(qreal x, qreal y, qreal z);
  void getxyz(qreal &x, qreal &y, qreal &z);
  void setMode(TurtleModeEnum newMode);
  TurtleModeEnum getMode();
  qreal getHeading(char axis);
  void setxy(qreal x, qreal y);
  void setx(qreal x);
  void sety(qreal y);
  void setz(qreal z);
  void setPenColor(const QColor &c);
  void setPenSize(double aPenSize);
  bool isPenSizeValid(double aPenSize);
  double getPenSize();
  const QColor &getPenColor();
  void home(bool canDraw = true);
  DatumP print();
  void drawArc(qreal angle, qreal radius);

  void beginFillWithColor(const QColor &aFillColor);
  void endFill();

  void setScrunch(double x, double y);
  void getScrunch(double &x, double &y);
};

Turtle *mainTurtle();

#endif // TURTLE_H
