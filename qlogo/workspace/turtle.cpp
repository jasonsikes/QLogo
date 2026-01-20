
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

#include "workspace/turtle.h"
#include "interface/logointerface.h"
#include "flowcontrol.h"
#include <cmath>
#include <limits>

// Some support functions for moving the turtle.

Turtle::Turtle() : turtleTransform(QTransform())
{
    Config::get().setMainTurtle(this);
    penColor = Config::get().initialCanvasForegroundColor;
}

Turtle::~Turtle()
{
    Config::get().setMainTurtle(nullptr);
}

void Turtle::setPenIsDown(bool aIsPenDown)
{
    penIsDown = aIsPenDown;
    Config::get().mainInterface()->setPenIsDown(penIsDown);
}

// Move the turtle. If over a boundary, wrap.
double Turtle::wrapTurtle(double lineStartU,
                        double lineStartV,
                        double lineEndU,
                        double lineEndV,
                        double boundU,
                        double boundV,
                        bool isXBoundary,
                        double mult)
{
    Q_ASSERT (std::abs(lineEndU - lineStartU) > std::numeric_limits<double>::epsilon());

    double crossV = lineStartV + (mult * boundU - lineStartU) * (lineEndV - lineStartV) / (lineEndU - lineStartU);
    if ((crossV >= -boundV) && (crossV <= boundV))
    {
        qreal m31 = isXBoundary ? mult * boundU : crossV;
        qreal m32 = isXBoundary ? crossV : mult * boundU;
        QTransform tempTurtleTransform(turtleTransform.m11(),
                                       turtleTransform.m12(),
                                       turtleTransform.m13(),
                                       turtleTransform.m21(),
                                       turtleTransform.m22(),
                                       turtleTransform.m23(),
                                       m31, m32, turtleTransform.m33());
        Config::get().mainInterface()->setTurtlePos(&tempTurtleTransform);
        Config::get().mainInterface()->emitVertex();
        if (penIsDown)
            Config::get().mainInterface()->setPenIsDown(false);

        m31 = isXBoundary ? -mult * boundU : crossV;
        m32 = isXBoundary ? crossV : -mult * boundU;
        turtleTransform = QTransform{turtleTransform.m11(),
                                    turtleTransform.m12(),
                                    turtleTransform.m13(),
                                    turtleTransform.m21(),
                                    turtleTransform.m22(),
                                    turtleTransform.m23(),
                                    m31, m32, turtleTransform.m33()};

        Config::get().mainInterface()->setTurtlePos(&turtleTransform);
        Config::get().mainInterface()->emitVertex();
        if (penIsDown)
            Config::get().mainInterface()->setPenIsDown(true);
        lineEndU -= 2 * mult * boundU;
    }
    return lineEndU;
}

// Move the turtle to a new position, wrapping around the edges of the canvas if
// the new position is outside the canvas.

void Turtle::moveTurtleWrap(const QTransform &newTransform)
{
    double lineEndX = newTransform.dx();
    double lineEndY = newTransform.dy();
    double boundX = Config::get().mainInterface()->boundX();
    double boundY = Config::get().mainInterface()->boundY();

    while ((lineEndX < -boundX) || (lineEndX > boundX) || (lineEndY < -boundY) || (lineEndY > boundY))
    {
        double lineStartX = turtleTransform.dx();
        double lineStartY = turtleTransform.dy();

        if (lineEndX > boundX)
        {
            double newLineEndX = wrapTurtle(lineStartX, lineStartY, lineEndX, lineEndY, boundX, boundY, true, 1);
            bool wrapped = (newLineEndX != lineEndX);
            lineEndX = newLineEndX;
            if (wrapped)
                continue;
        }

        if (lineEndX < -boundX)
        {
            double newLineEndX = wrapTurtle(lineStartX, lineStartY, lineEndX, lineEndY, boundX, boundY, true, -1);
            bool wrapped = (newLineEndX != lineEndX);
            lineEndX = newLineEndX;
            if (wrapped)
                continue;
        }

        if (lineEndY > boundY)
        {
            double newLineEndY = wrapTurtle(lineStartY, lineStartX, lineEndY, lineEndX, boundY, boundX, false, 1);
            bool wrapped = (newLineEndY != lineEndY);
            lineEndY = newLineEndY;
            if (wrapped)
                continue;
        }

        if (lineEndY < -boundY)
        {
            double newLineEndY = wrapTurtle(lineStartY, lineStartX, lineEndY, lineEndX, boundY, boundX, false, -1);
            bool wrapped = (newLineEndY != lineEndY);
            lineEndY = newLineEndY;
            if (wrapped)
                continue;
        }
    }

    turtleTransform = QTransform{newTransform.m11(),
                                 newTransform.m12(),
                                 newTransform.m13(),
                                 newTransform.m21(),
                                 newTransform.m22(),
                                 newTransform.m23(),
                                 lineEndX,
                                 lineEndY,
                                 newTransform.m33()};
    Config::get().mainInterface()->setTurtlePos(&turtleTransform);
    Config::get().mainInterface()->emitVertex();
}

// Move the turtle to a new position, but only if the new position is within the
// canvas. If the new position is outside the canvas, an error is thrown.

void Turtle::moveTurtleFence(const QTransform &newTransform)
{
    double lineEndX = newTransform.dx();
    double lineEndY = newTransform.dy();
    double boundX = Config::get().mainInterface()->boundX();
    double boundY = Config::get().mainInterface()->boundY();

    if ((lineEndX < -boundX) || (lineEndX > boundX) || (lineEndY < -boundY) || (lineEndY > boundY))
    {
        throw FCError::turtleOutOfBounds();
    }
    turtleTransform = newTransform;
    Config::get().mainInterface()->setTurtlePos(&turtleTransform);
    Config::get().mainInterface()->emitVertex();
}

// Move the turtle to a new position, adjusting the canvas boundaries,
// if needed.
void Turtle::moveTurtleWindow(const QTransform &newTransform)
{
    double candidateX = std::abs(newTransform.dx()) + penSize;
    double candidateY = std::abs(newTransform.dy()) + penSize;
    double boundX = Config::get().mainInterface()->boundX();
    double boundY = Config::get().mainInterface()->boundY();

    if ((candidateX > boundX) || (candidateY > boundY))
    {
        boundX = (candidateX > boundX) ? candidateX : boundX;
        boundY = (candidateY > boundY) ? candidateY : boundY;
        Config::get().mainInterface()->setBounds(boundX, boundY);
    }

    turtleTransform = newTransform;
    Config::get().mainInterface()->setTurtlePos(&turtleTransform);
    Config::get().mainInterface()->emitVertex();
}

void Turtle::moveTurtle(const QTransform &newTransform)
{
    switch (mode)
    {
    case turtleWrap:
        moveTurtleWrap(newTransform);
        break;
    case turtleFence:
        moveTurtleFence(newTransform);
        break;
    case turtleWindow:
        moveTurtleWindow(newTransform);
        break;
    default:
        qWarning() << "Invalid turtle mode: " << mode;
        moveTurtleWindow(newTransform);
        break;
    }
}

void Turtle::drawArc(double angle, double radius)
{
    Config::get().mainInterface()->drawArc(angle, radius);
}

void Turtle::forward(double steps)
{
    QTransform newTransform(turtleTransform.m11(),
                           turtleTransform.m12(),
                           turtleTransform.m13(),
                           turtleTransform.m21(),
                           turtleTransform.m22(),
                           turtleTransform.m23(),
                           turtleTransform.dx() + steps * turtleTransform.m21(),
                           turtleTransform.dy() + steps * turtleTransform.m22(),
                           turtleTransform.m33());
    moveTurtle(newTransform);
}

void Turtle::rotate(double angle)
{
    // Logo uses clockwise rotation (positive angles rotate clockwise),
    // but QTransform::rotate() uses counter-clockwise rotation (standard math convention).
    // Negate the angle to match Logo's behavior.
    turtleTransform.rotate(-angle);
    Config::get().mainInterface()->setTurtlePos(&turtleTransform);
}

std::pair<double, double> Turtle::getxy() const
{
    return {turtleTransform.dx(), turtleTransform.dy()};
}

void Turtle::setMode(TurtleModeEnum newMode)
{
    mode = newMode;
    if (mode != turtleWindow)
    {
        double boundX = Config::get().mainInterface()->boundX();
        double boundY = Config::get().mainInterface()->boundY();
        double posX = turtleTransform.dx();
        double posY = turtleTransform.dy();
        if ((posX < -boundX) || (posX > boundX) || (posY < -boundY) || (posY > boundY))
        {
            // Move the turtle to the home position when switching to bounded mode
            // and the turtle is out of bounds
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
    constexpr double RADIANS_TO_DEGREES = 180.0 / PI;
    double s = turtleTransform.m12();
    double c = turtleTransform.m11();

    double retval = atan2(s, c) * RADIANS_TO_DEGREES;
    while (retval < 0)
        retval += 360;
    return retval;
}

void Turtle::setxy(double x, double y)
{
    QTransform newTransform(turtleTransform.m11(),
                           turtleTransform.m12(),
                           turtleTransform.m13(),
                           turtleTransform.m21(),
                           turtleTransform.m22(),
                           turtleTransform.m23(),
                           x,
                           y,
                           turtleTransform.m33());
    moveTurtle(newTransform);
}

void Turtle::setx(double x)
{
    double y = turtleTransform.dy();
    setxy(x, y);
}

void Turtle::sety(double y)
{
    double x = turtleTransform.dx();
    setxy(x, y);
}

void Turtle::moveToHome()
{
    moveTurtle(QTransform());
}

void Turtle::setPenColor(const QColor &c)
{
    penColor = c;
    Config::get().mainInterface()->setCanvasForegroundColor(c);
}

const QColor &Turtle::getPenColor() const
{
    return penColor;
}

std::pair<double, double> Turtle::getScale() const
{
    return {scaleX, scaleY};
}

void Turtle::setScale(double newScaleX, double newScaleY)
{
    Q_ASSERT(std::abs(scaleX) > std::numeric_limits<double>::epsilon());
    Q_ASSERT(std::abs(scaleY) > std::numeric_limits<double>::epsilon());

    double ratioX = newScaleX / scaleX;
    double ratioY = newScaleY / scaleY;

    turtleTransform.scale(ratioX, ratioY);

    scaleX = newScaleX;
    scaleY = newScaleY;

    Config::get().mainInterface()->setTurtlePos(&turtleTransform);
}

void Turtle::setPenMode(PenModeEnum aPenMode)
{
    if (penMode != aPenMode)
    {
        penMode = aPenMode;
        Config::get().mainInterface()->setPenmode(penMode);
    }
}

PenModeEnum Turtle::getPenMode() const
{
    return penMode;
}

void Turtle::setPenSize(double aPenSize)
{
    penSize = aPenSize;
    Config::get().mainInterface()->setPensize(penSize);
}

bool Turtle::isPenSizeValid(double aPenSize) const
{
    return Config::get().mainInterface()->isPenSizeValid(aPenSize);
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

    Config::get().mainInterface()->beginPolygon(fillColor);
}

void Turtle::endFill()
{
    isFilling = false;
    Config::get().mainInterface()->endPolygon();
}

DatumPtr Turtle::print()
{
    QString s = QString("%1 %2 %3\n"
                        "%4 %5 %6\n"
                        "%7 %8 %9\n")
                    .arg(turtleTransform.m11())
                    .arg(turtleTransform.m12())
                    .arg(turtleTransform.m13())
                    .arg(turtleTransform.m21())
                    .arg(turtleTransform.m22())
                    .arg(turtleTransform.m23())
                    .arg(turtleTransform.dx())
                    .arg(turtleTransform.dy())
                    .arg(turtleTransform.m33());

    return DatumPtr(s);
}
