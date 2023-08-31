
//===-- qlogo/kernel.cpp - Kernel class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Kernel class, which is the
/// executor proper of the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "kernel.h"
#include "parser.h"
#include <QColor>
#include <QFont>
#include <QImage>
#include <QApplication> // quit()
#include <stdlib.h> // arc4random_uniform()

#include "error.h"
#include "library.h"
#include "turtle.h"

#include "logocontroller.h"

// The maximum depth of procedure iterations before error is thrown.
const int maxIterationDepth = 1000;

ProcedureScope::ProcedureScope(Kernel *exec, DatumP procname) {
  ++(exec->procedureIterationDepth);
  procedureHistory = exec->callingProcedure;
  exec->callingProcedure = exec->currentProcedure;
  exec->currentProcedure = procname;
  lineHistory = exec->callingLine;
  exec->callingLine = exec->currentLine;
  kernel = exec;
}

ProcedureScope::~ProcedureScope() {
  --(kernel->procedureIterationDepth);
  kernel->currentProcedure = kernel->callingProcedure;
  kernel->callingProcedure = procedureHistory;
  kernel->currentLine = kernel->callingLine;
  kernel->callingLine = lineHistory;
}

StreamRedirect::StreamRedirect(Kernel *srcExec, QTextStream *newReadStream,
                               QTextStream *newWriteStream) {
  exec = srcExec;
  originalWriteStream = srcExec->writeStream;
  originalSystemWriteStream = srcExec->systemWriteStream;
  originalReadStream = srcExec->readStream;
  originalSystemReadStream = srcExec->systemReadStream;

  exec->writeStream = newWriteStream;
  exec->systemWriteStream = newWriteStream;
  exec->readStream = newReadStream;
  exec->systemReadStream = newReadStream;
}

StreamRedirect::~StreamRedirect() {
  exec->writeStream = originalWriteStream;
  exec->readStream = originalReadStream;
  exec->systemWriteStream = originalSystemWriteStream;
  exec->systemReadStream = originalSystemReadStream;
}


// This doesn't do anything or get called. It's just a token that gets passed
// when GOTO is used
DatumP Kernel::excGotoToken(DatumP) { return nothing; }


bool Kernel::isInputRedirected() { return readStream != NULL; }

bool Kernel::numbersFromList(QVector<double> &retval, DatumP l) {
    if ( ! l.isList())
        return false;
  ListIterator iter = l.listValue()->newIterator();

  retval.clear();
  while (iter.elementExists()) {
    DatumP n = iter.element();
    if (!n.isWord())
      return false;
    double v = n.wordValue()->numberValue();
    if (!n.wordValue()->didNumberConversionSucceed())
      return false;
    retval.push_back(v);
  }
  return true;
}

bool Kernel::colorFromDatumP(QColor &retval, DatumP colorP) {
  if (colorP.isWord()) {
    double colorNum = colorP.wordValue()->numberValue();
    if (colorP.wordValue()->didNumberConversionSucceed()) {
      if ((colorNum != round(colorNum)) || (colorNum < 0) ||
          (colorNum >= palette.size()))
        return false;
      retval = palette[colorNum];
      if (!retval.isValid())
        retval = palette[0];
      return true;
    }
    retval = QColor(colorP.wordValue()->printValue().toLower());
    return retval.isValid();
  } else if (colorP.isList()) {
    QVector<double> v;
    if (!numbersFromList(v, colorP.listValue()))
      return false;
    if (v.size() != 3)
      return false;
    for (int i = 0; i < 3; ++i) {
      if ((v[i] < 0) || (v[i] > 100))
        return false;
      v[i] *= 255.0 / 100;
    }
    retval = QColor(v[0], v[1], v[2], 255);
    return true;
  }
  return false;
}

bool Kernel::getLineAndRunIt(bool shouldHandleError) {
  QString prompt;
  if (currentProcedure.isASTNode()) {
    prompt =
        currentProcedure.astnodeValue()->nodeName.wordValue()->printValue();
  }
  prompt += "? ";
  ProcedureScope ps(this, nothing);

  try {
    DatumP line = readlistWithPrompt(prompt, true, systemReadStream);
    if (line == nothing)
      return false; // EOF
    if (line.listValue()->size() == 0)
      return true;

    DatumP result = runList(line);
    if (result != nothing)
      Error::dontSay(result);
  } catch (Error *e) {
    if (shouldHandleError) {
      if (e->tag.isWord()) {
          if (e->tag.wordValue()->keyValue() == "TOPLEVEL") {
              sysPrint("\n");
              return true;
          }
          if (e->tag.wordValue()->keyValue() == "SYSTEM") {
              sysPrint("\n");
              mainController()->systemStop();
              return false;
          }
      }
      sysPrint(e->errorText.printValue());
      if (e->procedure != nothing)
        sysPrint(QString(" in ") +
                 e->procedure.astnodeValue()->nodeName.printValue());
      sysPrint("\n");
      if (e->instructionLine != nothing) {
        sysPrint(parser->unreadDatum(e->instructionLine, true));
        sysPrint("\n");
      }
      registerError(nothing);
    } else {
      throw e;
    }
  }
  return true;
}

DatumP Kernel::registerError(DatumP anError, bool allowErract,
                             bool allowRecovery) {
  const QString erract = "ERRACT";
  currentError = anError;
  ProcedureHelper::setIsErroring(anError != nothing);
  if (anError != nothing) {
    Error *e = currentError.errorValue();
    if (e->code == 35) {
      e->procedure = callingProcedure;
      e->instructionLine = callingLine;
    } else {
      e->procedure = currentProcedure;
      e->instructionLine = currentLine;
    }
    DatumP erractP = variables.datumForName(erract);
    bool shouldPause = (currentProcedure != nothing) &&
        (erractP != nothing) && (erractP.datumValue()->size() > 0);

    if (allowErract && shouldPause) {
      sysPrint(e->errorText.printValue());
      sysPrint("\n");
      ProcedureHelper::setIsErroring(false);
      currentError = nothing;

      DatumP retval = pause();

      if (retval == nothing)
        Error::throwError(DatumP(new Word("TOPLEVEL")), nothing);
      if (allowRecovery) {
        return retval;
      }
      sysPrint(
          QString("You don't say what to do with %1").arg(retval.printValue()));
      return nothing;
    } else {
      throw anError.errorValue();
    }
  }
  return nothing;
}

void Kernel::initPalette() {
  const int paletteSize = 101;
  palette.clear();
  palette.reserve(paletteSize);
  palette.push_back(QColor(QStringLiteral("black")));       // 0
  palette.push_back(QColor(QStringLiteral("blue")));        // 1
  palette.push_back(QColor(QStringLiteral("green")));       // 2
  palette.push_back(QColor(QStringLiteral("cyan")));        // 3
  palette.push_back(QColor(QStringLiteral("red")));         // 4
  palette.push_back(QColor(QStringLiteral("magenta")));     // 5
  palette.push_back(QColor(QStringLiteral("yellow")));      // 6
  palette.push_back(QColor(QStringLiteral("white")));       // 7
  palette.push_back(QColor(QStringLiteral("brown")));       // 8
  palette.push_back(QColor(QStringLiteral("tan")));         // 9
  palette.push_back(QColor(QStringLiteral("forestgreen"))); // 10
  palette.push_back(QColor(QStringLiteral("aqua")));        // 11
  palette.push_back(QColor(QStringLiteral("salmon")));      // 12
  palette.push_back(QColor(QStringLiteral("purple")));      // 13
  palette.push_back(QColor(QStringLiteral("orange")));      // 14
  palette.push_back(QColor(QStringLiteral("grey")));        // 15
  palette.resize(paletteSize);
  turtle->setPenColor(palette[7]);
}

void Kernel::initLibrary() { executeText(libraryStr); }


// TODO: System vars need standardization
void Kernel::initVariables(void)
{
    const QString logoPlatform = "LOGOPLATFORM";
    const QString logoVersion = "LOGOVERSION";
    const QString allowGetSet = "ALLOWGETSET";

    DatumP platform(new Word(LOGOPLATFORM));
    DatumP version(new Word(LOGOVERSION));
    DatumP trueDatumP(&trueWord);
    variables.setDatumForName(platform, logoPlatform);
    variables.setDatumForName(version, logoVersion);
    variables.setDatumForName(trueDatumP, allowGetSet);
    variables.bury(logoPlatform);
    variables.bury(logoVersion);
    variables.bury(allowGetSet);
}

Kernel::Kernel() {
  readStream = NULL;
  systemReadStream = NULL;
  writeStream = NULL;
  systemWriteStream = NULL;

  turtle = new Turtle;
  parser = new Parser(this);
  ProcedureHelper::setParser(parser);
  Error::setKernel(this);

  initVariables();
  initPalette();

  filePrefix = nothing;

  initVariables();
}

Kernel::~Kernel() {
  closeAll();
  delete parser;
  delete turtle;
}

uint32_t Kernel::randomFromRange(uint32_t start, uint32_t end) {
  uint32_t range = end - start + 1;

  uint32_t x = arc4random_uniform(range);

  return x + start;
}

DatumP Kernel::readRawLineWithPrompt(const QString prompt,
                                     QTextStream *stream) {
    return parser->readrawlineWithPrompt(prompt, stream);
}

DatumP Kernel::readChar() {
  if (readStream == NULL) {
      return mainController()->readchar();
  }

  if (readStream->atEnd())
    return DatumP(new List);
  QString line = readStream->read(1);
  if (readStream->status() != QTextStream::Ok)
    Error::fileSystem();
  return DatumP(new Word(line));
}

DatumP Kernel::readlistWithPrompt(const QString &prompt,
                                  bool shouldRemoveComments,
                                  QTextStream *stream) {
    return parser->readlistWithPrompt(prompt, shouldRemoveComments, stream);
}

DatumP Kernel::readWordWithPrompt(const QString prompt, QTextStream *stream) {
    return parser->readwordWithPrompt(prompt, stream);
}

void Kernel::makeVarLocal(const QString &varname) {
  if (variables.size() <= 1)
    return;
  if (variables.isStepped(varname)) {
    QString line = varname + " shadowed by local in procedure call";
    if (currentProcedure != nothing) {
      line +=
          " in " +
          currentProcedure.astnodeValue()->nodeName.wordValue()->printValue();
    }
    sysPrint(line + "\n");
  }
  variables.setVarAsLocal(varname);
}

DatumP Kernel::executeProcedureCore(DatumP node) {
  ProcedureHelper h(this, node);
  // The first child is the body of the procedure
  DatumP proc = h.datumAtIndex(0);

  // The remaining children are the parameters
  int childIndex = 1;

  // first assign the REQUIRED params
  QList<QString> &requiredInputs = proc.procedureValue()->requiredInputs;
  for (auto &name : requiredInputs) {
    DatumP value = h.datumAtIndex(childIndex);
    ++childIndex;
    makeVarLocal(name);
    variables.setDatumForName(value, name);
  }

  // then assign the OPTIONAL params
  QList<QString> &optionalInputs = proc.procedureValue()->optionalInputs;
  QList<DatumP> &optionalDefaults = proc.procedureValue()->optionalDefaults;

  auto defaultIter = optionalDefaults.begin();
  for (auto &name : optionalInputs) {
    DatumP value;
    if (childIndex < h.countOfChildren()) {
      value = h.datumAtIndex(childIndex);
      ++childIndex;
    } else {
      value = runList(*defaultIter);
    }
    makeVarLocal(name);
    variables.setDatumForName(value, name);
    ++defaultIter;
  }

  // Finally, take in the remainder (if any) as a list.
  if (proc.procedureValue()->restInput != "") {
    const QString &name = proc.procedureValue()->restInput;
    DatumP remainderList = new List;
    while (childIndex < h.countOfChildren()) {
      DatumP value = h.datumAtIndex(childIndex);
      remainderList.listValue()->append(value);
      ++childIndex;
    }
    makeVarLocal(name);
    variables.setDatumForName(remainderList, name);
  }

  // Execute the commands in the procedure.

  DatumP retval;
  {
    ProcedureScope ps(this, node);
    ListIterator iter =
        proc.procedureValue()->instructionList.listValue()->newIterator();
    bool isStepped = parser->isStepped(
        node.astnodeValue()->nodeName.wordValue()->keyValue());
    while (iter.elementExists() && (retval == nothing)) {
      currentLine = iter.element();
      if (isStepped) {
        QString line = h.indent() + parser->unreadDatum(currentLine, true);
        sysPrint(line);
        readRawLineWithPrompt(" >>>", systemReadStream);
      }
      retval = runList(currentLine);
      if (retval.isASTNode()) {
        ASTNode *a = retval.astnodeValue();
        if (a->kernel == &Kernel::excGotoToken) {
          QString tag = a->childAtIndex(0).wordValue()->keyValue();
          DatumP startingLine = proc.procedureValue()->tagToLine[tag];
          iter =
              proc.procedureValue()->instructionList.listValue()->newIterator();
          while (iter.elementExists() && (currentLine != startingLine)) {
            currentLine = iter.element();
          }
          retval = runList(currentLine, tag);
        }
      }
    }
  }

  if ((retval != nothing) && !retval.isASTNode())
    Error::dontSay(retval);

  if (h.isTraced && retval.isASTNode()) {
    KernelMethod method = retval.astnodeValue()->kernel;
    if (method == &Kernel::excStop) {
        if (retval.astnodeValue()->countOfChildren() > 0) {
            retval = retval.astnodeValue()->childAtIndex(0);
            if (retval != nothing) {
                Error::dontSay(retval);
            }
        } else {
            retval = nothing;
        }
      } else if (method == &Kernel::excOutput) {
        DatumP p = retval.astnodeValue()->childAtIndex(0);
        KernelMethod temp_method = p.astnodeValue()->kernel;
        DatumP temp_retval = (this->*temp_method)(p);
        if (temp_retval == nothing)
          Error::didntOutput(p.astnodeValue()->nodeName,
                             retval.astnodeValue()->nodeName);
        retval = temp_retval;
      } else if (method == &Kernel::excDotMaybeoutput) {
        retval = retval.astnodeValue()->childAtIndex(0);
      } else {
        retval = (this->*method)(retval);
      } // /if method == ...
  }
  return h.ret(retval);
}

DatumP Kernel::executeProcedure(DatumP node) {
  VarFrame s(&variables);

  if (procedureIterationDepth > maxIterationDepth) {
      Error::stackOverflow();
    }
  DatumP retval = executeProcedureCore(node);
  ASTNode *lastOutputCmd = NULL;

  while (retval.isASTNode()) {
      KernelMethod method = retval.astnodeValue()->kernel;
      if ((method == &Kernel::excOutput) || (method == &Kernel::excDotMaybeoutput) || ((method == &Kernel::excStop) && (retval.astnodeValue()->countOfChildren() > 0))) {
          if (method == &Kernel::excOutput) {
              lastOutputCmd = retval.astnodeValue();
            }
          node = retval.astnodeValue()->childAtIndex(0);
          method = node.astnodeValue()->kernel;

          // if the output is a procedure, then trampoline
          if (method == &Kernel::executeProcedure) {
              retval = executeProcedureCore(node);
            } else {
              retval = (this->*method)(node);
            }
          if ((retval == nothing) && (lastOutputCmd != NULL)) {
              Error::didntOutput(node.astnodeValue()->nodeName, lastOutputCmd->nodeName);
            }
        } else if (method == &Kernel::excStop) {
          if (lastOutputCmd == NULL) {
              return nothing;
            } else {
              Error::didntOutput(node.astnodeValue()->nodeName,
                                 lastOutputCmd->nodeName);
            }
        } else {
          retval = (this->*method)(retval);
        }
    } // /while isASTNode

  return retval;
}

DatumP Kernel::executeMacro(DatumP node) {
    bool wasExecutingMacro = isRunningMacroResult;
    isRunningMacroResult = true;
    try {
        while (node.isASTNode()) {
            node = executeProcedure(node);
            if (!node.isList()) {
                isRunningMacroResult = wasExecutingMacro;
                return Error::macroReturned(node);
            }
            node = runList(node);
        }
    } catch (Error *e) {
        isRunningMacroResult = wasExecutingMacro;
        throw e;
    }

    isRunningMacroResult = wasExecutingMacro;
    return node;
}

ASTNode *Kernel::astnodeValue(DatumP caller, DatumP value) {
  if (!value.isASTNode())
    Error::doesntLike(caller.astnodeValue()->nodeName, value);
  return value.astnodeValue();
}

DatumP Kernel::executeLiteral(DatumP node) {
  return node.astnodeValue()->childAtIndex(0);
}

DatumP Kernel::executeValueOf(DatumP node) {
  DatumP varnameP = node.astnodeValue()->childAtIndex(0);
  QString varName = varnameP.wordValue()->keyValue();
  DatumP retval = variables.datumForName(varName);
  if (retval == nothing)
    return (Error::noValueRecoverable(varnameP));
  return retval;
}

SignalsEnum_t Kernel::interruptCheck()
{
    SignalsEnum_t latestSignal = mainController()->latestSignal();
    if (latestSignal == toplevelSignal) {
        if (currentProcedure != nothing)
            Error::throwError(DatumP(new Word("TOPLEVEL")), nothing);
    } else if (latestSignal == pauseSignal) {
        if (currentProcedure != nothing)
            pause();
    } else if (latestSignal == systemSignal) {
        Error::throwError(DatumP(new Word("SYSTEM")), nothing);
    }
    return latestSignal;
}

DatumP Kernel::runList(DatumP listP, const QString startTag) {
  bool shouldSearchForTag = (startTag != "");
  DatumP retval;

  interruptCheck();

  if (listP.isWord())
    listP = parser->runparse(listP);

  if (!listP.isList()) {
    Error::noHow(listP);
  }

  bool tagHasBeenFound = !shouldSearchForTag;

  QList<DatumP> *parsedList = parser->astFromList(listP.listValue());
  for (int i = 0; i < parsedList->size(); ++i) {
    if (retval != nothing) {
      if (retval.isASTNode()) {
        return retval;
      }
      Error::dontSay(retval);
    }
    DatumP statement = (*parsedList)[i];
    KernelMethod method = statement.astnodeValue()->kernel;
    if (tagHasBeenFound) {
        if (isRunningMacroResult && (method == &Kernel::executeMacro) && (i == parsedList->size()-1)) {
            return statement;
        }
      retval = (this->*method)(statement);
    } else {
      if (method == &Kernel::excTag) {
        ASTNode *child =
            statement.astnodeValue()->childAtIndex(0).astnodeValue();
        if (child->kernel == &Kernel::executeLiteral) {
          DatumP v = child->childAtIndex(0);
          if (v.isWord()) {
            QString tag = v.wordValue()->keyValue();
            tagHasBeenFound = (startTag == tag);
          }
        }
      }
    }
  }

  return retval;
}

DatumP Kernel::excWait(DatumP node) {
  ProcedureHelper h(this, node);
  double value = h.validatedNumberAtIndex(
      0, [](double candidate) { return candidate >= 0; });
  mainController()->mwait((1000.0 / 60) * value);
  return nothing;
}

DatumP Kernel::excNoop(DatumP node) {
  ProcedureHelper h(this, node);
  return h.ret();
}

DatumP Kernel::excErrorNoGui(DatumP node) {
  ProcedureHelper h(this, node);
  Error::noGraphics();
  return h.ret();
}

DatumP Kernel::pause() {
    if (isPausing) {
        sysPrint("Already Pausing");
        return nothing;
    }
  ProcedureScope procScope(this, nothing);
  isPausing = true;
  StreamRedirect streamScope(this, NULL, NULL);

  sysPrint("Pausing...\n");

  forever {
    try {
      bool shouldContinue = true;
      while (shouldContinue) {
        shouldContinue = getLineAndRunIt(false);
      }
    } catch (Error *e) {
      if ((e->code == 14) && (e->tag.wordValue()->keyValue() == "PAUSE")) {
        DatumP retval = e->output;
        registerError(nothing);
        isPausing = false;
        return retval;
      }
      if ((e->code == 14) && ((e->tag.wordValue()->keyValue() == "TOPLEVEL")
                              || (e->tag.wordValue()->keyValue() == "SYSTEM"))) {
          isPausing = false;
        throw e;
      }
      sysPrint(e->errorText.printValue());
      sysPrint("\n");
      registerError(nothing);
    }
  }
  isPausing = false;
  return nothing;
}
