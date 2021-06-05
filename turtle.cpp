
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

#include <QVector3D>
#include <QVector4D>

#include "logocontroller.h"
#include "qlogocontroller.h"

Turtle *_mainTurtle = NULL;

Turtle *mainTurtle() {
  Q_ASSERT(_mainTurtle != NULL);
  return _mainTurtle;
}

Turtle::Turtle() : matrix(QMatrix4x4()), isVisible(true), penIsDown(true) {
  Q_ASSERT(_mainTurtle == NULL);
  _mainTurtle = this;
  setPenColor(QColor("white"));
  mode = turtleWrap;
}

Turtle::~Turtle() { _mainTurtle = NULL; }

void Turtle::preTurtleMovement() {
  if (penIsDown) {
    lineStart = matrix.column(3).toVector3DAffine();
    }
}

void Turtle::drawTurtleWrap() {
  QVector3D lineEnd = matrix.column(3).toVector3DAffine();
  double boundX = mainController()->boundX();
  double boundY = mainController()->boundY();

  while ((lineEnd.x() < -boundX) || (lineEnd.x() > boundX) ||
         (lineEnd.y() < -boundY) || (lineEnd.y() > boundY)) {

    if (lineEnd.x() > boundX) {
      float cy = lineStart.y() +
                 (boundX - lineStart.x()) * (lineEnd.y() - lineStart.y()) /
                     (lineEnd.x() - lineStart.x());
      if ((cy >= -boundY) && (cy <= boundY)) {
        QVector3D e = QVector3D(boundX, cy, 0);
        if (penIsDown)
          mainController()->drawLine(lineStart, e, penColor, penColor);
        lineStart = QVector3D(-boundX, cy, lineEnd.z());
        lineEnd =
            QVector3D(lineEnd.x() - 2 * boundX, lineEnd.y(), lineEnd.z());
        qreal w = matrix(3, 3);
        matrix(0, 3) = lineEnd.x() * w;
        matrix(1, 3) = lineEnd.y() * w;
        matrix(2, 3) = lineEnd.z() * w;
        continue;
      }
    }

    if (lineEnd.x() < -boundX) {
      float cy = lineStart.y() +
                 (-boundX - lineStart.x()) * (lineEnd.y() - lineStart.y()) /
                     (lineEnd.x() - lineStart.x());
      if ((cy >= -boundY) && (cy <= boundY)) {
        QVector3D e = QVector3D(-boundX, cy, 0);
        if (penIsDown)
          mainController()->drawLine(lineStart, e, penColor, penColor);
        lineStart = QVector3D(boundX, cy, lineEnd.z());
        lineEnd =
            QVector3D(lineEnd.x() + 2 * boundX, lineEnd.y(), lineEnd.z());
        qreal w = matrix(3, 3);
        matrix(0, 3) = lineEnd.x() * w;
        matrix(1, 3) = lineEnd.y() * w;
        matrix(2, 3) = lineEnd.z() * w;
        continue;
      }
    }

    if (lineEnd.y() > boundY) {
      float cx = lineStart.x() +
                 (boundY - lineStart.y()) * (lineEnd.x() - lineStart.x()) /
                     (lineEnd.y() - lineStart.y());
      if ((cx >= -boundX) && (cx <= boundX)) {
        QVector3D e = QVector3D(cx, boundY, 0);
        if (penIsDown)
          mainController()->drawLine(lineStart, e, penColor, penColor);
        lineStart = QVector3D(cx, -boundY, lineEnd.z());
        lineEnd =
            QVector3D(lineEnd.x(), lineEnd.y() - 2 * boundY, lineEnd.z());
        qreal w = matrix(3, 3);
        matrix(0, 3) = lineEnd.x() * w;
        matrix(1, 3) = lineEnd.y() * w;
        matrix(2, 3) = lineEnd.z() * w;
        continue;
      }
    }

    if (lineEnd.y() < -boundY) {
      float cx = lineStart.x() +
                 (-boundY - lineStart.y()) * (lineEnd.x() - lineStart.x()) /
                     (lineEnd.y() - lineStart.y());
      if ((cx >= -boundX) && (cx <= boundX)) {
        QVector3D e = QVector3D(cx, -boundY, 0);
        if (penIsDown)
          mainController()->drawLine(lineStart, e, penColor, penColor);
        lineStart = QVector3D(cx, boundY, lineEnd.z());
        lineEnd =
            QVector3D(lineEnd.x(), lineEnd.y() + 2 * boundY, lineEnd.z());
        qreal w = matrix(3, 3);
        matrix(0, 3) = lineEnd.x() * w;
        matrix(1, 3) = lineEnd.y() * w;
        matrix(2, 3) = lineEnd.z() * w;
        continue;
      }
    }
  }

  if (isFilling) {
    fillVertices.push_back(lineEnd);
    fillVertexColors.push_back(fillColor);
  } else if (penIsDown) {
    mainController()->drawLine(lineStart, lineEnd, penColor, penColor);
  }
}

void Turtle::drawTurtleFence() {
  QVector3D lineEnd = matrix.column(3).toVector3DAffine();
  double boundX = mainController()->boundX();
  double boundY = mainController()->boundY();

  if ((lineEnd.x() < -boundX) || (lineEnd.x() > boundX) ||
      (lineEnd.y() < -boundY) || (lineEnd.y() > boundY)) {
    qreal w = matrix(3, 3);
    matrix(0, 3) = lineStart.x() * w;
    matrix(1, 3) = lineStart.y() * w;
    matrix(2, 3) = lineStart.z() * w;

    Error::turtleOutOfBounds();
  }

  if (isFilling) {
    fillVertices.push_back(lineEnd);
    fillVertexColors.push_back(fillColor);
  } else if (penIsDown) {
    mainController()->drawLine(lineStart, lineEnd, penColor, penColor);
  }
}

void Turtle::drawTurtleWindow() {
  QVector3D lineEnd = matrix.column(3).toVector3DAffine();

  if (isFilling) {
    fillVertices.push_back(lineEnd);
    fillVertexColors.push_back(fillColor);
  } else if (penIsDown) {
    mainController()->drawLine(lineStart, lineEnd, penColor, penColor);
  }
}

void Turtle::postTurtleMovement() {
  switch (mode) {
  case turtleWrap:
    drawTurtleWrap();
    break;
  case turtleFence:
    drawTurtleFence();
    break;
  case turtleWindow:
    drawTurtleWindow();
    break;
  default:
    qDebug() << "aaaayup!";
    Q_ASSERT(false);
    break;
  }
  mainController()->setTurtlePos(matrix);
}

void Turtle::drawArc(qreal angle, qreal radius) {
  if (penIsDown) {
    // 4 degrees between segments is my limit of perception
    qreal countOfSegments = fabs(round(angle / 4));
    angle *= M_PI / 180;
    if (countOfSegments < 2)
      countOfSegments = 2;

    for (qreal segment = 1; segment <= countOfSegments; ++segment) {
      qreal a1 = (segment - 1) / countOfSegments * angle;
      qreal a2 = segment / countOfSegments * angle;
      qreal p1x = -sin(a1) * radius;
      qreal p1y = cos(a1) * radius;
      qreal p2x = -sin(a2) * radius;
      qreal p2y = cos(a2) * radius;
      QVector3D v1 = (matrix * QVector4D(p1x, p1y, 0, 1)).toVector3DAffine();
      QVector3D v2 = (matrix * QVector4D(p2x, p2y, 0, 1)).toVector3DAffine();
      mainController()->drawLine(v1, v2, penColor, penColor);
    }
  }
}

void Turtle::move(qreal x, qreal y, qreal z) {
  preTurtleMovement();
  matrix.translate(x, y, z);
  postTurtleMovement();
}

void Turtle::rotate(qreal angle, char axis) {
  qreal x = 0;
  qreal y = 0;
  qreal z = 0;
  switch (axis) {
  case 'X':
    x = 1;
    break;
  case 'Y':
    y = 1;
    break;
  case 'Z':
    z = 1;
    break;
  default:
    Q_ASSERT(false);
  }
  matrix.rotate(angle, x, y, z);
  mainController()->setTurtlePos(matrix);
}

void Turtle::setxyz(qreal x, qreal y, qreal z) {
  preTurtleMovement();
  qreal w = matrix(3, 3);
  matrix(0, 3) = x * scrunchX * w;
  matrix(1, 3) = y * scrunchY * w;
  matrix(2, 3) = z * w;
  postTurtleMovement();
}

void Turtle::getxyz(qreal &x, qreal &y, qreal &z) {
  qreal w = matrix(3, 3);
  x = matrix(0, 3) / scrunchX / w;
  y = matrix(1, 3) / scrunchY / w;
  z = matrix(2, 3) / w;
}

void Turtle::setMode(TurtleModeEnum newMode) {
  mode = newMode;
  if (mode != turtleWindow) {
      double boundX = mainController()->boundX();
      double boundY = mainController()->boundY();

      QVector3D pos = matrix.column(3).toVector3DAffine();
    if ((pos.x() < -boundX) || (pos.x() > boundX) || (pos.y() < -boundY) ||
        (pos.y() > boundY)) {
      matrix.setToIdentity();
      mainController()->setTurtlePos(matrix);
    }
  }
}

TurtleModeEnum Turtle::getMode() { return mode; }

qreal Turtle::getHeading(char axis) {
  qreal a = 0;
  qreal b = 0;

  switch (axis) {
  case 'X':
    a = matrix(2, 1);
    b = matrix(2, 2);
    break;
  case 'Y':
    a = -matrix(2, 0);
    b = sqrt(matrix(2, 1) * matrix(2, 1) + matrix(2, 2) * matrix(2, 2));
    break;
  case 'Z':
    a = matrix(1, 0);
    b = matrix(0, 0);
    break;
  default:
    Q_ASSERT(false);
  }

  qreal retval = atan2(a, b) * 180 / M_PI;
  if (retval < 0)
    retval += 360;
  return retval;
}

void Turtle::setxy(qreal x, qreal y) {
  preTurtleMovement();
  qreal w = matrix(3, 3);
  matrix(0, 3) = x * scrunchX * w;
  matrix(1, 3) = y * scrunchY * w;
  postTurtleMovement();
}

void Turtle::setx(qreal x) {
  preTurtleMovement();
  matrix(0, 3) = x * scrunchX * matrix(3, 3);
  postTurtleMovement();
}

void Turtle::sety(qreal y) {
  preTurtleMovement();
  matrix(1, 3) = y * scrunchY * matrix(3, 3);
  postTurtleMovement();
}

void Turtle::setz(qreal z) {
  preTurtleMovement();
  matrix(2, 3) = z * matrix(3, 3);
  postTurtleMovement();
}

void Turtle::home(bool canDraw) {
  if (canDraw)
    preTurtleMovement();
  matrix =
      QMatrix4x4(scrunchX, 0, 0, 0, 0, scrunchY, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
  if (canDraw)
    postTurtleMovement();
}

void Turtle::setPenColor(const QColor &c) { penColor = c; }

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

void Turtle::beginFillWithColor(const QColor &aFillColor) {
  if (isFilling) {
    fillVertices.clear();
    fillVertexColors.clear();
    Error::alreadyFilling();
  }
  isFilling = true;
  fillVertices.clear();
  fillVertexColors.clear();
  QVector3D startPos = matrix.column(3).toVector3DAffine();
  fillColor = aFillColor;
  fillVertices.push_back(startPos);
  fillVertexColors.push_back(fillColor);
}

void Turtle::endFill() {
  isFilling = false;
  if (fillVertices.size() >= 3) {
    mainController()->drawPolygon(fillVertices, fillVertexColors);
  }
}

DatumP Turtle::print() {
  QString retval = "";
  for (int row = 0; row < 4; ++row) {
    QString s = "%1 %2 %3 %4\n";
    for (int col = 0; col < 4; ++col) {
      s = s.arg(matrix(row, col), -2, 'f', 4);
    }
    retval += s;
  }
  return DatumP(new Word(retval));
}

void Turtle::setScrunch(double x, double y) {
  QMatrix4x4 m = QMatrix4x4(x / scrunchX, 0, 0, 0, 0, y / scrunchY, 0, 0, 0, 0,
                            1, 0, 0, 0, 0, 1);
  matrix *= m;
  scrunchX = x;
  scrunchY = y;
  mainController()->setTurtlePos(matrix);
}

void Turtle::getScrunch(double &x, double &y) {
  x = scrunchX;
  y = scrunchY;
}
