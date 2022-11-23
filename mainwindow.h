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
/// main window portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include <QMainWindow>
#include <QtGui/QOpenGLFunctions>
#include <QProcess>
#include <QDataStream>
#include <functional>

class Canvas;
class Console;
class EditorWindow;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

    enum windowMode_t {
        windowMode_noWait,
        windowMode_waitForChar,
        windowMode_waitForRawline,
    };

protected:
    void closeEvent ( QCloseEvent * event );

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();
  void show();

private:
  Ui::MainWindow *ui;

  QProcess *logoProcess;

  windowMode_t windowMode;
  bool hasShownCanvas = false;
  EditorWindow *editWindow = NULL;

  int startLogo();
  void beginReadRawline();
  void beginReadChar();
  void sendMessage(std::function<void (QDataStream*)> func);

  void initialize();
  void introduceCanvas();
  void openEditorWindow(const QString startingText);

  void sendCanvasImage();

public slots:
  void readStandardOutput();
  void readStandardError();
  void processStarted();
  void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void errorOccurred(QProcess::ProcessError error);

  void sendRawlineSlot(const QString &line);
  void sendCharSlot(QChar c);
  void splitterHasMovedSlot(int, int);
  void editingHasEndedSlot(QString text);
};

#endif // MAINWINDOW_H
