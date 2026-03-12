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

#include <QByteArray>
#include <QChar>
#include <QColor>
#include <QDataStream>
#include <QDebug>
#include <QIODevice>

#ifndef _WIN32
#include <unistd.h>
#endif

constexpr double PI = 3.14159265358979323846;

using message_t = quint8;

class LogoInterface;
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
    ~Config()
    {
        Q_ASSERT(mLogoInterface_ == nullptr);
    }

    LogoInterface *mLogoInterface_ = nullptr;

  public:
    Config() = default;
    Config(const Config &) = delete;
    Config(Config *) = delete;
    Config(Config &&) = delete;
    Config &operator=(const Config &) = delete;
    Config &operator=(Config &&) = delete;

#ifdef DEBUG
const bool debugBuild_ = true;
#else
const bool debugBuild_ = false;
#endif
    
    
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
    const QChar escapeChar_ = QChar(27);

    /// @brief The escape string is the escape character as a string.
    const QString escapeString_ = QString(escapeChar_);

    /// @brief The initial X bound of the canvas.
    const float initialBoundX_ = 150;

    /// @brief The initial Y bound of the canvas.
    const float initialBoundY_ = 150;

    /// @brief The initial pen size of the canvas.
    const float initialPensize_ = 1;

    /// @brief The initial foreground color of the canvas.
    const QColor initialCanvasForegroundColor_ = QColorConstants::White;

    /// @brief The initial background color of the canvas.
    const QColor initialCanvasBackgroundColor_ = QColorConstants::Black;

    // The canvas size proportions for each mode. 0.0 means Canvas is
    // completely hidden. 0.8 means Canvas takes up 80% of available space (remaining
    // 20% belongs to the Console).
    const float textScreenSize_ = 0.0f;
    const float fullScreenSize_ = 0.8f;
    const float splitScreenSize_ = 0.8f;
    const float initScreenSize_ = textScreenSize_;

    LogoInterface *mainInterface()
    {
        Q_ASSERT(mLogoInterface_ != nullptr);
        return mLogoInterface_;
    }

    void setMainLogoInterface(LogoInterface *aLogoInterface)
    {
        Q_ASSERT((mLogoInterface_ == nullptr) || (aLogoInterface == nullptr));
        mLogoInterface_ = aLogoInterface;
    }

    // Set to true iff qlogo is communicating with Psychi.
    bool hasGUI_ = false;

    // Set to true iff compiler should show IR code.
    bool showIR_ = false;

    // Set to true iff compiler should show the CFG view.
    bool showCFG_ = false;

    // Set to true iff compiler should show the full module IR (after coroutine lowering).
    bool showModuleIR_ = false;

    // Set to true if evaluator should trace its execution (for debugging).
    bool traceEvaluator_ = false;

    // Set to true if Compiler should verify the generated functions.
    // Use for development. Compiler may generate bad code in unreachable
    // sections, i.e. when handling parsing errors.
    // Default to true in debug build.
    bool verifyIR_ = debugBuild_;

    // Set to true iff compiler should show the CFG view.
    bool showCON_ = false;

    // ARGV initialization parameters
    QStringList ARGV_;

    /// @brief The path to the library database file.
    QString paramLibraryDatabaseFilepath_;

    /// @brief The path to the help database file.
    QString paramHelpDatabaseFilepath_;

    // TODO: These should be set in the CMake file

    /// @brief The default library database filename.
    const char *defaultLibraryDbFilename_ = "qlogo_library.db";

    /// @brief The default help database filename.
    const char *defaultHelpDbFilename_ = "qlogo_help.db";
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
    static QProcess *process_;

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
    MessageTemplate(message_t header) : bufferStream_(&buffer_, QIODevice::WriteOnly)
    {
        buffer_.clear();
        bufferStream_ << header;
    }

    ~MessageTemplate()
    {
        qint64 datalen = buffer_.size();
        buffer_.prepend(reinterpret_cast<const char *>(&datalen), sizeof(qint64));
        qint64 datawritten = WriterPolicy::write(buffer_);
        Q_ASSERT(datawritten == buffer_.size());
    }

    template <class T>
    MessageTemplate &operator<<(const T &x)
    {
        bufferStream_ << x;
        return *this;
    }

  private:
    QByteArray buffer_;
    QDataStream bufferStream_;
};

#endif // CONSTANTS_H
