
//===-- qlogo/editorwindow.cpp - EditorWindow class implementation ---*- C++ -*-===//
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
/// This file contains the implementation of the EditorWindow class, which is
/// the editor window portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include "gui/editorwindow.h"
#include "ui_editorwindow.h"
#include <QCloseEvent>
#include <QKeyEvent>
#include <QTimer>

/// @brief The key sequence for reverting changes.
const QKeySequence::StandardKey revertChangesKey = QKeySequence::Close;

/// @brief The key sequence for saving changes.
const QKeySequence::StandardKey saveChangesKey = QKeySequence::Save;

EditorWindow::EditorWindow(QWidget *parent) : QMainWindow(parent), ui_(std::make_unique<Ui::EditorWindow>())
{
    ui_->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    Qt::WindowFlags closeFlag = Qt::WindowCloseButtonHint;
    flags = flags & (~closeFlag);
    setWindowFlags(flags);

    connect(ui_->acceptButton, &QPushButton::clicked, this, &EditorWindow::acceptChanges);
    connect(ui_->revertButton, &QPushButton::clicked, this, &EditorWindow::revertChanges);
    ui_->plainTextEdit->installEventFilter(this);
}

EditorWindow::~EditorWindow() = default;

void EditorWindow::setContents(const QString &startingText)
{
    ended_ = false;
    ui_->plainTextEdit->setPlainText(startingText);
}

void EditorWindow::setTextFormat(const QTextCharFormat &qtcf)
{
    ui_->plainTextEdit->setFont(qtcf.font());
    QPalette palette = ui_->plainTextEdit->palette();
    palette.setBrush(QPalette::Text, qtcf.foreground());
    palette.setBrush(QPalette::Base, qtcf.background());
    ui_->plainTextEdit->setPalette(palette);
}

void EditorWindow::show()
{
    QMainWindow::show();

    QTimer::singleShot(0, ui_->plainTextEdit, SLOT(setFocus()));
}

void EditorWindow::finish(bool accepted)
{
    if (ended_)
        return;
    ended_ = true;
    const QString text = accepted ? ui_->plainTextEdit->toPlainText() : QString();
    emit editingHasEndedSignal(accepted, text);
    close();
}

void EditorWindow::acceptChanges()
{
    finish(true);
}

void EditorWindow::revertChanges()
{
    finish(false);
}

void EditorWindow::closeEvent(QCloseEvent *event)
{
    // Window-manager close must not leave qlogo blocked on END_EDIT_TEXT.
    if (!ended_)
    {
        ended_ = true;
        emit editingHasEndedSignal(false, QString());
    }
    QMainWindow::closeEvent(event);
}

bool EditorWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        auto *keyEvent = static_cast<QKeyEvent *>(event);

        if (keyEvent->matches(saveChangesKey))
        {
            acceptChanges();
            return true;
        }
        if (keyEvent->matches(revertChangesKey))
        {
            revertChanges();
            return true;
        }

        return false;
    }
    else
    {
        // standard event processing
        return QObject::eventFilter(watched, event);
    }
}
