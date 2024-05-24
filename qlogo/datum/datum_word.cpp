
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

#include "datum/word.h"
#include "datum/datump.h"
#include <qdebug.h>
#include <QObject>

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

Word::Word() {
  number = nan("");
  rawString = QString();
  keyStringIsValid = false;
  printableStringIsValid = false;
}

Word::Word(const QString other, bool aIsForeverSpecial) {
  number = nan("");
  keyStringIsValid = false;
  printableStringIsValid = false;
  sourceIsNumber = false;
  isForeverSpecial = aIsForeverSpecial;

  rawString = other;
}

Word::Word(double other) {
  number = other;
  rawString = QString();
  keyStringIsValid = false;
  printableStringIsValid = false;
  sourceIsNumber = true;
}


void Word::genRawString()
{
  if ( rawString.isNull()) {
    Q_ASSERT( ! isnan(number));
    rawString.setNum(number);
  }
}



void Word::genPrintString()
{
  if ( ! printableStringIsValid) {
    genRawString();
    printableString = rawString;
    rawToChar(printableString);
    printableStringIsValid = true;
  }
}


void Word::genKeyString()
{
  if ( ! keyStringIsValid) {
    genPrintString();
    keyString = printableString;
    for (int i = 0; i < keyString.size(); ++i) {
      QChar s = printableString[i];
      QChar d = s.toUpper();
      if (s != d)
          keyString[i] = d;
    }
    keyStringIsValid = true;
  }
}

Datum::DatumType Word::isa() { return wordType; }

QString Word::keyValue() {
  genKeyString();
  return keyString;
}

QString Word::rawValue() {
  genRawString();
  return rawString;
}

double Word::numberValue() {
    if (isnan(number)) {
    bool numberIsValid;
    genPrintString();
    number = printableString.toDouble(&numberIsValid);
    if ( ! numberIsValid)
        number = nan("");
  }
  return number;
}

QString Word::printValue(bool fullPrintp, int printDepthLimit,
                         int printWidthLimit) {
  genPrintString();
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

  // TODO: should this be the same as "unread"?
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
  genPrintString();
  return printableString.size();
}

bool Word::isEqual(DatumPtr other, bool ignoreCase) {
    if (sourceIsNumber) {
        double otherNumber = other.wordValue()->numberValue();
        if (isnan(otherNumber))
            return false;
        return number == other.wordValue()->numberValue();
    }
    if (other.wordValue()->sourceIsNumber) {
        return (numberValue() == other.wordValue()->numberValue());
    }
    genPrintString();
    Qt::CaseSensitivity cs = ignoreCase ? Qt::CaseInsensitive : Qt::CaseSensitive;
    return printableString.compare(other.wordValue()->printValue(), cs) == 0;
}

bool Word::isIndexInRange(int anIndex) {
  --anIndex;
  genPrintString();
  return ((anIndex >= 0) && (anIndex < printableString.size()));
}

bool Word::containsDatum(DatumPtr aDatum, bool ignoreCase) {
  if (!aDatum.isWord())
    return false;
  genPrintString();
  Qt::CaseSensitivity cs = ignoreCase ? Qt::CaseInsensitive : Qt::CaseSensitive;
  return printableString.contains(aDatum.wordValue()->printValue(), cs);
}

bool Word::isMember(DatumPtr aDatum, bool ignoreCase) {
  return containsDatum(aDatum, ignoreCase);
}

DatumPtr Word::fromMember(DatumPtr aDatum, bool ignoreCase) {
  genPrintString();
  Qt::CaseSensitivity cs = ignoreCase ? Qt::CaseInsensitive : Qt::CaseSensitive;
  const QString &searchString = aDatum.wordValue()->printValue();
  int pos = printableString.indexOf(searchString, 0, cs);
  QString retval;
  if (pos >= 0) {
    retval = printableString.right(rawString.size() - pos);
  }
  return DatumPtr(retval);
}

DatumPtr Word::first() {
  genPrintString();
  Q_ASSERT(printableString.size() > 0);
  return DatumPtr(QString(printableString[0]));
}

DatumPtr Word::last() {
  genPrintString();
  Q_ASSERT(printableString.size() > 0);
  return DatumPtr(QString(printableString[printableString.size() - 1]));
}

DatumPtr Word::butfirst() {
  genPrintString();
  Q_ASSERT(printableString.size() > 0);
  return DatumPtr(printableString.right(printableString.size() - 1));
}

