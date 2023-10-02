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

class Parser;
class Kernel;
class QTextStream;

struct Cmd_t {
  KernelMethod method;
  int countOfMinParams;
  int countOfDefaultParams;
  int countOfMaxParams;
};

typedef DatumPtr (Parser::*ParserMethod)(void);

class Parser : public Workspace {
  qint64 lastProcedureCreatedTimestamp;
  DatumPtr tokenizeListWithPrompt(const QString &prompt, int level, bool isArray,
                                bool shouldRemoveComments,
                                QTextStream *readStream);
  bool isReadingList = false;
  DatumPtr listSourceText;
  DatumPtr lastReadListSource();
  DatumPtr currentToken;
  Kernel *kernel;

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

  QHash<QString, DatumPtr> procedures;
  QHash<QString, Cmd_t> primitiveAlternateNames;

public:
  DatumPtr readrawlineWithPrompt(const QString &prompt, QTextStream *readStream);
  DatumPtr readwordWithPrompt(const QString &prompt, QTextStream *readStream);
  Parser(Kernel *aKernel);
  DatumPtr readlistWithPrompt(const QString &prompt, bool shouldRemoveComments,
                            QTextStream *readStream);
  DatumPtr runparse(DatumPtr src);
  QList<DatumPtr> *astFromList(List *aList);

  DatumPtr createProcedure(DatumPtr cmd, DatumPtr text, DatumPtr sourceText);
  void defineProcedure(DatumPtr cmd, DatumPtr procnameP, DatumPtr text,
                       DatumPtr sourceText);
  void inputProcedure(DatumPtr nodeP, QTextStream *readStream);
  void copyProcedure(DatumPtr newnameP, DatumPtr oldnameP);
  void eraseProcedure(DatumPtr procnameP);
  void eraseAllProcedures();

  DatumPtr procedureText(DatumPtr procnameP);
  DatumPtr procedureFulltext(DatumPtr procnameP, bool shouldValidate = true);
  QString procedureTitle(DatumPtr procnameP);

  bool isProcedure(QString procname);
  bool isMacro(QString procname);
  bool isPrimitive(QString procname);
  bool isDefined(QString procname);

  DatumPtr allProcedureNames(showContents_t showWhat);
  DatumPtr allPrimitiveProcedureNames();
  DatumPtr arity(DatumPtr nameP);

  DatumPtr astnodeWithLiterals(DatumPtr cmd, DatumPtr params);

  QString unreadDatum(DatumPtr aDatum, bool isInList = false);
  QString unreadList(List *aList, bool isInList = false);
  QString unreadWord(Word *aWord, bool isInList = false);
  QString unreadArray(Array *anArray);

  QString printoutDatum(DatumPtr aDatum);
};

class Procedure : public Datum {

  void addToPool();

public:
  Procedure() {}
  QStringList requiredInputs;
  QStringList optionalInputs;
  QList<DatumPtr> optionalDefaults;
  QString restInput;
  int defaultNumber;
  int countOfMinParams;
  int countOfMaxParams;
  QHash<const QString, DatumPtr> tagToLine;
  bool isMacro;
  DatumPtr sourceText;

  DatumPtr instructionList;
  DatumType isa() { return Datum::procedureType; }

  void init() {
    instructionList = nothing;
    countOfMaxParams = -1;
    countOfMinParams = 0;
    requiredInputs.clear();
    optionalInputs.clear();
    optionalDefaults.clear();
    restInput = "";
    defaultNumber = 0;
    tagToLine.clear();
    isMacro = false;
    sourceText = nothing;
  }
};



#endif // PARSER_H
