#ifndef TURTLE_H
#define TURTLE_H

//===-- qlogo/turtle.h - Turtle class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Turtle class, which maintains
/// the turtle state. The turtle state includes the turtle's position,
/// orientation, pen state, and visibility. The turtle's position is represented
/// by the transformation matrix's translation part. The turtle's orientation
/// is represented by the transformation matrix's rotation part. The pen state
/// is represented by the pen's color, size, and whether it is up or down. The
/// turtle's visibility is represented by a boolean flag.
///
//===----------------------------------------------------------------------===//

#include "QtGui/qtransform.h"
#include "datum.h"
#include "sharedconstants.h"
#include <QColor>
#include <QMatrix4x4>

/// @brief The Turtle class is responsible for maintaining the state of the turtle,
/// including its position, orientation, and pen state.
class Turtle
{
    QTransform turtlePosition;

    bool wrapTurtle(qreal lineStartU,
                    qreal lineStartV,
                    qreal &lineEndU,
                    qreal lineEndV,
                    qreal boundU,
                    qreal boundV,
                    bool isXBoundary,
                    qreal mult);
    void moveTurtle(const QTransform &newPosition);
    void moveTurtleWrap(const QTransform &newPosition);
    void moveTurtleFence(const QTransform &newPosition);
    void moveTurtleWindow(const QTransform &newPosition);

    QColor penColor;
    TurtleModeEnum mode = turtleFence;
    bool isFilling = false;

    PenModeEnum penMode = penModePaint;

    qreal penSize = Config::get().initialPensize;

    bool turtleIsVisible;
    bool penIsDown;

  public:
    /// @brief Constructor for the Turtle class.
    Turtle();

    /// @brief Destructor for the Turtle class.
    ~Turtle();

    /// @brief Get the current turtle position and orientation.
    /// @return The current turtle position and orientation.
    const QTransform &getMatrix(void)
    {
        return turtlePosition;
    }

    /// @brief Check if the turtle is visible.
    /// @return True if the turtle is visible, false otherwise.
    bool isTurtleVisible()
    {
        return turtleIsVisible;
    }

    /// @brief Set the visibility of the turtle.
    /// @param aIsVisible The new visibility state of the turtle.
    void setIsTurtleVisible(bool aIsVisible)
    {
        turtleIsVisible = aIsVisible;
    }

    /// @brief Check if the pen is down.
    /// @return True if the pen is down, false otherwise.
    bool isPenDown()
    {
        return penIsDown;
    }

    /// @brief Set the pen down state.
    /// @param aIsPenDown The new pen down state.
    void setPenIsDown(bool aIsPenDown);

    /// @brief Set the pen mode.
    /// @param aPenMode The new pen mode.
    void setPenMode(PenModeEnum aPenMode);

    /// @brief Get the pen mode.
    /// @return The current pen mode.
    PenModeEnum getPenMode();

    /// @brief Rotate the turtle by a given angle.
    /// @param angle The angle to rotate the turtle by.
    void rotate(qreal angle);

    /// @brief Move the turtle forward by a given number of steps.
    /// @param steps The number of steps to move the turtle forward.
    void forward(qreal steps);

    /// @brief Set the turtle mode.
    /// @param newMode The new turtle mode.
    void setMode(TurtleModeEnum newMode);

    /// @brief Get the turtle mode.
    /// @return The current turtle mode.
    TurtleModeEnum getMode();

    /// @brief Get the turtle heading.
    /// @return The current turtle heading.
    qreal getHeading();

    /// @brief Get the turtle position.
    /// @param x The x coordinate of the turtle.
    /// @param y The y coordinate of the turtle.
    void getxy(qreal &x, qreal &y);

    /// @brief Set the turtle position.
    /// @param x The new x coordinate of the turtle.
    /// @param y The new y coordinate of the turtle.
    void setxy(qreal x, qreal y);

    /// @brief Set the x coordinate of the turtle.
    /// @param x The new x coordinate of the turtle.
    void setx(qreal x);

    /// @brief Set the y coordinate of the turtle.
    /// @param y The new y coordinate of the turtle.
    void sety(qreal y);

    /// @brief Set the pen color.
    /// @param c The new pen color.
    void setPenColor(const QColor &c);

    /// @brief Set the pen size.
    /// @param aPenSize The new pen size.
    void setPenSize(qreal aPenSize);

    /// @brief Check if the pen size is valid.
    /// @param aPenSize The pen size to check.
    /// @return True if the pen size is valid, false otherwise.
    bool isPenSizeValid(qreal aPenSize);

    /// @brief Get the pen size.
    /// @return The current pen size.
    qreal getPenSize();

    /// @brief Get the pen color.
    /// @return The current pen color.
    const QColor &getPenColor();

    /// @brief Move the turtle to the home position.
    void moveToHome();

    /// @brief Print the turtle state.
    /// @return The turtle state as a Word.
    /// @note This is a debugging tool. It will return a string containing
    /// three lines with three values in each line representing the turtle's
    /// transformation matrix.
    DatumPtr print();

    /// @brief Draw an arc of a given angle and radius.
    /// @param angle The angle of the arc.
    /// @param radius The radius of the arc.
    void drawArc(qreal angle, qreal radius);

    /// @brief Begin filling with a given color.
    /// @param fillColor The color to fill with.
    void beginFillWithColor(const QColor &fillColor);

    /// @brief End filling.
    void endFill();
};

#endif // TURTLE_H
