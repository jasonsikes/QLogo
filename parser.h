#ifndef PARSER_H
#define PARSER_H

//===-- qlogo/parser.h - Parser class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Parser class, which is responsible
/// for parsing text and keeping user-defined functions.
///
//===----------------------------------------------------------------------===//

#include "datum.h"
//#include "vars.h"
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

typedef DatumP (Parser::*ParserMethod)(void);

class Parser : public Workspace {
  qint64 lastProcedureCreatedTimestamp;
  DatumP tokenizeListWithPrompt(const QString &prompt, int level, bool isArray,
                                bool shouldRemoveComments,
                                QTextStream *readStream);
  bool isReadingList = false;
  DatumP listSourceText;
  DatumP lastReadListSource();
  DatumP currentToken;
  Kernel *kernel;

  // For runparse and it's supporting methods:
  List *runparseRetval;
  QString::iterator runparseCIter;
  QString::iterator runparseCEnd;
  bool isRunparseSourceSpecial;
  void runparseSpecialchars(void);
  void runparseMinus(void);
  DatumP runparseNumber(void); // returns a number if successful
  void runparseQuotedWord();
  void runparseString();

  void advanceToken();
  ListIterator listIter;

  DatumP parseExp();
  DatumP parseSumexp();
  DatumP parseMulexp();
  DatumP parseminusexp();
  DatumP parseTermexp();
  DatumP parseCommand(bool isVararg);
  DatumP astnodeFromCommand(DatumP command, int &minParams, int &defaultParams,
                            int &maxParams);

  QHash<QString, DatumP> procedures;
  QHash<QString, Cmd_t> primitiveAlternateNames;

public:
  DatumP readrawlineWithPrompt(const QString &prompt, QTextStream *readStream);
  DatumP readwordWithPrompt(const QString &prompt, QTextStream *readStream);
  Parser(Kernel *aKernel);
  DatumP readlistWithPrompt(const QString &prompt, bool shouldRemoveComments,
                            QTextStream *readStream);
  DatumP runparse(DatumP src);
  QList<DatumP> *astFromList(List *aList);

  DatumP createProcedure(DatumP cmd, DatumP text, DatumP sourceText);
  void defineProcedure(DatumP cmd, DatumP procnameP, DatumP text,
                       DatumP sourceText);
  void inputProcedure(DatumP nodeP, QTextStream *readStream);
  void copyProcedure(DatumP newnameP, DatumP oldnameP);
  void eraseProcedure(DatumP procnameP);
  void eraseAllProcedures();

  DatumP procedureText(DatumP procnameP);
  DatumP procedureFulltext(DatumP procnameP, bool shouldValidate = true);
  QString procedureTitle(DatumP procnameP);

  bool isProcedure(QString procname);
  bool isMacro(QString procname);
  bool isPrimitive(QString procname);
  bool isDefined(QString procname);

  DatumP allProcedureNames(showContents_t showWhat);
  DatumP allPrimitiveProcedureNames();
  DatumP arity(DatumP nameP);

  DatumP astnodeWithLiterals(DatumP cmd, DatumP params);

  QString unreadDatum(DatumP aDatum, bool isInList = false);
  QString unreadList(List *aList, bool isInList = false);
  QString unreadWord(Word *aWord, bool isInList = false);
  QString unreadArray(Array *anArray);

  QString printoutDatum(DatumP aDatum);
};

class Procedure : public Datum {

public:
  QStringList requiredInputs;
  QStringList optionalInputs;
  QList<DatumP> optionalDefaults;
  QString restInput;
  int defaultNumber;
  int countOfMinParams;
  int countOfMaxParams;
  QHash<const QString, DatumP> tagToLine;
  bool isMacro;
  DatumP sourceText;

  DatumP instructionList;
  DatumType isa() { return Datum::procedureType; }

  Procedure() {
    instructionList = DatumP(new List);
    countOfMaxParams = -1;
    countOfMinParams = 0;
  }
};



#endif // PARSER_H
