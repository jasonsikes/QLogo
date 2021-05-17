#ifndef CONSTANTS_H
#define CONSTANTS_H

//===-- qlogo/constants.h - Shared constant values -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
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
/// This file contains constants and data structures shared between the QLogo/logo
/// programs.
///
//===----------------------------------------------------------------------===//


#include <QChar>
#include <QDebug>

using message_t = quint8;

enum messageCategory : message_t {
    W_ZERO = 0,             // Zeroes get ignored
    W_INITIALIZE,           // The initialization message, either request or response

    C_CONSOLE_PRINT_STRING, // Print text to the GUI
    C_CONSOLE_REQUEST_LINE, // Ask the GUI for a raw line.
    C_CONSOLE_REQUEST_CHAR, // Ask the GUI for a single char.
    C_CONSOLE_RAWLINE_READ, // A line returned from the GUI
    C_CONSOLE_CHAR_READ,    // A char returned from the GUI

    C_CANVAS_UPDATE_TURTLE_POS,       // Tell the GUI to update the turtle matrix
    C_CANVAS_DRAW_LINE,               // Draw a line on the canvas
    C_CANVAS_DRAW_POLYGON,            // Draw a polygon on the canvas
    C_CANVAS_DRAW_LABEL,              // Draw a label on the canvas
    C_CANVAS_CLEAR_SCREEN,            // Clear the graphics screen
    C_CANVAS_SET_BACKGROUND_COLOR,    // Set the canvas background color
    C_CANVAS_SET_PENSIZE,             // Set the drawing pen size
};

const QChar escapeChar(27);

#define dv(x) qDebug()<<#x<<'='<<x


const float initialBoundX = 150;
const float initialBoundY = 150;

const float startingPensize = 1;

enum PenModeEnum { penModePaint, penModeErase, penModeReverse };

enum TurtleModeEnum { turtleWrap, turtleFence, turtleWindow };


#endif // CONSTANTS_H
