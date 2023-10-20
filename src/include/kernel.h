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

#include "datum_datump.h"

#include "procedurehelper.h"
#include "propertylists.h"
#include "vars.h"
#include "sharedconstants.h"
#include "textstream.h"
#include "help.h"

#include <QColor>
#include <QFile>
#include <QFont>
#include <QSet>
#include <QVector>

class Parser;

class Turtle;
class ProcedureScope;

class Kernel {
  friend class ProcedureScope;
  friend class StreamRedirect;
  Parser *parser;
  Vars variables;
  DatumPtr filePrefix;
  int repcount = -1;
  int pauseLevel = 0;
  int procedureIterationDepth = 0;
  bool isRunningMacroResult = false;
  bool isPausing = false;

  QVector<QColor> palette;
  PropertyLists plists;

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
  uint32_t randomFromRange(uint32_t start, uint32_t end);

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

  Turtle *turtle;
  bool isInputRedirected();
  void initLibrary();

  // DATA STRUCTURE PRIMITIVES
  // =========================

  // CONSTRUCTORS
  // ------------
  DatumPtr excWord(DatumPtr node);
  DatumPtr excList(DatumPtr node);
  DatumPtr excSentence(DatumPtr node);
  DatumPtr excFput(DatumPtr node);
  DatumPtr excLput(DatumPtr node);
  DatumPtr excArray(DatumPtr node);
  DatumPtr excListtoarray(DatumPtr node);
  DatumPtr excArraytolist(DatumPtr node);

  // SELECTORS
  // ---------
  DatumPtr excFirst(DatumPtr node);
  DatumPtr excButfirst(DatumPtr node);
  DatumPtr excFirsts(DatumPtr node);
  DatumPtr excButfirsts(DatumPtr node);
  DatumPtr excLast(DatumPtr node);
  DatumPtr excButlast(DatumPtr node);
  DatumPtr excItem(DatumPtr node);

  // MUTATORS
  // --------
  DatumPtr excSetitem(DatumPtr node);
  DatumPtr excDotSetfirst(DatumPtr node);
  DatumPtr excDotSetbf(DatumPtr node);
  DatumPtr excDotSetitem(DatumPtr node);

  // PREDICATES
  // ----------
  DatumPtr excWordp(DatumPtr node);
  DatumPtr excListp(DatumPtr node);
  DatumPtr excArrayp(DatumPtr node);
  DatumPtr excEmptyp(DatumPtr node);
  DatumPtr excBeforep(DatumPtr node);
  DatumPtr excDotEq(DatumPtr node);
  DatumPtr excMemberp(DatumPtr node);
  DatumPtr excSubstringp(DatumPtr node);
  DatumPtr excNumberp(DatumPtr node);
  DatumPtr excVbarredp(DatumPtr node);

  // QUERIES
  // -------
  DatumPtr excCount(DatumPtr node);
  DatumPtr excAscii(DatumPtr node);
  DatumPtr excRawascii(DatumPtr node);
  DatumPtr excChar(DatumPtr node);
  DatumPtr excMember(DatumPtr node);
  DatumPtr excLowercase(DatumPtr node);
  DatumPtr excUppercase(DatumPtr node);
  DatumPtr excStandout(DatumPtr node);
  DatumPtr excParse(DatumPtr node);
  DatumPtr excRunparse(DatumPtr node);
  DatumPtr excReadlist(DatumPtr node);
  DatumPtr excReadword(DatumPtr node);
  DatumPtr excReadrawline(DatumPtr node);
  DatumPtr excReadchar(DatumPtr node);
  DatumPtr excReadchars(DatumPtr node);
  DatumPtr excShell(DatumPtr node);

  DatumPtr excSetprefix(DatumPtr node);
  DatumPtr excPrefix(DatumPtr node);
  DatumPtr excOpenread(DatumPtr node);
  DatumPtr excOpenwrite(DatumPtr node);
  DatumPtr excOpenappend(DatumPtr node);
  DatumPtr excOpenupdate(DatumPtr node);
  DatumPtr excAllopen(DatumPtr node);
  DatumPtr excSetread(DatumPtr node);
  DatumPtr excSetwrite(DatumPtr node);
  DatumPtr excReader(DatumPtr node);
  DatumPtr excWriter(DatumPtr node);
  DatumPtr excReadpos(DatumPtr node);
  DatumPtr excWritepos(DatumPtr node);
  DatumPtr excSetreadpos(DatumPtr node);
  DatumPtr excSetwritepos(DatumPtr node);

  DatumPtr excTo(DatumPtr node);

  DatumPtr excEofp(DatumPtr node);
  DatumPtr excKeyp(DatumPtr node);
  DatumPtr excCleartext(DatumPtr node);
  DatumPtr excCursorInsert(DatumPtr node);
  DatumPtr excCursorOverwrite(DatumPtr node);
  DatumPtr excCursorMode(DatumPtr node);

  DatumPtr excClose(DatumPtr node);
  DatumPtr excCloseall(DatumPtr node);
  DatumPtr excErasefile(DatumPtr node);
  DatumPtr excDribble(DatumPtr node);
  DatumPtr excNodribble(DatumPtr node);

  DatumPtr runList(DatumPtr listP, const QString startTag = "");

  DatumPtr executeLiteral(DatumPtr node);
  DatumPtr executeValueOf(DatumPtr node);
  DatumPtr excMake(DatumPtr node);
  DatumPtr excSetfoo(DatumPtr node);
  DatumPtr excFoo(DatumPtr node);
  DatumPtr excPrint(DatumPtr node);
  DatumPtr excShow(DatumPtr node);
  DatumPtr excType(DatumPtr node);
  DatumPtr excRepeat(DatumPtr node);
  DatumPtr excSetcursor(DatumPtr node);
  DatumPtr excCursor(DatumPtr node);
  DatumPtr excSettextcolor(DatumPtr node);
  DatumPtr excIncreasefont(DatumPtr node);
  DatumPtr excDecreasefont(DatumPtr node);
  DatumPtr excSettextsize(DatumPtr node);
  DatumPtr excTextsize(DatumPtr node);
  DatumPtr excFont(DatumPtr node);
  DatumPtr excSetfont(DatumPtr node);
  DatumPtr excAllfonts(DatumPtr node);

  DatumPtr excEqualp(DatumPtr node);
  DatumPtr excNotequal(DatumPtr node);
  DatumPtr excLessp(DatumPtr node);
  DatumPtr excGreaterp(DatumPtr node);
  DatumPtr excGreaterequalp(DatumPtr node);
  DatumPtr excLessequalp(DatumPtr node);
  DatumPtr excSum(DatumPtr node);
  DatumPtr excDifference(DatumPtr node);
  DatumPtr excProduct(DatumPtr node);
  DatumPtr excQuotient(DatumPtr node);
  DatumPtr excRemainder(DatumPtr node);
  DatumPtr excModulo(DatumPtr node);
  DatumPtr excInt(DatumPtr node);
  DatumPtr excRound(DatumPtr node);
  DatumPtr excPower(DatumPtr node);
  DatumPtr excBitand(DatumPtr node);
  DatumPtr excBitor(DatumPtr node);
  DatumPtr excBitxor(DatumPtr node);
  DatumPtr excBitnot(DatumPtr node);
  DatumPtr excAshift(DatumPtr node);
  DatumPtr excLshift(DatumPtr node);
  DatumPtr excAnd(DatumPtr node);
  DatumPtr excOr(DatumPtr node);
  DatumPtr excNot(DatumPtr node);

  DatumPtr executeProcedure(DatumPtr node);
  DatumPtr executeMacro(DatumPtr node);
  DatumPtr excThing(DatumPtr node);
  DatumPtr excGlobal(DatumPtr node);

  DatumPtr excWait(DatumPtr node);

  DatumPtr excSqrt(DatumPtr node);
  DatumPtr excExp(DatumPtr node);
  DatumPtr excLog10(DatumPtr node);
  DatumPtr excLn(DatumPtr node);
  DatumPtr excSin(DatumPtr node);
  DatumPtr excRadsin(DatumPtr node);
  DatumPtr excCos(DatumPtr node);
  DatumPtr excRadcos(DatumPtr node);
  DatumPtr excArctan(DatumPtr node);
  DatumPtr excRadarctan(DatumPtr node);

  DatumPtr excForm(DatumPtr node);

  DatumPtr excRandom(DatumPtr node);

  DatumPtr excMinus(DatumPtr node);

  DatumPtr excForward(DatumPtr node);
  DatumPtr excBack(DatumPtr node);
  DatumPtr excRight(DatumPtr node);
  DatumPtr excLeft(DatumPtr node);
  DatumPtr excClearscreen(DatumPtr node);
  DatumPtr excClean(DatumPtr node);
  DatumPtr excPenup(DatumPtr node);
  DatumPtr excPendown(DatumPtr node);
  DatumPtr excPendownp(DatumPtr node);
  DatumPtr excShowturtle(DatumPtr node);
  DatumPtr excHideturtle(DatumPtr node);
  DatumPtr excHome(DatumPtr node);
  DatumPtr excSetXYZ(DatumPtr);
  DatumPtr excSetXY(DatumPtr);
  DatumPtr excSetpos(DatumPtr);
  DatumPtr excPos(DatumPtr node);
  DatumPtr excMatrix(DatumPtr node);
  DatumPtr excSetX(DatumPtr);
  DatumPtr excSetY(DatumPtr);
  DatumPtr excSetZ(DatumPtr);
  DatumPtr excHeading(DatumPtr node);
  DatumPtr excSetheading(DatumPtr node);
  DatumPtr excArc(DatumPtr node);
  DatumPtr excTowards(DatumPtr node);
  DatumPtr excScrunch(DatumPtr node);
  DatumPtr excSetscrunch(DatumPtr node);
  DatumPtr excLabel(DatumPtr node);
  DatumPtr excLabelheight(DatumPtr node);
  DatumPtr excSetlabelheight(DatumPtr node);
  DatumPtr excShownp(DatumPtr node);
  DatumPtr excSetpencolor(DatumPtr node);
  DatumPtr excPencolor(DatumPtr node);
  DatumPtr excSetpalette(DatumPtr node);
  DatumPtr excPalette(DatumPtr node);
  DatumPtr excBackground(DatumPtr node);
  DatumPtr excSetbackground(DatumPtr node);
  DatumPtr excSavepict(DatumPtr node);

  DatumPtr excMousepos(DatumPtr node);
  DatumPtr excClickpos(DatumPtr node);

  DatumPtr excBounds(DatumPtr node);
  DatumPtr excSetbounds(DatumPtr node);

  DatumPtr excPenpaint(DatumPtr node);
  DatumPtr excPenreverse(DatumPtr node);
  DatumPtr excPenerase(DatumPtr node);
  DatumPtr excPenmode(DatumPtr node);
  DatumPtr excSetpensize(DatumPtr node);
  DatumPtr excPensize(DatumPtr node);

  DatumPtr excWrap(DatumPtr node);
  DatumPtr excFence(DatumPtr node);
  DatumPtr excWindow(DatumPtr node);
  DatumPtr excTurtlemode(DatumPtr node);

  DatumPtr excTextscreen(DatumPtr node);
  DatumPtr excFullscreen(DatumPtr node);
  DatumPtr excSplitscreen(DatumPtr node);
  DatumPtr excScreenmode(DatumPtr node);

  DatumPtr excFilled(DatumPtr node);

  DatumPtr excButtonp(DatumPtr node);
  DatumPtr excButton(DatumPtr node);

  DatumPtr excDefine(DatumPtr node);
  DatumPtr excText(DatumPtr node);
  DatumPtr excFulltext(DatumPtr node);
  DatumPtr excCopydef(DatumPtr node);

  DatumPtr excLocal(DatumPtr node);

  DatumPtr excPprop(DatumPtr node);
  DatumPtr excGprop(DatumPtr node);
  DatumPtr excRemprop(DatumPtr node);
  DatumPtr excPlist(DatumPtr node);

  DatumPtr excProcedurep(DatumPtr node);
  DatumPtr excPrimitivep(DatumPtr node);
  DatumPtr excDefinedp(DatumPtr node);
  DatumPtr excNamep(DatumPtr node);
  DatumPtr excPlistp(DatumPtr node);

  DatumPtr excContents(DatumPtr node);
  DatumPtr excBuried(DatumPtr node);
  DatumPtr excTraced(DatumPtr node);
  DatumPtr excStepped(DatumPtr node);
  DatumPtr excProcedures(DatumPtr node);
  DatumPtr excPrimitives(DatumPtr node);
  DatumPtr excNames(DatumPtr node);
  DatumPtr excPlists(DatumPtr node);
  DatumPtr excArity(DatumPtr node);
  DatumPtr excNodes(DatumPtr node);

  DatumPtr excPrintout(DatumPtr node);
  DatumPtr excPot(DatumPtr node);

  DatumPtr excErase(DatumPtr node);
  DatumPtr excErall(DatumPtr node);
  DatumPtr excErps(DatumPtr node);
  DatumPtr excErns(DatumPtr node);
  DatumPtr excErpls(DatumPtr node);
  DatumPtr excBury(DatumPtr node);
  DatumPtr excUnbury(DatumPtr node);
  DatumPtr excBuriedp(DatumPtr node);
  DatumPtr excTrace(DatumPtr node);
  DatumPtr excUntrace(DatumPtr node);
  DatumPtr excTracedp(DatumPtr node);
  DatumPtr excStep(DatumPtr node);
  DatumPtr excUnstep(DatumPtr node);
  DatumPtr excSteppedp(DatumPtr node);
  DatumPtr excEdit(DatumPtr node);
  DatumPtr excEditfile(DatumPtr node);
  DatumPtr excSave(DatumPtr node);
  DatumPtr excLoad(DatumPtr node);
  DatumPtr excHelp(DatumPtr node);

  // CONTROL STRUCTURES

  DatumPtr excRun(DatumPtr node);
  DatumPtr excRunresult(DatumPtr node);
  DatumPtr excForever(DatumPtr node);
  DatumPtr excRepcount(DatumPtr node);
  DatumPtr excIf(DatumPtr node);
  DatumPtr excIfelse(DatumPtr node);
  DatumPtr excTest(DatumPtr node);
  DatumPtr excIftrue(DatumPtr node);
  DatumPtr excIffalse(DatumPtr node);
  DatumPtr excStop(DatumPtr node);
  DatumPtr excOutput(DatumPtr node);
  DatumPtr excCatch(DatumPtr node);
  DatumPtr excThrow(DatumPtr node);
  DatumPtr excError(DatumPtr node);
  DatumPtr excPause(DatumPtr node);
  DatumPtr excContinue(DatumPtr node);
  DatumPtr excBye(DatumPtr node);
  DatumPtr excDotMaybeoutput(DatumPtr node);
  DatumPtr excTag(DatumPtr);
  DatumPtr excGoto(DatumPtr node);
  DatumPtr excGotoToken(DatumPtr);

  // TEMPLATE-BASED ITERATION

  DatumPtr excApply(DatumPtr node);
  DatumPtr excNamedSlot(DatumPtr node); // '?'

  // MACROS

  DatumPtr excMacro(DatumPtr node);
  DatumPtr excMacrop(DatumPtr node);

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
  Kernel *kernel;
  DatumPtr procedureHistory;
  DatumPtr lineHistory;

public:
  ProcedureScope(Kernel *exec, DatumPtr procname);
  ~ProcedureScope();
};

class StreamRedirect {
  TextStream *originalWriteStream;
  TextStream *originalSystemWriteStream;
  TextStream *originalReadStream;
  TextStream *originalSystemReadStream;
  Kernel *exec;

public:
  StreamRedirect(Kernel *srcExec, TextStream *newReadStreamc,
                 TextStream *newWriteStream);
  ~StreamRedirect();
};

#endif // EXECUTOR_H
