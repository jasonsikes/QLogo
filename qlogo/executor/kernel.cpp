
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
#include "datum/word.h"
#include "datum/astnode.h"
#include <QColor>
#include <QFont>
#include <QImage>
#include <QApplication> // quit()
#include <stdlib.h> // arc4random_uniform()

#include "error.h"
#include "library.h"
#include "runparser.h"
#include "turtle.h"

#include "controller/logocontroller.h"

// The maximum depth of procedure iterations before error is thrown.
const int maxIterationDepth = 1000;

Kernel* _mainKernel = NULL;

Kernel* mainKernel() {
  Q_ASSERT(_mainKernel != NULL);
  return _mainKernel;
}

ProcedureScope::ProcedureScope(DatumPtr procname) {
  ++(_mainKernel->procedureIterationDepth);
  procedureHistory = _mainKernel->callingProcedure;
  _mainKernel->callingProcedure = _mainKernel->currentProcedure;
  _mainKernel->currentProcedure = procname;
  lineHistory = _mainKernel->callingLine;
  _mainKernel->callingLine = _mainKernel->currentLine;
}

ProcedureScope::~ProcedureScope() {
  --(_mainKernel->procedureIterationDepth);
  _mainKernel->currentProcedure = _mainKernel->callingProcedure;
  _mainKernel->callingProcedure = procedureHistory;
  _mainKernel->currentLine = _mainKernel->callingLine;
  _mainKernel->callingLine = lineHistory;
}

StreamRedirect::StreamRedirect(TextStream *newReadStream,
                               TextStream *newWriteStream) {
  originalWriteStream = _mainKernel->writeStream;
  originalSystemWriteStream = _mainKernel->systemWriteStream;
  originalReadStream = _mainKernel->readStream;
  originalSystemReadStream = _mainKernel->systemReadStream;

  _mainKernel->writeStream = newWriteStream;
  _mainKernel->systemWriteStream = newWriteStream;
  _mainKernel->readStream = newReadStream;
  _mainKernel->systemReadStream = newReadStream;
}

StreamRedirect::~StreamRedirect() {
  _mainKernel->writeStream = originalWriteStream;
  _mainKernel->readStream = originalReadStream;
  _mainKernel->systemWriteStream = originalSystemWriteStream;
  _mainKernel->systemReadStream = originalSystemReadStream;
}


// This doesn't do anything or get called. It's just a token that gets passed
// when GOTO is used
DatumPtr Kernel::excGotoToken(DatumPtr) { return nothing; }


bool Kernel::isInputRedirected() { return readStream != stdioStream; }

bool Kernel::numbersFromList(QVector<double> &retval, DatumPtr l) {
    if ( ! l.isList())
        return false;
  ListIterator iter = l.listValue()->newIterator();

  retval.clear();
  while (iter.elementExists()) {
    DatumPtr n = iter.element();
    if (!n.isWord())
      return false;
    double v = n.wordValue()->numberValue();
    if (isnan(v))
      return false;
    retval.push_back(v);
  }
  return true;
}

bool Kernel::colorFromDatumPtr(QColor &retval, DatumPtr colorP) {
  if (colorP.isWord()) {
    double colorNum = colorP.wordValue()->numberValue();
    if ( ! isnan(colorNum)) {
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
    if ((v.size() != 3) && (v.size() != 4))
      return false;
    for (int i = 0; i < v.size(); ++i) {
      if ((v[i] < 0) || (v[i] > 100))
        return false;
      v[i] *= 255.0 / 100;
    }
    double alpha = (v.size() == 4) ? v[3] : 255;
    retval = QColor(v[0], v[1], v[2], alpha);
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
  ProcedureScope ps(nothing);

  try {
    DatumPtr line = systemReadStream->readlistWithPrompt(prompt, true);
    if (line == nothing)
      return false; // EOF
    if (line.listValue()->isEmpty())
      return true;

    DatumPtr result = runList(line);
    if (result != nothing)
      Error::dontSay(result);
  } catch (Error *e) {
    if (shouldHandleError) {
      if (e->tag.isWord()) {
          if (e->tag.wordValue()->keyValue() == QObject::tr("TOPLEVEL")) {
              sysPrint("\n");
              return true;
          }
          if (e->tag.wordValue()->keyValue() == QObject::tr("SYSTEM")) {
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
        sysPrint(procedures->unreadDatum(e->instructionLine, true));
        sysPrint("\n");
      }
      registerError(nothing);
    } else {
      throw e;
    }
  }
  return true;
}

DatumPtr Kernel::registerError(DatumPtr anError, bool allowErract,
                             bool allowRecovery) {
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
    DatumPtr erractP = variables.datumForName(QObject::tr("ERRACT"));
    bool shouldPause = (currentProcedure != nothing) &&
                       ((erractP.isList() && ( ! erractP.listValue()->isEmpty()))
                                                         || (erractP.isWord() && (erractP.wordValue()->rawValue().size() > 0)));

    if (allowErract && shouldPause) {
      sysPrint(e->errorText.printValue());
      sysPrint("\n");
      ProcedureHelper::setIsErroring(false);
      currentError = nothing;

      DatumPtr retval = pause();

      if (retval == nothing)
        Error::throwError(DatumPtr(QObject::tr("TOPLEVEL")), nothing);
      if (allowRecovery) {
        return retval;
      }
      sysPrint(
          QObject::tr("You don't say what to do with %1").arg(retval.printValue()));
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
}

void Kernel::initLibrary() { executeText(libraryStr); }


// TODO: System vars need standardization
void Kernel::initVariables(void)
{
    DatumPtr platform(LOGOPLATFORM);
    DatumPtr version(LOGOVERSION);
    DatumPtr trueDatumPtr(QObject::tr("true"));

    variables.setDatumForName(platform, QObject::tr("LOGOPLATFORM"));
    variables.setDatumForName(version, QObject::tr("LOGOVERSION"));
    variables.setDatumForName(trueDatumPtr, QObject::tr("ALLOWGETSET"));
    variables.bury(QObject::tr("LOGOPLATFORM"));
    variables.bury(QObject::tr("LOGOVERSION"));
    variables.bury(QObject::tr("ALLOWGETSET"));
}

Kernel::Kernel() {
  Q_ASSERT(_mainKernel == NULL);
  _mainKernel = this;
  stdioStream = new TextStream(NULL);
  readStream = stdioStream;
  systemReadStream = stdioStream;
  writeStream = stdioStream;
  systemWriteStream = stdioStream;

  turtle = new Turtle;
  procedures = new Procedures;
  parser = new Parser;

  initVariables();
  initPalette();

  filePrefix = new List();
}

Kernel::~Kernel() {
  closeAll();
  delete parser;
  delete procedures;
  delete turtle;

}

void Kernel::makeVarLocal(const QString &varname) {
  if (variables.size() <= 1)
    return;
  if (variables.isStepped(varname)) {
    QString line = varname + QObject::tr(" shadowed by local in procedure call");
    if (currentProcedure != nothing) {
      line +=
          " in " +
          currentProcedure.astnodeValue()->nodeName.wordValue()->printValue();
    }
    sysPrint(line + "\n");
  }
  variables.setVarAsLocal(varname);
}

DatumPtr Kernel::executeProcedureCore(DatumPtr node) {
  ProcedureHelper h(this, node);
  // The first child is the body of the procedure
  DatumPtr proc = h.datumAtIndex(0);

  // The remaining children are the parameters
  int childIndex = 1;

  // first assign the REQUIRED params
  QList<QString> &requiredInputs = proc.procedureValue()->requiredInputs;
  for (auto &name : requiredInputs) {
    DatumPtr value = h.datumAtIndex(childIndex);
    ++childIndex;
    makeVarLocal(name);
    variables.setDatumForName(value, name);
  }

  // then assign the OPTIONAL params
  QList<QString> &optionalInputs = proc.procedureValue()->optionalInputs;
  QList<DatumPtr> &optionalDefaults = proc.procedureValue()->optionalDefaults;

  auto defaultIter = optionalDefaults.begin();
  for (auto &name : optionalInputs) {
    DatumPtr value;
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
    DatumPtr remainderList = DatumPtr(new List());
    while (childIndex < h.countOfChildren()) {
      DatumPtr value = h.datumAtIndex(childIndex);
      remainderList.listValue()->append(value);
      ++childIndex;
    }
    makeVarLocal(name);
    variables.setDatumForName(remainderList, name);
  }

  // Execute the commands in the procedure.

  DatumPtr retval;
  {
    ProcedureScope ps(node);
    ListIterator iter =
        proc.procedureValue()->instructionList.listValue()->newIterator();
    bool isStepped = procedures->isStepped(
        node.astnodeValue()->nodeName.wordValue()->keyValue());
    while (iter.elementExists() && (retval == nothing)) {
      currentLine = iter.element();
      if (isStepped) {
        QString line = h.indent() + procedures->unreadDatum(currentLine, true);
        sysPrint(line);
        systemReadStream->readrawlineWithPrompt(" >>>");
      }
      retval = runList(currentLine);
      if (retval.isASTNode()) {
        ASTNode *a = retval.astnodeValue();
        if (a->kernel == &Kernel::excGotoToken) {
          QString tag = a->childAtIndex(0).wordValue()->keyValue();
          DatumPtr startingLine = proc.procedureValue()->tagToLine[tag];
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
        DatumPtr p = retval.astnodeValue()->childAtIndex(0);
        KernelMethod temp_method = p.astnodeValue()->kernel;
        DatumPtr temp_retval = (this->*temp_method)(p);
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

DatumPtr Kernel::executeProcedure(DatumPtr node) {
  VarFrame s(&variables);

  if (procedureIterationDepth > maxIterationDepth) {
      Error::stackOverflow();
    }
  DatumPtr retval = executeProcedureCore(node);
  ASTNode *lastOutputCmd = NULL;

  while (retval.isASTNode()) {
      KernelMethod method = retval.astnodeValue()->kernel;
      if ((method == &Kernel::excOutput)
          || (method == &Kernel::excDotMaybeoutput)
          || ((method == &Kernel::excStop)
              && (retval.astnodeValue()->countOfChildren() > 0))) {
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

DatumPtr Kernel::executeMacro(DatumPtr node) {
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

ASTNode *Kernel::astnodeValue(DatumPtr caller, DatumPtr value) {
  if (!value.isASTNode())
    Error::doesntLike(caller.astnodeValue()->nodeName, value);
  return value.astnodeValue();
}

DatumPtr Kernel::executeLiteral(DatumPtr node) {
  return node.astnodeValue()->childAtIndex(0);
}

DatumPtr Kernel::executeValueOf(DatumPtr node) {
  DatumPtr varnameP = node.astnodeValue()->childAtIndex(0);
  QString varName = varnameP.wordValue()->keyValue();
  DatumPtr retval = variables.datumForName(varName);
  if (retval == nothing)
    return (Error::noValueRecoverable(varnameP));
  return retval;
}

SignalsEnum_t Kernel::interruptCheck()
{
    SignalsEnum_t latestSignal = mainController()->latestSignal();
    if (latestSignal == toplevelSignal) {
        if (currentProcedure != nothing)
            Error::throwError(DatumPtr(QObject::tr("TOPLEVEL")), nothing);
    } else if (latestSignal == pauseSignal) {
        if (currentProcedure != nothing)
            pause();
    } else if (latestSignal == systemSignal) {
        Error::throwError(DatumPtr(QObject::tr("SYSTEM")), nothing);
    }
    return latestSignal;
}

DatumPtr Kernel::runList(DatumPtr listP, const QString startTag) {
  bool shouldSearchForTag = (startTag != "");
  DatumPtr retval;

  interruptCheck();

  if (listP.isWord())
    listP = runparse(listP);

  if (!listP.isList()) {
    Error::noHow(listP);
  }

  bool tagHasBeenFound = !shouldSearchForTag;

  QList<DatumPtr> *parsedList = parser->astFromList(listP.listValue());
  for (int i = 0; i < parsedList->size(); ++i) {
    if (retval != nothing) {
      if (retval.isASTNode()) {
        return retval;
      }
      Error::dontSay(retval);
    }
    DatumPtr statement = (*parsedList)[i];
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
          DatumPtr v = child->childAtIndex(0);
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


/***DOC WAIT
WAIT time

    command.  Delays further execution for "time" 60ths of a second.
    Also causes any buffered characters destined for the terminal to
    be printed immediately.  WAIT 0 can be used to achieve this
    buffer flushing without actually waiting.

COD***/
//CMD WAIT 1 1 1
DatumPtr Kernel::excWait(DatumPtr node) {
  ProcedureHelper h(this, node);
  double value = h.validatedNumberAtIndex(
      0, [](double candidate) { return candidate >= 0; });
  mainController()->mwait((1000.0 / 60) * value);
  return nothing;
}

DatumPtr Kernel::excNoop(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret();
}

DatumPtr Kernel::excErrorNoGui(DatumPtr node) {
  ProcedureHelper h(this, node);
  Error::noGraphics();
  return h.ret();
}

DatumPtr Kernel::pause() {
    if (isPausing) {
        sysPrint(QObject::tr("Already Pausing"));
        return nothing;
    }
  ProcedureScope procScope(nothing);
  isPausing = true;
  StreamRedirect streamScope(stdioStream, stdioStream);

  sysPrint(QObject::tr("Pausing...\n"));

  forever {
    try {
      bool shouldContinue = true;
      while (shouldContinue) {
        shouldContinue = getLineAndRunIt(false);
      }
    } catch (Error *e) {
      if ((e->code == 14) && (e->tag.wordValue()->keyValue() == QObject::tr("PAUSE"))) {
        DatumPtr retval = e->output;
        registerError(nothing);
        isPausing = false;
        return retval;
      }
      if ((e->code == 14) && ((e->tag.wordValue()->keyValue() == QObject::tr("TOPLEVEL"))
                              || (e->tag.wordValue()->keyValue() == QObject::tr("SYSTEM")))) {
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
