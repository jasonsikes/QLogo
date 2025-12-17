#ifndef CONSTANTS_H
#define CONSTANTS_H

//===-- qlogo/sharedconstants.h - Shared constant values -------*- C++ -*-===//
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
/// This file contains constants and data structures shared between the QLogo/logo
/// programs as well as global parameters that may be used by either program.
///
//===----------------------------------------------------------------------===//

#include <QChar>
#include <QColor>
#include <QDebug>
#include <QDataStream>
#include <QByteArray>
#include <QIODevice>

#ifndef _WIN32
#include <unistd.h>
#endif

using message_t = quint8;

class Turtle;
class Kernel;
class Procedures;
class LogoController;
class Compiler;

enum messageCategory : message_t
{
    W_ZERO = 0,             // Zeroes get ignored
    W_INITIALIZE,           // The initialization message, either request or response
    W_CLOSE_PIPE,           // The interpreter tells the GUI to close the iter-process pipe
    W_SET_SCREENMODE,       // Set the screenmode (splitscreen, fullscreen, textscreen)
    W_FILE_DIALOG_GET_PATH, // Query user for a file path using modal file dialog.

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

/// @brief The configuration for the qlgog/Psychi programs.
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
        Q_ASSERT(mTurtle == nullptr);
        Q_ASSERT(mKernel == nullptr);
        Q_ASSERT(mProcedures == nullptr);
        Q_ASSERT(mLogoController == nullptr);
        Q_ASSERT(mCompiler == nullptr);
    }

    Turtle *mTurtle = nullptr;
    Kernel *mKernel = nullptr;
    Procedures *mProcedures = nullptr;
    LogoController *mLogoController = nullptr;
    Compiler *mCompiler = nullptr;

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
    const float textScreenSize = 0.0f;
    const float fullScreenSize = 0.8f;
    const float splitScreenSize = 0.8f;
    const float initScreenSize = textScreenSize;

    Turtle *mainTurtle()
    {
        Q_ASSERT(mTurtle != nullptr);
        return mTurtle;
    }

    Kernel *mainKernel()
    {
        Q_ASSERT(mKernel != nullptr);
        return mKernel;
    }

    Procedures *mainProcedures()
    {
        Q_ASSERT(mProcedures != nullptr);
        return mProcedures;
    }

    LogoController *mainController()
    {
        Q_ASSERT(mLogoController != nullptr);
        return mLogoController;
    }

    Compiler *mainCompiler()
    {
        Q_ASSERT(mCompiler != nullptr);
        return mCompiler;
    }

    void setMainTurtle(Turtle *aTurtle)
    {
        Q_ASSERT((mTurtle == nullptr) || (aTurtle == nullptr));
        mTurtle = aTurtle;
    }

    void setMainKernel(Kernel *aKernel)
    {
        Q_ASSERT((mKernel == nullptr) || (aKernel == nullptr));
        mKernel = aKernel;
    }

    void setMainProcedures(Procedures *aProcedures)
    {
        Q_ASSERT((mProcedures == nullptr) || (aProcedures == nullptr));
        mProcedures = aProcedures;
    }

    void setMainLogoController(LogoController *aLogoController)
    {
        Q_ASSERT((mLogoController == nullptr) || (aLogoController == nullptr));
        mLogoController = aLogoController;
    }

    void setMainCompiler(Compiler *aCompiler)
    {
        Q_ASSERT((mCompiler == nullptr) || (aCompiler == nullptr));
        mCompiler = aCompiler;
    }

    // Set to true iff qlogo is communicating with Psychi.
    bool hasGUI = false;

    // Set to true iff compiler should show IR code.
    bool showIR = false;

    // Set to true iff compiler should show the CFG view.
    bool showCFG = false;

    // Set to true if Compiler should verify the generated functions.
    // Use for development. Compiler may generate bad code in unreachable
    // sections, i.e. when handling parsing errors.
    bool verifyIR = false;

    // Set to true iff compiler should show the CFG view.
    bool showCON = false;

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

struct Transform {
    double m[9];

    Transform() {
        m[0] = 1.0; m[1] = 0.0; m[2] = 0.0;
        m[3] = 0.0; m[4] = 1.0; m[5] = 0.0;
        m[6] = 0.0; m[7] = 0.0; m[8] = 1.0;
    }

    Transform(double a0, double a1, double a2,
              double a3, double a4, double a5,
              double a6, double a7, double a8)
    {
        m[0] = a0;
        m[1] = a1;
        m[2] = a2;
        m[3] = a3;
        m[4] = a4;
        m[5] = a5;
        m[6] = a6;
        m[7] = a7;
        m[8] = a8;
    }

    Transform(const Transform& other) {
        std::copy(other.m, other.m + 9, m);
    }

    Transform& operator=(const Transform& other) {
        if (this != &other) {
            std::copy(other.m, other.m + 9, m);
        }
        return *this;
    }

    Transform(Transform&& other) noexcept {
        std::copy(other.m, other.m + 9, m);
    }

    Transform& operator=(Transform&& other) noexcept {
        if (this != &other) {
            std::copy(other.m, other.m + 9, m);
        }
        return *this;
    }

    ~Transform() = default;
};

inline QDataStream& operator<<(QDataStream& out, const Transform& transform) {
    for (int i = 0; i < 9; ++i) {
        out << transform.m[i];
    }
    return out;
}

inline QDataStream& operator>>(QDataStream& in, Transform& transform) {
    for (int i = 0; i < 9; ++i) {
        in >> transform.m[i];
    }
    return in;
}

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

    /// @brief The window turtle mode, where the canvas bounds grow to accommodate the
    /// turtle's position as needed.
    turtleWindow
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

class QProcess;

/// @brief Policy class for writing messages to a QProcess.
///
/// The process pointer must be set before using message<ProcessMessageWriter>.
/// Include <QProcess> where implementing write().
struct ProcessMessageWriter
{
    static QProcess *process;
    
    static qint64 write(const QByteArray &buffer);
};

/// @brief Policy class for writing messages to stdout.
struct StdoutMessageWriter
{
    static qint64 write(const QByteArray &buffer);
};

/// @brief Interface for sending messages between processes.
///
/// This template class is used to send messages between processes. It presents a
/// QDataStream interface for "<<" stream operations and then the destructor will
/// send the message using the writer policy's write method.
///
/// @tparam WriterPolicy A policy class with a static write() method that takes
///                      a const QByteArray& and returns qint64.
template <typename WriterPolicy>
struct MessageTemplate
{
    MessageTemplate() : bufferStream(&buffer, QIODevice::WriteOnly)
    {
        buffer.clear();
    }

    ~MessageTemplate()
    {
        qint64 datalen = buffer.size();
        buffer.prepend(reinterpret_cast<const char *>(&datalen), sizeof(qint64));
        qint64 datawritten = WriterPolicy::write(buffer);
        Q_ASSERT(datawritten == buffer.size());
    }

    template <class T>
    MessageTemplate &operator<<(const T &x)
    {
        bufferStream << x;
        return *this;
    }

  private:
    QByteArray buffer;
    QDataStream bufferStream;
};

#endif // CONSTANTS_H
