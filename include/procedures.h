#ifndef PROCEDURES_H
#define PROCEDURES_H

//===-- qlogo/procedures.h - Procedures class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Procedures class, which is responsible
/// for organizing all procedures in QLogo: primitives, user-defined, and library.
///
//===----------------------------------------------------------------------===//

#include "datum_datump.h"
#include "datum_list.h"
#include "datum_iterator.h"
#include "workspace.h"
#include <QHash>

struct Cmd_t {
  KernelMethod method;
  int countOfMinParams;
  int countOfDefaultParams;
  int countOfMaxParams;
};

class Procedures : public Workspace {
  QHash<QString, Cmd_t> stringToCmd;

  QHash<QString, DatumPtr> procedures;
  qint64 lastProcedureCreatedTimestamp;

public:
  Procedures();

  qint64 timeOfLastProcedureCreation() { return lastProcedureCreatedTimestamp; }

  QList<DatumPtr> *astFromList(List *aList);

  DatumPtr createProcedure(DatumPtr cmd, DatumPtr text, DatumPtr sourceText);
  void defineProcedure(DatumPtr cmd, DatumPtr procnameP, DatumPtr text,
                       DatumPtr sourceText);

  void copyProcedure(DatumPtr newnameP, DatumPtr oldnameP);
  void eraseProcedure(DatumPtr procnameP);
  void eraseAllProcedures();

  DatumPtr astnodeFromCommand(DatumPtr command, int &minParams, int &defaultParams,
                              int &maxParams);

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
  int countOfMinParams;
  int countOfDefaultParams;
  int countOfMaxParams;
  QHash<const QString, DatumPtr> tagToLine;
  bool isMacro;
  DatumPtr sourceText;

  DatumPtr instructionList;
  DatumType isa() { return Datum::procedureType; }

  void init() {
    instructionList = nothing;
    countOfMaxParams = -1;
    countOfDefaultParams = 0;
    countOfMinParams = 0;
    requiredInputs.clear();
    optionalInputs.clear();
    optionalDefaults.clear();
    restInput = "";
    tagToLine.clear();
    isMacro = false;
    sourceText = nothing;
  }
};

Procedures* mainProcedures();


#endif // PROCEDURES_H
