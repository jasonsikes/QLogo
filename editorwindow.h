#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

//===-- qlogo/editorwindow.h - EditorWindow class definition -------*- C++
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
/// This file contains the declaration of the EditorWindow class, which is the
/// editor window portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include <QMainWindow>
#include <QTextCharFormat>

namespace Ui {
class EditorWindow;
}

class EditorWindow : public QMainWindow {
  Q_OBJECT

  bool eventFilter(QObject *watched, QEvent *event);

public slots:

  void acceptChanges();
  void revertChanges();

public:
  explicit EditorWindow(QWidget *parent = 0);
  ~EditorWindow();
  void setContents(const QString startingText);
  void show();
  void setTextFormat(const QTextCharFormat &qtcf);

signals:

  void
  editingHasEndedSignal(QString text);

private:
  Ui::EditorWindow *ui;
};

#endif // EDITORWINDOW_H
