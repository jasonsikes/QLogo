#ifndef CONSOLE_H
#define CONSOLE_H

//===-- qlogo/console.h - Console class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Console class, which is the
/// text portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include <QTextEdit>

class QMimeData;

class Console : public QTextEdit {
  Q_OBJECT

  enum ConsoleMode_t { inactiveMode, lineMode, charMode };

  ConsoleMode_t inputMode = inactiveMode;
  QString keyQueue;

  // Line input history traversing
  QStringList lineInputHistory;
  int lineInputHistoryScrollingCurrentIndex;
  void replaceLineWithHistoryIndex(int newIndex);

protected:
  int beginningOfLine = 0;
  bool keyQueueHasChars;
  void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
  void insertFromMimeData(const QMimeData *source) Q_DECL_OVERRIDE;

  void processLineModeKeyPressEvent(QKeyEvent *event);
  void processCharModeKeyPressEvent(QKeyEvent *event);

  void processModeKeyPressEvent(QKeyEvent *event);
  void dumpNextLineFromQueue();

  void returnLine(const QString line);

public:
  QTextCharFormat textFormat;
  Console(QWidget *parent = 0);
  ~Console();
  bool charsInQueue();
  void printString(const QString &text);
  void requestCharacter(void);
  void requestLineWithPrompt(const QString &prompt);
  void getCursorPos(int &row, int &col);
  void setTextSize(double pointSize);
  void setCursorPosition(QVector<int> position);
};

#endif // CONSOLE_H
