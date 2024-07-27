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
/// programs as well as global parameters that may be used by either program.
///
//===----------------------------------------------------------------------===//

#include <QChar>
#include <QColor>
#include <QDebug>

using message_t = quint8;

class Turtle;
class Kernel;
class Procedures;
class LogoController;

enum messageCategory : message_t
{
    W_ZERO = 0,       // Zeroes get ignored
    W_INITIALIZE,     // The initialization message, either request or response
    W_CLOSE_PIPE,     // The interpreter tells the GUI to close the iter-process pipe
    W_SET_SCREENMODE, // Set the screenmode (splitscreen, fullscreen, textscreen)

    S_SYSTEM,   // SYSTEM signal (End everything)
    S_TOPLEVEL, // TOPLEVEL signal (End currently-running procedure, drop back to prompt)
    S_PAUSE,    // PAUSE signal (stop currently running procedure, may resume later)

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

    C_CANVAS_UPDATE_TURTLE_POS,     // Update the turtle matrix
    C_CANVAS_EMIT_VERTEX,           // Add a line/polygon point at turtle position
    C_CANVAS_SET_FOREGROUND_COLOR,  // Set the foreground color for future line drawing
    C_CANVAS_SET_BACKGROUND_COLOR,  // Set the canvas background color
    C_CANVAS_SET_BACKGROUND_IMAGE,  // Set the canvas background image
    C_CANVAS_BEGIN_POLYGON,         // Begin drawing a polygon at turtle position
    C_CANVAS_END_POLYGON,           // End drawing a polygon
    C_CANVAS_SET_TURTLE_IS_VISIBLE, // Show/hide the turtle
    C_CANVAS_DRAW_LABEL,            // Draw a label on the canvas at turtle's position
    C_CANVAS_DRAW_ARC,              // Draw an arc
    C_CANVAS_CLEAR_SCREEN,          // Clear the graphics screen
    C_CANVAS_SETBOUNDS,             // Set the X and Y bounds of the drawing surface area
    C_CANVAS_SET_IS_BOUNDED,        // Determine whether canvas draws in a box or whole widget
    C_CANVAS_SET_PENSIZE,           // Set the drawing pen size
    C_CANVAS_SET_PENUPDOWN,         // Set the drawing pen up or down
    C_CANVAS_SET_FONT_NAME,         // Set the label font name
    C_CANVAS_SET_FONT_SIZE,         // Set the label font size
    C_CANVAS_GET_IMAGE,             // Request a screenshot of the current image
    C_CANVAS_GET_SVG,               // Request a SVG representation of the current image
    C_CANVAS_MOUSE_BUTTON_DOWN,     // A mouse button was pressed
    C_CANVAS_MOUSE_MOVED,           // Mouse moved over the canvas
    C_CANVAS_MOUSE_BUTTON_UP,       // A mouse button was released
    C_CANVAS_SET_PENMODE,           // Set canvas pen mode
};

/// @brief The configuration for the QLogo-GUI/logo programs.
///
/// This class is a singleton that contains global parameters that may be used by
/// either program.
class Config
{
  private:
    Config()
    {
    }
    Config(const Config &);
    Config &operator=(const Config &);
    ~Config()
    {
        Q_ASSERT(mTurtle == NULL);
        Q_ASSERT(mKernel == NULL);
        Q_ASSERT(mProcedures == NULL);
        Q_ASSERT(mLogoController == NULL);
    }

    Turtle *mTurtle = NULL;
    Kernel *mKernel = NULL;
    Procedures *mProcedures = NULL;
    LogoController *mLogoController = NULL;

  public:
    /// @brief Get the singleton instance of the Config class.
    /// @return The singleton instance of the Config class.
    static Config &get()
    {
        static Config instance;
        return instance;
    }

    /// The escape character is the separator between Console messages and
    /// Console control characters. Currently, the only control is switching
    /// STANDOUT modes.
    const QChar escapeChar = QChar(27);

    /// @brief The escape string is the escape character as a string.
    const QString escapeString = QString(escapeChar);

    /// @brief The initial X bound of the canvas.
    const float initialBoundX = 150;

    /// @brief The initial Y bound of the canvas.
    const float initialBoundY = 150;

    /// @brief The initial pen size of the canvas.
    const float initialPensize = 1;

    /// @brief The initial foreground color of the canvas.
    const QColor initialCanvasForegroundColor = QColorConstants::White;

    /// @brief The initial background color of the canvas.
    const QColor initialCanvasBackgroundColor = QColorConstants::Black;

    // The canvas size proportions for each mode. 0.0 means Canvas is
    // completely hidden. 0.8 means Canvas takes up 80% of available space (remaining
    // 20% belongs to the Console).
    const float textScreenSize = 0.0;
    const float fullScreenSize = 0.8;
    const float splitScreenSize = 0.8;
    const float initScreenSize = textScreenSize;

    Turtle *mainTurtle()
    {
        Q_ASSERT(mTurtle != NULL);
        return mTurtle;
    }

    Kernel *mainKernel()
    {
        Q_ASSERT(mKernel != NULL);
        return mKernel;
    }

    Procedures *mainProcedures()
    {
        Q_ASSERT(mProcedures != NULL);
        return mProcedures;
    }

    LogoController *mainController()
    {
        Q_ASSERT(mLogoController != NULL);
        return mLogoController;
    }

    void setMainTurtle(Turtle *aTurtle)
    {
        Q_ASSERT((mTurtle == NULL) || (aTurtle == NULL));
        mTurtle = aTurtle;
    }

    void setMainKernel(Kernel *aKernel)
    {
        Q_ASSERT((mKernel == NULL) || (aKernel == NULL));
        mKernel = aKernel;
    }

    void setMainProcedures(Procedures *aProcedures)
    {
        Q_ASSERT((mProcedures == NULL) || (aProcedures == NULL));
        mProcedures = aProcedures;
    }

    void setMainLogoController(LogoController *aLogoController)
    {
        Q_ASSERT((mLogoController == NULL) || (aLogoController == NULL));
        mLogoController = aLogoController;
    }

    // Set to true iff qlogo is communicating with QLogo-GUI.
    bool hasGUI = false;

    // ARGV initialization parameters
    QStringList ARGV;

    /// @brief The path to the library database file.
    QString paramLibraryDatabaseFilepath;

    /// @brief The path to the help database file.
    QString paramHelpDatabaseFilepath;

    // TODO: These should be set in the CMake file

    /// @brief The default library database filename.
    const char *defaultLibraryDbFilename = "qlogo_library.db";

    /// @brief The default help database filename.
    const char *defaultHelpDbFilename = "qlogo_help.db";
};


enum PenModeEnum
{
    /// @brief The paint pen mode, draws the current foreground color.
    penModePaint,

    /// @brief The erase pen mode, draws the current background color.
    penModeErase,

    /// @brief The reverse pen mode, inverts the colors already on the canvas.
    penModeReverse
};

enum TurtleModeEnum
{
    /// @brief The wrap turtle mode, wraps the turtle around the canvas.
    turtleWrap,

    /// @brief The fence turtle mode, prevents the turtle from leaving the canvas.
    turtleFence,

    /// @brief The window turtle mode, turtle can leave the canvas.
    turtleWindow
};

enum SignalsEnum_t : int
{
    noSignal = 0,

    /// CTRL-Backslash, kill logo [ THROW "SYSTEM ]
    systemSignal,

    /// CTRL-C, kill running script [ THROW "TOPLEVEL ]
    toplevelSignal,

    /// CTRL-Z, pause running script [ PAUSE ]
    pauseSignal
};

enum ScreenModeEnum
{
    /// @brief The initial screen mode, the Console takes all available space.
    initScreenMode,

    /// @brief The text screen mode, the Console takes all available space.
    textScreenMode,

    /// @brief The full screen mode, the Canvas takes up 80% of available space.
    fullScreenMode,

    /// @brief The split screen mode, the Canvas takes up 80% of available space.
    splitScreenMode
};

#endif // CONSTANTS_H
