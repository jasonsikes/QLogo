#ifndef CONSOLE_H
#define CONSOLE_H

//===-- qlogo/console.h - Console class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Console class, which is the
/// text portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include <QTextEdit>

/// @brief The Console class is the text terminal of the user interface.
class Console : public QTextEdit
{
    Q_OBJECT

  protected:
    enum consoleMode_t
    {
        consoleModeNoWait,
        consoleModeWaitingForChar,
        consoleModeWaitingForRawline,
    };
    consoleMode_t consoleMode;
    int beginningOfRawline;
    int beginningOfRawlineInBlock;

    // Line input history
    QStringList lineInputHistory;
    int lineInputHistoryScrollingCurrentIndex;

    // Keypress and paste buffers
    QString keyQueue;

    QTextCharFormat textFormat;

    bool isPrintingStandout = false;
    void writeTextFragment(const QString text);
    void standout();

    // Key press events
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void processLineModeKeyPressEvent(QKeyEvent *event);
    void processCharModeKeyPressEvent(QKeyEvent *);
    void processNoWaitKeyPressEvent(QKeyEvent *event);

    void replaceLineWithHistoryIndex(int newIndex);

    void insertNextLineFromQueue();
    void insertNextCharFromQueue();
    void insertFromMimeData(const QMimeData *source) Q_DECL_OVERRIDE;

  public:
    /// @brief Constructor
    /// @param parent The Qt parent widget
    Console(QWidget *parent = 0);

    /// @brief Destructor
    ~Console();

    /// @brief Print a string to the console
    /// @param text The text to print
    void printString(const QString text);

    /// @brief Request a rawline from the user
    /// @param prompt The prompt to display to the user
    void requestRawlineWithPrompt(const QString prompt);

    /// @brief Request a character from the user
    void requestChar();

    /// @brief Get the cursor position
    /// @param row The row of the cursor
    /// @param col The column of the cursor
    void getCursorPos(int &row, int &col);

    /// @brief Set the cursor position
    /// @param row The row of the cursor
    /// @param col The column of the cursor
    void setTextCursorPosition(int row, int col);

    /// @brief Set the font name
    /// @param aName The name of the font
    void setTextFontName(const QString aName);

    /// @brief Set the font size
    /// @param aSize The size of the font
    void setTextFontSize(qreal aSize);

    /// @brief Set the font color
    /// @param foreground The foreground color
    /// @param background The background color
    void setTextFontColor(QColor foreground, QColor background);

    /// @brief Get the font
    /// @return The font
    const QTextCharFormat getFont()
    {
        return textFormat;
    }

  signals:

    /// @brief Send a rawline to the qlogo interpreter
    /// @param rawLine The rawline to send
    void sendRawlineSignal(const QString &rawLine);

    /// @brief Send a character to the qlogo interpreter
    /// @param c The character to send
    void sendCharSignal(QChar c);
};

#endif // CONSOLE_H
