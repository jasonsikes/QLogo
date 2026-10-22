
//===-- qlogo/logointerface.h - LogoInterface class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the LogoInterface class, which is responsible for
/// handling user interaction through standard input and output with no special control characters.
/// This class can be subclassed for different types of user interaction, such as the Psychi
/// interface or a Curses-type interface.
///
//===----------------------------------------------------------------------===//

#ifndef LOGOINTERFACE_H
#define LOGOINTERFACE_H

#include "flowcontrol.h"
#include "sharedconstants.h"
#include <QColor>
#include <QFont>
#include <QImage>
#include <QList>
#include <QObject>
#include <QPen>
#include <QTextStream>
#include <QThread>
#include <QVector2D>
#include <atomic>
#include <csignal>

class QTransform;
class Kernel;

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

/// @brief The LogoInterface class is the superclass for all interfaces that handle user interaction.
/// It provides a set of common methods that are used by all interfaces. It also implements
/// some methods that are specific to the text-based interface.
class LogoInterface : public QObject
{
    Q_OBJECT

  public:
    /// @brief Drain pending Psychi pipe messages without blocking.
    /// @note Console interfaces have no message queue. The GUI override records
    /// S_SYSTEM / S_TOPLEVEL / S_PAUSE so the evaluator can consume them at a
    /// safepoint. Safe to call when the queue is empty.
    virtual void processInputMessageQueue()
    {
    }

    /// @brief Constructor
    /// @param parent The Qt parent object.
    LogoInterface(QObject *parent = nullptr);

    /// @brief Destructor
    ~LogoInterface() override;

    /// @brief Returns the most recent interrupt signal that was received, if any. Resets the signal.
    /// @return The most recent interrupt signal that was received, if any.
    /// @note This method is meant to be called by the main loop to check for
    /// signals that are sent to the application. The common no-signal path is a
    /// relaxed load; consume uses an atomic exchange so a handler cannot lose a
    /// signal between the read and the clear.
    static SignalsEnum_t latestSignal()
    {
        if (lastSignal_.load(std::memory_order_relaxed) == noSignal)
            return noSignal;
        return static_cast<SignalsEnum_t>(lastSignal_.exchange(noSignal, std::memory_order_relaxed));
    }

    /// @brief True if a signal has been recorded and not yet consumed by latestSignal().
    /// @details Does not reset the signal. Lets a blocking reader tell an
    /// interrupted read from end of input without trusting errno.
    static bool signalPending()
    {
        return lastSignal_.load(std::memory_order_relaxed) != noSignal;
    }

    /// @brief Record an interrupt from a Unix signal or a Psychi pipe message.
    /// @param incoming The signal to record.
    /// @details SYSTEM outranks TOPLEVEL, which outranks PAUSE. Safe to call
    /// from a signal handler: it only does lock-free atomic RMW on lastSignal_.
    static void noteSignal(SignalsEnum_t incoming);

    /// @brief Initializes the interface.
    /// @return True if the interface is ready; false if it could not start, in
    /// which case it has already reported the reason and the caller should exit
    /// normally with a failure code.
    /// @note The base method does nothing and succeeds. Subclasses can override
    /// this method to perform initialization tasks.
    virtual bool initialize()
    {
        return true;
    }

    /// @brief Reads a line of input from the user, with a prompt.
    /// @param prompt The prompt to display to the user.
    /// @return The line of input from the user.
    virtual QString inputRawlineWithPrompt(const QString &prompt);

    /// @brief Reads a character from the user.
    /// @return The character that was read from the user.
    virtual DatumPtr readchar();

    /// @brief Checks if the input stream is at the end.
    /// @return True if the input stream is at the end, false otherwise.
    virtual bool atEnd();

    /// @brief Prints a string to the console.
    /// @param text The text to print to the console.
    virtual void printToConsole(const QString &text);

    /// @brief Initializes the signal handlers.
    /// @note This method sets up signal handlers for SIGINT, SIGTSTP, and SIGQUIT.
    static void initSignals();

    /// @brief Restores the default signal handlers.
    /// @note This method restores the default signal handlers for SIGINT, SIGTSTP, and SIGQUIT.
    static void restoreSignals();

    /// @brief The most recent signal that was received.
    /// The value of this variable is set by the handle_signal function. When the latestSignal
    /// method is called, the value is reset to noSignal. A stronger signal (SYSTEM, then
    /// TOPLEVEL, then PAUSE) is kept if a second signal arrives before the first is consumed.
    /// @note Lock-free so the signal handler and the evaluator can RMW without
    /// masking. static_assert in the .cpp requires is_always_lock_free.
    static std::atomic<sig_atomic_t> lastSignal_;

    /// @brief Stops communication and initiates cleanup tasks.
    virtual void closeInterface();

    /// @brief Give the terminal to an external editor (EDIT / EDITFILE).
    /// @note The base method does nothing. The TUI override suspends ncurses so
    /// the editor can use stdin, stdout, and stderr.
    virtual void suspendForExternalEditor()
    {
    }

    /// @brief Restore the interface after an external editor exits.
    virtual void resumeAfterExternalEditor()
    {
    }

    /// @brief Displays a file dialog modal.
    /// @return The path of the selected file.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to display a file dialog modal.
    virtual QString fileDialogModal()
    {
        exit(1);
        return {};
    }

    /// @brief Present @p text in the editor and wait until the user is done.
    /// @param text The text to edit.
    /// @param accepted Set to true if the user saved, false if they reverted.
    /// @param title Window title; empty keeps the default "Editor".
    /// @return The edited text if saved; empty if reverted.
    /// @note GUI only. The base method aborts. CLI and TUI have no editor.
    virtual QString editText(const QString &text, bool *accepted, const QString &title = {})
    {
        Q_UNUSED(text);
        Q_UNUSED(accepted);
        Q_UNUSED(title);
        exit(1);
        return {};
    }

    /// @brief Sets the turtle position.
    /// @param newTurtlePos The new position of the turtle.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the turtle position.
    virtual void setTurtlePos(QTransform *newTurtlePosPtr)
    {
        exit(1);
    }

    /// @brief Emits a vertex to the graphics system.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to emit a vertex. A vertex can be part of a polygon or a polyline or both.
    virtual void emitVertex()
    {
        exit(1);
    }

    /// @brief Begins a polygon.
    /// @param color The color of the polygon.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to begin a polygon.
    virtual void beginPolygon(const QColor &color)
    {
        exit(1);
    }

    /// @brief Ends a polygon.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to end a polygon.
    virtual void endPolygon()
    {
        exit(1);
    }

    /// @brief Clears the canvas.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to clear the canvas.
    virtual void clearCanvas()
    {
        exit(1);
    }

    /// @brief Draws a label to the canvas.
    /// @param text The text to draw to the canvas.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to draw a label.
    virtual void drawLabel(const QString &text)
    {
        exit(1);
    }

    /// @brief Draws an arc to the canvas.
    /// @param angle The angle of the arc.
    /// @param radius The radius of the arc.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to draw an arc.
    virtual void drawArc(double angle, double radius)
    {
        exit(1);
    }

    /// @brief Sets the label font name.
    /// @param name The name of the font to use for labels.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the label font name.
    virtual void setLabelFontName(const QString &name)
    {
        exit(1);
    }

    /// @brief Adds the standout control characters to the given string.
    /// @param src The string to add the standout to.
    /// @return The string with the standout added.
    /// @note The base method returns the original string. Subclasses can override this method to
    /// perform the necessary tasks to add a standout control characters to the string.
    virtual QString addStandoutToString(const QString &src)
    {
        return src;
    };

    /// @brief Checks if the key queue has characters.
    /// @return True if the key queue has characters, false otherwise.
    /// @note The base method calls and returns the result from QTextStream::atEnd().
    virtual bool keyQueueHasChars();

    /// @brief LINEP: true if a complete line is waiting on the terminal.
    /// @return True if a line can be read without waiting for more typing.
    /// @note The base method peeks stdin in line mode and does not switch to
    /// cbreak. Subclasses that do not read Logo input from stdin must override
    /// this so a peek cannot steal protocol bytes.
    virtual bool lineQueueHasLine();

    /// @brief Sets the bounds of the canvas.
    /// @param x The x coordinate of the upper right corner of the canvas.
    /// @param y The y coordinate of the upper right corner of the canvas.
    /// @note The lower left corner of the canvas is the negative of the upper right corner.
    /// The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the bounds of the canvas.
    virtual void setBounds(double x, double y)
    {
        exit(1);
    }

    /// @brief Sets the canvas scrunch (display-space X and Y scale) factors.
    /// @param x The X scale factor.
    /// @param y The Y scale factor.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// send the new scale to the graphics window.
    virtual void setScrunch(double x, double y)
    {
        Q_UNUSED(x);
        Q_UNUSED(y);
        exit(1);
    }

    /// @brief Returns the x coordinate of the upper right corner of the canvas.
    /// @return The x coordinate of the upper right corner of the canvas.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the x coordinate of the upper right corner of the canvas.
    virtual double boundX() const
    {
        exit(1);
        return 0;
    }

    /// @brief Returns the y coordinate of the upper right corner of the canvas.
    /// @return The y coordinate of the upper right corner of the canvas.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the y coordinate of the upper right corner of the canvas.
    virtual double boundY() const
    {
        exit(1);
        return 0;
    }

    /// @brief Sets the foreground color of the canvas.
    /// @param color The color to set the foreground to.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the foreground color of the canvas.
    virtual void setCanvasForegroundColor(const QColor &color)
    {
        exit(1);
    }

    /// @brief Sets the background color of the canvas.
    /// @param color The color to set the background to.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the background color of the canvas.
    virtual void setCanvasBackgroundColor(const QColor &color)
    {
        exit(1);
    }

    /// @brief Sets the background image of the canvas.
    /// @param image The image to set the background to.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the background image of the canvas.
    virtual void setCanvasBackgroundImage(const QImage &image)
    {
        exit(1);
    }

    /// @brief Returns the current background color of the canvas.
    /// @return The current background color of the canvas.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the background color of the canvas.
    virtual const QColor getCanvasBackgroundColor() const
    {
        exit(1);
        return {};
    }

    /// @brief Returns the current state of the canvas as an image.
    /// @return The current state of the canvas as a QImage.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the image of the canvas.
    virtual QImage getCanvasImage()
    {
        exit(1);
        return {};
    }

    /// @brief Returns the current state of the canvas as an SVG.
    /// @return The current state of the canvas as an SVG.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the state of the canvas as an SVG.
    virtual QByteArray getSvgImage()
    {
        exit(1);
        return {};
    }

    /// @brief Returns if a mouse button is down.
    /// @return True if a mouse button is down, false otherwise.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the state of a mouse button.
    virtual bool getIsMouseButtonDown()
    {
        exit(1);
        return false;
    }

    /// @brief Returns the ID of the mouse button that was pressed and resets the state.
    /// @return The ID of the mouse button that was pressed.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the ID of the mouse button that was pressed and reset the button.
    virtual int getAndResetButtonID()
    {
        exit(1);
        return 0;
    }

    /// @brief Returns the position of the last mouse click.
    /// @return The position of the last mouse click as a QVector2D.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the position of the last mouse click.
    virtual QVector2D lastMouseclickPosition()
    {
        exit(1);
        return {};
    }

    /// @brief Returns the current position of the mouse.
    /// @return The current position of the mouse as a QVector2D.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the current position of the mouse.
    virtual QVector2D mousePosition()
    {
        exit(1);
        return {};
    }

    /// @brief Clears the text on the screen.
    /// @note The base method does nothing; the command-line interface has no
    /// text window. Subclasses can override this method to clear the GUI console.
    virtual void clearScreenText()
    {
    }

    /// @brief Sets the cursor position.
    /// @param x The row of the cursor.
    /// @param y The column of the cursor.
    /// @note The base method is a no-op; the command-line interface has no
    /// text window. Subclasses can override this method to set the GUI cursor.
    virtual void setTextCursorPos(int x, int y)
    {
        Q_UNUSED(x);
        Q_UNUSED(y);
    }

    /// @brief Returns the cursor position.
    /// @param x The row of the cursor.
    /// @param y The column of the cursor.
    /// @note The base method reports [0 0]; the command-line interface has no
    /// text window. Subclasses can override this method to return the GUI cursor.
    virtual void getTextCursorPos(int &x, int &y)
    {
        x = 0;
        y = 0;
    }

    /// @brief Sets the text color.
    /// @param text The color of the text.
    /// @param background The background color of the text.
    /// @note The base method is a no-op; the command-line interface has no
    /// text window. Subclasses can override this method to set the GUI colors.
    /// An invalid background leaves the existing background unchanged.
    virtual void setTextColor(const QColor &text, const QColor &background)
    {
        Q_UNUSED(text);
        Q_UNUSED(background);
    }

    /// @brief Sets the text font size.
    /// @param size The point size of the font.
    /// @note The base method is a no-op; the command-line interface has no
    /// text window. Subclasses can override this method to set the GUI font size.
    virtual void setTextFontSize(double size)
    {
        Q_UNUSED(size);
    }

    /// @brief Returns the text font size.
    /// @return The text font point size.
    /// @note The base method reports 12; the command-line interface has no
    /// text window. Subclasses can override this method to return the GUI font size.
    virtual double getTextFontSize() const
    {
        return 12;
    }

    /// @brief Returns the text font name.
    /// @return The text font name.
    /// @note The base method reports an empty name; the command-line interface
    /// has no text window. Subclasses can override this method to return the GUI font name.
    virtual QString getTextFontName() const
    {
        return QStringLiteral("");
    }

    /// @brief Sets the text font name.
    /// @param name The name of the font.
    /// @note The base method is a no-op; the command-line interface has no
    /// text window. Subclasses can override this method to set the GUI font name.
    virtual void setTextFontName(const QString &name)
    {
        Q_UNUSED(name);
    }

    /// @brief Returns all the font names.
    /// @return A list of all the font names.
    /// @note The command-line interface has no GUI font system, so the base
    /// method returns an empty list. Psychi overrides this with the families
    /// sent at GUI initialization.
    virtual QStringList getAllFontNames() const
    {
        return {};
    }

    /// @brief Sets the label font size.
    /// @param size The point size of the label font.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the label font size.
    virtual void setLabelFontSize(double size)
    {
        exit(1);
    }

    /// @brief Returns the label font size.
    /// @return The label font point size.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the label font size.
    virtual double getLabelFontSize() const
    {
        exit(1);
        return 12;
    }

    /// @brief Returns the label font name.
    /// @return The label font name.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the label font name.
    virtual QString getLabelFontName() const
    {
        exit(1);
        return {};
    }

    /// @brief Sets if the turtle is visible.
    /// @param visible True if the turtle should be drawn, false means the turtle should not be drawn.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set if the turtle is visible.
    virtual void setTurtleIsVisible(int visible)
    {
        exit(1);
    }

    /// @brief Sets the pen mode.
    /// @param mode The mode to set the pen to.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the pen mode.
    virtual void setPenmode(PenModeEnum mode)
    {
        exit(1);
    }

    /// @brief Sets if the pen is down.
    /// @param down True if the pen is down, false otherwise.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set if the pen is down.
    virtual void setPenIsDown(bool down)
    {
        exit(1);
    }

    /// @brief Sets the screen mode.
    /// @param mode The mode to set the screen to.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the screen mode.
    virtual void setScreenMode(ScreenModeEnum mode)
    {
        exit(1);
    }

    /// @brief Returns the screen mode.
    /// @return The screen mode.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the screen mode.
    virtual ScreenModeEnum getScreenMode() const
    {
        exit(1);
        return textScreenMode;
    }

    /// @brief Sets the pen size.
    /// @param size The size of the pen, in pixels.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the pen size.
    virtual void setPensize(double size)
    {
        exit(1);
    }

    /// @brief Sets the pen's line style, cap, join, and optional custom dash pattern.
    /// @param style The Qt pen style.
    /// @param cap The Qt pen cap style.
    /// @param join The Qt pen join style.
    /// @param dashPattern Dash lengths used only when style is Qt::CustomDashLine.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the pen pattern.
    virtual void setPenPattern(Qt::PenStyle, Qt::PenCapStyle, Qt::PenJoinStyle, const QList<qreal> &)
    {
        exit(1);
    }

    /// @brief Queries the graphics engine if the pen size is valid.
    /// @param size The size of the pen, in pixels.
    /// @return True if the pen size is valid, false otherwise.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return if the pen size is valid.
    virtual bool isPenSizeValid(double size) const
    {
        exit(1);
        return false;
    }

    /// @brief Sets if the canvas is bounded.
    /// @param bounded True if the canvas is currently bounded, false otherwise.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set if the canvas is bounded.
    virtual void setIsCanvasBounded(bool bounded)
    {
        exit(1);
    }

    /// @brief Returns true if the canvas is bounded.
    /// @return True if the canvas is bounded, false otherwise.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return if the canvas is bounded.
    virtual bool isCanvasBounded() const
    {
        exit(1);
        return false;
    }

    /// @brief Sets the splitter size ratios.
    /// @param ratio1 The ratio of the first splitter.
    /// @param ratio2 The ratio of the second splitter.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the splitter size ratios.
    virtual void setSplitterSizeRatios(float ratio1, float ratio2)
    {
        exit(1);
    }

    /// @brief Delay execution for the given number of milliseconds.
    /// @param msecs Milliseconds to wait. Zero flushes terminal output without sleeping.
    /// @details The command-line interface flushes stdout and sleeps with nanosleep,
    /// consuming a pending terminal signal if the sleep is interrupted. The GUI
    /// override asks Psychi to run the timer so the UI stays responsive.
    virtual void mwait(unsigned long msecs);

  protected:
    bool inputAtEof_ = false;

    /// @brief Consume a pending terminal signal after a blocking read returned EINTR.
    /// @details Throws FCError for TOPLEVEL and SYSTEM. Returns after PAUSE (or if
    /// no Logo signal was pending) so the caller can retry the read.
    void retryAfterInterrupt();

    /// @brief Wait until stdin has data, a hangup, or a consumed terminal signal.
    /// @return True if stdin is readable, false on hangup or error.
    /// @details Polls in short slices so a signal delivered to another thread
    /// is still noticed. Throws FCError for TOPLEVEL and SYSTEM.
    bool waitForInteractiveInput();
};

#endif // LOGOINTERFACE_H
