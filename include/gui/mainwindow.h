#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//===-- qlogo/mainwindow.h - MainWindow class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the MainWindow class, which is the
/// main window of the QLogo GUI.
///
//===----------------------------------------------------------------------===//

#include "sharedconstants.h"
#include <QDataStream>
#include <QMainWindow>
#include <QProcess>
#include <QtGui/QOpenGLFunctions>
#include <functional>

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

    QProcess *logoProcess;

    windowMode_t windowMode;
    bool hasShownCanvas = false;
    EditorWindow *editWindow = NULL;

    int startLogo();
    void beginReadRawlineWithPrompt(const QString prompt);
    void beginReadChar();
    void sendConsoleCursorPosition();

    void sendMessage(std::function<void(QDataStream *)> func);

    void initialize();
    void introduceCanvas();
    void setSplitterforMode(ScreenModeEnum mode);
    void openEditorWindow(const QString startingText);

    void sendCanvasImage();
    void sendCanvasSvg();

  protected:
    void closeEvent(QCloseEvent *event);
    QString findQlogoExe();

  public:

    /// @brief Constructor.
    ///
    /// @param parent The Qt parent widget.
    explicit MainWindow(QWidget *parent = 0);

    /// @brief Destructor.
    ~MainWindow();

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

    /// @brief Handle the start of the QLogo process.
    /// @note This is mostly for debugging to ensure the process is started
    /// correctly.
    void processStarted();

    /// @brief Handle the end of the QLogo process. Initiate shutdown of the GUI.
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

    /// @brief Handle an error in the QLogo process.
    /// @note This is mostly for debugging to print out a debug message.
    void errorOccurred(QProcess::ProcessError error);

    /// @brief Handle a rawline being sent to the QLogo process.
    void sendRawlineSlot(const QString &line);

    /// @brief Handle a character being sent to the QLogo process.
    void sendCharSlot(QChar c);

    /// @brief Handle the splitter being moved.
    void splitterHasMovedSlot(int, int);

    /// @brief Handle the editing being ended.
    void editingHasEndedSlot(QString text);

    /// @brief Handle a mouse button being clicked.
    void mouseclickedSlot(QPointF QPointF, int buttonID);

    /// @brief Handle a mouse being moved.
    void mousemovedSlot(QPointF position);

    /// @brief Handle a mouse button being released.
    void mousereleasedSlot();
};

#endif // MAINWINDOW_H
