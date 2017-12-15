
//===-- qlogo/console.cpp - Console class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Console class, which is the
/// text portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include "console.h"
#include <QDebug>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QTextBlock>

#include CONTROLLER_HEADER

const QKeySequence::StandardKey toplevelKey = QKeySequence::Close;
const QKeySequence::StandardKey pauseKey = QKeySequence::Save;

Console::Console(QWidget *parent) : QTextEdit(parent) {
  keyQueueHasChars = false;
  textFormat.setForeground(QBrush(QWidget::palette().color(QPalette::Text)));
  textFormat.setBackground(QBrush(QWidget::palette().color(QPalette::Base)));
}

Console::~Console() {}

void Console::keyPressEvent(QKeyEvent *event) {
  QString text = event->text();

  switch (inputMode) {
  case lineMode:
    processLineModeKeyPressEvent(event);
    break;
  case charMode:
    processCharModeKeyPressEvent(event);
    break;
  case inactiveMode:
    if (event->matches(toplevelKey)) {
      mainController()->addEventToQueue(toplevelEvent);
    } else if (event->matches(pauseKey)) {
      mainController()->addEventToQueue(pauseEvent);
    } else if (text != "") {
      keyQueue.push_back(text);
      keyQueueHasChars = true;
      mainController()->addEventToQueue(characterEvent);
    }
  }
}

// TODO: the control functions should be broken out.
void Console::printString(const QString &text) {
  QTextCursor tc = textCursor();
  QStringList stringList = text.split(escapeChar);
  bool isEscaped = false;
  for (auto i = stringList.begin(); i != stringList.end(); ++i) {
    if (isEscaped) {
      switch ((*i)[0].toLatin1()) {
      case C_STANDOUT: {
        QBrush bg = textFormat.background();
        textFormat.setBackground(textFormat.foreground());
        textFormat.setForeground(bg);
        break;
      }
      case C_SET_TEXT_SIZE: {
        QStringRef sizeStringref = i->rightRef(i->size() - 1);
        double pointSize = sizeStringref.toDouble();
        if (pointSize > 0) {
          QFont f = textFormat.font();
          f.setPointSize(pointSize);
          textFormat.setFont(f);
        }
        break;
      }
      case C_SET_CURSOR_POS: {
        QStringRef rowcolStringref = i->rightRef(i->size() - 1);
        QVector<QStringRef> ary = rowcolStringref.split(C_DELIM);
        if (ary.size() == 2) {
          bool rowOK, colOK;
          int row = ary[0].toInt(&rowOK);
          int col = ary[1].toInt(&colOK);
          if (rowOK && colOK)
            moveCursorToPos(row, col);
        }
        break;
      }
      case C_SET_TEXT_COLOR: {
        QStringRef rowcolStringref = i->rightRef(i->size() - 1);
        QVector<QStringRef> ary = rowcolStringref.split(C_DELIM);
        if (ary.size() == 2) {
          textFormat.setForeground(QBrush(QColor(ary[0].toString())));
          textFormat.setBackground(QBrush(QColor(ary[1].toString())));
          QPalette p = palette();
          p.setBrush(QPalette::Base, QBrush(QColor(ary[1].toString())));
          setPalette(p);
        }
        break;
      }
      case C_CLEAR_TEXT: {
        QTextEdit::clear();
        break;
      }
      case C_SET_FONT: {
        QString fontNameString = i->right(i->size() - 1);
        QFont f = textFormat.font();
        f.setFamily(fontNameString);
        textFormat.setFont(f);
        break;
      }
      default:
        break;
      }
    } else {
      tc.setCharFormat(textFormat);
      tc.insertText(*i);
    }
    isEscaped = !isEscaped;
  }
  ensureCursorVisible();
}

void Console::requestCharacter(void) {
  if (keyQueue.size() > 0) {
    QString key = keyQueue.left(1);
    keyQueue = keyQueue.right(keyQueue.size() - 1);
    if (keyQueue.size() == 0)
      keyQueueHasChars = false;
    mainController()->receiveString(key);
  } else {
    inputMode = charMode;
  }
}

void Console::requestLineWithPrompt(const QString &prompt) {
  moveCursor(QTextCursor::End);
  printString(prompt);
  inputMode = lineMode;
  beginningOfLine = textCursor().position();
  lineInputHistory.push_back("");
  lineInputHistoryScrollingCurrentIndex = lineInputHistory.size() - 1;
  dumpNextLineFromQueue();
}

// void Console::checkCursor() {
//  QTextCursor tc = textCursor();
//  if (tc.position() < beginningOfLine) {
//    tc.setPosition(beginningOfLine);
//    setTextCursor(tc);
//  }
//}

void Console::moveCursorToPos(int row, int col) {
  int countOfRows = document()->blockCount();
  while (countOfRows <= row) {
    moveCursor(QTextCursor::End);
    textCursor().insertBlock();
    ++countOfRows;
  }

  QTextBlock line = document()->findBlockByNumber(row);
  int countOfCols = line.length();

  QTextCursor tc = textCursor();
  if (countOfCols <= col) {
    tc.setPosition(line.position());
    tc.movePosition(QTextCursor::EndOfBlock);
    QString fill = QString(col - countOfCols + 1, QChar(' '));
    tc.insertText(fill);
    tc.movePosition(QTextCursor::EndOfBlock);
  } else {
    tc.setPosition(line.position() + col);
  }
  setTextCursor(tc);
}

void Console::getCursorPos(int &row, int &col) {
  QTextCursor tc = textCursor();
  row = tc.blockNumber();
  col = tc.positionInBlock();
}

void Console::processCharModeKeyPressEvent(QKeyEvent *event) {
  // If it's a paste event, pass it through so we can get the text
  if (event->matches(QKeySequence::Paste)) {
    QTextEdit::keyPressEvent(event);
    if (keyQueue.size() > 0) {
      QString text = keyQueue.left(1);
      keyQueue = keyQueue.right(keyQueue.size() - 1);
      keyQueueHasChars = (keyQueue.size() > 0);
      inputMode = inactiveMode;
      mainController()->receiveString(text);
    }
  } else if (event->matches(toplevelKey)) {
    mainController()->receiveString(toplevelString);
  } else if (event->matches(pauseKey)) {
    mainController()->receiveString(pauseString);
  } else {
    QString text = event->text();
    if (text != "") {
      inputMode = inactiveMode;
      mainController()->receiveString(text);
    }
  }
}

void Console::dumpNextLineFromQueue() {
  if (keyQueue.size() > 0) {
    int loc = 0;
    while ((loc < keyQueue.size()) && (keyQueue[loc] != '\n')) {
      ++loc;
    }

    moveCursor(QTextCursor::End);
    textCursor().insertText(keyQueue.left(loc));
    keyQueue = keyQueue.right(keyQueue.size() - loc);
    moveCursor(QTextCursor::End);
    ensureCursorVisible();
    if ((keyQueue.size() > 0) && (keyQueue[0] == '\n')) {
      inputMode = inactiveMode;
      QString field = toPlainText();
      QString line = field.right(field.size() - beginningOfLine);
      textCursor().insertBlock();
      keyQueue = keyQueue.right(keyQueue.size() - 1);
      keyQueueHasChars = (keyQueue.size() > 0);
      returnLine(line);
    }
  }
}

void Console::returnLine(const QString line) {
  lineInputHistory.last() = line;
  mainController()->receiveString(line);
}

void Console::processLineModeKeyPressEvent(QKeyEvent *event) {
  int key = event->key();
  QTextCursor tc = textCursor();

  // These will only work if the cursor and anchor are
  // strictly after the prompt:
  if ((tc.position() > beginningOfLine) && (tc.anchor() > beginningOfLine) &&
      ((key == Qt::Key_Backspace) ||
       event->matches(QKeySequence::MoveToPreviousChar))) {
    QTextEdit::keyPressEvent(event);
    return;
  }

  // These will only work if the cursor and anchor are
  // on or after the prompt:
  if ((tc.position() >= beginningOfLine) && (tc.anchor() >= beginningOfLine)) {
    if (event->matches(QKeySequence::MoveToPreviousLine)) {
      if (lineInputHistoryScrollingCurrentIndex > 0) {
        replaceLineWithHistoryIndex(lineInputHistoryScrollingCurrentIndex - 1);
      }
      return;
    }
    if (event->matches(QKeySequence::MoveToNextLine)) {
      if (lineInputHistoryScrollingCurrentIndex < lineInputHistory.size() - 1) {
        replaceLineWithHistoryIndex(lineInputHistoryScrollingCurrentIndex + 1);
      }
      return;
    }
    if (event->matches(QKeySequence::Paste)) {
      QTextEdit::keyPressEvent(event);
      dumpNextLineFromQueue();
      return;
    }
    if (event->matches(QKeySequence::Cut) ||
        event->matches(QKeySequence::MoveToNextChar) ||
        ((event->text() != "") && (event->text()[0] >= ' '))) {
      QTextEdit::keyPressEvent(event);
      return;
    }
  }

  // The cursor keys will move the cursor to the beginning of the line
  // if either cursor or anchor are before the start of line
  if ((tc.position() < beginningOfLine) || (tc.anchor() < beginningOfLine)) {
    int pos = tc.position();
    int anc = tc.anchor();
    if (anc > pos)
      pos = anc;
    if (pos < beginningOfLine)
      pos = beginningOfLine;
    if (event->matches(QKeySequence::MoveToNextChar) ||
        event->matches(QKeySequence::MoveToNextLine) ||
        event->matches(QKeySequence::MoveToPreviousLine) ||
        event->matches(QKeySequence::MoveToPreviousChar)) {
      tc.setPosition(pos);
      setTextCursor(tc);
      return;
    }
  }

  // Select and Copy can be used with the cursor anywhere
  if (event->matches(QKeySequence::Copy) ||
      event->matches(QKeySequence::SelectAll) ||
      event->matches(QKeySequence::SelectEndOfBlock) ||
      event->matches(QKeySequence::SelectEndOfDocument) ||
      event->matches(QKeySequence::SelectEndOfLine) ||
      event->matches(QKeySequence::SelectNextChar) ||
      event->matches(QKeySequence::SelectNextLine) ||
      event->matches(QKeySequence::SelectPreviousChar) ||
      event->matches(QKeySequence::SelectPreviousLine) ||
      event->matches(QKeySequence::SelectStartOfBlock) ||
      event->matches(QKeySequence::SelectStartOfDocument) ||
      event->matches(QKeySequence::SelectStartOfLine)) {
    QTextEdit::keyPressEvent(event);
    return;
  }

  if (event->matches(toplevelKey)) {
    mainController()->receiveString(toplevelString);
    return;
  }
  if (event->matches(pauseKey)) {
    mainController()->receiveString(pauseString);
    return;
  }
  if (event->matches(QKeySequence::InsertLineSeparator) ||
      event->matches(QKeySequence::InsertParagraphSeparator)) {
    inputMode = inactiveMode;
    QString field = toPlainText();
    QString line = field.right(field.size() - beginningOfLine);
    moveCursor(QTextCursor::End);
    textCursor().insertBlock();
    returnLine(line);
    return;
  }

  // All else is ignored
}

void Console::replaceLineWithHistoryIndex(int newIndex) {
  // if the line that has been entered so far is defferent than
  // the line at the current index, save it at the last.
  QString field = toPlainText();
  QString line = field.right(field.size() - beginningOfLine);
  QString historyLine = lineInputHistory[lineInputHistoryScrollingCurrentIndex];
  if (line != historyLine) {
    lineInputHistory.last() = line;
  }
  // Now replace the line with that at newIndex
  historyLine = lineInputHistory[newIndex];
  QTextCursor cursor = textCursor();
  cursor.setPosition(beginningOfLine);
  cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
  cursor.removeSelectedText();
  cursor.insertText(historyLine);
  lineInputHistoryScrollingCurrentIndex = newIndex;
}

bool Console::charsInQueue() { return keyQueueHasChars; }

void Console::insertFromMimeData(const QMimeData *source) {
  keyQueue += source->text();
}
