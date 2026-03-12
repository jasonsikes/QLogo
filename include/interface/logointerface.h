
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
#include <QObject>
#include <QThread>
#include <QVector2D>
#include <QTextStream>

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

    virtual void processInputMessageQueue()
    {
    }

  public:
    /// @brief Constructor
    /// @param parent The Qt parent object.
    LogoInterface(QObject *parent = nullptr);

    /// @brief Destructor
    ~LogoInterface() override;

    /// @brief Returns the most recent interrupt signal that was received, if any. Resets the signal.
    /// @return The most recent interrupt signal that was received, if any.
    /// @note This method is meant to be called by the main loop to check for
    /// signals that are sent to the application.
    SignalsEnum_t latestSignal();

    /// @brief Initializes the interface.
    /// @note The base method does nothing. Subclasses can override this method to perform
    /// initialization tasks.
    virtual void initialize()
    {
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
    /// method is called, the value is reset to noSignal.
    /// @note This is a static member to allow access from the signal handler.
    static volatile SignalsEnum_t lastSignal_;

    /// @brief Stops communication and initiates cleanup tasks.
    virtual void closeInterface();

    virtual QString fileDialogModal()
    {
        exit(1);
        return {};
    }

    /// @brief Edits a text string.
    /// @param text The text to edit.
    /// @return The edited text.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to edit the text.
    virtual QString editText(const QString &text)
    {
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

    /// @brief Sets the dribble file path.
    /// @param filePath The path to the dribble file. Use an empty string to disable dribbling.
    /// @return True if the dribble file was set successfully, false otherwise.
    bool setDribble(const QString &filePath);

    /// @brief Checks if the dribble file is open.
    /// @return True if the dribble file is open, false otherwise.
    bool isDribbling();

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
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to clear the text on the screen.
    virtual void clearScreenText()
    {
        exit(1);
    }

    /// @brief Sets the cursor position.
    /// @param x The x coordinate of the cursor.
    /// @param y The y coordinate of the cursor.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the cursor position.
    virtual void setTextCursorPos(int x, int y)
    {
        exit(1);
    }

    /// @brief Returns the cursor position.
    /// @param x The x coordinate of the cursor.
    /// @param y The y coordinate of the cursor.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the cursor position.
    virtual void getTextCursorPos(int &x, int &y)
    {
        exit(1);
    }

    /// @brief Sets the text color.
    /// @param text The color of the text.
    /// @param background The background color of the text.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the text color.
    virtual void setTextColor(const QColor &text, const QColor &background)
    {
        exit(1);
    }

    /// @brief Sets the text font size.
    /// @param size The point size of the font.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the text font size.
    virtual void setTextFontSize(double size)
    {
        exit(1);
    }

    /// @brief Returns the text font size.
    /// @return The text font point size.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the text font size.
    virtual double getTextFontSize() const
    {
        exit(1);
        return 12;
    }

    /// @brief Returns the text font name.
    /// @return The text font name.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return the text font name.
    virtual QString getTextFontName() const
    {
        exit(1);
        return {};
    }

    /// @brief Sets the text font name.
    /// @param name The name of the font.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the text font name.
    virtual void setTextFontName(const QString &name)
    {
        exit(1);
    }

    /// @brief Returns all the font names.
    /// @return A list of all the font names.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return all the font names.
    virtual QStringList getAllFontNames() const
    {
        exit(1);
        return {};
    }

    /// @brief Sets the cursor overwrite mode.
    /// @param mode True if the cursor should overwrite, false indicates the cursor should insert.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to set the cursor overwrite mode.
    virtual void setCursorOverwriteMode(bool mode)
    {
        exit(1);
    }

    /// @brief Returns if the cursor is in overwrite mode.
    /// @return True if the cursor is in overwrite mode, false indicates the cursor is in insert mode.
    /// @note The base method triggers an error message. Subclasses can override this method to
    /// perform the necessary tasks to return if the cursor is in overwrite mode.
    virtual bool cursorOverwriteMode() const
    {
        exit(1);
        return false;
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

  protected:
    QTextStream *dribbleStream_;

    QTextStream inStream_;
    QTextStream outStream_;
};

#endif // LOGOINTERFACE_H
