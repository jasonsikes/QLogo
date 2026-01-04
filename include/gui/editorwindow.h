#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

//===-- qlogo/editorwindow.h - EditorWindow class definition ---*- C++ -*-===//
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
    bool eventFilter(QObject *watched, QEvent *event) override;

  public:
    /// @brief Constructor.
    explicit EditorWindow(QWidget *parent = nullptr);

    /// @brief Destructor.
    ~EditorWindow() override;

    /// @brief Set the contents of the editor window.
    ///
    /// This function sets the contents of the editor window to the given text.
    ///
    /// @param startingText The text to present to the user for editing.
    void setContents(const QString &startingText);

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
