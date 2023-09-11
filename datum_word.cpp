
//===-- qlogo/datum_word.cpp - Word class implementation -------*-
// C++ -*-===//
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
/// This file contains the implementation of the Word class, which is the basic
/// unit of data in QLogo. A word may be a string or a number.
///
//===----------------------------------------------------------------------===//

#include "datum_word.h"
#include "datum_datump.h"
#include "stringconstants.h"
#include <qdebug.h>

QChar rawToChar(const QChar &src) {
  const ushort rawToAsciiMap[] = {
      2,  58, 3,  32, 4,  9,  5,  10, 6,  40,  11, 63,  14, 43, 15, 126,
      16, 41, 17, 91, 18, 93, 19, 45, 20, 42,  21, 47,  22, 61, 23, 60,
      24, 62, 25, 34, 26, 92, 28, 59, 29, 124, 30, 123, 31, 125};
  ushort v = src.unicode();
  if (v >= 32)
    return src;
  for (const ushort *i = rawToAsciiMap; *i <= v; i += 2) {
    if (*i == v) {
      return QChar(*(i + 1));
    }
  }
  return src;
}

void rawToChar(QString &src) {
  for (int i = 0; i < src.size(); ++i) {
    QChar s = src[i];
    QChar d = rawToChar(s);
    if (s != d)
      src[i] = d;
  }
}

QChar charToRaw(const QChar &src) {
  const ushort asciiToRawMap[] = {
      126, 15, 125, 31, 124, 29, 123, 30, 93, 18, 92, 26, 91, 17, 63, 11,
      62,  24, 61,  22, 60,  23, 59,  28, 58, 2,  47, 21, 45, 19, 43, 14,
      42,  20, 41,  16, 40,  6,  34,  25, 32, 3,  10, 5,  9,  4,  0,  0};
  ushort v = src.unicode();
  for (const ushort *i = asciiToRawMap; *i >= v; i += 2) {
    if (*i == v) {
      return QChar(*(i + 1));
    }
  }
  return src;
}

Word::Word() { dirtyFlag = stringIsDirty; }

Word::Word(const QString other, bool aIsForeverSpecial, bool canBeDestroyed) {
  dirtyFlag = stringIsDirty;
  isForeverSpecial = aIsForeverSpecial;
  isDestroyable = canBeDestroyed;

  rawString = other;
  printableString = rawString;
  for (int i = 0; i < printableString.size(); ++i) {
    QChar s = printableString[i];
    QChar d = rawToChar(s);
    if (s != d)
      printableString[i] = d;
  }
}

Word::Word(double other) {
  number = other;
  dirtyFlag = numberIsDirty;
}

Datum::DatumType Word::isa() { return wordType; }

QString Word::name() {
  return k.word();
}

Word::~Word() {}

QString Word::keyValue() {
  if (dirtyFlag == numberIsDirty)
    rawValue();
  if ((keyString.size() == 0) && (printableString.size() != 0)) {
    keyString = printableString;
    for (int i = 0; i < keyString.size(); ++i) {
      QChar s = keyString[i];
      QChar d = s.toUpper();
      if (s != d)
        keyString[i] = d;
    }
  }
  return keyString;
}

QString Word::rawValue() {
  if (dirtyFlag == numberIsDirty) {
    rawString.setNum(number);
    printableString = rawString;
    dirtyFlag = allClean;
  }
  return rawString;
}

double Word::numberValue() {
  if (dirtyFlag == stringIsDirty) {
    number = printableString.toDouble(&numberConversionSucceeded);
    if (numberConversionSucceeded)
      dirtyFlag = allClean;
  }
  return number;
}

bool Word::didNumberConversionSucceed() {
  return numberConversionSucceeded || (dirtyFlag != stringIsDirty);
}

QString Word::printValue(bool fullPrintp, int printDepthLimit,
                         int printWidthLimit) {
  if ((dirtyFlag == numberIsDirty) || (dirtyFlag == allClean))
    return rawValue();
  if (!fullPrintp && (printDepthLimit != 0) && (printWidthLimit < 0))
    return printableString;
  if (printDepthLimit == 0)
    return "...";
  QChar s, c;
  if (!fullPrintp) {
    if ((printWidthLimit >= 0) && (printWidthLimit <= 10))
      printWidthLimit = 10;
    if ((printWidthLimit >= 0) && (printableString.size() > printWidthLimit))
      return printableString.left(printWidthLimit) + "...";
    return printableString;
  }
  QString temp = rawString;
  QString retval;
  if (temp.size() == 0)
    return "||";
  bool shouldShowBars = false;
  for (int i = 0; i < temp.size(); ++i) {
    QChar t = temp[i];
    if (t < ' ') {
      shouldShowBars = true;
      break;
    }
  }
  for (int i = 0; i < temp.size(); ++i) {
    s = temp[i];
    if (shouldShowBars) {
      s = rawToChar(s);
    } else {
      c = charToRaw(s);
      if (s != c) {
        retval.append('\\');
      }
    }
    retval.append(s);
  }
  if (shouldShowBars) {
    retval.prepend('|');
    retval.append('|');
  }
  return retval;
}

QString Word::showValue(bool fullPrintp, int printDepthLimit,
                        int printWidthLimit) {
  return printValue(fullPrintp, printDepthLimit, printWidthLimit);
}

int Word::size() {
  rawValue();
  return rawString.size();
}

bool Word::isEqual(DatumP other, bool ignoreCase) {
  if (dirtyFlag != stringIsDirty) {
    bool answer = (number == other.wordValue()->numberValue());
    if (!other.wordValue()->didNumberConversionSucceed())
      return false;
    return answer;
  }
  if (other.wordValue()->dirtyFlag != stringIsDirty) {
    bool answer = (numberValue() == other.wordValue()->numberValue());
    if (!didNumberConversionSucceed())
      return false;
    return answer;
  }
  if (ignoreCase) {
    return rawValue().toUpper() == other.wordValue()->rawValue().toUpper();
  }
  return rawValue() == other.wordValue()->rawValue();
}

bool Word::isIndexInRange(int anIndex) {
  --anIndex;
  rawValue();
  return ((anIndex >= 0) && (anIndex < rawString.size()));
}

DatumP Word::datumAtIndex(int anIndex) {
  Q_ASSERT(isIndexInRange(anIndex));
  --anIndex;
  return DatumP(rawString.mid(anIndex, 1));
}

bool Word::containsDatum(DatumP aDatum, bool ignoreCase) {
  if (!aDatum.isWord())
    return false;
  rawValue();
  Qt::CaseSensitivity cs = ignoreCase ? Qt::CaseInsensitive : Qt::CaseSensitive;
  return rawString.contains(aDatum.wordValue()->rawValue(), cs);
}

bool Word::isMember(DatumP aDatum, bool ignoreCase) {
  return containsDatum(aDatum, ignoreCase);
}

DatumP Word::fromMember(DatumP aDatum, bool ignoreCase) {
  rawValue();
  Qt::CaseSensitivity cs = ignoreCase ? Qt::CaseInsensitive : Qt::CaseSensitive;
  const QString &searchString = aDatum.wordValue()->rawValue();
  int pos = rawString.indexOf(searchString, 0, cs);
  QString retval;
  if (pos >= 0) {
    retval = rawString.right(rawString.size() - pos);
  }
  return DatumP(retval);
}

DatumP Word::first() {
  rawValue();
  Q_ASSERT(rawString.size() > 0);
  return DatumP(QString(rawString[0]));
}

DatumP Word::last() {
  rawValue();
  Q_ASSERT(rawString.size() > 0);
  return DatumP(QString(rawString[rawString.size() - 1]));
}

DatumP Word::butlast() {
  const QString &w = rawValue();
  Word *retval = new Word(w.left(w.size() - 1));
  return DatumP(retval);
}

DatumP Word::butfirst() {
  rawValue();
  Q_ASSERT(rawString.size() > 0);
  return DatumP(rawString.right(rawString.size() - 1));
}


// TODO: move these for translation
Word trueWord("true", false, false);
Word falseWord("false", false, false);

