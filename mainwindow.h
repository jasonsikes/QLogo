#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//===-- qlogo/mainwindow.h - MainWindow class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the MainWindow class, which is the
/// main window portion of the user interface.
///
//===----------------------------------------------------------------------===//



#include <QMainWindow>
#include <QtGui/QOpenGLFunctions>

class Controller;
class Canvas;
class Console;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

//  Controller *controller;
  enum WaitingMode { notWaiting, waitingForKeypress, waitingForLine };
  WaitingMode waitingFor;
  void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

public:

  enum MainWindowMessage : char {
      C_CONSOLE_PRINT_STRING,
      C_CONSOLE_SET_TEXT_SIZE,
      C_CONSOLE_SET_CURSOR_POS,
      C_CONSOLE_SET_TEXT_COLOR,
      C_CONSOLE_CLEAR_TEXT,
      C_CONSOLE_SET_FONT,
      C_CONSOLE_REQUEST_CHARACTER,
      C_CONSOLE_REQUEST_LINE,
      C_CONSOLE_REQUEST_CURSOR_POS,
      C_CANVAS_SET_TURTLE_POS
  };

  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

  void show();

  /// Returns 'true' if there are keystrokes in the input buffer
  bool consoleHasChars();

  /// Returns a pointer to the Canvas object.
  Canvas *mainCanvas();

  /// Returns a pointer to the Consoleobject.
  Console *mainConsole();

  /// Programatically set the ratio of canvas size versus console size.
  void setSplitterSizeRatios(float canvasRatio, float consoleRatio);

public slots:

  void hideCanvas();

  /// Accept a message (command) from the iterpreter.
  ///
  /// This is the means by which output is presented to the user.
  /// Messages passed from the Controller have a single QChar prefix.
  /// The remainder of the message will be formatted according to the prefix:
  ///
  /// Console messages:
  ///
  /// C_CONSOLE_PRINT_STRING
  ///  : int length (as reported by QString::size)
  ///  : QChar* data (as returned by QString::constData)
  ///
  /// C_CONSOLE_SET_TEXT_SIZE
  ///  : double new size of text
  ///
  /// C_CONSOLE_SET_CURSOR_POS
  ///  : int row
  ///  : int column
  ///
  /// C_CONSOLE_SET_TEXT_COLOR
  ///  : qint16 foreground red
  ///  : qint16 foreground green
  ///  : qint16 foreground blue
  ///  : qint16 foreground alpha
  ///  : qint16 background red
  ///  : qint16 background green
  ///  : qint16 background blue
  ///  : qint16 background alpha
  ///
  /// C_CONSOLE_CLEAR_TEXT
  ///
  /// C_CONSOLE_SET_FONT
  ///  : int length (as reported by QString::size)
  ///  : QChar* data (as returned by QString::constData)
  ///
  /// C_CONSOLE_REQUEST_CHARACTER
  ///
  /// C_CONSOLE_REQUEST_LINE
  ///
  /// C_CONSOLE_REQUEST_CURSOR_POS
  ///
  ///
  /// Canvas Messages
  ///
  /// C_CANVAS_SET_TURTLE_POS
  ///  : float* data (as reported by QMatrix4x4::data)
  void takeMesage(const QByteArray &message);

private:
  Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
