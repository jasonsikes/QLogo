
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

QTransform matrixWithNewXY(const QTransform &src, double x, double y)
{
    return QTransform(src.m11(), src.m12(), src.m13(), src.m21(), src.m22(), src.m23(), x, y, src.m33());
}

void Turtle::setPenIsDown(bool aIsPenDown)
{
    penIsDown = aIsPenDown;
    Config::get().mainController()->setPenIsDown(penIsDown);
}

// Move the turtle to a new position, wrapping around the edges of the canvas if
// the new position is outside the canvas.

void Turtle::moveTurtleWrap(const QTransform &newPosition)
{
    double lineEndX = newPosition.dx();
    double lineEndY = newPosition.dy();
    double boundX = Config::get().mainController()->boundX();
    double boundY = Config::get().mainController()->boundY();

    while ((lineEndX < -boundX) || (lineEndX > boundX) || (lineEndY < -boundY) || (lineEndY > boundY))
    {
        double lineStartX = turtlePosition.dx();
        double lineStartY = turtlePosition.dy();

        if (lineEndX > boundX)
        {
            double cY = lineStartY + (boundX - lineStartX) * (lineEndY - lineStartY) / (lineEndX - lineStartX);
            if ((cY >= -boundY) && (cY <= boundY))
            {
                Config::get().mainController()->setTurtlePos(matrixWithNewXY(turtlePosition, boundX, cY));
                Config::get().mainController()->emitVertex();
                if (penIsDown)
                    Config::get().mainController()->setPenIsDown(false);
                turtlePosition = matrixWithNewXY(turtlePosition, -boundX, cY);
                Config::get().mainController()->setTurtlePos(turtlePosition);
                Config::get().mainController()->emitVertex();
                if (penIsDown)
                    Config::get().mainController()->setPenIsDown(true);
                lineEndX -= 2 * boundX;

                continue;
            }
        }

        if (lineEndX < -boundX)
        {
            double cY = lineStartY + (-boundX - lineStartX) * (lineEndY - lineStartY) / (lineEndX - lineStartX);
            if ((cY >= -boundY) && (cY <= boundY))
            {
                Config::get().mainController()->setTurtlePos(matrixWithNewXY(turtlePosition, -boundX, cY));
                Config::get().mainController()->emitVertex();
                if (penIsDown)
                    Config::get().mainController()->setPenIsDown(false);
                turtlePosition = matrixWithNewXY(turtlePosition, boundX, cY);
                Config::get().mainController()->setTurtlePos(turtlePosition);
                Config::get().mainController()->emitVertex();
                if (penIsDown)
                    Config::get().mainController()->setPenIsDown(true);
                lineEndX += 2 * boundX;

                continue;
            }
        }

        if (lineEndY > boundY)
        {
            double cX = lineStartX + (boundY - lineStartY) * (lineEndX - lineStartX) / (lineEndY - lineStartY);
            if ((cX >= -boundX) && (cX <= boundX))
            {
                Config::get().mainController()->setTurtlePos(matrixWithNewXY(turtlePosition, cX, boundY));
                Config::get().mainController()->emitVertex();
                if (penIsDown)
                    Config::get().mainController()->setPenIsDown(false);
                turtlePosition = matrixWithNewXY(turtlePosition, cX, -boundY);
                Config::get().mainController()->setTurtlePos(turtlePosition);
                Config::get().mainController()->emitVertex();
                if (penIsDown)
                    Config::get().mainController()->setPenIsDown(true);
                lineEndY -= 2 * boundY;

                continue;
            }
        }

        if (lineEndY < -boundY)
        {
            double cX = lineStartX + (-boundY - lineStartY) * (lineEndX - lineStartX) / (lineEndY - lineStartY);
            if ((cX >= -boundX) && (cX <= boundX))
            {
                Config::get().mainController()->setTurtlePos(matrixWithNewXY(turtlePosition, cX, -boundY));
                Config::get().mainController()->emitVertex();
                if (penIsDown)
                    Config::get().mainController()->setPenIsDown(false);
                turtlePosition = matrixWithNewXY(turtlePosition, cX, boundY);
                Config::get().mainController()->setTurtlePos(turtlePosition);
                Config::get().mainController()->emitVertex();
                if (penIsDown)
                    Config::get().mainController()->setPenIsDown(true);
                lineEndY += 2 * boundY;

                continue;
            }
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
    double lineEndX = newPosition.dx();
    double lineEndY = newPosition.dy();
    double boundX = Config::get().mainController()->boundX();
    double boundY = Config::get().mainController()->boundY();

    if ((lineEndX < -boundX) || (lineEndX > boundX) || (lineEndY < -boundY) || (lineEndY > boundY))
    {
        Error::turtleOutOfBounds();
    }
    turtlePosition = newPosition;
    Config::get().mainController()->setTurtlePos(turtlePosition);
    Config::get().mainController()->emitVertex();
}

// Move the turtle to a new position, ignoring the canvas boundaries.

void Turtle::moveTurtleWindow(const QTransform &newPosition)
{
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

void Turtle::drawArc(double angle, double radius)
{
    Config::get().mainController()->drawArc(angle, radius);
}

void Turtle::forward(double steps)
{
    QTransform newPos = turtlePosition;
    newPos.translate(0, steps);
    moveTurtle(newPos);
}

void Turtle::rotate(double angle)
{
    turtlePosition.rotate(angle, Qt::ZAxis);
    Config::get().mainController()->setTurtlePos(turtlePosition);
}

void Turtle::getxy(double &x, double &y)
{
    x = turtlePosition.dx();
    y = turtlePosition.dy();
}

void Turtle::setMode(TurtleModeEnum newMode)
{
    mode = newMode;
    if (mode != turtleWindow)
    {
        double boundX = Config::get().mainController()->boundX();
        double boundY = Config::get().mainController()->boundY();
        double posX = turtlePosition.dx();
        double posY = turtlePosition.dy();
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

double Turtle::getHeading()
{
    double s = turtlePosition.m12();
    double c = turtlePosition.m11();

    double retval = atan2(s, c) * 180 / M_PI;
    if (retval < 0)
        retval += 360;
    return retval;
}

void Turtle::setxy(double x, double y)
{
    QTransform newPosition(matrixWithNewXY(turtlePosition, x, y));
    moveTurtle(newPosition);
}

void Turtle::setx(double x)
{
    double y = turtlePosition.dy();
    setxy(x, y);
}

void Turtle::sety(double y)
{
    double x = turtlePosition.dx();
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

void Turtle::setPenSize(double aPenSize)
{
    penSize = aPenSize;
    Config::get().mainController()->setPensize(penSize);
}

bool Turtle::isPenSizeValid(double aPenSize)
{
    return Config::get().mainController()->isPenSizeValid(aPenSize);
}

double Turtle::getPenSize()
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
