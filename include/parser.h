#ifndef PARSER_H
#define PARSER_H

//===-- qlogo/parser.h - Parser class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Parser class, which is responsible
/// for parsing text and keeping user-defined functions.
///
//===----------------------------------------------------------------------===//

#include "datum_datump.h"
#include "datum_list.h"
#include "datum_iterator.h"
#include "workspace.h"
#include <QHash>

class Procedures;
class Kernel;
class TextStream;

class Parser {
  DatumPtr currentToken;

  // For runparse and it's supporting methods:
  List *runparseRetval;
  QString::iterator runparseCIter;
  QString::iterator runparseCEnd;
  bool isRunparseSourceSpecial;
  void runparseSpecialchars(void);
  void runparseMinus(void);
  DatumPtr runparseNumber(void); // returns a number if successful
  void runparseQuotedWord();
  void runparseString();

  void advanceToken();
  ListIterator listIter;

  DatumPtr parseExp();
  DatumPtr parseSumexp();
  DatumPtr parseMulexp();
  DatumPtr parseminusexp();
  DatumPtr parseTermexp();
  DatumPtr parseCommand(bool isVararg);
  DatumPtr parseStopIfExists(DatumPtr command);
  DatumPtr astnodeFromCommand(DatumPtr command, int &minParams, int &defaultParams,
                            int &maxParams);

public:
  DatumPtr runparse(DatumPtr src);
  QList<DatumPtr> *astFromList(List *aList);

  void inputProcedure(DatumPtr nodeP, TextStream *readStream);
};



#endif // PARSER_H
