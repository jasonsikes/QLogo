
//===-- qlogo/turtle.cpp - Turtle class implementation -------*- C++ -*-===//
//
// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
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
#include "controller/logocontroller.h"
#include "error.h"
#include <math.h>

// Some support functions for moving the turtle.

QTransform matrixWithNewXY(const QTransform &src, qreal x, qreal y)
{
    return QTransform(src.m11(), src.m12(), src.m13(), src.m21(), src.m22(), src.m23(), x, y, src.m33());
}

Turtle::Turtle() : turtlePosition(QTransform()), turtleIsVisible(true), penIsDown(true)
{
    Config::get().setMainTurtle(this);
    penColor = Config::get().initialCanvasForegroundColor;
    mode = turtleWrap;
}

Turtle::~Turtle()
{
    Config::get().setMainTurtle(NULL);
}

void Turtle::setPenIsDown(bool aIsPenDown)
{
    penIsDown = aIsPenDown;
    Config::get().mainController()->setPenIsDown(penIsDown);
}

// Move the turtle. If over a boundary, wrap.
bool Turtle::wrapTurtle(qreal lineStartU,
                        qreal lineStartV,
                        qreal &lineEndU,
                        qreal lineEndV,
                        qreal boundU,
                        qreal boundV,
                        bool isXBoundary,
                        qreal mult)
{
    qreal crossV = lineStartV + (mult * boundU - lineStartU) * (lineEndV - lineStartV) / (lineEndU - lineStartU);
    if ((crossV >= -boundV) && (crossV <= boundV))
    {
        QTransform tempTurtlePos = isXBoundary ? matrixWithNewXY(turtlePosition, mult * boundU, crossV)
                                               : matrixWithNewXY(turtlePosition, crossV, mult * boundU);
        Config::get().mainController()->setTurtlePos(tempTurtlePos);
        Config::get().mainController()->emitVertex();
        if (penIsDown)
            Config::get().mainController()->setPenIsDown(false);
        turtlePosition = isXBoundary ? matrixWithNewXY(turtlePosition, -mult * boundU, crossV)
                                     : matrixWithNewXY(turtlePosition, crossV, -mult * boundU);
        Config::get().mainController()->setTurtlePos(turtlePosition);
        Config::get().mainController()->emitVertex();
        if (penIsDown)
            Config::get().mainController()->setPenIsDown(true);
        lineEndU -= 2 * mult * boundU;
        return true;
    }
    return false;
}

// Move the turtle to a new position, wrapping around the edges of the canvas if
// the new position is outside the canvas.

void Turtle::moveTurtleWrap(const QTransform &newPosition)
{
    qreal lineEndX = newPosition.dx();
    qreal lineEndY = newPosition.dy();
    qreal boundX = Config::get().mainController()->boundX();
    qreal boundY = Config::get().mainController()->boundY();

    while ((lineEndX < -boundX) || (lineEndX > boundX) || (lineEndY < -boundY) || (lineEndY > boundY))
    {
        qreal lineStartX = turtlePosition.dx();
        qreal lineStartY = turtlePosition.dy();

        if (lineEndX > boundX)
        {
            if (wrapTurtle(lineStartX, lineStartY, lineEndX, lineEndY, boundX, boundY, true, 1))
                continue;
        }

        if (lineEndX < -boundX)
        {
            if (wrapTurtle(lineStartX, lineStartY, lineEndX, lineEndY, boundX, boundY, true, -1))
                continue;
        }

        if (lineEndY > boundY)
        {
            if (wrapTurtle(lineStartY, lineStartX, lineEndY, lineEndX, boundY, boundX, false, 1))
                continue;
        }

        if (lineEndY < -boundY)
        {
            if (wrapTurtle(lineStartY, lineStartX, lineEndY, lineEndX, boundY, boundX, false, -1))
                continue;
        }
    }

    turtlePosition = matrixWithNewXY(newPosition, lineEndX, lineEndY);
    Config::get().mainController()->setTurtlePos(turtlePosition);
    Config::get().mainController()->emitVertex();
}

// Move the turtle to a new position, but only if the new position is within the
// canvas. If the new position is outside the canvas, an error is thrown.

void Turtle::moveTurtleFence(const QTransform &newPosition)
{
    qreal lineEndX = newPosition.dx();
    qreal lineEndY = newPosition.dy();
    qreal boundX = Config::get().mainController()->boundX();
    qreal boundY = Config::get().mainController()->boundY();

    if ((lineEndX < -boundX) || (lineEndX > boundX) || (lineEndY < -boundY) || (lineEndY > boundY))
    {
        Error::turtleOutOfBounds();
    }
    turtlePosition = newPosition;
    Config::get().mainController()->setTurtlePos(turtlePosition);
    Config::get().mainController()->emitVertex();
}

// Move the turtle to a new position, adjusting the canvas boundaries,
// if needed.
void Turtle::moveTurtleWindow(const QTransform &newPosition)
{
    qreal candidateX = std::abs(newPosition.dx()) + penSize;
    qreal candidateY = std::abs(newPosition.dy()) + penSize;
    qreal boundX = Config::get().mainController()->boundX();
    qreal boundY = Config::get().mainController()->boundY();

    if ((candidateX > boundX) || (candidateY > boundY))
    {
        boundX = (candidateX > boundX) ? candidateX : boundX;
        boundY = (candidateY > boundY) ? candidateY : boundY;
        Config::get().mainController()->setBounds(boundX, boundY);
    }

    turtlePosition = newPosition;
    Config::get().mainController()->setTurtlePos(turtlePosition);
    Config::get().mainController()->emitVertex();
}

void Turtle::moveTurtle(const QTransform &newPosition)
{
    switch (mode)
    {
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

void Turtle::drawArc(qreal angle, qreal radius)
{
    Config::get().mainController()->drawArc(angle, radius);
}

void Turtle::forward(qreal steps)
{
    QTransform newPos = turtlePosition;
    newPos.translate(0, steps);
    moveTurtle(newPos);
}

void Turtle::rotate(qreal angle)
{
    turtlePosition.rotate(angle, Qt::ZAxis);
    Config::get().mainController()->setTurtlePos(turtlePosition);
}

void Turtle::getxy(qreal &x, qreal &y)
{
    x = turtlePosition.dx();
    y = turtlePosition.dy();
}

void Turtle::setMode(TurtleModeEnum newMode)
{
    mode = newMode;
    if (mode != turtleWindow)
    {
        qreal boundX = Config::get().mainController()->boundX();
        qreal boundY = Config::get().mainController()->boundY();
        qreal posX = turtlePosition.dx();
        qreal posY = turtlePosition.dy();
        if ((posX < -boundX) || (posX > boundX) || (posY < -boundY) || (posY > boundY))
        {
            moveTurtle(QTransform());
        }
    }
}

TurtleModeEnum Turtle::getMode()
{
    return mode;
}

qreal Turtle::getHeading()
{
    qreal s = turtlePosition.m12();
    qreal c = turtlePosition.m11();

    qreal retval = atan2(s, c) * 180 / M_PI;
    if (retval < 0)
        retval += 360;
    return retval;
}

void Turtle::setxy(qreal x, qreal y)
{
    QTransform newPosition(matrixWithNewXY(turtlePosition, x, y));
    moveTurtle(newPosition);
}

void Turtle::setx(qreal x)
{
    qreal y = turtlePosition.dy();
    setxy(x, y);
}

void Turtle::sety(qreal y)
{
    qreal x = turtlePosition.dx();
    setxy(x, y);
}

void Turtle::moveToHome()
{
    moveTurtle(QTransform());
}

void Turtle::setPenColor(const QColor &c)
{
    penColor = c;
    Config::get().mainController()->setCanvasForegroundColor(c);
}

const QColor &Turtle::getPenColor()
{
    return penColor;
}

void Turtle::setPenMode(PenModeEnum aPenMode)
{
    if (penMode != aPenMode)
    {
        penMode = aPenMode;
        Config::get().mainController()->setPenmode(penMode);
    }
}

PenModeEnum Turtle::getPenMode()
{
    return penMode;
}

void Turtle::setPenSize(qreal aPenSize)
{
    penSize = aPenSize;
    Config::get().mainController()->setPensize(penSize);
}

bool Turtle::isPenSizeValid(qreal aPenSize)
{
    return Config::get().mainController()->isPenSizeValid(aPenSize);
}

qreal Turtle::getPenSize()
{
    return penSize;
}

void Turtle::beginFillWithColor(const QColor &fillColor)
{
    if (isFilling)
    {
        Error::alreadyFilling();
    }
    isFilling = true;

    Config::get().mainController()->beginPolygon(fillColor);
}

void Turtle::endFill()
{
    isFilling = false;
    Config::get().mainController()->endPolygon();
}

DatumPtr Turtle::print()
{
    QString s = QString("%1 %2 %3\n"
                        "%4 %5 %6\n"
                        "%7 %8 %9\n")
                    .arg(turtlePosition.m11())
                    .arg(turtlePosition.m12())
                    .arg(turtlePosition.m13())
                    .arg(turtlePosition.m21())
                    .arg(turtlePosition.m22())
                    .arg(turtlePosition.m23())
                    .arg(turtlePosition.m31())
                    .arg(turtlePosition.m32())
                    .arg(turtlePosition.m33());

    return DatumPtr(s);
}
