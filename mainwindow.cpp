
//===-- qlogo/mainwindow.cpp - MainWindow class implementation -------*- C++
//-*-===//
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
/// This file contains the implementation of the MainWindow class, which is the
/// main window portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include "mainwindow.h"
#include "canvas.h"
#include "datum.h"
#include "qlogo_controller.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  // There seems to be a bug involving window Maximize with an OpenGL widget.
  // So disable Maximize.
  setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint |
                 Qt::WindowCloseButtonHint);

  ui->splitter->setSizes(QList<int>() << 200 << 200);

  controller = new Controller();
  controller->setMainWindow(this);
  ui->mainConsole->setFocus();

  connect(ui->splitter, SIGNAL(splitterMoved(int, int)), controller,
          SLOT(splitterMoved(int, int)), Qt::AutoConnection);

  controller->start();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::show() {
  QMainWindow::show();
  QTimer::singleShot(0, this, &MainWindow::hideCanvas);
}

bool MainWindow::consoleHasChars() { return ui->mainConsole->charsInQueue(); }

Canvas *MainWindow::mainCanvas() { return ui->mainCanvas; }

Console *MainWindow::mainConsole() { return ui->mainConsole; }

void MainWindow::setSplitterSizeRatios(float canvasRatio, float consoleRatio) {
  QList<int> sizeList = ui->splitter->sizes();
  int sum = sizeList.first() + sizeList.last(); // there are only two
  ui->splitter->setSizes(QList<int>()
                         << canvasRatio * sum << consoleRatio * sum);
}

void MainWindow::hideCanvas() { setSplitterSizeRatios(0, 1); }

void MainWindow::closeEvent(QCloseEvent *event)
{
    controller->shutdownEvent();
    event->ignore();
}
