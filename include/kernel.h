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

#include "datum.h"

#include "procedurehelper.h"
#include "workspace/propertylists.h"
#include "sharedconstants.h"
#include "controller/textstream.h"
#include "help.h"
#include "workspace/procedures.h"
#include "workspace/callframe.h"

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
  friend class ProcedureHelper;
  friend class StreamRedirect;
  Parser *parser;
  Procedures *procedures;
  DatumPtr filePrefix;
  int repcount = -1;
  int pauseLevel = 0;
  bool isPausing = false;

  Turtle *turtle;

  QVector<QColor> palette;
  PropertyLists plists;
  QRandomGenerator randomGenerator;

  Help help;

  CallFrameStack callStack;

  QHash<QString, TextStream *> fileStreams;
  QSet<TextStream *> writableStreams;
  QSet<TextStream *> readableStreams;
  TextStream *readStream;
  TextStream *systemReadStream;
  TextStream *writeStream;
  TextStream *systemWriteStream;
  TextStream *stdioStream;

  DatumPtr currentError;
  DatumPtr currentLine;
  DatumPtr callingLine;
  DatumPtr editFileName;
  QString workspaceText;

  // Recursive searches need to make sure we don't get caught in infinite loops.
  // Remember what we searched so we don't search it again.
  QSet<void *> searchedContainers;
  QSet<void *>comparedContainers;


  ASTNode *astnodeValue(DatumPtr caller, DatumPtr value);
  bool numbersFromList(QVector<double> &retval, DatumPtr l);
  DatumPtr contentslistFromDatumPtr(DatumPtr sourceNode);
  void processContentsListWithMethod(DatumPtr contentsList,
                                     void (Workspace::*method)(const QString &aName));
  DatumPtr queryContentsListWithMethod(DatumPtr contentslist,
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

  // Compare two datums, return true iff equal.
  bool areDatumsEqual(DatumPtr datumP1, DatumPtr datumP2, bool ignoreCase);

  // Return the butfirst of a word or list
  DatumPtr butfirst(DatumPtr srcValue);

  // Determine if the given list contains at least as many items as the
  // integer given.
  bool doesListHaveCountOrMore(List *list, int count);

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
  DatumPtr runList(DatumPtr listP, QString startTag = QString());
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

class StreamRedirect {
  TextStream *originalWriteStream;
  TextStream *originalSystemWriteStream;
  TextStream *originalReadStream;
  TextStream *originalSystemReadStream;

  Parser *originalParser;

public:
  StreamRedirect(TextStream *newReadStreamc,
                 TextStream *newWriteStream,
                   Parser *newParser);
  ~StreamRedirect();
};

#endif // EXECUTOR_H
