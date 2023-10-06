
//===-- qlogo/parser.cpp - Parser class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Reader class, which is responsible
/// for reading text.
///
//===----------------------------------------------------------------------===//

#include "reader.h"
#include "logocontroller.h"
#include "datum_word.h"



const QString specialChars("+-()*%/<>=");

char lastNonSpaceChar(const QString &line) {
  char retval = ' ';
  for (int i = line.length() - 1; i >= 0; --i) {
    retval = line[i].toLatin1();
    if (retval != ' ')
      break;
  }
  return retval;
}


Reader::Reader(QTextStream *aReadStream)
{
  readStream = aReadStream;
}


QString Reader::readrawlineWithPrompt(const QString &prompt)
{
  DatumPtr retval;
  if (readStream == NULL) {
    retval = mainController()->readRawlineWithPrompt(prompt);
  } else {
    if (readStream->atEnd()) {
      return nothing;
    }
    QString str = readStream->readLine();
    if (readStream->status() != QTextStream::Ok)
      Error::fileSystem();
    retval = DatumPtr(str);
  }
  return retval;
}


DatumPtr Reader::readwordWithPrompt(const QString &prompt) {
  QString retval = "";
  bool isVbarred = false;
  bool isEscaped = false;

  DatumPtr line = readrawlineWithPrompt(prompt);
  if (line == nothing)
    return nothing;

  forever {
    if (line == nothing)
      return DatumPtr(retval);

    const QString &t = line.wordValue()->rawValue();
    for (auto c : t) {
      if (isEscaped) {
          isEscaped = false;
          retval.push_back(charToRaw(c));
          continue;
      }
      if (c == '|') {
          isVbarred = !isVbarred;
      }
      if (c == '\\') {
          isEscaped = true;
          continue;
      }

      retval.push_back(c);
    } // for (auto c : t)
    // The end of the line
    if (isEscaped) {
      isEscaped = false;
      retval.push_back('\n');
      line = readrawlineWithPrompt("\\ ");
      continue;
    }
    if (isVbarred) {
      retval.push_back(charToRaw('\n'));
      line = readrawlineWithPrompt("| ");
      continue;
    }
    if (lastNonSpaceChar(t) == '~') {
      retval.push_back('\n');
      line = readrawlineWithPrompt("~ ");
      continue;
    }

    // If (after all the work) the string we generated is the same as the rawline
    // we started with, return the original rawline.
    if (line.wordValue()->rawValue() == retval)
      return line;
    return DatumPtr(retval);

  }; // forever
}
