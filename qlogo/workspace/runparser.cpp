
//===-- runparser.cpp - Parser class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Runparser class, which is
/// responsible for parsing a List into a "runparsed" list.
///
//===----------------------------------------------------------------------===//

#include "runparser.h"
#include "QtCore/qiodevice.h"
#include "logocontroller.h"
#include "datum_word.h"
#include "textstream.h"
#include <qdatetime.h>
#include <qdebug.h>

// TODO: we could implement this into something a little faster.
const QString specialChars("+-()*%/<>=");


void Runparser::runparseSpecialchars(void) {
  QString retval = *runparseCIter;
  ++runparseCIter;
  if (runparseCIter != runparseCEnd) {
    QChar c = *runparseCIter;
    // there are some cases where special chars are combined
    if (((retval == "<") && (c == '=')) || ((retval == "<") && (c == '>')) ||
        ((retval == ">") && (c == '='))) {
      retval += c;
      ++runparseCIter;
    }
  }
  runparseRetval->append(DatumPtr(retval));
}

void Runparser::runparseString() {
  QString retval = "";

  if (*runparseCIter == '?') {
    retval = "?";
    ++runparseCIter;
    DatumPtr number = runparseNumber();
    if (number != nothing) {
      runparseRetval->append(DatumPtr(QString("(")));
      runparseRetval->append(DatumPtr(QString("?")));
      runparseRetval->append(number);
      runparseRetval->append(DatumPtr(QString(")")));
      return;
    }
  }

  while ((runparseCIter != runparseCEnd) &&
         (!specialChars.contains(*runparseCIter))) {
    retval += *runparseCIter;
    ++runparseCIter;
  }
  runparseRetval->append(DatumPtr(retval, isRunparseSourceSpecial));
}

void Runparser::runparseMinus() {
  QString::iterator nextCharIter = runparseCIter;
  ++nextCharIter;
  if (nextCharIter == runparseCEnd) {
    runparseSpecialchars();
    return;
  }

  DatumPtr number = runparseNumber();
  if (number != nothing) {
    runparseRetval->append(number);
    return;
  }

  // This is a minus function
  runparseRetval->append(DatumPtr(QString("0")));
  runparseRetval->append(DatumPtr(QString("--")));
  // discard the minus
  ++runparseCIter;
}

DatumPtr Runparser::runparseNumber() {
  if (runparseCIter == runparseCEnd)
    return nothing;
  QString::iterator iter = runparseCIter;
  QString result = "";
  bool hasDigit = false;
  QChar c = *iter;
  if (c == '-') {
    result = "-";
    ++iter;
  }

  if (iter == runparseCEnd)
    return nothing;
  c = *iter;
  while (c.isDigit()) {
    result += c;
    ++iter;
    if (iter == runparseCEnd)
      goto numberSuccessful;
    c = *iter;
    hasDigit = true;
  }
  if (c == '.') {
    result += c;
    ++iter;
    if ((iter == runparseCEnd) && hasDigit)
      goto numberSuccessful;
    c = *iter;
  }
  while (c.isDigit()) {
    result += c;
    ++iter;
    if (iter == runparseCEnd)
      goto numberSuccessful;
    c = *iter;
    hasDigit = true;
  }

  if (!hasDigit)
    return nothing;
  hasDigit = false;
  if ((c == 'e') || (c == 'E')) {
    result += c;
    ++iter;
    if (iter == runparseCEnd)
      return nothing;
    c = *iter;
  } else {
    goto numberSuccessful;
  }

  if ((c == '+') || (c == '-')) {
    result += c;
    ++iter;
    if (iter == runparseCEnd)
      return nothing;
    c = *iter;
  }
  while (c.isDigit()) {
    result += c;
    ++iter;
    hasDigit = true;
    if (iter == runparseCEnd)
      goto numberSuccessful;
    c = *iter;
  }

  if (!hasDigit)
    return nothing;

  // at this point we have a number. If there is anything else here then we
  // don't have a number
  if (!specialChars.contains(c))
    return nothing;

numberSuccessful:
  double value = result.toDouble();
  runparseCIter = iter;
  return DatumPtr(value);
}

void Runparser::runparseQuotedWord() {
  QString retval = "";
  while ((runparseCIter != runparseCEnd) && (*runparseCIter != '(') &&
         (*runparseCIter != ')')) {
    retval += *runparseCIter;
    ++runparseCIter;
  }
  runparseRetval->append(DatumPtr(retval, isRunparseSourceSpecial));
}


DatumPtr Runparser::doRunparse(DatumPtr src) {
  if (src.isWord()) {
    QString text = src.wordValue()->rawValue();
    QTextStream srcStream(&text, QIODevice::ReadOnly);
    TextStream stream(&srcStream);
    src = stream.readlistWithPrompt("", false);
  }
  runparseRetval = List::alloc();
  ListIterator iter = src.listValue()->newIterator();

  while (iter.elementExists()) {
    DatumPtr element = iter.element();
    if (element.isWord()) {
      QString oldWord = element.wordValue()->rawValue();
      isRunparseSourceSpecial = element.wordValue()->isForeverSpecial;

      runparseCIter = oldWord.begin();
      runparseCEnd = oldWord.end();
      while (runparseCIter != runparseCEnd) {
        QChar c = *runparseCIter;
        if (specialChars.contains(c)) {
          if ((c == '-') && (runparseCIter == oldWord.begin()) &&
              (oldWord != "-"))
            runparseMinus();
          else
            runparseSpecialchars();
          continue;
        }
        if (c == '"') {
          runparseQuotedWord();
          continue;
        }

        DatumPtr number = runparseNumber();
        if (number == nothing) {
          runparseString();
        } else {
          runparseRetval->append(number);
        }
      } // while (cIter != oldWord.end())
    } else {
      // The element is not a word so we'll just push back whatever it was
      runparseRetval->append(element);
    }
  }
  return DatumPtr(runparseRetval);
}


DatumPtr runparse(DatumPtr src) {
    Runparser rp;
    return rp.doRunparse(src);
}
