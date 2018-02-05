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
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

  void show();
  bool consoleHasChars();

  Canvas *mainCanvas();
  Console *mainConsole();
  void setSplitterSizeRatios(float canvasRatio, float consoleRatio);

public slots:
  void hideCanvas();

private:
  Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
