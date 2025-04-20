
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
#include "flowcontrol.h"
#include <math.h>

// Some support functions for moving the turtle.

Turtle::Turtle() : turtlePosition(Transform()), turtleIsVisible(true), penIsDown(true)
{
    Config::get().setMainTurtle(this);
    penColor = Config::get().initialCanvasForegroundColor;
    mode = turtleWindow;
}

Turtle::~Turtle()
{
    Config::get().setMainTurtle(nullptr);
}

void Turtle::setPenIsDown(bool aIsPenDown)
{
    penIsDown = aIsPenDown;
    Config::get().mainController()->setPenIsDown(penIsDown);
}

// Move the turtle. If over a boundary, wrap.
bool Turtle::wrapTurtle(double lineStartU,
                        double lineStartV,
                        double &lineEndU,
                        double lineEndV,
                        double boundU,
                        double boundV,
                        bool isXBoundary,
                        double mult)
{
    double crossV = lineStartV + (mult * boundU - lineStartU) * (lineEndV - lineStartV) / (lineEndU - lineStartU);
    if ((crossV >= -boundV) && (crossV <= boundV))
    {
        Transform tempTurtlePos = turtlePosition;
        if (isXBoundary)
        {
            tempTurtlePos.m[6] = mult * boundU;
            tempTurtlePos.m[7] = crossV;
        }
        else
        {
            tempTurtlePos.m[6] = crossV;
            tempTurtlePos.m[7] = mult * boundU;
        }
        Config::get().mainController()->setTurtlePos(tempTurtlePos);
        Config::get().mainController()->emitVertex();
        if (penIsDown)
            Config::get().mainController()->setPenIsDown(false);

        if (isXBoundary)
        {
            turtlePosition.m[6] = -mult * boundU;
            turtlePosition.m[7] = crossV;
        }
        else
        {
            turtlePosition.m[6] = crossV;
            turtlePosition.m[7] = -mult * boundU;
        }
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

void Turtle::moveTurtleWrap(const Transform &newPosition)
{
    double lineEndX = newPosition.m[6];
    double lineEndY = newPosition.m[7];
    double boundX = Config::get().mainController()->boundX();
    double boundY = Config::get().mainController()->boundY();

    while ((lineEndX < -boundX) || (lineEndX > boundX) || (lineEndY < -boundY) || (lineEndY > boundY))
    {
        double lineStartX = turtlePosition.m[6];
        double lineStartY = turtlePosition.m[7];

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

    turtlePosition = newPosition;
    turtlePosition.m[6] = lineEndX;
    turtlePosition.m[7] = lineEndY;
    Config::get().mainController()->setTurtlePos(turtlePosition);
    Config::get().mainController()->emitVertex();
}

// Move the turtle to a new position, but only if the new position is within the
// canvas. If the new position is outside the canvas, an error is thrown.

void Turtle::moveTurtleFence(const Transform &newPosition)
{
    double lineEndX = newPosition.m[6];
    double lineEndY = newPosition.m[7];
    double boundX = Config::get().mainController()->boundX();
    double boundY = Config::get().mainController()->boundY();

    if ((lineEndX < -boundX) || (lineEndX > boundX) || (lineEndY < -boundY) || (lineEndY > boundY))
    {
        throw FCError::turtleOutOfBounds();
    }
    turtlePosition = newPosition;
    Config::get().mainController()->setTurtlePos(turtlePosition);
    Config::get().mainController()->emitVertex();
}

// Move the turtle to a new position, adjusting the canvas boundaries,
// if needed.
void Turtle::moveTurtleWindow(const Transform &newPosition)
{
    double candidateX = std::abs(newPosition.m[6]) + penSize;
    double candidateY = std::abs(newPosition.m[7]) + penSize;
    double boundX = Config::get().mainController()->boundX();
    double boundY = Config::get().mainController()->boundY();

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

void Turtle::moveTurtle(const Transform &newPosition)
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
    Transform newPos = turtlePosition;
    newPos.m[6] += steps * turtlePosition.m[3];
    newPos.m[7] += steps * turtlePosition.m[4];
    moveTurtle(newPos);
}

void Turtle::rotate(double angle)
{
    double rad = angle * M_PI / 180.0;
    double sina = std::sin(rad);
    double cosa = std::cos(rad);

    double m11 = turtlePosition.m[0];
    double m12 = turtlePosition.m[1];
    double m21 = turtlePosition.m[3];
    double m22 = turtlePosition.m[4];

    turtlePosition.m[0] = cosa * m11 - sina * m21;
    turtlePosition.m[1] = cosa * m12 - sina * m22;
    turtlePosition.m[3] = sina * m11 + cosa * m21;
    turtlePosition.m[4] = sina * m12 + cosa * m22;
    Config::get().mainController()->setTurtlePos(turtlePosition);
}

void Turtle::getxy(double &x, double &y)
{
    x = turtlePosition.m[6];
    y = turtlePosition.m[7];
}

void Turtle::setMode(TurtleModeEnum newMode)
{
    mode = newMode;
    if (mode != turtleWindow)
    {
        double boundX = Config::get().mainController()->boundX();
        double boundY = Config::get().mainController()->boundY();
        double posX = turtlePosition.m[6];
        double posY = turtlePosition.m[7];
        if ((posX < -boundX) || (posX > boundX) || (posY < -boundY) || (posY > boundY))
        {
            moveTurtle(Transform());
        }
    }
}

TurtleModeEnum Turtle::getMode()
{
    return mode;
}

double Turtle::getHeading()
{
    double s = turtlePosition.m[1];
    double c = turtlePosition.m[0];

    double retval = atan2(s, c) * 180 / M_PI;
    if (retval < 0)
        retval += 360;
    return retval;
}

void Turtle::setxy(double x, double y)
{
    Transform newPosition(turtlePosition);
    newPosition.m[6] = x;
    newPosition.m[7] = y;
    moveTurtle(newPosition);
}

void Turtle::setx(double x)
{
    double y = turtlePosition.m[7];
    setxy(x, y);
}

void Turtle::sety(double y)
{
    double x = turtlePosition.m[6];
    setxy(x, y);
}

void Turtle::moveToHome()
{
    moveTurtle(Transform());
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
        throw FCError::alreadyFilling();
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
                    .arg(turtlePosition.m[0])
                    .arg(turtlePosition.m[1])
                    .arg(turtlePosition.m[2])
                    .arg(turtlePosition.m[3])
                    .arg(turtlePosition.m[4])
                    .arg(turtlePosition.m[5])
                    .arg(turtlePosition.m[6])
                    .arg(turtlePosition.m[7])
                    .arg(turtlePosition.m[8]);

    return DatumPtr(s);
}
