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
#include "QtGui/qtransform.h"
#include "sharedconstants.h"

#include "datum/datum.h"

class Turtle {
    QTransform turtlePosition;

    void moveTurtle(const QTransform &newPosition);
    void moveTurtleWrap(const QTransform &newPosition);
    void moveTurtleFence(const QTransform &newPosition);
    void moveTurtleWindow(const QTransform &newPosition);

  QColor penColor;
  TurtleModeEnum mode = turtleFence;
  bool isFilling = false;

  PenModeEnum penMode = penModePaint;

  double penSize = initialPensize;

  bool turtleIsVisible;
  bool penIsDown;

public:
  Turtle();
  ~Turtle();

  const QTransform &getMatrix(void) { return turtlePosition; }

  bool isTurtleVisible() { return turtleIsVisible; }
  void setIsTurtleVisible(bool aIsVisible) { turtleIsVisible = aIsVisible; }

  bool isPenDown() { return penIsDown; }
  void setPenIsDown(bool aIsPenDown);
  void setPenMode(PenModeEnum aPenMode);
  PenModeEnum getPenMode();

  void rotate(double angle);
  void forward(double steps);
  void setMode(TurtleModeEnum newMode);
  TurtleModeEnum getMode();
  double getHeading();
  void getxy(double &x, double &y);
  void setxy(double x, double y);
  void setx(double x);
  void sety(double y);
  void setPenColor(const QColor &c);
  void setPenSize(double aPenSize);
  bool isPenSizeValid(double aPenSize);
  double getPenSize();
  const QColor &getPenColor();
  void moveToHome();
  DatumPtr print();
  void drawArc(double angle, double radius);

  void beginFillWithColor(const QColor &fillColor);
  void endFill();
};

Turtle *mainTurtle();

#endif // TURTLE_H
