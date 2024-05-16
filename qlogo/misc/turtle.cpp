
//===-- qlogo/turtle.cpp - Turtle class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Turtle class, which maintains
/// the turtle state.
///
//===----------------------------------------------------------------------===//

#define _USE_MATH_DEFINES

#include "turtle.h"
#include "error.h"

#include <math.h>

#include "controller/logocontroller.h"

Turtle *_mainTurtle = NULL;

Turtle *mainTurtle() {
  Q_ASSERT(_mainTurtle != NULL);
  return _mainTurtle;
}

Turtle::Turtle() : turtlePosition(QTransform()), turtleIsVisible(true), penIsDown(true) {
  Q_ASSERT(_mainTurtle == NULL);
  _mainTurtle = this;
  penColor = initialCanvasForegroundColor;
  mode = turtleWrap;
}

Turtle::~Turtle() { _mainTurtle = NULL; }

QTransform matrixWithNewXY(const QTransform &src,qreal x, qreal y)
{
    return QTransform(src.m11(),src.m12(),src.m13(),
                      src.m21(),src.m22(),src.m23(),
                      x,       y,         src.m33());
}


void Turtle::setPenIsDown(bool aIsPenDown)
{
    penIsDown = aIsPenDown;
    mainController()->setPenIsDown(penIsDown);
}


void Turtle::moveTurtleWrap(const QTransform &newPosition) {
    qreal lineEndX = newPosition.dx();
    qreal lineEndY = newPosition.dy();
  double boundX = mainController()->boundX();
  double boundY = mainController()->boundY();

  while ((lineEndX < -boundX) || (lineEndX > boundX) ||
         (lineEndY < -boundY) || (lineEndY > boundY)) {
      qreal lineStartX = turtlePosition.dx();
      qreal lineStartY = turtlePosition.dy();

      if (lineEndX > boundX) {
          qreal cY = lineStartY +
                     (boundX - lineStartX) * (lineEndY - lineStartY) /
                         (lineEndX - lineStartX);
          if ((cY >= -boundY) && (cY <= boundY)) {
              mainController()->setTurtlePos(matrixWithNewXY(turtlePosition, boundX, cY));
              mainController()->emitVertex();
              if (penIsDown)
                  mainController()->setPenIsDown(false);
              turtlePosition = matrixWithNewXY(turtlePosition, -boundX, cY);
              mainController()->setTurtlePos(turtlePosition);
              mainController()->emitVertex();
              if (penIsDown)
                  mainController()->setPenIsDown(true);
              lineEndX -= 2 * boundX;

              continue;
          }
      }

      if (lineEndX < -boundX) {
          qreal cY = lineStartY +
                     (-boundX - lineStartX) * (lineEndY - lineStartY) /
                         (lineEndX - lineStartX);
          if ((cY >= -boundY) && (cY <= boundY)) {
              mainController()->setTurtlePos(matrixWithNewXY(turtlePosition, -boundX, cY));
              mainController()->emitVertex();
              if (penIsDown)
                  mainController()->setPenIsDown(false);
              turtlePosition = matrixWithNewXY(turtlePosition, boundX, cY);
              mainController()->setTurtlePos(turtlePosition);
              mainController()->emitVertex();
              if (penIsDown)
                  mainController()->setPenIsDown(true);
              lineEndX += 2 * boundX;

              continue;
          }
      }

      if (lineEndY > boundY) {
          qreal cX = lineStartX +
                     (boundY - lineStartY) * (lineEndX - lineStartX) /
                         (lineEndY - lineStartY);
          if ((cX >= -boundX) && (cX <= boundX)) {
              mainController()->setTurtlePos(matrixWithNewXY(turtlePosition, cX, boundY));
              mainController()->emitVertex();
              if (penIsDown)
                  mainController()->setPenIsDown(false);
              turtlePosition = matrixWithNewXY(turtlePosition, cX, -boundY);
              mainController()->setTurtlePos(turtlePosition);
              mainController()->emitVertex();
              if (penIsDown)
                  mainController()->setPenIsDown(true);
              lineEndY -= 2 * boundY;

              continue;
          }
      }

      if (lineEndY < -boundY) {
          qreal cX = lineStartX +
                     (-boundY - lineStartY) * (lineEndX - lineStartX) /
                         (lineEndY - lineStartY);
          if ((cX >= -boundX) && (cX <= boundX)) {
              mainController()->setTurtlePos(matrixWithNewXY(turtlePosition, cX, -boundY));
              mainController()->emitVertex();
              if (penIsDown)
                  mainController()->setPenIsDown(false);
              turtlePosition = matrixWithNewXY(turtlePosition, cX, boundY);
              mainController()->setTurtlePos(turtlePosition);
              mainController()->emitVertex();
              if (penIsDown)
                  mainController()->setPenIsDown(true);
              lineEndY += 2 * boundY;

              continue;
          }
      }
  }

  turtlePosition = matrixWithNewXY(newPosition, lineEndX, lineEndY);
  mainController()->setTurtlePos(turtlePosition);
  mainController()->emitVertex();
}

void Turtle::moveTurtleFence(const QTransform &newPosition) {
    qreal lineEndX = newPosition.dx();
    qreal lineEndY = newPosition.dy();
  double boundX = mainController()->boundX();
  double boundY = mainController()->boundY();

  if ((lineEndX < -boundX) || (lineEndX > boundX) ||
      (lineEndY < -boundY) || (lineEndY > boundY)) {
    Error::turtleOutOfBounds();
  }
  turtlePosition = newPosition;
  mainController()->setTurtlePos(turtlePosition);
  mainController()->emitVertex();
}

void Turtle::moveTurtleWindow(const QTransform &newPosition) {
    turtlePosition = newPosition;
    mainController()->setTurtlePos(turtlePosition);
    mainController()->emitVertex();
}

void Turtle::moveTurtle(const QTransform &newPosition) {
  switch (mode) {
  case turtleWrap:
    moveTurtleWrap(newPosition);
    break;
  case turtleFence:
    moveTurtleFence(newPosition);
    break;
  case turtleWindow:
    moveTurtleWindow(newPosition);
    break;
  }
}

void Turtle::drawArc(qreal angle, qreal radius) {
    mainController()->drawArc(angle, radius);
}

void Turtle::forward(qreal steps) {
    QTransform newPos = turtlePosition;
    newPos.translate(0, steps);
    moveTurtle(newPos);
}

void Turtle::rotate(qreal angle) {
  turtlePosition.rotate(angle, Qt::ZAxis);
  mainController()->setTurtlePos(turtlePosition);
}

void Turtle::getxy(qreal &x, qreal &y) {
    x = turtlePosition.dx();
    y = turtlePosition.dy();
}

void Turtle::setMode(TurtleModeEnum newMode) {
  mode = newMode;
  if (mode != turtleWindow) {
      qreal boundX = mainController()->boundX();
      qreal boundY = mainController()->boundY();
      qreal posX = turtlePosition.dx();
      qreal posY = turtlePosition.dy();
    if ((posX < -boundX) || (posX > boundX) || (posY < -boundY) ||
        (posY > boundY)) {
          moveTurtle(QTransform());
      }
  }
}

TurtleModeEnum Turtle::getMode() { return mode; }

qreal Turtle::getHeading() {
    qreal s = turtlePosition.m12();
    qreal c = turtlePosition.m11();

  qreal retval = atan2(s, c) * 180 / M_PI;
  if (retval < 0)
    retval += 360;
  return retval;
}

void Turtle::setxy(qreal x, qreal y) {
    QTransform newPosition(matrixWithNewXY(turtlePosition, x, y));
    moveTurtle(newPosition);
}

void Turtle::setx(qreal x) {
    qreal y = turtlePosition.dy();
    setxy(x, y);
}

void Turtle::sety(qreal y) {
    qreal x = turtlePosition.dx();
    setxy(x, y);
}

void Turtle::moveToHome() {
    moveTurtle(QTransform());
}

void Turtle::setPenColor(const QColor &c) {
    penColor = c;
    mainController()->setCanvasForegroundColor(c);
}

const QColor &Turtle::getPenColor() { return penColor; }

void Turtle::setPenMode(PenModeEnum aPenMode) {
  if (penMode != aPenMode) {
    penMode = aPenMode;
    mainController()->setPenmode(penMode);
  }
}

PenModeEnum Turtle::getPenMode() { return penMode; }

void Turtle::setPenSize(double aPenSize) {
  penSize = aPenSize;
  mainController()->setPensize(penSize);
}

bool Turtle::isPenSizeValid(double aPenSize) {
  return mainController()->isPenSizeValid(aPenSize);
}

double Turtle::getPenSize() { return penSize; }

void Turtle::beginFillWithColor(const QColor &fillColor) {
  if (isFilling) {
    Error::alreadyFilling();
  }
  isFilling = true;

  mainController()->beginPolygon(fillColor);
}

void Turtle::endFill() {
  isFilling = false;
    mainController()->endPolygon();
}

DatumPtr Turtle::print() {
    QString s = QString("%1 %2 %3\n"
                        "%4 %5 %6\n"
                        "%7 %8 %9\n")
                    .arg(turtlePosition.m11()).arg(turtlePosition.m12()).arg(turtlePosition.m13())
                    .arg(turtlePosition.m21()).arg(turtlePosition.m22()).arg(turtlePosition.m23())
                    .arg(turtlePosition.m31()).arg(turtlePosition.m32()).arg(turtlePosition.m33());


    return DatumPtr(s);
}

