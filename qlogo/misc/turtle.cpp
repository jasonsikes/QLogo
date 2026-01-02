
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

#include "turtle.h"
#include "controller/logocontroller.h"
#include "flowcontrol.h"

// Some support functions for moving the turtle.

Turtle::Turtle() : turtlePosition(QTransform()), turtleIsVisible(true), penIsDown(true)
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
        QTransform tempTurtlePos = turtlePosition;
        if (isXBoundary)
        {
            tempTurtlePos = QTransform(tempTurtlePos.m11(),
                                       tempTurtlePos.m12(),
                                       tempTurtlePos.m13(),
                                       tempTurtlePos.m21(),
                                       tempTurtlePos.m22(),
                                       tempTurtlePos.m23(),
                                       mult * boundU,
                                       crossV,
                                       tempTurtlePos.m33());
        }
        else
        {
            tempTurtlePos = QTransform(tempTurtlePos.m11(),
                                       tempTurtlePos.m12(),
                                       tempTurtlePos.m13(),
                                       tempTurtlePos.m21(),
                                       tempTurtlePos.m22(),
                                       tempTurtlePos.m23(),
                                       crossV,
                                       mult * boundU,
                                       tempTurtlePos.m33());
        }
        Config::get().mainController()->setTurtlePos(&tempTurtlePos);
        Config::get().mainController()->emitVertex();
        if (penIsDown)
            Config::get().mainController()->setPenIsDown(false);

        if (isXBoundary)
        {
            turtlePosition = QTransform(turtlePosition.m11(),
                                        turtlePosition.m12(),
                                        turtlePosition.m13(),
                                        turtlePosition.m21(),
                                        turtlePosition.m22(),
                                        turtlePosition.m23(),
                                        -mult * boundU,
                                        crossV,
                                        turtlePosition.m33());
        }
        else
        {
            turtlePosition = QTransform(turtlePosition.m11(),
                                        turtlePosition.m12(),
                                        turtlePosition.m13(),
                                        turtlePosition.m21(),
                                        turtlePosition.m22(),
                                        turtlePosition.m23(),
                                        crossV,
                                        -mult * boundU,
                                        turtlePosition.m33());
        }
        Config::get().mainController()->setTurtlePos(&turtlePosition);
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

    turtlePosition = QTransform(newPosition.m11(),
                                newPosition.m12(),
                                newPosition.m13(),
                                newPosition.m21(),
                                newPosition.m22(),
                                newPosition.m23(),
                                lineEndX,
                                lineEndY,
                                newPosition.m33());
    Config::get().mainController()->setTurtlePos(&turtlePosition);
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
        throw FCError::turtleOutOfBounds();
    }
    turtlePosition = newPosition;
    Config::get().mainController()->setTurtlePos(&turtlePosition);
    Config::get().mainController()->emitVertex();
}

// Move the turtle to a new position, adjusting the canvas boundaries,
// if needed.
void Turtle::moveTurtleWindow(const QTransform &newPosition)
{
    double candidateX = std::abs(newPosition.dx()) + penSize;
    double candidateY = std::abs(newPosition.dy()) + penSize;
    double boundX = Config::get().mainController()->boundX();
    double boundY = Config::get().mainController()->boundY();

    if ((candidateX > boundX) || (candidateY > boundY))
    {
        boundX = (candidateX > boundX) ? candidateX : boundX;
        boundY = (candidateY > boundY) ? candidateY : boundY;
        Config::get().mainController()->setBounds(boundX, boundY);
    }

    turtlePosition = newPosition;
    Config::get().mainController()->setTurtlePos(&turtlePosition);
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
    QTransform newPos = QTransform(turtlePosition.m11(),
                                   turtlePosition.m12(),
                                   turtlePosition.m13(),
                                   turtlePosition.m21(),
                                   turtlePosition.m22(),
                                   turtlePosition.m23(),
                                   turtlePosition.dx() + steps * turtlePosition.m21(),
                                   turtlePosition.dy() + steps * turtlePosition.m22(),
                                   turtlePosition.m33());
    moveTurtle(newPos);
}

void Turtle::rotate(double angle)
{
    // Logo uses clockwise rotation (positive angles rotate clockwise),
    // but QTransform::rotate() uses counter-clockwise rotation (standard math convention).
    // Negate the angle to match Logo's behavior.
    turtlePosition.rotate(-angle);
    Config::get().mainController()->setTurtlePos(&turtlePosition);
}

void Turtle::getxy(double &x, double &y) const
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

TurtleModeEnum Turtle::getMode() const
{
    return mode;
}

double Turtle::getHeading() const
{
    double s = turtlePosition.m12();
    double c = turtlePosition.m11();

    double retval = atan2(s, c) * (180.0 / PI);
    if (retval < 0)
        retval += 360;
    return retval;
}

void Turtle::setxy(double x, double y)
{
    QTransform newPosition = QTransform(turtlePosition.m11(),
                                        turtlePosition.m12(),
                                        turtlePosition.m13(),
                                        turtlePosition.m21(),
                                        turtlePosition.m22(),
                                        turtlePosition.m23(),
                                        x,
                                        y,
                                        turtlePosition.m33());
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

const QColor &Turtle::getPenColor() const
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

PenModeEnum Turtle::getPenMode() const
{
    return penMode;
}

void Turtle::setPenSize(double aPenSize)
{
    penSize = aPenSize;
    Config::get().mainController()->setPensize(penSize);
}

bool Turtle::isPenSizeValid(double aPenSize) const
{
    return Config::get().mainController()->isPenSizeValid(aPenSize);
}

double Turtle::getPenSize() const
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
                    .arg(turtlePosition.m11())
                    .arg(turtlePosition.m12())
                    .arg(turtlePosition.m13())
                    .arg(turtlePosition.m21())
                    .arg(turtlePosition.m22())
                    .arg(turtlePosition.m23())
                    .arg(turtlePosition.dx())
                    .arg(turtlePosition.dy())
                    .arg(turtlePosition.m33());

    return DatumPtr(s);
}
