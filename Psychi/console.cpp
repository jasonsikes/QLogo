
//===-- qlogo/console.cpp - Console class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Console class, which is the
/// text portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include "gui/console.h"
#include "sharedconstants.h"
#include <QDebug>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QTextBlock>

Console::Console(QWidget *parent) : QTextEdit(parent)
{
    consoleMode = consoleModeNoWait;
    textFormat.setForeground(QBrush(QWidget::palette().color(QPalette::Text)));
}

// Write a fragment of text
void Console::writeTextFragment(const QString &text)
{
    QTextCursor tc = textCursor();
    // If we are overwriting, delete the previous text before inserting
    if (overwriteMode())
    {
        int len = text.length();
        int pos = tc.positionInBlock();
        int lineLen = tc.block().length() - 1; // minus one for "newline"
        if (pos < lineLen)
        {
            if ((pos + len) > lineLen)
            {
                tc.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                tc.removeSelectedText();
            }
            else if (len > 0)
            {
                tc.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, len);
                tc.removeSelectedText();
            }
        }
    }
    tc.setCharFormat(textFormat);
    tc.insertText(text);
}

void Console::standout()
{
    if (isPrintingStandout)
    {
        textFormat.setForeground(textFormat.background());
        textFormat.setBackground(QBrush(Qt::transparent));
    }
    else
    {
        textFormat.setBackground(textFormat.foreground());
        textFormat.setForeground(palette().brush(QPalette::Base));
    }
    isPrintingStandout = !isPrintingStandout;
}

void Console::printString(const QString &text)
{
    // Because STANDOUT requires characters added to strings, we have to
    // handle them here.
    QStringList stringList = text.split(Config::get().escapeChar);
    for (auto i = stringList.begin(); i != stringList.end(); ++i)
    {

        if (i != stringList.begin())
        {
            standout();
        }
        writeTextFragment(*i);
    }
    ensureCursorVisible();
}

void Console::setTextFontName(const QString &aName)
{
    QFont f = textFormat.font();
    f.setFamily(aName);
    textFormat.setFont(f);
}

void Console::setTextFontSize(qreal aSize)
{
    QFont f = textFormat.font();
    f.setPointSizeF(aSize);
    textFormat.setFont(f);
}

void Console::setTextFontColor(QColor foreground, QColor background)
{
    textFormat.setForeground(QBrush(foreground));
    if (background.isValid())
    {
        QBrush brush = QBrush(background);
        QPalette p = palette();
        p.setBrush(QPalette::Base, brush);
        setPalette(p);
    }
}

void Console::requestRawlineWithPrompt(const QString &prompt)
{
    consoleMode = consoleModeWaitingForRawline;
    moveCursor(QTextCursor::End);
    printString(prompt);
    beginningOfRawline = textCursor().position();
    beginningOfRawlineInBlock = textCursor().positionInBlock();
    lineInputHistory.push_back("");
    lineInputHistoryScrollingCurrentIndex = lineInputHistory.size() - 1;

    insertNextLineFromQueue();
}

void Console::requestChar()
{
    consoleMode = consoleModeWaitingForChar;

    insertNextCharFromQueue();
}

void Console::getCursorPos(int &row, int &col)
{
    QTextCursor tc = textCursor();
    row = tc.blockNumber();
    col = tc.positionInBlock();
}

// Set the cursor position.
//
// The QTextEdit widget isn't a grid, like most terminals. Instead, it is
// organized into "blocks" with each block containing a string of text.
// We use this organization style to implement rows and columns. Each block
// is a row, and each character position in a block is a column.
// So to set the cursor position, we first move to the block, then move the
// cursor to the correct position. We add blocks and spaces as needed to
// move the cursor to the correct position.
void Console::setTextCursorPosition(int row, int col)
{
    int countOfRows = document()->blockCount();
    while (countOfRows <= row)
    {
        moveCursor(QTextCursor::End);
        textCursor().insertBlock();
        ++countOfRows;
    }

    QTextBlock line = document()->findBlockByNumber(row);
    int countOfCols = line.length();

    QTextCursor tc = textCursor();
    if (countOfCols <= col)
    {
        tc.setPosition(line.position());
        tc.movePosition(QTextCursor::EndOfBlock);
        QString fill = QString(col - countOfCols + 1, QChar(' '));
        tc.insertText(fill);
        tc.movePosition(QTextCursor::EndOfBlock);
    }
    else
    {
        tc.setPosition(line.position() + col);
    }
    setTextCursor(tc);
}

void Console::keyPressEvent(QKeyEvent *event)
{
    // TODO: User interface interrupt event handling
    switch (consoleMode)
    {
    case consoleModeWaitingForRawline:
        processLineModeKeyPressEvent(event);
        break;
    case consoleModeWaitingForChar:
        processCharModeKeyPressEvent(event);
        break;
    default:
        processNoWaitKeyPressEvent(event);
    }
}

void Console::processCharModeKeyPressEvent(QKeyEvent *event)
{
    QString t = event->text();
    if (t.length() > 0)
    {
        consoleMode = consoleModeNoWait;
        if (t.length() > 1)
        {
            keyQueue.push_back(t.right(t.length() - 1));
        }
        emit sendCharSignal(t[0]);
    }
}

void Console::processNoWaitKeyPressEvent(QKeyEvent *event)
{
    keyQueue.push_back(event->text());
}

// Process a key press event in line mode. We try to keep the cursor within
// the last line of text to support terminal actions. However, we also need
// to allow the user to select and copy regions of text that are outside the
// last line.
//
// We also need to handle the arrow keys and other special keys that are used to
// move the cursor within the text as well scroll through the buffer of
// previous lines entered by the user.
void Console::processLineModeKeyPressEvent(QKeyEvent *event)
{
    int eventKey = event->key();
    QTextCursor tc = textCursor();

    // These will only work if the cursor and anchor are
    // strictly after the prompt:
    if ((tc.position() > beginningOfRawline) && (tc.anchor() > beginningOfRawline) &&
        ((eventKey == Qt::Key_Backspace) || event->matches(QKeySequence::MoveToPreviousChar)))
    {
        QTextEdit::keyPressEvent(event);
        return;
    }

    // These will only work if the cursor and anchor are
    // on or after the prompt:
    if ((tc.position() >= beginningOfRawline) && (tc.anchor() >= beginningOfRawline))
    {
        if (event->matches(QKeySequence::MoveToPreviousLine))
        {
            if (lineInputHistoryScrollingCurrentIndex > 0)
            {
                replaceLineWithHistoryIndex(lineInputHistoryScrollingCurrentIndex - 1);
            }
            return;
        }
        if (event->matches(QKeySequence::MoveToNextLine))
        {
            if (lineInputHistoryScrollingCurrentIndex < lineInputHistory.size() - 1)
            {
                replaceLineWithHistoryIndex(lineInputHistoryScrollingCurrentIndex + 1);
            }
            return;
        }
        if (event->matches(QKeySequence::Paste))
        {
            QTextEdit::keyPressEvent(event);
            insertNextLineFromQueue();
            return;
        }
        if (event->matches(QKeySequence::Cut) || event->matches(QKeySequence::MoveToNextChar) ||
            ((event->text() != "") && (event->text().at(0) >= ' ') && (eventKey != Qt::Key_Backspace)))
        {
            QTextEdit::keyPressEvent(event);
            return;
        }
    }

    // The cursor keys will move the cursor to the beginning of the line
    // if either cursor or anchor are before the start of line
    if ((tc.position() < beginningOfRawline) || (tc.anchor() < beginningOfRawline))
    {
        int pos = tc.position();
        int anc = tc.anchor();
        if (anc > pos)
            pos = anc;
        if (pos < beginningOfRawline)
            pos = beginningOfRawline;
        if (event->matches(QKeySequence::MoveToNextChar) || event->matches(QKeySequence::MoveToNextLine) ||
            event->matches(QKeySequence::MoveToPreviousLine) || event->matches(QKeySequence::MoveToPreviousChar))
        {
            tc.setPosition(pos);
            setTextCursor(tc);
            return;
        }
    }

    // Select and Copy can be used with the cursor anywhere
    if (event->matches(QKeySequence::Copy) || event->matches(QKeySequence::SelectAll) ||
        event->matches(QKeySequence::SelectEndOfBlock) || event->matches(QKeySequence::SelectEndOfDocument) ||
        event->matches(QKeySequence::SelectEndOfLine) || event->matches(QKeySequence::SelectNextChar) ||
        event->matches(QKeySequence::SelectNextLine) || event->matches(QKeySequence::SelectPreviousChar) ||
        event->matches(QKeySequence::SelectPreviousLine) || event->matches(QKeySequence::SelectStartOfBlock) ||
        event->matches(QKeySequence::SelectStartOfDocument) || event->matches(QKeySequence::SelectStartOfLine))
    {
        QTextEdit::keyPressEvent(event);
        return;
    }

    // Send the raw line to the interpreter
    if (event->matches(QKeySequence::InsertLineSeparator) || event->matches(QKeySequence::InsertParagraphSeparator))
    {
        consoleMode = consoleModeNoWait;
        QString block = tc.block().text();
        QString line = block.right(block.size() - beginningOfRawlineInBlock);
        moveCursor(QTextCursor::End);
        textCursor().insertBlock();
        lineInputHistory.last() = line;
        emit sendRawlineSignal(line);
        return;
    }

    // All else is ignored
}

void Console::replaceLineWithHistoryIndex(int newIndex)
{
    // if the line that has been entered so far is defferent than
    // the line at the current index, save it at the last.
    QString block = document()->lastBlock().text();
    QString line = block.right(block.size() - beginningOfRawlineInBlock);
    QString historyLine = lineInputHistory[lineInputHistoryScrollingCurrentIndex];
    if (line != historyLine)
    {
        lineInputHistory.last() = line;
    }
    // Now replace the line with that at newIndex
    historyLine = lineInputHistory[newIndex];
    QTextCursor cursor = textCursor();
    cursor.setPosition(beginningOfRawline);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.insertText(historyLine);
    lineInputHistoryScrollingCurrentIndex = newIndex;
}

void Console::insertNextLineFromQueue()
{
    if (keyQueue.size() > 0)
    {
        int loc = keyQueue.indexOf("\n");
        if (loc < 0)
            loc = keyQueue.size();

        moveCursor(QTextCursor::End);
        textCursor().insertText(keyQueue.left(loc));
        keyQueue = keyQueue.right(keyQueue.size() - loc);
        moveCursor(QTextCursor::End);
        ensureCursorVisible();
        if ((keyQueue.size() > 0) && (keyQueue[0] == '\n'))
        {
            consoleMode = consoleModeNoWait;
            QString block = document()->lastBlock().text();
            QString line = block.right(block.size() - beginningOfRawlineInBlock);
            textCursor().insertBlock();
            keyQueue = keyQueue.right(keyQueue.size() - 1);
            emit sendRawlineSignal(line);
        }
    }
}

void Console::insertNextCharFromQueue()
{
    if (keyQueue.size() > 0)
    {
        consoleMode = consoleModeNoWait;
        QChar c = keyQueue[0];
        keyQueue = keyQueue.right(keyQueue.size() - 1);
        emit sendCharSignal(c);
    }
}

void Console::insertFromMimeData(const QMimeData *source)
{
    keyQueue += source->text();
}
