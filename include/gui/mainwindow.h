#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//===-- qlogo/mainwindow.h - MainWindow class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the MainWindow class, which is the
/// main window of the QLogo GUI.
///
//===----------------------------------------------------------------------===//

#include "sharedconstants.h"
#include <QDataStream>
#include <QMainWindow>
#include <QProcess>

class Canvas;
class Console;
class EditorWindow;

namespace Ui
{
class MainWindow;
}

/// @brief The main window of the QLogo GUI.
///
/// This class is the main window of the QLogo GUI. It contains the main
/// user interface and handles user input. It also handles the communication
/// with the QLogo process. This is similar to a terminal-based interface,
/// but it also has a graphical canvas to display the Turtle graphics.
///
/// To facilitate turtle, text, mouse, and keyboard communication, this class
/// uses a custom protocol to send and receive messages to and from the QLogo
/// process.
class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum windowMode_t
    {
        windowMode_noWait,
        windowMode_waitForChar,
        windowMode_waitForRawline,
    };

    Ui::MainWindow *ui;

    QByteArray readBuffer;
    qint64 readBufferLen = 0;

    windowMode_t windowMode;
    bool hasShownCanvas = false;
    EditorWindow *editWindow = nullptr;

    int startLogo();
    void beginReadRawlineWithPrompt(const QString &prompt);
    void beginReadChar();
    void sendConsoleCursorPosition();

    void initialize();
    void introduceCanvas();
    void setSplitterforMode(ScreenModeEnum mode);
    void openEditorWindow(const QString &startingText);

    void sendCanvasImage();
    void sendCanvasSvg();

    // Show user a file dialog modal window.
    void fileDialogModal();

    void processReadBuffer();

  protected:
    void closeEvent(QCloseEvent *event) override;
    QString findQlogoExe();

  public:
    /// @brief Constructor.
    ///
    /// @param parent The Qt parent widget.
    explicit MainWindow(QWidget *parent = nullptr);

    /// @brief Destructor.
    ~MainWindow() override;

    /// @brief Show the main window.
    void show();

    /// @brief Handle a mouse button being pressed.
    ///
    /// @param position The position of the mouse button.
    /// @param buttonID The ID of the button.
    void mouseButtonWasPressed(QVector2D position, int buttonID);

  public slots:

    /// @brief Handle the standard output of the QLogo process.
    void readStandardOutput();

    /// @brief Handle the standard error of the QLogo process.
    void readStandardError();

    /// @brief Handle the end of the QLogo process. Initiate shutdown of the GUI.
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

    /// @brief Handle a rawline being sent to the QLogo process.
    void sendRawlineSlot(const QString &line);

    /// @brief Handle a character being sent to the QLogo process.
    void sendCharSlot(QChar c);

    /// @brief Handle the splitter being moved.
    void splitterHasMovedSlot(int, int);

    /// @brief Handle the editing being ended.
    void editingHasEndedSlot(const QString &text);

    /// @brief Handle a mouse button being clicked.
    void mouseclickedSlot(QPointF QPointF, int buttonID);

    /// @brief Handle a mouse being moved.
    void mousemovedSlot(QPointF position);

    /// @brief Handle a mouse button being released.
    void mousereleasedSlot();
};

#endif // MAINWINDOW_H
