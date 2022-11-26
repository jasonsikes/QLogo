#ifndef CONSOLE_H
#define CONSOLE_H

//===-- qlogo/console.h - Console class definition -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
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


class Console : public QTextEdit {
  Q_OBJECT

protected:

    // Key press events
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void processLineModeKeyPressEvent(QKeyEvent *event);
    void processCharModeKeyPressEvent(QKeyEvent *);
    void processNoWaitKeyPressEvent(QKeyEvent *event);

    enum consoleMode_t {
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
    void replaceLineWithHistoryIndex(int newIndex);

    // Keypress and paste buffers
    QString keyQueue;
    void insertNextLineFromQueue();
    void insertNextCharFromQueue();
    void insertFromMimeData(const QMimeData *source) Q_DECL_OVERRIDE;

    void writeTextFragment(const QString text);

    QTextCharFormat textFormat;

public:
  Console(QWidget *parent = 0);
  ~Console();

  void printString(const QString text);
  void requestRawlineWithPrompt(const QString prompt);
  void requestChar();

  void getCursorPos(int &row, int &col);
  void setTextCursorPosition(int row, int col);
  void setTextFontName(const QString aName);
  void setTextFontSize(double aSize);
  const QTextCharFormat getFont() { return textFormat;}

signals:
  void sendRawlineSignal(const QString &rawLine);
  void sendCharSignal(QChar c);
};

#endif // CONSOLE_H
