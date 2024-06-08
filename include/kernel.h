#ifndef EXECUTOR_H
#define EXECUTOR_H

//===-- qlogo/kernel.h - Kernel class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Kernel class, which is the
/// executor proper of the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "datum/datump.h"

#include "procedurehelper.h"
#include "workspace/propertylists.h"
#include "workspace/vars.h"
#include "sharedconstants.h"
#include "controller/textstream.h"
#include "help.h"
#include "workspace/procedures.h"

#include <QColor>
#include <QFile>
#include <QFont>
#include <QSet>
#include <QVector>
#include <QRandomGenerator>

class Parser;

class Turtle;
class ProcedureScope;

class Kernel {
  friend class ProcedureScope;
  friend class StreamRedirect;
  Parser *parser;
  Procedures *procedures;
  Vars variables;
  DatumPtr filePrefix;
  int repcount = -1;
  int pauseLevel = 0;
  int procedureIterationDepth = 0;
  bool isRunningMacroResult = false;
  bool isPausing = false;

  Turtle *turtle;

  QVector<QColor> palette;
  PropertyLists plists;
  QRandomGenerator randomGenerator;

  Help help;

  QHash<QString, TextStream *> fileStreams;
  QSet<TextStream *> writableStreams;
  QSet<TextStream *> readableStreams;
  TextStream *readStream;
  TextStream *systemReadStream;
  TextStream *writeStream;
  TextStream *systemWriteStream;
  TextStream *stdioStream;

  DatumPtr currentError;
  DatumPtr currentProcedure;
  DatumPtr currentLine;
  DatumPtr callingProcedure;
  DatumPtr callingLine;
  DatumPtr editFileName;
  QString workspaceText;

  // Recursive searches need to make sure we don't get caught in infinite loops.
  // Remember what we searched so we don't search it again.
  QSet<void *> searchedContainers;

  ASTNode *astnodeValue(DatumPtr caller, DatumPtr value);
  bool numbersFromList(QVector<double> &retval, DatumPtr l);
  DatumPtr contentslistFromDatumPtr(DatumPtr sourceNode);
  void processContentsListWithMethod(
      DatumPtr contentsList, void (Workspace::*method)(const QString &aName));
  DatumPtr
  queryContentsListWithMethod(DatumPtr contentslist,
                              bool (Workspace::*method)(const QString &aName));
  void makeVarLocal(const QString &varname);
  DatumPtr executeProcedureCore(DatumPtr node);
  void inputProcedure(DatumPtr nodeP);

  bool colorFromDatumPtr(QColor &retval, DatumPtr colorP);

  QString filepathForFilename(DatumPtr filenameP);
  TextStream *openFileStream(DatumPtr filenameP, QIODevice::OpenMode mode);
  TextStream *createStringStream(DatumPtr filenameP, QIODevice::OpenMode mode);
  TextStream *getStream(ProcedureHelper &h);
  TextStream *open(ProcedureHelper &h, QIODevice::OpenMode openFlags);
  void close(const QString &filename);
  void closeAll();
  void editAndRunFile();
  void editAndRunWorkspaceText();

  void initPalette(void);

  /// Initialize LOGO system variables
  void initVariables(void);

  DatumPtr buildContentsList(showContents_t showWhat);
  QString createPrintoutFromContentsList(DatumPtr contentslist,
                                         bool shouldValidate = true);

  /// Check for interrupts and handle them accordingly.
  SignalsEnum_t interruptCheck();

  bool searchContainerForDatum(DatumPtr containerP, DatumPtr thingP, bool ignoreCase);

  // Return the butfirst of a word or list
  DatumPtr butfirst(DatumPtr srcValue);

public:
  Kernel();
  ~Kernel();
  bool getLineAndRunIt(bool shouldHandleError = true);
  QString executeText(const QString &text);
  void stdPrint(const QString &text);
  void sysPrint(const QString &text);
  DatumPtr registerError(DatumPtr anError, bool allowErract = false,
                       bool allowRecovery = false);
  DatumPtr pause();

  bool isInputRedirected();
  void initLibrary();
  DatumPtr runList(DatumPtr listP, const QString startTag = "");
  DatumPtr excGotoToken(DatumPtr);
  DatumPtr executeProcedure(DatumPtr node);
  DatumPtr executeMacro(DatumPtr node);
  DatumPtr executeLiteral(DatumPtr node);
  DatumPtr executeValueOf(DatumPtr node);
  DatumPtr excSetfoo(DatumPtr node);
  DatumPtr excFoo(DatumPtr node);


#include "primitives.h"
  

  DatumPtr excNoop(DatumPtr node); // Some LOGO commands have no action in QLogo
  DatumPtr excErrorNoGui(DatumPtr node); // Some LOGO commands require a GUI which might not exist

  // SPECIAL VARIABLES

  bool varLOADNOISILY();
  bool varALLOWGETSET();
  DatumPtr varBUTTONACT();
  DatumPtr varKEYACT();
  bool varFULLPRINTP();
  int varPRINTDEPTHLIMIT();
  int varPRINTWIDTHLIMIT();
  DatumPtr varSTARTUP();
  bool varUNBURYONEDIT();
  bool varCASEIGNOREDP();
};

class ProcedureScope {
  DatumPtr procedureHistory;
  DatumPtr lineHistory;

public:
  ProcedureScope(DatumPtr procname);
  ~ProcedureScope();
};

class StreamRedirect {
  TextStream *originalWriteStream;
  TextStream *originalSystemWriteStream;
  TextStream *originalReadStream;
  TextStream *originalSystemReadStream;

public:
  StreamRedirect(TextStream *newReadStreamc,
                 TextStream *newWriteStream);
  ~StreamRedirect();
};

Kernel* mainKernel();

#endif // EXECUTOR_H
