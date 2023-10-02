#ifndef TURTLE_H
#define TURTLE_H

//===-- qlogo/turtle.h - Turtle class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Turtle class, which maintains
/// the turtle state.
///
//===----------------------------------------------------------------------===//

#include <QColor>
#include <QMatrix4x4>
#include "sharedconstants.h"

#include "datum.h"
#include <math.h>

class Turtle {
  QMatrix4x4 matrix;
  void preTurtleMovement();
  void postTurtleMovement();
  void drawTurtleWrap();
  void drawTurtleFence();
  void drawTurtleWindow();
  QColor penColor;
  QVector3D lineStart;
  TurtleModeEnum mode = turtleFence;
  bool isFilling = false;
  QList<QVector3D> fillVertices;
  QList<QColor> fillVertexColors;
  QColor fillColor;
  PenModeEnum penMode = penModePaint;
  double penSize = startingPensize;

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
  DatumPtr print();
  void drawArc(qreal angle, qreal radius);

  void beginFillWithColor(const QColor &aFillColor);
  void endFill();
};

Turtle *mainTurtle();

#endif // TURTLE_H
