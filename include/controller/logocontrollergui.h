
//===-- qlogo/logocontrollergui.h - LogoControllerGUI class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the LogoControllerGUI class, which is
/// responsible for handling user input and output through Psychi, the QLogo-GUI
/// terminal application. In addition to text input and output, the GUI controller
/// also receives mouse and keyboard events from the GUI, and provides a way to
/// communicate Turtle movements and drawing commands to the QLogo canvas.
///
//===----------------------------------------------------------------------===//

#ifndef LOGOCONTROLLERGUI_H
#define LOGOCONTROLLERGUI_H

#include "controller/inputqueue.h"
#include "controller/logocontroller.h"
#include "sharedconstants.h"
#include <QDataStream>
#include <QFile>
#include <QFont>

/// @brief  The LogoControllerGUI class is a subclass of the LogoController class.
/// It provides a way to communicate user input and output through Psychi, the
/// QLogo-GUI terminal application.
class LogoControllerGUI : public LogoController
{
    InputQueue messageQueue;
    message_t getMessage();
    void waitForMessage(message_t expectedType);

    // Return values from getMessage()
    QString rawLine;
    QChar rawChar;

    QString filePath;

    int cursorRow;
    int cursorCol;

    // cursorOverwriteMode:
    // true:  cursor overwrites previously-written text
    // false: cursor inserts text (default)
    bool cursoreModeIsOverwrite = false;

    // Text returned from editor winow
    QString editorText;

    double penSize;

    double xbound = Config::get().initialBoundX;
    double ybound = Config::get().initialBoundY;
    bool canvasIsBounded = true;

    QVector2D mousePos = QVector2D(0, 0);
    QVector2D clickPos = QVector2D(0, 0);
    int lastButtonpressID = 0;
    bool isMouseButtonDown = false;

    QColor currentBackgroundColor = Config::get().initialCanvasBackgroundColor;
    QColor currentForegroundColor = Config::get().initialCanvasForegroundColor;
    QImage canvasImage;
    QByteArray canvasSvg;

    QStringList allFontNames;
    QString textFontName;
    double textFontSize;

    ScreenModeEnum screenMode;

    QString labelFontName;
    double labelFontSize;

    void processInputMessageQueue();

  public:
    /// @brief Constructor for the LogoControllerGUI class.
    /// @param parent The parent Qt object.
    LogoControllerGUI(QObject *parent = 0);

    /// @brief Destructor for the LogoControllerGUI class.
    ~LogoControllerGUI();

    /// @brief Initiates a system stop.
    void systemStop();

    /// @brief Initializes the GUI controller.
    void initialize();

    /// @brief Prints a string to the console.
    /// @param s The string to print.
    void printToConsole(const QString &s);

    /// @brief Prompts the user for input and returns the input as a string.
    /// @param prompt The prompt to display to the user, if any.
    /// @return The string entered by the user.
    QString inputRawlineWithPrompt(const QString &prompt);

    /// @brief Reads a character from the input queue.
    /// @return The character read from the input queue.
    /// @note This function is blocking. It will wait for a character to be
    /// available in the input queue.
    DatumPtr readchar();

    QString fileDialogModal();

    /// @brief Edits text in the GUI.
    /// @param startText The text to edit.
    /// @return The edited text.
    QString editText(QString startText);

    /// @brief Sets the turtle position.
    /// @param newTurtlePos The new turtle position.
    void setTurtlePos(const Transform &newTurtlePos);

    /// @brief Sets the turtle visibility.
    /// @param isVisible True if the turtle is visible, false otherwise.
    void setTurtleIsVisible(int isVisible);

    /// @brief Sets the pen mode.
    /// @param aMode The new pen mode.
    void setPenmode(PenModeEnum aMode);

    /// @brief Emits a vertex to the canvas.
    void emitVertex();

    /// @brief Sets the pen state.
    /// @param penIsDown True if the pen should be down, false otherwise.
    void setPenIsDown(bool penIsDown);

    /// @brief Sets the canvas foreground color.
    /// @param color The new foreground color.
    void setCanvasForegroundColor(const QColor &color);

    /// @brief Sets the canvas background color.
    /// @param color The new background color.
    void setCanvasBackgroundColor(const QColor &color);

    /// @brief Sets the canvas background image.
    /// @param image The new background image.
    void setCanvasBackgroundImage(QImage image);

    /// @brief Begins a polygon.
    /// @param color The color of the polygon.
    void beginPolygon(const QColor &color);

    /// @brief Ends a polygon.
    void endPolygon();

    /// @brief Clears the canvas.
    void clearCanvas();

    /// @brief Draws a label to the canvas.
    /// @param text The text to draw.
    /// @note Draws a label at the current turtle position.
    void drawLabel(QString text);

    /// @brief Draws an arc to the canvas.
    /// @param angle The angle of the arc.
    /// @param radius The radius of the arc.
    void drawArc(double angle, double radius);

    /// @brief Gets if the mouse button is down.
    /// @return True if the mouse button is down, false otherwise.
    bool getIsMouseButtonDown();

    /// @brief Gets and resets the button ID.
    /// @return The button ID.
    /// @note The button ID is reset to indicate that the button
    /// has not been pressed since the last time this function returned a buttonID.
    int getAndResetButtonID();

    /// @brief Gets the last mouse click position.
    /// @return The last mouse click position.
    QVector2D lastMouseclickPosition();

    /// @brief Gets the current mouse position.
    /// @return The current mouse position.
    QVector2D mousePosition();

    /// @brief Sets the screen mode.
    /// @param newMode The new screen mode.
    void setScreenMode(ScreenModeEnum newMode);

    /// @brief Gets the current screen mode.
    /// @return The current screen mode.
    ScreenModeEnum getScreenMode();

    /// @brief Sets the canvas bounds.
    /// @param x The X coordinate of the upper-right corner of the bounding box.
    /// @param y The Y coordinate of the upper-right corner of the bounding box.
    /// @note The X and Y coordinates of the lower left are the negative of the upper-right coordinates.
    /// The origin [0,0] is always in the center of the canvas.
    void setBounds(double x, double y);

    /// @brief Gets the X coordinate of the upper-right corner of the bounding box.
    /// @return The X coordinate of the upper-right corner of the bounding box.
    double boundX() const
    {
        return xbound;
    }

    /// @brief Gets the Y coordinate of the upper-right corner of the bounding box.
    /// @return The Y coordinate of the upper-right corner of the bounding box.
    double boundY() const
    {
        return ybound;
    }

    /// @brief Gets the canvas background color.
    /// @return The canvas background color.
    const QColor getCanvasBackgroundColor(void) const;

    /// @brief Sets if the canvas is bounded.
    /// @param aIsBounded True if the canvas should be bounded, false otherwise.
    void setIsCanvasBounded(bool aIsBounded);

    /// @brief Gets if the canvas is bounded.
    /// @return True if the canvas is bounded, false otherwise.
    bool isCanvasBounded() const;

    /// @brief Checks if a candidate pen size is valid.
    /// @param candidate The candidate pen size.
    /// @return True if the candidate pen size is valid, false otherwise.
    bool isPenSizeValid(double candidate) const
    {
        return candidate >= 0;
    }

    /// @brief Gets the canvas image.
    /// @return The current canvas image.
    /// @note This method modifies state (waits for message), so it cannot be const.
    QImage getCanvasImage();

    /// @brief Gets the canvas SVG image.
    /// @return The current canvas as a SVG image.
    /// @note This method modifies state (waits for message), so it cannot be const.
    QByteArray getSvgImage();

    /// @brief Sets the text font size.
    /// @param aSize The new font size.
    void setTextFontSize(double aSize);

    /// @brief Gets the text font size.
    /// @return The current text font size.
    double getTextFontSize() const;

    /// @brief Gets the text font name.
    /// @return The current text font name.
    QString getTextFontName() const;

    /// @brief Sets the text font name.
    /// @param aFontName The new font name.
    void setTextFontName(QString aFontName);

    /// @brief Gets all available font names.
    /// @return A list of all available font names.
    QStringList getAllFontNames() const
    {
        return allFontNames;
    }

    /// @brief Adds standout control characters to a string.
    /// @param src The string to add the standout to.
    /// @return The string with the standout control characters added.
    QString addStandoutToString(QString src);

    /// @brief Gets the text cursor position.
    /// @param row The row of the text cursor position.
    /// @param col The column of the text cursor position.
    void getTextCursorPos(int &row, int &col);

    /// @brief Sets the text cursor position.
    /// @param row The row of the new text cursor position.
    /// @param col The column of the new text cursor position.
    void setTextCursorPos(int row, int col);

    /// @brief Sets the text color.
    /// @param foregroundColor The new foreground color.
    /// @param backgroundColor The new background color.
    void setTextColor(const QColor &foregroundColor, const QColor &backgroundColor);

    /// @brief Sets the cursor overwrite mode.
    /// @param isOverwriteMode True if the cursor should overwrite text, false otherwise.
    void setCursorOverwriteMode(bool isOverwriteMode);

    /// @brief Gets if the cursor is in overwrite mode.
    /// @return True if the cursor is in overwrite mode, false otherwise.
    bool cursorOverwriteMode();

    /// @brief Sets the label font size.
    /// @param aSize The new font size.
    void setLabelFontSize(double aSize);

    /// @brief Gets the label font size.
    /// @return The current label font size.
    double getLabelFontSize();

    /// @brief Gets the label font name.
    /// @return The current label font name.
    QString getLabelFontName();

    /// @brief Sets the label font name.
    /// @param aName The new font name.
    void setLabelFontName(QString aName);

    /// @brief Sets the label font name.
    /// @param aName The new font name.

    /// @brief Sets the pen size.
    /// @param aSize The new pen size.
    void setPensize(qreal);

    /// @brief Waits for a number of milliseconds.
    /// @param msecs The number of milliseconds to wait.
    void mwait(unsigned long msecs);

    /// @brief Clears all of the text from the GUI console.
    void clearScreenText();
};

#endif // LOGOCONTROLLERGUI_H
