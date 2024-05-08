
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

#include "gui/editorwindow.h"
#include "QDebug"
#include "ui_editorwindow.h"
#include <QTimer>

const QKeySequence::StandardKey revertChangesKey = QKeySequence::Close;
const QKeySequence::StandardKey saveChangesKey = QKeySequence::Save;

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

void EditorWindow::setContents(const QString startingText) {
  ui->plainTextEdit->setPlainText(startingText);
}

void EditorWindow::setTextFormat(const QTextCharFormat &qtcf) {
  ui->plainTextEdit->setFont(qtcf.font());
  QPalette palette = ui->plainTextEdit->palette();
  palette.setBrush(QPalette::Text, qtcf.foreground());
  palette.setBrush(QPalette::Base, qtcf.background());
  ui->plainTextEdit->setPalette(palette);
}

void EditorWindow::show() {
  QMainWindow::show();

  QTimer::singleShot(0, ui->plainTextEdit, SLOT(setFocus()));
}

void EditorWindow::acceptChanges() {
  QString text = ui->plainTextEdit->toPlainText();
  emit editingHasEndedSignal(text);
  close();
}

void EditorWindow::revertChanges() {
  emit editingHasEndedSignal("");
  close();
}

bool EditorWindow::eventFilter(QObject *watched, QEvent *event) {
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

    if (keyEvent->matches(saveChangesKey)) {
      acceptChanges();
      return true;
    }
    if (keyEvent->matches(revertChangesKey)) {
      revertChanges();
      return true;
    }

    return false;
  } else {
    // standard event processing
    return QObject::eventFilter(watched, event);
  }
}
