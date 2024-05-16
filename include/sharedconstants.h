#ifndef CONSTANTS_H
#define CONSTANTS_H

//===-- qlogo/sharedconstants.h - Shared constant values -------*- C++ -*-===//
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
/// This file contains constants and data structures shared between the QLogo/logo
/// programs.
///
//===----------------------------------------------------------------------===//


#include <QChar>
#include <QDebug>
#include <QColor>

using message_t = quint8;

enum messageCategory : message_t {
    W_ZERO = 0,             // Zeroes get ignored
    W_INITIALIZE,           // The initialization message, either request or response
    W_CLOSE_PIPE,           // The interpreter tells the GUI to close the iter-process pipe
    W_SET_SCREENMODE,       // Set the screenmode (splitscreen, fullscreen, textscreen)

    S_SYSTEM,             // SYSTEM signal (End everything)
    S_TOPLEVEL,           // TOPLEVEL signal (End currently-running procedure, drop back to prompt)
    S_PAUSE,              // PAUSE signal (stop currently running procedure, may resume later)

    C_CONSOLE_PRINT_STRING,        // Print text to the GUI
    C_CONSOLE_REQUEST_LINE,        // Ask the GUI for a raw line
    C_CONSOLE_REQUEST_CHAR,        // Ask the GUI for a single char
    C_CONSOLE_RAWLINE_READ,        // A line returned from the GUI
    C_CONSOLE_CHAR_READ,           // A char returned from the GUI
    C_CONSOLE_SET_FONT_NAME,       // Set the console font name
    C_CONSOLE_SET_FONT_SIZE,       // Set the console font size
    C_CONSOLE_BEGIN_EDIT_TEXT,     // Open the text editor window
    C_CONSOLE_END_EDIT_TEXT,       // Text editor has finished
    C_CONSOLE_TEXT_CURSOR_POS,     // Position of text cursor (row,col)
    C_CONSOLE_SET_TEXT_CURSOR_POS, // Set text cursor position
    C_CONSOLE_SET_CURSOR_MODE,     // Set text cursor insert/overwrite
    C_CONSOLE_SET_TEXT_COLOR,      // Set text color foreground&background
    C_CONSOLE_CLEAR_SCREEN_TEXT,   // Clear text from Console

    C_CANVAS_UPDATE_TURTLE_POS,       // Update the turtle matrix
    C_CANVAS_EMIT_VERTEX,             // Add a line/polygon point at turtle position
    C_CANVAS_SET_FOREGROUND_COLOR,    // Set the foreground color for future line drawing
    C_CANVAS_SET_BACKGROUND_COLOR,    // Set the canvas background color
    C_CANVAS_SET_BACKGROUND_IMAGE,    // Set the canvas background image
    C_CANVAS_BEGIN_POLYGON,           // Begin drawing a polygon at turtle position
    C_CANVAS_END_POLYGON,             // End drawing a polygon
    C_CANVAS_SET_TURTLE_IS_VISIBLE,   // Show/hide the turtle
    C_CANVAS_DRAW_LABEL,              // Draw a label on the canvas at turtle's position
    C_CANVAS_CLEAR_SCREEN,            // Clear the graphics screen
    C_CANVAS_SETBOUNDS,               // Set the X and Y bounds of the drawing surface area
    C_CANVAS_SET_IS_BOUNDED,          // Determine whether canvas draws in a box or whole widget
    C_CANVAS_SET_PENSIZE,             // Set the drawing pen size
    C_CANVAS_SET_PENUPDOWN,           // Set the drawing pen up or down
    C_CANVAS_SET_FONT_NAME,           // Set the label font name
    C_CANVAS_SET_FONT_SIZE,           // Set the label font size
    C_CANVAS_GET_IMAGE,               // Request a screenshot of the current image
    C_CANVAS_MOUSE_BUTTON_DOWN,       // A mouse button was pressed
    C_CANVAS_MOUSE_MOVED,             // Mouse moved over the canvas
    C_CANVAS_MOUSE_BUTTON_UP,         // A mouse button was released
    C_CANVAS_SET_PENMODE,             // Set canvas pen mode
};

const QChar escapeChar(27);
const QString escapeString(escapeChar);

#define dv(x) qDebug()<<#x<<'='<<x


const float initialBoundX = 150;
const float initialBoundY = 150;

const float initialPensize = 1;

const QColor initialCanvasBackgroundColor = QColorConstants::Black;
const QColor initialCanvasForegroundColor = QColorConstants::White;

enum PenModeEnum { penModePaint, penModeErase, penModeReverse };

enum TurtleModeEnum { turtleWrap, turtleFence, turtleWindow };

enum SignalsEnum_t : int {
    noSignal = 0,
    systemSignal,            // CTRL-Backslash, kill logo            [ THROW "SYSTEM ]
    toplevelSignal,          // CTRL-C,         kill running script  [ THROW "TOPLEVEL ]
    pauseSignal              // CTRL-Z,         pause running script [ PAUSE ]
};


enum ScreenModeEnum {
  initScreenMode,
  textScreenMode,
  fullScreenMode,
  splitScreenMode
};

// Canvas size proportions for each mode.
const float textScreenSize  = 0.0;
const float fullScreenSize  = 0.8;
const float splitScreenSize = 0.8;
const float initScreenSize  = 0.0;


#endif // CONSTANTS_H
