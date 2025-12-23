
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
#include "QDebug"
#include "ui_editorwindow.h"
#include <QTimer>

/// @brief The key sequence for reverting changes.
const QKeySequence::StandardKey revertChangesKey = QKeySequence::Close;

/// @brief The key sequence for saving changes.
const QKeySequence::StandardKey saveChangesKey = QKeySequence::Save;

EditorWindow::EditorWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::EditorWindow)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    Qt::WindowFlags closeFlag = Qt::WindowCloseButtonHint;
    flags = flags & (~closeFlag);
    setWindowFlags(flags);

    connect(ui->acceptButton, SIGNAL(clicked(bool)), this, SLOT(acceptChanges()));
    connect(ui->revertButton, SIGNAL(clicked(bool)), this, SLOT(revertChanges()));
    ui->plainTextEdit->installEventFilter(this);
}

EditorWindow::~EditorWindow()
{
    delete ui;
}

void EditorWindow::setContents(const QString startingText)
{
    ui->plainTextEdit->setPlainText(startingText);
}

void EditorWindow::setTextFormat(const QTextCharFormat &qtcf)
{
    ui->plainTextEdit->setFont(qtcf.font());
    QPalette palette = ui->plainTextEdit->palette();
    palette.setBrush(QPalette::Text, qtcf.foreground());
    palette.setBrush(QPalette::Base, qtcf.background());
    ui->plainTextEdit->setPalette(palette);
}

void EditorWindow::show()
{
    QMainWindow::show();

    QTimer::singleShot(0, ui->plainTextEdit, SLOT(setFocus()));
}

void EditorWindow::acceptChanges()
{
    QString text = ui->plainTextEdit->toPlainText();
    emit editingHasEndedSignal(text);
    close();
}

void EditorWindow::revertChanges()
{
    emit editingHasEndedSignal("");
    close();
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
