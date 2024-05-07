#ifndef RUNPARSER_H
#define RUNPARSER_H

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
#include <QHash>

class Procedures;
class Kernel;
class TextStream;

class Runparser {
  List *runparseRetval;
  QString::iterator runparseCIter;
  QString::iterator runparseCEnd;
  bool isRunparseSourceSpecial;
  void runparseSpecialchars(void);
  void runparseMinus(void);
  DatumPtr runparseNumber(void); // returns a number if successful
  void runparseQuotedWord();
  void runparseString();

  ListIterator listIter;

public:
  DatumPtr doRunparse(DatumPtr src);
};


/*
     * RUNPARSE wordorlist

        outputs the list that would result if the input word or list were
        entered as an instruction line; characters such as infix operators
        and parentheses are separate members of the output.  Note that
        sublists of a runparsed list are not themselves runparsed.
     */
DatumPtr runparse(DatumPtr src);

#endif // RUNPARSER_H
