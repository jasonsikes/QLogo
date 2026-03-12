
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


Turtle::Turtle() : turtleTransform_(QTransform())
{
    penColor_ = Config::get().initialCanvasForegroundColor_;
}

void Turtle::setPenIsDown(bool aIsPenDown)
{
    penIsDown_ = aIsPenDown;
    Config::get().mainInterface()->setPenIsDown(penIsDown_);
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
        QTransform tempTurtleTransform(turtleTransform_.m11(),
                                       turtleTransform_.m12(),
                                       turtleTransform_.m13(),
                                       turtleTransform_.m21(),
                                       turtleTransform_.m22(),
                                       turtleTransform_.m23(),
                                       m31, m32, turtleTransform_.m33());
        Config::get().mainInterface()->setTurtlePos(&tempTurtleTransform);
        Config::get().mainInterface()->emitVertex();
        if (penIsDown_)
            Config::get().mainInterface()->setPenIsDown(false);

        m31 = isXBoundary ? -mult * boundU : crossV;
        m32 = isXBoundary ? crossV : -mult * boundU;
        turtleTransform_ = QTransform{turtleTransform_.m11(),
                                    turtleTransform_.m12(),
                                    turtleTransform_.m13(),
                                    turtleTransform_.m21(),
                                    turtleTransform_.m22(),
                                    turtleTransform_.m23(),
                                    m31, m32, turtleTransform_.m33()};

        Config::get().mainInterface()->setTurtlePos(&turtleTransform_);
        Config::get().mainInterface()->emitVertex();
        if (penIsDown_)
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
        double lineStartX = turtleTransform_.dx();
        double lineStartY = turtleTransform_.dy();

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

    turtleTransform_ = QTransform{newTransform.m11(),
                                 newTransform.m12(),
                                 newTransform.m13(),
                                 newTransform.m21(),
                                 newTransform.m22(),
                                 newTransform.m23(),
                                 lineEndX,
                                 lineEndY,
                                 newTransform.m33()};
    Config::get().mainInterface()->setTurtlePos(&turtleTransform_);
    Config::get().mainInterface()->emitVertex();
}

// Returns the point where the line from start to end first crosses a boundary,
// or the end point if it remains within bounds.
QPointF clampLineToBounds(double lineStartX, double lineStartY,
                          double lineEndX, double lineEndY,
                          double boundX, double boundY)
{
    const double dx = lineEndX - lineStartX;
    const double dy = lineEndY - lineStartY;
    double minT = 1.0;

    const auto considerBoundary = [&](double denom, double numerator) {
        if (std::abs(denom) > std::numeric_limits<double>::epsilon())
        {
            const double t = numerator / denom;
            if (t >= 0 && t < minT && t <= 1.0)
                minT = t;
        }
    };

    considerBoundary(dx, boundX - lineStartX);
    considerBoundary(dx, -boundX - lineStartX);
    considerBoundary(dy, boundY - lineStartY);
    considerBoundary(dy, -boundY - lineStartY);

    QPointF result(lineStartX + minT * dx, lineStartY + minT * dy);

    // When stopped at a boundary, nudge inward so we stay just inside
    if (minT < 1.0)
    {
        const double eps = std::max({boundX, boundY, 1.0}) * 1e-9;
        const double tol = std::max(eps, std::numeric_limits<double>::epsilon() * 10);
        if (std::abs(result.x() - boundX) < tol)
            result.setX(boundX - eps);
        else if (std::abs(result.x() + boundX) < tol)
            result.setX(-boundX + eps);
        if (std::abs(result.y() - boundY) < tol)
            result.setY(boundY - eps);
        else if (std::abs(result.y() + boundY) < tol)
            result.setY(-boundY + eps);
    }

    return result;
}

// Move the turtle to a new position, stopping at the boundary if the destination
// would be outside the canvas.
// returns an error if the turtle is out of bounds, else returns nullptr.
Datum* Turtle::moveTurtleFence(const QTransform &newTransform)
{
    const double lineStartX = turtleTransform_.dx();
    const double lineStartY = turtleTransform_.dy();
    const double lineEndX = newTransform.dx();
    const double lineEndY = newTransform.dy();
    const double boundX = Config::get().mainInterface()->boundX();
    const double boundY = Config::get().mainInterface()->boundY();

    const QPointF clamped = clampLineToBounds(lineStartX, lineStartY,
                                              lineEndX, lineEndY,
                                              boundX, boundY);

    turtleTransform_ = QTransform{newTransform.m11(),
                                 newTransform.m12(),
                                 newTransform.m13(),
                                 newTransform.m21(),
                                 newTransform.m22(),
                                 newTransform.m23(),
                                 clamped.x(),
                                 clamped.y(),
                                 newTransform.m33()};
    Config::get().mainInterface()->setTurtlePos(&turtleTransform_);
    Config::get().mainInterface()->emitVertex();

    if (lineEndX != clamped.x() || lineEndY != clamped.y())
    {
        return FCError::turtleOutOfBounds();
    }
    return nullptr;
}

// Move the turtle to a new position, adjusting the canvas boundaries,
// if needed.
void Turtle::moveTurtleWindow(const QTransform &newTransform)
{
    double candidateX = std::abs(newTransform.dx()) + penSize_;
    double candidateY = std::abs(newTransform.dy()) + penSize_;
    double boundX = Config::get().mainInterface()->boundX();
    double boundY = Config::get().mainInterface()->boundY();

    if ((candidateX > boundX) || (candidateY > boundY))
    {
        boundX = (candidateX > boundX) ? candidateX : boundX;
        boundY = (candidateY > boundY) ? candidateY : boundY;
        Config::get().mainInterface()->setBounds(boundX, boundY);
    }

    turtleTransform_ = newTransform;
    Config::get().mainInterface()->setTurtlePos(&turtleTransform_);
    Config::get().mainInterface()->emitVertex();
}

Datum* Turtle::moveTurtle(const QTransform &newTransform)
{
    switch (mode_)
    {
    case turtleWrap:
        moveTurtleWrap(newTransform);
        return nullptr;
    case turtleFence:
        return moveTurtleFence(newTransform);
    case turtleWindow:
        moveTurtleWindow(newTransform);
        return nullptr;
    default:
        qWarning() << "Invalid turtle mode: " << mode_;
        moveTurtleWindow(newTransform);
        return nullptr;
    }
}

void Turtle::drawArc(double angle, double radius)
{
    Config::get().mainInterface()->drawArc(angle, radius);
}

Datum* Turtle::forward(double steps)
{
    QTransform newTransform(turtleTransform_.m11(),
                           turtleTransform_.m12(),
                           turtleTransform_.m13(),
                           turtleTransform_.m21(),
                           turtleTransform_.m22(),
                           turtleTransform_.m23(),
                           turtleTransform_.dx() + steps * turtleTransform_.m21(),
                           turtleTransform_.dy() + steps * turtleTransform_.m22(),
                           turtleTransform_.m33());
    return moveTurtle(newTransform);
}

void Turtle::rotate(double angle)
{
    // Logo uses clockwise rotation (positive angles rotate clockwise),
    // but QTransform::rotate() uses counter-clockwise rotation (standard math convention).
    // Negate the angle to match Logo's behavior.
    turtleTransform_.rotate(-angle);
    Config::get().mainInterface()->setTurtlePos(&turtleTransform_);
}

QPointF Turtle::getxy() const
{
    return {turtleTransform_.dx(), turtleTransform_.dy()};
}

void Turtle::setMode(TurtleModeEnum newMode)
{
    mode_ = newMode;
    if (mode_ != turtleWindow)
    {
        double boundX = Config::get().mainInterface()->boundX();
        double boundY = Config::get().mainInterface()->boundY();
        double posX = turtleTransform_.dx();
        double posY = turtleTransform_.dy();
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
    return mode_;
}

double Turtle::getHeading() const
{
    constexpr double RADIANS_TO_DEGREES = 180.0 / PI;
    double s = turtleTransform_.m12();
    double c = turtleTransform_.m11();

    double retval = atan2(s, c) * RADIANS_TO_DEGREES;
    while (retval < 0)
        retval += 360;
    return retval;
}

Datum* Turtle::setxy(double x, double y)
{
    QTransform newTransform(turtleTransform_.m11(),
                           turtleTransform_.m12(),
                           turtleTransform_.m13(),
                           turtleTransform_.m21(),
                           turtleTransform_.m22(),
                           turtleTransform_.m23(),
                           x,
                           y,
                           turtleTransform_.m33());
    return moveTurtle(newTransform);
}

Datum* Turtle::setx(double x)
{
    double y = turtleTransform_.dy();
    return setxy(x, y);
}

Datum* Turtle::sety(double y)
{
    double x = turtleTransform_.dx();
    return setxy(x, y);
}

void Turtle::moveToHome()
{
    moveTurtle(QTransform());
}

void Turtle::setPenColor(const QColor &c)
{
    penColor_ = c;
    Config::get().mainInterface()->setCanvasForegroundColor(c);
}

const QColor &Turtle::getPenColor() const
{
    return penColor_;
}

QSizeF Turtle::getScale() const
{
    return {scaleX_, scaleY_};
}

void Turtle::setScale(double newScaleX, double newScaleY)
{
    Q_ASSERT(std::abs(scaleX_) > std::numeric_limits<double>::epsilon());
    Q_ASSERT(std::abs(scaleY_) > std::numeric_limits<double>::epsilon());

    double ratioX = newScaleX / scaleX_;
    double ratioY = newScaleY / scaleY_;

    turtleTransform_.scale(ratioX, ratioY);

    scaleX_ = newScaleX;
    scaleY_ = newScaleY;

    Config::get().mainInterface()->setTurtlePos(&turtleTransform_);
}

void Turtle::setPenMode(PenModeEnum aPenMode)
{
    if (penMode_ != aPenMode)
    {
        penMode_ = aPenMode;
        Config::get().mainInterface()->setPenmode(penMode_);
    }
}

PenModeEnum Turtle::getPenMode() const
{
    return penMode_;
}

void Turtle::setPenSize(double aPenSize)
{
    penSize_ = aPenSize;
    Config::get().mainInterface()->setPensize(penSize_);
}

bool Turtle::isPenSizeValid(double aPenSize) const
{
    return Config::get().mainInterface()->isPenSizeValid(aPenSize);
}

double Turtle::getPenSize() const
{
    return penSize_;
}

void Turtle::beginFillWithColor(const QColor &fillColor)
{
    if (isFilling_)
    {
        throw FCError::alreadyFilling();
    }
    isFilling_ = true;

    Config::get().mainInterface()->beginPolygon(fillColor);
}

void Turtle::endFill()
{
    isFilling_ = false;
    Config::get().mainInterface()->endPolygon();
}

DatumPtr Turtle::print()
{
    QString s = QString("%1 %2 %3\n"
                        "%4 %5 %6\n"
                        "%7 %8 %9\n")
                    .arg(turtleTransform_.m11())
                    .arg(turtleTransform_.m12())
                    .arg(turtleTransform_.m13())
                    .arg(turtleTransform_.m21())
                    .arg(turtleTransform_.m22())
                    .arg(turtleTransform_.m23())
                    .arg(turtleTransform_.dx())
                    .arg(turtleTransform_.dy())
                    .arg(turtleTransform_.m33());

    return DatumPtr(s);
}
