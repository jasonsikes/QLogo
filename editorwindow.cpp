
//===-- qlogo/editorwindow.cpp - EditorWindow class implementation -------*- C++
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
/// This file contains the implementation of the EditorWindow class, which is
/// the editor window portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include "editorwindow.h"
#include "QDebug"
#include "ui_editorwindow.h"
#include <QTimer>

EditorWindow::EditorWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::EditorWindow) {
  ui->setupUi(this);

  Qt::WindowFlags flags = windowFlags();
  Qt::WindowFlags closeFlag = Qt::WindowCloseButtonHint;
  flags = flags & (~closeFlag);
  setWindowFlags(flags);

  connect(ui->acceptButton, SIGNAL(clicked(bool)), this, SLOT(acceptChanges()));
  connect(ui->revertButton, SIGNAL(clicked(bool)), this, SLOT(revertChanges()));
  ui->plainTextEdit->installEventFilter(this);
}

EditorWindow::~EditorWindow() { delete ui; }

void EditorWindow::setContents(QString *startingText) {
  ui->plainTextEdit->setPlainText(*startingText);
}

void EditorWindow::setTextFormat(const QTextCharFormat &qtcf) {
  ui->plainTextEdit->setCurrentCharFormat(qtcf);
}

void EditorWindow::show() {
  QMainWindow::show();

  QTimer::singleShot(0, ui->plainTextEdit, SLOT(setFocus()));
}

void EditorWindow::acceptChanges() {
  text = ui->plainTextEdit->toPlainText();
  editingHasEndedSignal(&text);
  close();
}

void EditorWindow::revertChanges() {
  editingHasEndedSignal(NULL);
  close();
}

bool EditorWindow::eventFilter(QObject *watched, QEvent *event) {
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

    if (keyEvent->matches(QKeySequence::Save)) {
        acceptChanges();
        return true;
    }
    if (keyEvent->matches(QKeySequence::Close)) {
        revertChanges();
        return true;
    }

    return false;
  } else {
    // standard event processing
    return QObject::eventFilter(watched, event);
  }
}
