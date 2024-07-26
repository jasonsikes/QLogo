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

namespace Ui
{
class EditorWindow;
}

/// @brief The editor window.
///
/// This class is the editor window for the QLogo GUI. It is a QMainWindow that
/// contains a QTextEdit widget to edit text when the user requests to edit the
/// text.
class EditorWindow : public QMainWindow
{
    Q_OBJECT

    Ui::EditorWindow *ui;
    bool eventFilter(QObject *watched, QEvent *event);

  public:
    /// @brief Constructor.
    explicit EditorWindow(QWidget *parent = 0);

    /// @brief Destructor.
    ~EditorWindow();

    /// @brief Set the contents of the editor window.
    ///
    /// This function sets the contents of the editor window to the given text.
    ///
    /// @param startingText The text to present to the user for editing.
    void setContents(const QString startingText);

    /// @brief Show the editor window.
    void show();

    /// @brief Set the text format.
    ///
    /// This function sets the text format for the editor window.
    ///
    /// @param qtcf The text format to set.
    void setTextFormat(const QTextCharFormat &qtcf);

  signals:

    /// @brief The editing has ended.
    ///
    /// This signal is emitted when the user has ended editing the text.
    ///
    /// @param text The text that the user has edited. May be an empty string if
    /// the user cancelled the editing.
    void editingHasEndedSignal(QString text);

  public slots:

    /// @brief Accept the changes.
    ///
    /// This function is called when the user accepts the changes to the text.
    void acceptChanges();

    /// @brief Revert the changes.
    ///
    /// This function is called when the user cancels the editing.
    void revertChanges();
};

#endif // EDITORWINDOW_H
