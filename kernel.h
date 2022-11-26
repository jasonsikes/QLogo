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

#include "help.h"
#include "procedurehelper.h"
#include "propertylists.h"
#include "vars.h"
#include "constants.h"

#include <QColor>
#include <QFile>
#include <QFont>
#include <QSet>
#include <QVector>

class Parser;
class QTextStream;

class Turtle;
class ProcedureScope;

class Kernel {
  friend class ProcedureScope;
  friend class StreamRedirect;
  Parser *parser;
  Vars variables;
  DatumP filePrefix;
  Help help;
  long repcount = -1;
  int pauseLevel = 0;
  int procedureIterationDepth = 0;
  bool isRunningMacroResult = false;
  bool isPausing = false;

  QVector<QColor> palette;
  PropertyLists plists;

  QHash<QString, QTextStream *> fileStreams;
  QSet<QTextStream *> writableStreams;
  QSet<QTextStream *> readableStreams;
  QTextStream *readStream;
  QTextStream *systemReadStream;
  QTextStream *writeStream;
  QTextStream *systemWriteStream;
  void lprint(QTextStream *stream, const QString &text);
  DatumP readRawLineWithPrompt(const QString prompt, QTextStream *stream);
  DatumP readChar();
  DatumP readlistWithPrompt(const QString &prompt, bool shouldRemoveComments,
                            QTextStream *stream);
  DatumP readWordWithPrompt(const QString prompt, QTextStream *stream);

  DatumP currentError;
  DatumP currentProcedure;
  DatumP currentLine;
  DatumP callingProcedure;
  DatumP callingLine;
  DatumP editFileName;
  QString workspaceText;

  ASTNode *astnodeValue(DatumP caller, DatumP value);
  bool numbersFromList(QVector<double> &retval, DatumP l);
  DatumP contentslistFromDatumP(DatumP sourceNode);
  void processContentsListWithMethod(
      DatumP contentsList, void (Workspace::*method)(const QString &aName));
  DatumP
  queryContentsListWithMethod(DatumP contentslist,
                              bool (Workspace::*method)(const QString &aName));
  void makeVarLocal(const QString &varname);
  DatumP executeProcedureCore(DatumP node);
  void inputProcedure(DatumP nodeP);

  bool colorFromDatumP(QColor &retval, DatumP colorP);
  long randomFromRange(long start, long end);

  QString filepathForFilename(DatumP filenameP);
  QTextStream *openFileStream(DatumP filenameP, QIODevice::OpenMode mode);
  QTextStream *createStringStream(DatumP filenameP, QIODevice::OpenMode mode);
  QTextStream *getStream(ProcedureHelper &h);
  QTextStream *open(ProcedureHelper &h, QIODevice::OpenMode openFlags);
  void close(const QString &filename);
  void closeAll();
  void editAndRunFile();
  void editAndRunWorkspaceText();

  void initPalette(void);

  /// Initialize LOGO system variables
  void initVariables(void);

  DatumP buildContentsList(showContents_t showWhat);
  QString createPrintoutFromContentsList(DatumP contentslist,
                                         bool shouldValidate = true);

  /// Check for interrupts and handle them accordingly.
  SignalsEnum_t interruptCheck();

public:
  Kernel();
  ~Kernel();
  bool getLineAndRunIt(bool shouldHandleError = true);
  QString executeText(const QString &text);
  void stdPrint(const QString &text);
  void sysPrint(const QString &text);
  DatumP registerError(DatumP anError, bool allowErract = false,
                       bool allowRecovery = false);
  DatumP pause();

  Turtle *turtle;
  bool isInputRedirected();
  void initLibrary();

  // DATA STRUCTURE PRIMITIVES
  // =========================

  // CONSTRUCTORS
  // ------------
  DatumP excWord(DatumP node);
  DatumP excList(DatumP node);
  DatumP excSentence(DatumP node);
  DatumP excFput(DatumP node);
  DatumP excLput(DatumP node);
  DatumP excArray(DatumP node);
  DatumP excListtoarray(DatumP node);
  DatumP excArraytolist(DatumP node);

  // SELECTORS
  // ---------
  DatumP excFirst(DatumP node);
  DatumP excButfirst(DatumP node);
  DatumP excFirsts(DatumP node);
  DatumP excButfirsts(DatumP node);
  DatumP excLast(DatumP node);
  DatumP excButlast(DatumP node);
  DatumP excItem(DatumP node);

  // MUTATORS
  // --------
  DatumP excSetitem(DatumP node);
  DatumP excDotSetfirst(DatumP node);
  DatumP excDotSetbf(DatumP node);
  DatumP excDotSetitem(DatumP node);

  // PREDICATES
  // ----------
  DatumP excWordp(DatumP node);
  DatumP excListp(DatumP node);
  DatumP excArrayp(DatumP node);
  DatumP excEmptyp(DatumP node);
  DatumP excBeforep(DatumP node);
  DatumP excDotEq(DatumP node);
  DatumP excMemberp(DatumP node);
  DatumP excSubstringp(DatumP node);
  DatumP excNumberp(DatumP node);
  DatumP excVbarredp(DatumP node);

  // QUERIES
  // -------
  DatumP excCount(DatumP node);
  DatumP excAscii(DatumP node);
  DatumP excRawascii(DatumP node);
  DatumP excChar(DatumP node);
  DatumP excMember(DatumP node);
  DatumP excLowercase(DatumP node);
  DatumP excUppercase(DatumP node);
  DatumP excStandout(DatumP node);
  DatumP excParse(DatumP node);
  DatumP excRunparse(DatumP node);
  DatumP excReadlist(DatumP node);
  DatumP excReadword(DatumP node);
  DatumP excReadrawline(DatumP node);
  DatumP excReadchar(DatumP node);
  DatumP excReadchars(DatumP node);
  DatumP excShell(DatumP node);

  DatumP excSetprefix(DatumP node);
  DatumP excPrefix(DatumP node);
  DatumP excOpenread(DatumP node);
  DatumP excOpenwrite(DatumP node);
  DatumP excOpenappend(DatumP node);
  DatumP excOpenupdate(DatumP node);
  DatumP excAllopen(DatumP node);
  DatumP excSetread(DatumP node);
  DatumP excSetwrite(DatumP node);
  DatumP excReader(DatumP node);
  DatumP excWriter(DatumP node);
  DatumP excReadpos(DatumP node);
  DatumP excWritepos(DatumP node);
  DatumP excSetreadpos(DatumP node);
  DatumP excSetwritepos(DatumP node);

  DatumP excTo(DatumP node);

  DatumP excEofp(DatumP node);
  DatumP excKeyp(DatumP node);
  DatumP excCleartext(DatumP node);
  DatumP excCursorInsert(DatumP node);
  DatumP excCursorOverwrite(DatumP node);
  DatumP excCursorMode(DatumP node);

  DatumP excClose(DatumP node);
  DatumP excCloseall(DatumP node);
  DatumP excErasefile(DatumP node);
  DatumP excDribble(DatumP node);
  DatumP excNodribble(DatumP node);

  DatumP runList(DatumP listP, const QString startTag = "");

  DatumP executeLiteral(DatumP node);
  DatumP executeValueOf(DatumP node);
  DatumP excMake(DatumP node);
  DatumP excSetfoo(DatumP node);
  DatumP excFoo(DatumP node);
  DatumP excPrint(DatumP node);
  DatumP excShow(DatumP node);
  DatumP excType(DatumP node);
  DatumP excRepeat(DatumP node);
  DatumP excSetcursor(DatumP node);
  DatumP excCursor(DatumP node);
  DatumP excSettextcolor(DatumP node);
  DatumP excIncreasefont(DatumP node);
  DatumP excDecreasefont(DatumP node);
  DatumP excSettextsize(DatumP node);
  DatumP excTextsize(DatumP node);
  DatumP excFont(DatumP node);
  DatumP excSetfont(DatumP node);
  DatumP excAllfonts(DatumP node);

  DatumP excEqualp(DatumP node);
  DatumP excNotequal(DatumP node);
  DatumP excLessp(DatumP node);
  DatumP excGreaterp(DatumP node);
  DatumP excGreaterequalp(DatumP node);
  DatumP excLessequalp(DatumP node);
  DatumP excSum(DatumP node);
  DatumP excDifference(DatumP node);
  DatumP excProduct(DatumP node);
  DatumP excQuotient(DatumP node);
  DatumP excRemainder(DatumP node);
  DatumP excModulo(DatumP node);
  DatumP excInt(DatumP node);
  DatumP excRound(DatumP node);
  DatumP excPower(DatumP node);
  DatumP excBitand(DatumP node);
  DatumP excBitor(DatumP node);
  DatumP excBitxor(DatumP node);
  DatumP excBitnot(DatumP node);
  DatumP excAshift(DatumP node);
  DatumP excLshift(DatumP node);
  DatumP excAnd(DatumP node);
  DatumP excOr(DatumP node);
  DatumP excNot(DatumP node);

  DatumP executeProcedure(DatumP node);
  DatumP executeMacro(DatumP node);
  DatumP excThing(DatumP node);
  DatumP excGlobal(DatumP node);

  DatumP excWait(DatumP node);

  DatumP excSqrt(DatumP node);
  DatumP excExp(DatumP node);
  DatumP excLog10(DatumP node);
  DatumP excLn(DatumP node);
  DatumP excSin(DatumP node);
  DatumP excRadsin(DatumP node);
  DatumP excCos(DatumP node);
  DatumP excRadcos(DatumP node);
  DatumP excArctan(DatumP node);
  DatumP excRadarctan(DatumP node);

  DatumP excForm(DatumP node);

  DatumP excRandom(DatumP node);
  DatumP excRerandom(DatumP node);

  DatumP excMinus(DatumP node);

  DatumP excForward(DatumP node);
  DatumP excBack(DatumP node);
  DatumP excRight(DatumP node);
  DatumP excLeft(DatumP node);
  DatumP excClearscreen(DatumP node);
  DatumP excClean(DatumP node);
  DatumP excPenup(DatumP node);
  DatumP excPendown(DatumP node);
  DatumP excPendownp(DatumP node);
  DatumP excShowturtle(DatumP node);
  DatumP excHideturtle(DatumP node);
  DatumP excHome(DatumP node);
  DatumP excSetXYZ(DatumP);
  DatumP excSetXY(DatumP);
  DatumP excSetpos(DatumP);
  DatumP excPos(DatumP node);
  DatumP excMatrix(DatumP node);
  DatumP excSetX(DatumP);
  DatumP excSetY(DatumP);
  DatumP excSetZ(DatumP);
  DatumP excHeading(DatumP node);
  DatumP excSetheading(DatumP node);
  DatumP excArc(DatumP node);
  DatumP excTowards(DatumP node);
  DatumP excScrunch(DatumP node);
  DatumP excSetscrunch(DatumP node);
  DatumP excLabel(DatumP node);
  DatumP excLabelheight(DatumP node);
  DatumP excSetlabelheight(DatumP node);
  DatumP excShownp(DatumP node);
  DatumP excSetpencolor(DatumP node);
  DatumP excPencolor(DatumP node);
  DatumP excSetpalette(DatumP node);
  DatumP excPalette(DatumP node);
  DatumP excBackground(DatumP node);
  DatumP excSetbackground(DatumP node);
  DatumP excSavepict(DatumP node);

  DatumP excMousepos(DatumP node);
  DatumP excClickpos(DatumP node);

  DatumP excBounds(DatumP node);
  DatumP excSetbounds(DatumP node);

  DatumP excPenpaint(DatumP node);
  DatumP excPenreverse(DatumP node);
  DatumP excPenerase(DatumP node);
  DatumP excPenmode(DatumP node);
  DatumP excSetpensize(DatumP node);
  DatumP excPensize(DatumP node);

  DatumP excWrap(DatumP node);
  DatumP excFence(DatumP node);
  DatumP excWindow(DatumP node);
  DatumP excTurtlemode(DatumP node);

  DatumP excTextscreen(DatumP node);
  DatumP excFullscreen(DatumP node);
  DatumP excSplitscreen(DatumP node);
  DatumP excScreenmode(DatumP node);

  DatumP excFilled(DatumP node);

  DatumP excButtonp(DatumP node);
  DatumP excButton(DatumP node);

  DatumP excDefine(DatumP node);
  DatumP excText(DatumP node);
  DatumP excFulltext(DatumP node);
  DatumP excCopydef(DatumP node);

  DatumP excLocal(DatumP node);

  DatumP excPprop(DatumP node);
  DatumP excGprop(DatumP node);
  DatumP excRemprop(DatumP node);
  DatumP excPlist(DatumP node);

  DatumP excProcedurep(DatumP node);
  DatumP excPrimitivep(DatumP node);
  DatumP excDefinedp(DatumP node);
  DatumP excNamep(DatumP node);
  DatumP excPlistp(DatumP node);

  DatumP excContents(DatumP node);
  DatumP excBuried(DatumP node);
  DatumP excTraced(DatumP node);
  DatumP excStepped(DatumP node);
  DatumP excProcedures(DatumP node);
  DatumP excPrimitives(DatumP node);
  DatumP excNames(DatumP node);
  DatumP excPlists(DatumP node);
  DatumP excArity(DatumP node);
  DatumP excNodes(DatumP node);

  DatumP excPrintout(DatumP node);
  DatumP excPot(DatumP node);

  DatumP excErase(DatumP node);
  DatumP excErall(DatumP node);
  DatumP excErps(DatumP node);
  DatumP excErns(DatumP node);
  DatumP excErpls(DatumP node);
  DatumP excBury(DatumP node);
  DatumP excUnbury(DatumP node);
  DatumP excBuriedp(DatumP node);
  DatumP excTrace(DatumP node);
  DatumP excUntrace(DatumP node);
  DatumP excTracedp(DatumP node);
  DatumP excStep(DatumP node);
  DatumP excUnstep(DatumP node);
  DatumP excSteppedp(DatumP node);
  DatumP excEdit(DatumP node);
  DatumP excEditfile(DatumP node);
  DatumP excSave(DatumP node);
  DatumP excLoad(DatumP node);
  DatumP excHelp(DatumP node);

  // CONTROL STRUCTURES

  DatumP excRun(DatumP node);
  DatumP excRunresult(DatumP node);
  DatumP excForever(DatumP node);
  DatumP excRepcount(DatumP node);
  DatumP excIf(DatumP node);
  DatumP excIfelse(DatumP node);
  DatumP excTest(DatumP node);
  DatumP excIftrue(DatumP node);
  DatumP excIffalse(DatumP node);
  DatumP excStop(DatumP node);
  DatumP excOutput(DatumP node);
  DatumP excCatch(DatumP node);
  DatumP excThrow(DatumP node);
  DatumP excError(DatumP node);
  DatumP excPause(DatumP node);
  DatumP excContinue(DatumP node);
  DatumP excBye(DatumP node);
  DatumP excDotMaybeoutput(DatumP node);
  DatumP excTag(DatumP);
  DatumP excGoto(DatumP node);
  DatumP excGotoToken(DatumP);

  // TEMPLATE-BASED ITERATION

  DatumP excApply(DatumP node);
  DatumP excNamedSlot(DatumP node); // '?'

  // MACROS

  DatumP excMacro(DatumP node);
  DatumP excMacrop(DatumP node);

  DatumP excNoop(DatumP node); // Some LOGO commands have no action in QLogo
  DatumP excErrorNoGui(DatumP node); // Some LOGO commands require a GUI which might not exist

  // SPECIAL VARIABLES

  bool varLOADNOISILY();
  bool varALLOWGETSET();
  DatumP varBUTTONACT();
  DatumP varKEYACT();
  bool varFULLPRINTP();
  int varPRINTDEPTHLIMIT();
  int varPRINTWIDTHLIMIT();
  DatumP varSTARTUP();
  bool varUNBURYONEDIT();
  bool varCASEIGNOREDP();
};

class ProcedureScope {
  Kernel *kernel;
  DatumP procedureHistory;
  DatumP lineHistory;

public:
  ProcedureScope(Kernel *exec, DatumP procname);
  ~ProcedureScope();
};

class StreamRedirect {
  QTextStream *originalWriteStream;
  QTextStream *originalSystemWriteStream;
  QTextStream *originalReadStream;
  QTextStream *originalSystemReadStream;
  Kernel *exec;

public:
  StreamRedirect(Kernel *srcExec, QTextStream *newReadStreamc,
                 QTextStream *newWriteStream);
  ~StreamRedirect();
};

#endif // EXECUTOR_H
