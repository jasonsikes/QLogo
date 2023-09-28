
//===-- qlogo/parser.cpp - Parser class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Parser class, which is
/// responsible for parsing text and keeping user-defined functions.
///
//===----------------------------------------------------------------------===//

#include "parser.h"
#include "logocontroller.h"
#include "error.h"
#include "kernel.h"
#include "datum_word.h"
#include "datum_astnode.h"
#include "datum_array.h"
#include <qdatetime.h>
#include <qdebug.h>
#include "stringconstants.h"

static DatumPool<Procedure> pool(20);

const QString specialChars("+-()*%/<>=");

char lastNonSpaceChar(const QString &line) {
  char retval = ' ';
  for (int i = line.length() - 1; i >= 0; --i) {
    retval = line[i].toLatin1();
    if (retval != ' ')
      break;
  }
  return retval;
}

// return the method pointer if a GUI is available,
// else return a pointer to the excErrorNoGUI method
KernelMethod ifGUI(KernelMethod method) {
  extern bool hasGUI;
  if (hasGUI) {
      return method;
    }
  return &Kernel::excErrorNoGui;
}

QHash<QString, Cmd_t> stringToCmd;

void Parser::defineProcedure(DatumP cmd, DatumP procnameP, DatumP text,
                             DatumP sourceText) {
  lastProcedureCreatedTimestamp = QDateTime::currentMSecsSinceEpoch();

  procnameP.wordValue()->numberValue();
  if (procnameP.wordValue()->didNumberConversionSucceed())
    Error::doesntLike(cmd, procnameP);

  QString procname = procnameP.wordValue()->keyValue();

  QChar firstChar = (procname)[0];
  if ((firstChar == '"') || (firstChar == ':'))
    Error::doesntLike(cmd, procnameP);

  if (stringToCmd.contains(procname))
    Error::isPrimative(procnameP);

  DatumP procBody = createProcedure(cmd, text, sourceText);

  procedures[procname] = procBody;

  if (kernel->isInputRedirected() && kernel->varUNBURYONEDIT()) {
    unbury(procname);
  }
}

DatumP Parser::createProcedure(DatumP cmd, DatumP text, DatumP sourceText) {
  Procedure *body = (Procedure *) pool.alloc();
  body->init();
  DatumP bodyP(body);

  QString cmdString = cmd.wordValue()->keyValue();
  bool isMacro = ((cmdString == k.dcMacro()) || (cmdString == k.dDefmacro()));

  body->defaultNumber = 0;
  body->countOfMinParams = 0;
  body->countOfMaxParams = 0;
  body->isMacro = isMacro;
  body->sourceText = sourceText;

  bool isOptionalDefined = false;
  bool isRestDefined = false;
  bool isDefaultDefined = false;

  // Required Inputs :FOO
  // Optional inputs [:BAZ 87]
  // Rest input      [:GARPLY]
  // default number  5

  ListIterator paramIter = text.listValue()->first().listValue()->newIterator();

  while (paramIter.elementExists()) {
    DatumP currentParam = paramIter.element();

    if (currentParam.isWord()) { // default 5 OR required :FOO
      double paramAsNumber = currentParam.wordValue()->numberValue();
      if (currentParam.wordValue()->didNumberConversionSucceed()) { // default 5
        if (isDefaultDefined)
          Error::doesntLike(cmd, currentParam);
        if ((paramAsNumber != floor(paramAsNumber)) ||
            (paramAsNumber < body->countOfMinParams) ||
            ((paramAsNumber > body->countOfMaxParams) &&
             (body->countOfMaxParams >= 0)))
          Error::doesntLike(cmd, currentParam);
        body->defaultNumber = paramAsNumber;
        isDefaultDefined = true;
      } else {
        if (isDefaultDefined || isRestDefined || isOptionalDefined)
          Error::doesntLike(cmd, currentParam);
        QString paramName =
            currentParam.wordValue()->keyValue(); // required :FOO
        if (paramName.startsWith(':') || paramName.startsWith('"'))
          paramName.remove(0, 1);
        if (paramName.size() < 1)
          Error::doesntLike(cmd, currentParam);
        body->requiredInputs.append(paramName);
        body->defaultNumber += 1;
        body->countOfMinParams += 1;
        body->countOfMaxParams += 1;
      }
    } else if (currentParam.isList()) { // Optional [:BAZ 87] or rest [:GARPLY]
      List *paramList = currentParam.listValue();

      if (paramList->size() == 0)
        Error::doesntLike(cmd, currentParam);

      if (paramList->size() == 1) { // rest input [:GARPLY]
        if (isRestDefined)
          Error::doesntLike(cmd, currentParam);
        DatumP param = paramList->first();
        if (param.isWord()) {
          QString restName = param.wordValue()->keyValue();
          if (restName.startsWith(':') || restName.startsWith('"'))
            restName.remove(0, 1);
          if (restName.size() < 1)
            Error::doesntLike(cmd, param);
          body->restInput = restName;
          isRestDefined = true;
          body->countOfMaxParams = -1;
        } else {
          Error::doesntLike(cmd, param);
        }
      } else { // Optional [:BAZ 87]
        if (isRestDefined || isDefaultDefined)
          Error::doesntLike(cmd, currentParam);
        DatumP param = paramList->first();
        DatumP defaultValue = paramList->butfirst();
        if (param.isWord()) {
          QString name = param.wordValue()->keyValue();
          if (name.startsWith(':') || name.startsWith('"'))
            name.remove(0, 1);
          if (name.size() < 1)
            Error::doesntLike(cmd, param);
          body->optionalInputs.append(name);
          body->optionalDefaults.append(defaultValue);
          isOptionalDefined = true;
          body->countOfMaxParams += 1;
        } else {
          Error::doesntLike(cmd, param);
        }
      } // endif optional or rest input
    } else {
      Error::doesntLike(cmd, currentParam);
    }
  } // /for each parameter

  body->instructionList = text.listValue()->butfirst();

  ListIterator lineIter = body->instructionList.listValue()->newIterator();
  while (lineIter.elementExists()) {
    DatumP lineP = lineIter.element();
    ListIterator wordIter = lineP.listValue()->newIterator();
    while (wordIter.elementExists()) {
      DatumP d = wordIter.element();
      if (d.isWord() && (d.wordValue()->keyValue() == k.tag()) &&
          wordIter.elementExists()) {
        DatumP d = wordIter.element();
        if (d.isWord()) {
          QString param = d.wordValue()->keyValue();
          if ((param.size() > 1) && (param)[0] == '"') {
            QString tag = param.right(param.size() - 1);
            body->tagToLine[tag] = lineP;
          }
        }
      }
    }
  }
  return bodyP;
}

void Parser::copyProcedure(DatumP newnameP, DatumP oldnameP) {
  lastProcedureCreatedTimestamp = QDateTime::currentMSecsSinceEpoch();
  QString newname = newnameP.wordValue()->keyValue();
  QString oldname = oldnameP.wordValue()->keyValue();

  if (stringToCmd.contains(newname))
    Error::isPrimative(newnameP);

  if (procedures.contains(oldname)) {
    procedures[newname] = procedures[oldname];
    return;
  }
  if (primitiveAlternateNames.contains(oldname)) {
    primitiveAlternateNames[newname] = primitiveAlternateNames[oldname];
    return;
  }
  if (stringToCmd.contains(oldname)) {
    primitiveAlternateNames[newname] = stringToCmd[oldname];
    return;
  }
  Error::noHow(oldnameP);
}

void Parser::eraseProcedure(DatumP procnameP) {
  lastProcedureCreatedTimestamp = QDateTime::currentMSecsSinceEpoch();

  QString procname = procnameP.wordValue()->keyValue();
  if (stringToCmd.contains(procname))
    Error::isPrimative(procnameP);
  procedures.remove(procname);
}

DatumP Parser::procedureText(DatumP procnameP) {
  QString procname = procnameP.wordValue()->keyValue();

  if (stringToCmd.contains(procname))
    Error::isPrimative(procnameP);
  if (!procedures.contains(procname))
    Error::noHow(procnameP);
  Procedure *body = procedures[procname].procedureValue();

  List *retval = List::alloc();

  List *inputs = List::alloc();

  for (auto &i : body->requiredInputs) {
    inputs->append(DatumP(i));
  }

  QList<DatumP>::iterator d = body->optionalDefaults.begin();
  for (auto &i : body->optionalInputs) {
    List *optInput = List::alloc(d->listValue());
    optInput->prepend(DatumP(i));
    ++d;
    inputs->append(DatumP(optInput));
  }

  if (body->restInput != "") {
    List *restInput = List::alloc();
    restInput->append(DatumP(body->restInput));
    inputs->append(DatumP(restInput));
  }

  if (body->defaultNumber != body->requiredInputs.size()) {
    inputs->append(DatumP(body->defaultNumber));
  }

  retval->append(DatumP(inputs));

  ListIterator b = body->instructionList.listValue()->newIterator();

  while (b.elementExists()) {
    retval->append(b.element());
  }

  return DatumP(retval);
}

DatumP Parser::procedureFulltext(DatumP procnameP, bool shouldValidate) {
  const QString procname = procnameP.wordValue()->keyValue();
  if (stringToCmd.contains(procname))
    Error::isPrimative(procnameP);

  if (procedures.contains(procname)) {
    Procedure *body = procedures[procname].procedureValue();

    if (body->sourceText == nothing) {
      List *retval = List::alloc();
      retval->append(DatumP(procedureTitle(procnameP)));

      ListIterator b = body->instructionList.listValue()->newIterator();

      while (b.elementExists()) {
        retval->append(DatumP(unreadList(b.element().listValue(), false)));
      }

      DatumP end(k.end());
      retval->append(end);
      return DatumP(retval);
    } else {
      return body->sourceText;
    }
  } else if (shouldValidate) {
    Error::noHow(procnameP);
  }
  List *retval = List::alloc();
  retval->append(
      DatumP(k.to_() + procnameP.wordValue()->printValue()));
  retval->append(DatumP(k.end()));
  return DatumP(retval);
}

QString Parser::procedureTitle(DatumP procnameP) {
  QString procname = procnameP.wordValue()->keyValue();

  if (stringToCmd.contains(procname))
    Error::isPrimative(procnameP);
  if (!procedures.contains(procname))
    Error::noHow(procnameP);

  Procedure *body = procedures[procname].procedureValue();

  DatumP firstlineP = DatumP(List::alloc());

  List *firstLine = firstlineP.listValue();

  if (body->isMacro)
    firstLine->append(DatumP(k.dMacro()));
  else
    firstLine->append(DatumP(k.to()));
  firstLine->append(procnameP);

  QString paramName;

  for (auto &i : body->requiredInputs) {
    paramName = i;
    paramName.prepend(':');
    firstLine->append(DatumP(paramName));
  }

  QList<DatumP>::iterator d = body->optionalDefaults.begin();
  for (auto &i : body->optionalInputs) {
    paramName = i;
    paramName.push_front(':');
    List *optInput = List::alloc(d->listValue());
    optInput->prepend(DatumP(paramName));
    firstLine->append(DatumP(optInput));
    ++d;
  }

  paramName = body->restInput;
  if (paramName != "") {
    paramName.push_front(':');
    List *restInput = List::alloc();
    restInput->append(DatumP(paramName));
    firstLine->append(DatumP(restInput));
  }

  if (body->defaultNumber != body->requiredInputs.size()) {
    firstLine->append(DatumP(body->defaultNumber));
  }

  QString retval = unreadList(firstLine, false);
  return retval;
}

// firstline is ASTNode
void Parser::inputProcedure(DatumP nodeP, QTextStream *readStream) {
  ASTNode *node = nodeP.astnodeValue();

  DatumP to = node->nodeName;
  if (node->countOfChildren() == 0)
    Error::notEnough(to);
  DatumP procnameP = node->childAtIndex(0);
  if (!procnameP.isWord())
    Error::doesntLike(to, procnameP);

  procnameP.wordValue()->numberValue();
  if (procnameP.wordValue()->didNumberConversionSucceed())
    Error::doesntLike(to, procnameP);

  QString procname = procnameP.wordValue()->keyValue();

  QChar firstChar = (procname)[0];
  if ((firstChar == '"') || (firstChar == ':') || (firstChar == '(') ||
      (firstChar == ')'))
    Error::doesntLike(to, procnameP);

  if (stringToCmd.contains(procname))
    Error::procDefined(procnameP);

  DatumP textP = DatumP(List::alloc());
  DatumP sourceText = lastReadListSource();
  DatumP firstLine = DatumP(List::alloc());
  for (int i = 1; i < node->countOfChildren(); ++i) {
    firstLine.listValue()->append(node->childAtIndex(i));
  }
  textP.listValue()->append(firstLine);

  // Now read in the body
  forever {
    DatumP line = readlistWithPrompt("> ", true, readStream);
    if ( ! line.isList()) // this must be the end of the input
        break;
    DatumP lineSource = lastReadListSource();
    ListIterator lineSourceIter = lineSource.listValue()->newIterator();
    while (lineSourceIter.elementExists()) {
      sourceText.listValue()->append(lineSourceIter.element());
    }
    if (line.listValue()->size() == 0)
      continue;
    DatumP first = line.listValue()->first();
    if (first.isWord()) {
      QString firstWord = first.wordValue()->keyValue();
      if (firstWord == k.end())
        break;
    }
    textP.listValue()->append(line);
  }

  defineProcedure(to, procnameP, textP, sourceText);

  kernel->sysPrint(procnameP.wordValue()->printValue());
  kernel->sysPrint(k._defined());
  lastProcedureCreatedTimestamp = QDateTime::currentMSecsSinceEpoch();
}

DatumP Parser::readrawlineWithPrompt(const QString &prompt,
                                     QTextStream *readStream) {
  DatumP retval;
  if (readStream == NULL) {
    retval = mainController()->readRawlineWithPrompt(prompt);
  } else {
    if (readStream->atEnd()) {
      return nothing;
    }
    QString str = readStream->readLine();
    if (readStream->status() != QTextStream::Ok)
      Error::fileSystem();
    retval = DatumP(str);
  }
  listSourceText.listValue()->append(retval);
  return retval;
}

DatumP Parser::readwordWithPrompt(const QString &prompt,
                                  QTextStream *readStream) {
  QString retval = "";
  bool isVbarred = false;
  bool isEscaped = false;

  DatumP line = readrawlineWithPrompt(prompt, readStream);
  if (line == nothing)
    return nothing;

  forever {
    if (line == nothing)
      return DatumP(retval);

    const QString &t = line.wordValue()->rawValue();
    retval.reserve(retval.size() + t.size());
    for (auto c : t) {
      if (isEscaped) {
        isEscaped = false;
        retval.push_back(charToRaw(c));
        continue;
      }
      if (c == '|') {
        isVbarred = !isVbarred;
      }
      if (c == '\\') {
        isEscaped = true;
        continue;
      }

      retval.push_back(c);
    } // i
    // The end of the line
    if (isEscaped) {
      isEscaped = false;
      retval.push_back('\n');
      line = readrawlineWithPrompt("\\ ", readStream);
      continue;
    }
    if (isVbarred) {
      retval.push_back(charToRaw('\n'));
      line = readrawlineWithPrompt("| ", readStream);
      continue;
    }
    if (lastNonSpaceChar(t) == '~') {
      retval.push_back('\n');
      line = readrawlineWithPrompt("~ ", readStream);
      continue;
    }

    return DatumP(retval);
  }; // forever
}

DatumP Parser::tokenizeListWithPrompt(const QString &prompt, int level,
                                      bool makeArray, bool shouldRemoveComments,
                                      QTextStream *readStream) {
  DatumP lineP;
  static QString src;
  static QString::iterator iter;

  if (level == 0) {
    lineP = readwordWithPrompt(prompt, readStream);

    if (lineP == nothing) return nothing;

    src = lineP.wordValue()->rawValue();
    iter = src.begin();
  }
  List *retval = List::alloc();
  DatumP retvalP(retval);
  QString currentWord = "";

  forever {
    bool isVbarred = false;
    bool isCurrentWordVbarred = false;

    while (iter != src.end()) {
      ushort c = iter->unicode();
      ++iter;

      if (isVbarred) {
        if (c == '|') {
          isVbarred = false;
          continue;
        }
        currentWord.push_back(charToRaw(c));
        continue;
      }
      if (c == '|') {
        isVbarred = true;
        isCurrentWordVbarred = true;
        continue;
      }

      if (c == '~') {
        // If this is the last character of the line then jump to the beginning
        // of the next line
        QString::iterator lookAhead = iter;
        while (*lookAhead == ' ')
          ++lookAhead;
        if (*lookAhead == '\n') {
          ++lookAhead;
          iter = lookAhead;
          continue;
        }
      }
      if (((c == ';') ||
           ((c == '#') && (iter != src.end()) && (iter->unicode() == '!'))) &&
          shouldRemoveComments) {
        // This is a comment
        while ((iter != src.end()) && (*iter != '\n'))
          ++iter;
        // Consume the eol
        if (iter != src.end())
          ++iter;
        continue;
      }
      if ((c == '#') && (iter != src.end()) && (iter->unicode() == '!') &&
          shouldRemoveComments) {
        // This is a comment
        while ((iter != src.end()) && (*iter != '\n'))
          ++iter;
        // Consume the eol
        if (iter != src.end())
          ++iter;
        continue;
      }
      if ((c == ' ') || (c == '\t') || (c == '[') || (c == ']') || (c == '{') ||
          (c == '}')) {
        // This is a delimiter
        if (currentWord.size() > 0) {
          retval->append(DatumP(currentWord, isCurrentWordVbarred));
          currentWord = "";
          isCurrentWordVbarred = false;
        }
        switch (c) {
        case '[':
          retval->append(tokenizeListWithPrompt(
              "", level + 1, false, shouldRemoveComments, readStream));
          break;
        case ']':
          if ((level == 0) || makeArray) {
            Error::unexpectedCloseSquare();
          }
          return retvalP;
        case '}': {
          if ((level == 0) || !makeArray) {
            Error::unexpectedCloseBrace();
          }
          int origin = 1;
          // See if array has a custom origin
          if (*iter == '@') {
            QString originStr = "";
            ++iter;
            while ((*iter >= '0') && (*iter <= '9')) {
              originStr += *iter;
              ++iter;
            }
            origin = originStr.toInt();
          }
          Array *ary = Array::alloc(origin, retval);
          return DatumP(ary);
        }
        case '{':
          retval->append(tokenizeListWithPrompt(
              "", level + 1, true, shouldRemoveComments, readStream));
          break;
        default:
          break;
        }
      } else {
        currentWord.push_back(c);
      }
    }
    // This is the end of the read. Add the last word to the list.
    if (currentWord.size() > 0) {
      retval->append(DatumP(currentWord, isCurrentWordVbarred));
      currentWord = "";
    }

    // If this is the base-level list then we can just return
    if (level == 0)
      return DatumP(retval);

    // Get some more source material if we can
    if (makeArray)
      lineP = readwordWithPrompt("{ ", readStream);
    else
      lineP = readwordWithPrompt("[ ", readStream);
    if (lineP != nothing) {
      src = lineP.wordValue()->rawValue();
      iter = src.begin();
      continue;
    }
    // We have exhausted our source. Return what we have.
    if (makeArray) {
      Array *ary = Array::alloc(1, retval);
      return DatumP(ary);
    }
    return retvalP;
  } // /forever
}

DatumP Parser::readlistWithPrompt(const QString &prompt,
                                  bool shouldRemoveComments,
                                  QTextStream *readStream) {
  listSourceText.listValue()->clear();
  isReadingList = true;
  DatumP retval;
  try {
    retval = tokenizeListWithPrompt(prompt, 0, false, shouldRemoveComments,
                                    readStream);
  } catch (Error *e) {
    isReadingList = false;
    throw e;
  }
  isReadingList = false;
  return retval;
}

DatumP Parser::lastReadListSource() {
  DatumP retval = listSourceText;
  listSourceText = List::alloc();
  return retval;
}

void Parser::runparseSpecialchars(void) {
  QString retval = *runparseCIter;
  ++runparseCIter;
  if (runparseCIter != runparseCEnd) {
    QChar c = *runparseCIter;
    // there are some cases where special chars are combined
    if (((retval == "<") && (c == '=')) || ((retval == "<") && (c == '>')) ||
        ((retval == ">") && (c == '='))) {
      retval += c;
      ++runparseCIter;
    }
  }
  runparseRetval->append(DatumP(retval));
}

void Parser::runparseString() {
  QString retval = "";

  if (*runparseCIter == '?') {
    retval = "?";
    ++runparseCIter;
    DatumP number = runparseNumber();
    if (number != nothing) {
      runparseRetval->append(DatumP(QString("(")));
      runparseRetval->append(DatumP(QString("?")));
      runparseRetval->append(number);
      runparseRetval->append(DatumP(QString(")")));
      return;
    }
  }

  while ((runparseCIter != runparseCEnd) &&
         (!specialChars.contains(*runparseCIter))) {
    retval += *runparseCIter;
    ++runparseCIter;
  }
  runparseRetval->append(DatumP(retval, isRunparseSourceSpecial));
}

void Parser::runparseMinus() {
  QString::iterator nextCharIter = runparseCIter;
  ++nextCharIter;
  if (nextCharIter == runparseCEnd) {
    runparseSpecialchars();
    return;
  }

  DatumP number = runparseNumber();
  if (number != nothing) {
    runparseRetval->append(number);
    return;
  }

  // This is a minus function
  runparseRetval->append(DatumP(QString("0")));
  runparseRetval->append(DatumP(QString("--")));
  // discard the minus
  ++runparseCIter;
}

DatumP Parser::runparseNumber() {
  if (runparseCIter == runparseCEnd)
    return nothing;
  QString::iterator iter = runparseCIter;
  QString result = "";
  bool hasDigit = false;
  QChar c = *iter;
  if (c == '-') {
    result = "-";
    ++iter;
  }

  if (iter == runparseCEnd)
    return nothing;
  c = *iter;
  while (c.isDigit()) {
    result += c;
    ++iter;
    if (iter == runparseCEnd)
      goto numberSuccessful;
    c = *iter;
    hasDigit = true;
  }
  if (c == '.') {
    result += c;
    ++iter;
    if ((iter == runparseCEnd) && hasDigit)
      goto numberSuccessful;
    c = *iter;
  }
  while (c.isDigit()) {
    result += c;
    ++iter;
    if (iter == runparseCEnd)
      goto numberSuccessful;
    c = *iter;
    hasDigit = true;
  }

  if (!hasDigit)
    return nothing;
  hasDigit = false;
  if ((c == 'e') || (c == 'E')) {
    result += c;
    ++iter;
    if (iter == runparseCEnd)
      return nothing;
    c = *iter;
  } else {
    goto numberSuccessful;
  }

  if ((c == '+') || (c == '-')) {
    result += c;
    ++iter;
    if (iter == runparseCEnd)
      return nothing;
    c = *iter;
  }
  while (c.isDigit()) {
    result += c;
    ++iter;
    hasDigit = true;
    if (iter == runparseCEnd)
      goto numberSuccessful;
    c = *iter;
  }

  if (!hasDigit)
    return nothing;

  // at this point we have a number. If there is anything else here then we
  // don't have a number
  if (!specialChars.contains(c))
    return nothing;

numberSuccessful:
  double value = result.toDouble();
  runparseCIter = iter;
  return DatumP(value);
}

void Parser::runparseQuotedWord() {
  QString retval = "";
  while ((runparseCIter != runparseCEnd) && (*runparseCIter != '(') &&
         (*runparseCIter != ')')) {
    retval += *runparseCIter;
    ++runparseCIter;
  }
  runparseRetval->append(DatumP(retval, isRunparseSourceSpecial));
}

/*
     * RUNPARSE wordorlist

        outputs the list that would result if the input word or list were
        entered as an instruction line; characters such as infix operators
        and parentheses are separate members of the output.  Note that
        sublists of a runparsed list are not themselves runparsed.
     */
DatumP Parser::runparse(DatumP src) {
  if (src.isWord()) {
    QString text = src.wordValue()->rawValue();
    QTextStream srcStream(&text, QIODevice::ReadOnly);
    src = readlistWithPrompt("", false, &srcStream);
  }
  runparseRetval = List::alloc();
  ListIterator iter = src.listValue()->newIterator();

  while (iter.elementExists()) {
    DatumP element = iter.element();
    if (element.isWord()) {
      QString oldWord = element.wordValue()->rawValue();
      isRunparseSourceSpecial = element.wordValue()->isForeverSpecial;

      runparseCIter = oldWord.begin();
      runparseCEnd = oldWord.end();
      while (runparseCIter != runparseCEnd) {
        QChar c = *runparseCIter;
        if (specialChars.contains(c)) {
          if ((c == '-') && (runparseCIter == oldWord.begin()) &&
              (oldWord != "-"))
            runparseMinus();
          else
            runparseSpecialchars();
          continue;
        }
        if (c == '"') {
          runparseQuotedWord();
          continue;
        }

        DatumP number = runparseNumber();
        if (number == nothing) {
          runparseString();
        } else {
          runparseRetval->append(number);
        }
      } // while (cIter != oldWord.end())
    } else {
      // The element is not a word so we'll just push back whatever it was
      runparseRetval->append(element);
    }
  }
  return DatumP(runparseRetval);
}

QList<DatumP> *Parser::astFromList(List *aList) {
  if (aList->astParseTimeStamp <= lastProcedureCreatedTimestamp) {
    aList->astParseTimeStamp = QDateTime::currentMSecsSinceEpoch();

    DatumP runParsedList = runparse(aList);

    listIter = runParsedList.listValue()->newIterator();
    aList->astList.clear();
    advanceToken();

    try {
        while (currentToken != nothing) {
            aList->astList.push_back(parseExp());
        }
    } catch (Error *e) {
        // If there was a syntax error, then delete the parsed list
        aList->astList.clear();
        aList->astParseTimeStamp = 0;
        throw e;
    }
  }
  return &aList->astList;
}

// Below methods parse into ASTs

DatumP Parser::parseExp() {
  DatumP left = parseSumexp();
  while ((currentToken.isa() == Datum::wordType) &&
         ((currentToken.wordValue()->printValue() == "=") ||
          (currentToken.wordValue()->printValue() == "<>") ||
          (currentToken.wordValue()->printValue() == ">") ||
          (currentToken.wordValue()->printValue() == "<") ||
          (currentToken.wordValue()->printValue() == ">=") ||
          (currentToken.wordValue()->printValue() == "<="))) {
    DatumP op = currentToken;
    advanceToken();
    DatumP right = parseSumexp();

    DatumP node = DatumP(ASTNode::alloc(op));
    if (right == nothing)
      Error::notEnough(op);

    if (op.wordValue()->printValue() == "=") {
      node.astnodeValue()->kernel = &Kernel::excEqualp;
    } else if (op.wordValue()->printValue() == "<>") {
      node.astnodeValue()->kernel = &Kernel::excNotequal;
    } else if (op.wordValue()->printValue() == "<") {
      node.astnodeValue()->kernel = &Kernel::excLessp;
    } else if (op.wordValue()->printValue() == ">") {
      node.astnodeValue()->kernel = &Kernel::excGreaterp;
    } else if (op.wordValue()->printValue() == "<=") {
      node.astnodeValue()->kernel = &Kernel::excLessequalp;
    } else {
      node.astnodeValue()->kernel = &Kernel::excGreaterequalp;
    }
    node.astnodeValue()->addChild(left);
    node.astnodeValue()->addChild(right);
    left = node;
  }
  return left;
}

DatumP Parser::parseSumexp() {
  DatumP left = parseMulexp();
  while ((currentToken.isa() == Datum::wordType) &&
         ((currentToken.wordValue()->printValue() == "+") ||
          (currentToken.wordValue()->printValue() == "-"))) {
    DatumP op = currentToken;
    advanceToken();
    DatumP right = parseMulexp();

    DatumP node = DatumP(ASTNode::alloc(op));
    if (right == nothing)
      Error::notEnough(op);

    if (op.wordValue()->printValue() == "+") {
      node.astnodeValue()->kernel = &Kernel::excSum;
    } else {
      node.astnodeValue()->kernel = &Kernel::excDifference;
    }
    node.astnodeValue()->addChild(left);
    node.astnodeValue()->addChild(right);
    left = node;
  }
  return left;
}

DatumP Parser::parseMulexp() {
  DatumP left = parseminusexp();
  while ((currentToken.isa() == Datum::wordType) &&
         ((currentToken.wordValue()->printValue() == "*") ||
          (currentToken.wordValue()->printValue() == "/") ||
          (currentToken.wordValue()->printValue() == "%"))) {
    DatumP op = currentToken;
    advanceToken();
    DatumP right = parseminusexp();

    DatumP node = DatumP(ASTNode::alloc(op));
    if (right == nothing)
      Error::notEnough(op);

    if (op.wordValue()->printValue() == "*") {
      node.astnodeValue()->kernel = &Kernel::excProduct;
    } else if (op.wordValue()->printValue() == "/") {
      node.astnodeValue()->kernel = &Kernel::excQuotient;
    } else {
      node.astnodeValue()->kernel = &Kernel::excRemainder;
    }
    node.astnodeValue()->addChild(left);
    node.astnodeValue()->addChild(right);
    left = node;
  }
  return left;
}

DatumP Parser::parseminusexp() {
  DatumP left = parseTermexp();
  while ((currentToken.isa() == Datum::wordType) &&
         ((currentToken.wordValue()->printValue() == "--"))) {
    DatumP op = currentToken;
    advanceToken();
    DatumP right = parseTermexp();

    DatumP node = DatumP(ASTNode::alloc(op));
    if (right == nothing)
      Error::notEnough(op);

    node.astnodeValue()->kernel = &Kernel::excDifference;
    node.astnodeValue()->addChild(left);
    node.astnodeValue()->addChild(right);
    left = node;
  }
  return left;
}

DatumP Parser::parseTermexp() {
  if (currentToken == nothing)
    return nothing;

  if (currentToken.isa() == Datum::listType) {
    DatumP node(ASTNode::alloc(k.word()));
    node.astnodeValue()->kernel = &Kernel::executeLiteral;
    node.astnodeValue()->addChild(currentToken);
    advanceToken();
    return node;
  }

  if (currentToken.isa() == Datum::arrayType) {
    DatumP node(ASTNode::alloc(k.array()));
    node.astnodeValue()->kernel = &Kernel::executeLiteral;
    node.astnodeValue()->addChild(currentToken);
    advanceToken();
    return node;
  }

  Q_ASSERT(currentToken.isa() == Datum::wordType);

  // See if it's an open paren
  if (currentToken.wordValue()->printValue() == "(") {
    // This may be an expression or a vararg function
    DatumP retval;

    advanceToken();
    if ((currentToken != nothing) && currentToken.isWord()) {
      QString cmdString = currentToken.wordValue()->keyValue();
      QChar firstChar = (cmdString)[0];
      if ((firstChar != '"') && (firstChar != ':') &&
          ((firstChar < '0') || (firstChar > '9')) &&
          !specialChars.contains(firstChar)) {
        retval = parseCommand(true);
      } else {
        retval = parseExp();
      }
    } else {
      retval = parseExp();
    }

    // Make sure there is a closing paren
    if ((!currentToken.isWord()) ||
        (currentToken.wordValue()->printValue() != ")")) {

      Error::parenNf();
    }

    advanceToken();
    retval = parseStopIfExists(retval);
    return retval;
  }

  QChar firstChar = currentToken.wordValue()->rawValue().at(0);
  if ((firstChar == '"') || (firstChar == ':')) {
    QString name = currentToken.wordValue()->rawValue().right(
        currentToken.wordValue()->rawValue().size() - 1);
    if (!currentToken.wordValue()->isForeverSpecial) {
      rawToChar(name);
    }
    if (firstChar == '"') {
      DatumP node(ASTNode::alloc(k.quotedname()));
      node.astnodeValue()->kernel = &Kernel::executeLiteral;
      node.astnodeValue()->addChild(
          DatumP(DatumP(name, currentToken.wordValue()->isForeverSpecial)));
      advanceToken();
      return node;
    } else {
      DatumP node(ASTNode::alloc(k.valueof()));
      node.astnodeValue()->kernel = &Kernel::executeValueOf;
      node.astnodeValue()->addChild(DatumP(name));
      advanceToken();
      return node;
    }
  }

  // See if it's a number
  double number = currentToken.wordValue()->numberValue();
  if (currentToken.wordValue()->didNumberConversionSucceed()) {
    DatumP node(ASTNode::alloc(k.number()));
    node.astnodeValue()->kernel = &Kernel::executeLiteral;
    node.astnodeValue()->addChild(DatumP(number));
    advanceToken();
    return node;
  }

  // If all else fails, it must be a function with the default number of params
  return parseStopIfExists(parseCommand(false));
}

// First, check to see that the next token is indeed the STOP command.
// If it is, create a new node for STOP, and add the command node
// as a child to the STOP node.
DatumP Parser::parseStopIfExists(DatumP command)
{
    if ((currentToken != nothing) && currentToken.isWord()
            && (currentToken.wordValue()->keyValue() == k.stop())) {
        // Consume and create the STOP node
        DatumP stopCmd = parseCommand(false);
        stopCmd.astnodeValue()->addChild(command);
        return stopCmd;
    }
    return command;
}

DatumP Parser::astnodeWithLiterals(DatumP cmd, DatumP params) {
  int minParams = 0, maxParams = 0, defaultParams = 0;
  DatumP node = astnodeFromCommand(cmd, minParams, defaultParams, maxParams);

  int countOfChildren = params.listValue()->size();
  if (countOfChildren < minParams)
    Error::notEnough(cmd);
  if ((countOfChildren > maxParams) && (maxParams != -1))
    Error::tooMany(cmd);

  ListIterator iter = params.listValue()->newIterator();
  while (iter.elementExists()) {
    DatumP p = iter.element();
    DatumP a = DatumP(ASTNode::alloc(k.literal()));
    a.astnodeValue()->kernel = &Kernel::executeLiteral;
    a.astnodeValue()->addChild(p);
    node.astnodeValue()->addChild(a);
  }
  return node;
}

DatumP Parser::astnodeFromCommand(DatumP cmdP, int &minParams,
                                  int &defaultParams, int &maxParams) {
  QString cmdString = cmdP.wordValue()->keyValue();

  Cmd_t command;
  DatumP node = DatumP(ASTNode::alloc(cmdP));
  if (procedures.contains(cmdString)) {
    DatumP procBody = procedures[cmdString];
    if (procBody.procedureValue()->isMacro)
      node.astnodeValue()->kernel = &Kernel::executeMacro;
    else
      node.astnodeValue()->kernel = &Kernel::executeProcedure;
    node.astnodeValue()->addChild(procBody);
    defaultParams = procBody.procedureValue()->defaultNumber;
    minParams = procBody.procedureValue()->countOfMinParams;
    maxParams = procBody.procedureValue()->countOfMaxParams;
  } else if (stringToCmd.contains(cmdString) ||
             primitiveAlternateNames.contains(cmdString)) {
    command = primitiveAlternateNames.contains(cmdString)
                  ? primitiveAlternateNames[cmdString]
                  : stringToCmd[cmdString];
    defaultParams = command.countOfDefaultParams;
    minParams = command.countOfMinParams;
    maxParams = command.countOfMaxParams;
    node.astnodeValue()->kernel = command.method;
  } else if (cmdString.startsWith(k.set()) && (cmdString.size() > 3) &&
             kernel->varALLOWGETSET()) {
    node.astnodeValue()->kernel = &Kernel::excSetfoo;
    defaultParams = 1;
    minParams = 1;
    maxParams = 1;
  } else if (kernel->varALLOWGETSET()) {
    node.astnodeValue()->kernel = &Kernel::excFoo;
    defaultParams = 0;
    minParams = 0;
    maxParams = 0;
  } else {
    Error::noHow(cmdP);
  }
  return node;
}

DatumP Parser::parseCommand(bool isVararg) {
  if (currentToken == nothing)
    return nothing;
  DatumP cmdP = currentToken;
  QString cmdString = cmdP.wordValue()->keyValue();

  if (cmdString == ")")
    Error::unexpectedCloseParen();

  int defaultParams = 0;
  int minParams = 0;
  int maxParams = 0;

  DatumP node = astnodeFromCommand(cmdP, minParams, defaultParams, maxParams);

  advanceToken();

  int countOfChildren = 0;
  // isVararg: read all parameters until ')'
  if (isVararg) {
    while ((currentToken != nothing) &&
           ((!currentToken.isWord()) ||
            (currentToken.wordValue()->printValue() != ")"))) {
      DatumP child;
      if (minParams < 0) {
        child = currentToken;
        advanceToken();
      } else {
        child = parseExp();
      }
      node.astnodeValue()->addChild(child);
      ++countOfChildren;
    }
  } else if (defaultParams <
             0) { // "Special form": read all parameters until EOL
    while (currentToken != nothing) {
      DatumP child;
      if (minParams < 0) {
        child = currentToken;
        advanceToken();
      } else {
        child = parseExp();
      }
      node.astnodeValue()->addChild(child);
      ++countOfChildren;
    }

  } else { // Read in the default number of params
    for (int i = defaultParams; i > 0; --i) {
      if (currentToken == nothing)
        Error::notEnough(cmdP);
      DatumP child = parseExp();
      node.astnodeValue()->addChild(child);
      ++countOfChildren;
    }
  }

  if (countOfChildren < minParams)
    Error::notEnough(node.astnodeValue()->nodeName);
  if ((countOfChildren > maxParams) && (maxParams > -1))
    Error::tooMany(node.astnodeValue()->nodeName);

  return node;
}

void Parser::advanceToken() {
  if (listIter.elementExists()) {
    currentToken = listIter.element();
  } else {
    currentToken = nothing;
  }
}

bool Parser::isProcedure(QString procname) {
  return (primitiveAlternateNames.contains(procname) ||
          stringToCmd.contains(procname) || procedures.contains(procname));
}

bool Parser::isMacro(QString procname) {
  if (procedures.contains(procname)) {
    DatumP procedure = procedures[procname];
    return procedure.procedureValue()->isMacro;
  }
  return false;
}

bool Parser::isPrimitive(QString procname) {
  return (primitiveAlternateNames.contains(procname) ||
          stringToCmd.contains(procname));
}

bool Parser::isDefined(QString procname) {
  return (procedures.contains(procname));
}

DatumP Parser::allProcedureNames(showContents_t showWhat) {
  List *retval = List::alloc();

  for (auto &name : procedures.keys()) {

    if (shouldInclude(showWhat, name))
      retval->append(DatumP(name));
  }
  return DatumP(retval);
}

void Parser::eraseAllProcedures() {
  for (auto &iter : procedures.keys()) {
    if (!isBuried(iter)) {
      procedures.remove(iter);
    }
  }
}

DatumP Parser::allPrimitiveProcedureNames() {
  List *retval = List::alloc();

  for (auto name : stringToCmd.keys()) {
    retval->append(DatumP(name));
  }
  return DatumP(retval);
}

DatumP Parser::arity(DatumP nameP) {
  int minParams, defParams, maxParams;
  QString procname = nameP.wordValue()->keyValue();

  if (procedures.contains(procname)) {
    DatumP command = procedures[procname];
    minParams = command.procedureValue()->countOfMinParams;
    defParams = command.procedureValue()->defaultNumber;
    maxParams = command.procedureValue()->countOfMaxParams;
  } else if (primitiveAlternateNames.contains(procname) ||
             stringToCmd.contains(procname)) {
    Cmd_t command = primitiveAlternateNames.contains(procname)
                        ? primitiveAlternateNames[procname]
                        : stringToCmd[procname];
    minParams = command.countOfMinParams;
    defParams = command.countOfDefaultParams;
    maxParams = command.countOfMaxParams;
  } else {
    Error::noHow(nameP);
    return nothing;
  }

  List *retval = List::alloc();
  retval->append(DatumP(minParams));
  retval->append(DatumP(defParams));
  retval->append(DatumP(maxParams));
  return DatumP(retval);
}

QString Parser::unreadDatum(DatumP aDatum, bool isInList) {
  switch (aDatum.isa()) {
  case Datum::wordType:
    return unreadWord(aDatum.wordValue(), isInList);
    break;
  case Datum::listType:
    return unreadList(aDatum.listValue(), isInList);
  case Datum::arrayType:
    return unreadArray(aDatum.arrayValue());
  default:
    Q_ASSERT(false);
  }
  return "";
}

QString Parser::unreadList(List *aList, bool isInList) {
  QString retval("");
  if (isInList)
    retval = "[";
  ListIterator i = aList->newIterator();
  while (i.elementExists()) {
    DatumP e = i.element();
    if ((retval != "[") && (retval != ""))
      retval.append(' ');
    retval.append(unreadDatum(e, true));
  }
  if (isInList)
    retval.append(']');
  return retval;
}

QString Parser::unreadArray(Array *anArray) {
  QString retval("{");
  ArrayIterator i = anArray->newIterator();
  while (i.elementExists()) {
    DatumP e = i.element();
    if (retval != "{")
      retval.append(' ');
    retval.append(unreadDatum(e, true));
  }
  retval.append('}');
  return retval;
}

QString Parser::unreadWord(Word *aWord, bool isInList) {
  aWord->numberValue();
  if (aWord->didNumberConversionSucceed())
    return aWord->showValue();

  QString retval("");
  if (!isInList)
    retval = "\"";

  const QString src = aWord->showValue();
  if (src.size() == 0)
    return retval + "||";

  if (aWord->isForeverSpecial) {
    retval.append('|');
    for (auto iter = src.begin(); iter != src.end(); ++iter) {
      QChar letter = *iter;
      if ((iter == src.begin()) && (letter == '"')) {
        retval = "\"|";
      } else {
        if (letter == '|') {
          retval.append('\\');
        }
        retval.append(letter);
      }
    }
    retval.append('|');
  } else {
    for (auto letter : src) {
      if ((letter == ' ') || (letter == '[') || (letter == ']') ||
          (letter == '{') || (letter == '}') || (letter == '|') ||
          (letter == '\n')) {
        retval.append('\\');
      }
      retval.append(letter);
    }
  }
  return retval;
}

QString Parser::printoutDatum(DatumP aDatum) {
  switch (aDatum.isa()) {
  case Datum::wordType:
    return unreadWord(aDatum.wordValue());
    break;
  case Datum::listType:
    return unreadList(aDatum.listValue(), true);
  case Datum::arrayType:
    return unreadArray(aDatum.arrayValue());
  default:
    Q_ASSERT(false);
  }
  return "";
}

Parser::Parser(Kernel *aKernel) {
  lastProcedureCreatedTimestamp = QDateTime::currentMSecsSinceEpoch();
  kernel = aKernel;
  listSourceText = List::alloc();
  if (stringToCmd.size() > 0)
    return;

  // DATA STRUCTURE PRIMITIVES (MIN, default, MAX)
  // (MIN = -1)     = All parameters are read as Words, e.g. "TO PROC :p1"
  // becomes ["TO", "PROC", ":p1"]
  // (default = -1) = All parameters are consumed
  // until end of line
  // (MAX = -1)     = All parameters are consumed within
  // parens
  stringToCmd[k.cword()] = {&Kernel::excWord, 0, 2, -1};
  stringToCmd[k.clist()] = {&Kernel::excList, 0, 2, -1};
  stringToCmd[k.sentence()] = {&Kernel::excSentence, 0, 2, -1};
  stringToCmd[k.se()] = {&Kernel::excSentence, 0, 2, -1};
  stringToCmd[k.fput()] = {&Kernel::excFput, 2, 2, 2};
  stringToCmd[k.lput()] = {&Kernel::excLput, 2, 2, 2};
  stringToCmd[k.carray()] = {&Kernel::excArray, 1, 1, 2};
  stringToCmd[k.listtoarray()] = {&Kernel::excListtoarray, 1, 1, 2};
  stringToCmd[k.arraytolist()] = {&Kernel::excArraytolist, 1, 1, 1};
  stringToCmd[k.readlist()] = {&Kernel::excReadlist, 0, 0, 0};
  stringToCmd[k.rl()] = {&Kernel::excReadlist, 0, 0, 0};
  stringToCmd[k.readword()] = {&Kernel::excReadword, 0, 0, 0};
  stringToCmd[k.rw()] = {&Kernel::excReadword, 0, 0, 0};
  stringToCmd[k.readrawline()] = {&Kernel::excReadrawline, 0, 0, 0};
  stringToCmd[k.readchar()] = {&Kernel::excReadchar, 0, 0, 0};
  stringToCmd[k.rc()] = {&Kernel::excReadchar, 0, 0, 0};
  stringToCmd[k.readchars()] = {&Kernel::excReadchars, 1, 1, 1};
  stringToCmd[k.rcs()] = {&Kernel::excReadchars, 1, 1, 1};
  stringToCmd[k.shell()] = {&Kernel::excShell, 1, 1, 2};

  stringToCmd[k.setprefix()] = {&Kernel::excSetprefix, 1, 1, 1};
  stringToCmd[k.prefix()] = {&Kernel::excPrefix, 0, 0, 0};
  stringToCmd[k.openread()] = {&Kernel::excOpenread, 1, 1, 1};
  stringToCmd[k.openwrite()] = {&Kernel::excOpenwrite, 1, 1, 1};
  stringToCmd[k.openappend()] = {&Kernel::excOpenappend, 1, 1, 1};
  stringToCmd[k.openupdate()] = {&Kernel::excOpenupdate, 1, 1, 1};
  stringToCmd[k.allopen()] = {&Kernel::excAllopen, 0, 0, 0};
  stringToCmd[k.setread()] = {&Kernel::excSetread, 1, 1, 1};
  stringToCmd[k.setwrite()] = {&Kernel::excSetwrite, 1, 1, 1};
  stringToCmd[k.reader()] = {&Kernel::excReader, 0, 0, 0};
  stringToCmd[k.writer()] = {&Kernel::excWriter, 0, 0, 0};
  stringToCmd[k.readpos()] = {&Kernel::excReadpos, 0, 0, 0};
  stringToCmd[k.writepos()] = {&Kernel::excWritepos, 0, 0, 0};
  stringToCmd[k.setreadpos()] = {&Kernel::excSetreadpos, 1, 1, 1};
  stringToCmd[k.setwritepos()] = {&Kernel::excSetwritepos, 1, 1, 1};
  stringToCmd[k.eofp()] = {&Kernel::excEofp, 0, 0, 0};
  stringToCmd[k.eofq()] = {&Kernel::excEofp, 0, 0, 0};
  stringToCmd[k.keyp()] = {&Kernel::excKeyp, 0, 0, 0};
  stringToCmd[k.keyq()] = {&Kernel::excKeyp, 0, 0, 0};
  stringToCmd[k.dribble()] = {&Kernel::excDribble, 1, 1, 1};
  stringToCmd[k.nodribble()] = {&Kernel::excNodribble, 0, 0, 0};

  stringToCmd[k.cleartext()] = {&Kernel::excCleartext, 0, 0, 0};
  stringToCmd[k.ct()] = {&Kernel::excCleartext, 0, 0, 0};
  stringToCmd[k.cursorinsert()] = {ifGUI(&Kernel::excCursorInsert), 0, 0, 0};
  stringToCmd[k.cursoroverwrite()] = {ifGUI(&Kernel::excCursorOverwrite), 0, 0, 0};
  stringToCmd[k.cursormode()] = {ifGUI(&Kernel::excCursorMode), 0, 0, 0};

  stringToCmd[k.close()] = {&Kernel::excClose, 1, 1, 1};
  stringToCmd[k.closeall()] = {&Kernel::excCloseall, 0, 0, 0};
  stringToCmd[k.erasefile()] = {&Kernel::excErasefile, 1, 1, 1};
  stringToCmd[k.erf()] = {&Kernel::excErasefile, 1, 1, 1};

  stringToCmd[k.first()] = {&Kernel::excFirst, 1, 1, 1};
  stringToCmd[k.last()] = {&Kernel::excLast, 1, 1, 1};
  stringToCmd[k.butfirst()] = {&Kernel::excButfirst, 1, 1, 1};
  stringToCmd[k.bf()] = {&Kernel::excButfirst, 1, 1, 1};
  stringToCmd[k.firsts()] = {&Kernel::excFirsts, 1, 1, 1};
  stringToCmd[k.butfirsts()] = {&Kernel::excButfirsts, 1, 1, 1};
  stringToCmd[k.bfs()] = {&Kernel::excButfirsts, 1, 1, 1};
  stringToCmd[k.butlast()] = {&Kernel::excButlast, 1, 1, 1};
  stringToCmd[k.bl()] = {&Kernel::excButlast, 1, 1, 1};
  stringToCmd[k.item()] = {&Kernel::excItem, 2, 2, 2};

  stringToCmd[k.setitem()] = {&Kernel::excSetitem, 3, 3, 3};
  stringToCmd[k.dsetfirst()] = {&Kernel::excDotSetfirst, 2, 2, 2};
  stringToCmd[k.dsetbf()] = {&Kernel::excDotSetbf, 2, 2, 2};
  stringToCmd[k.dsetitem()] = {&Kernel::excDotSetitem, 3, 3, 3};

  stringToCmd[k.wordp()] = {&Kernel::excWordp, 1, 1, 1};
  stringToCmd[k.wordq()] = {&Kernel::excWordp, 1, 1, 1};
  stringToCmd[k.listp()] = {&Kernel::excListp, 1, 1, 1};
  stringToCmd[k.listq()] = {&Kernel::excListp, 1, 1, 1};
  stringToCmd[k.arrayp()] = {&Kernel::excArrayp, 1, 1, 1};
  stringToCmd[k.arrayq()] = {&Kernel::excArrayp, 1, 1, 1};
  stringToCmd[k.emptyp()] = {&Kernel::excEmptyp, 1, 1, 1};
  stringToCmd[k.emptyq()] = {&Kernel::excEmptyp, 1, 1, 1};
  stringToCmd[k.equalp()] = {&Kernel::excEqualp, 2, 2, 2};
  stringToCmd[k.equalq()] = {&Kernel::excEqualp, 2, 2, 2};
  stringToCmd[k.notequalp()] = {&Kernel::excNotequal, 2, 2, 2};
  stringToCmd[k.notequalq()] = {&Kernel::excNotequal, 2, 2, 2};
  stringToCmd[k.beforep()] = {&Kernel::excBeforep, 2, 2, 2};
  stringToCmd[k.beforeq()] = {&Kernel::excBeforep, 2, 2, 2};
  stringToCmd[k.deq()] = {&Kernel::excDotEq, 2, 2, 2};
  stringToCmd[k.memberp()] = {&Kernel::excMemberp, 2, 2, 2};
  stringToCmd[k.memberq()] = {&Kernel::excMemberp, 2, 2, 2};
  stringToCmd[k.substringp()] = {&Kernel::excSubstringp, 2, 2, 2};
  stringToCmd[k.substringq()] = {&Kernel::excSubstringp, 2, 2, 2};
  stringToCmd[k.numberp()] = {&Kernel::excNumberp, 1, 1, 1};
  stringToCmd[k.numberq()] = {&Kernel::excNumberp, 1, 1, 1};
  stringToCmd[k.vbarredp()] = {&Kernel::excVbarredp, 1, 1, 1};
  stringToCmd[k.vbarredq()] = {&Kernel::excVbarredp, 1, 1, 1};

  stringToCmd[k.count()] = {&Kernel::excCount, 1, 1, 1};
  stringToCmd[k.ascii()] = {&Kernel::excAscii, 1, 1, 1};
  stringToCmd[k.rawascii()] = {&Kernel::excRawascii, 1, 1, 1};
  stringToCmd[k.kchar()] = {&Kernel::excChar, 1, 1, 1};
  stringToCmd[k.member()] = {&Kernel::excMember, 2, 2, 2};
  stringToCmd[k.lowercase()] = {&Kernel::excLowercase, 1, 1, 1};
  stringToCmd[k.uppercase()] = {&Kernel::excUppercase, 1, 1, 1};
  stringToCmd[k.standout()] = {ifGUI(&Kernel::excStandout), 1, 1, 1};
  stringToCmd[k.parse()] = {&Kernel::excParse, 1, 1, 1};
  stringToCmd[k.runparse()] = {&Kernel::excRunparse, 1, 1, 1};

  stringToCmd[k.minus()] = {&Kernel::excMinus, 1, 1, 1};
  stringToCmd["-"] = {&Kernel::excMinus, 1, 1, 1};
  stringToCmd["--"] = {&Kernel::excMinus, 1, 1, 1};

  stringToCmd[k.print()] = {&Kernel::excPrint, 0, 1, -1};
  stringToCmd[k.pr()] = {&Kernel::excPrint, 0, 1, -1};
  stringToCmd[k.type()] = {&Kernel::excType, 0, 1, -1};
  stringToCmd[k.show()] = {&Kernel::excShow, 0, 1, -1};
  stringToCmd[k.make()] = {&Kernel::excMake, 2, 2, 2};
  stringToCmd[k.repeat()] = {&Kernel::excRepeat, 2, 2, 2};
  stringToCmd[k.sqrt()] = {&Kernel::excSqrt, 1, 1, 1};
  stringToCmd[k.random()] = {&Kernel::excRandom, 1, 1, 2};
  stringToCmd[k.rerandom()] = {&Kernel::excNoop, 0, 0, 1};
  stringToCmd[k.thing()] = {&Kernel::excThing, 1, 1, 1};
  stringToCmd[k.wait()] = {&Kernel::excWait, 1, 1, 1};
  stringToCmd[k.setcursor()] = {ifGUI(&Kernel::excSetcursor), 1, 1, 1};
  stringToCmd[k.cursor()] = {ifGUI(&Kernel::excCursor), 0, 0, 0};
  stringToCmd[k.settextcolor()] = {ifGUI(&Kernel::excSettextcolor), 1, 2, 2};
  stringToCmd[k.settc()] = {ifGUI(&Kernel::excSettextcolor), 1, 2, 2};
  stringToCmd[k.increasefont()] = {ifGUI(&Kernel::excIncreasefont), 0, 0, 0};
  stringToCmd[k.decreasefont()] = {ifGUI(&Kernel::excDecreasefont), 0, 0, 0};
  stringToCmd[k.settextsize()] = {ifGUI(&Kernel::excSettextsize), 1, 1, 1};
  stringToCmd[k.textsize()] = {ifGUI(&Kernel::excTextsize), 0, 0, 0};
  stringToCmd[k.setfont()] = {ifGUI(&Kernel::excSetfont), 1, 1, 1};
  stringToCmd[k.font()] = {ifGUI(&Kernel::excFont), 0, 0, 0};
  stringToCmd[k.allfonts()] = {ifGUI(&Kernel::excAllfonts), 0, 0, 0};

  stringToCmd[k.forward()] = {ifGUI(&Kernel::excForward), 1, 1, 1};
  stringToCmd[k.fd()] = {ifGUI(&Kernel::excForward), 1, 1, 1};
  stringToCmd[k.back()] = {ifGUI(&Kernel::excBack), 1, 1, 1};
  stringToCmd[k.bk()] = {ifGUI(&Kernel::excBack), 1, 1, 1};
  stringToCmd[k.right()] = {ifGUI(&Kernel::excRight), 1, 1, 1};
  stringToCmd[k.rt()] = {ifGUI(&Kernel::excRight), 1, 1, 1};
  stringToCmd[k.left()] = {ifGUI(&Kernel::excLeft), 1, 1, 1};
  stringToCmd[k.lt()] = {ifGUI(&Kernel::excLeft), 1, 1, 1};
  stringToCmd[k.clearscreen()] = {ifGUI(&Kernel::excClearscreen), 0, 0, 0};
  stringToCmd[k.cs()] = {ifGUI(&Kernel::excClearscreen), 0, 0, 0};
  stringToCmd[k.clean()] = {ifGUI(&Kernel::excClean), 0, 0, 0};
  stringToCmd[k.penup()] = {ifGUI(&Kernel::excPenup), 0, 0, 0};
  stringToCmd[k.pu()] = {ifGUI(&Kernel::excPenup), 0, 0, 0};
  stringToCmd[k.pendown()] = {ifGUI(&Kernel::excPendown), 0, 0, 0};
  stringToCmd[k.pd()] = {ifGUI(&Kernel::excPendown), 0, 0, 0};
  stringToCmd[k.pendownp()] = {ifGUI(&Kernel::excPendownp), 0, 0, 0};
  stringToCmd[k.pendownq()] = {ifGUI(&Kernel::excPendownp), 0, 0, 0};
  stringToCmd[k.hideturtle()] = {ifGUI(&Kernel::excHideturtle), 0, 0, 0};
  stringToCmd[k.ht()] = {ifGUI(&Kernel::excHideturtle), 0, 0, 0};
  stringToCmd[k.showturtle()] = {ifGUI(&Kernel::excShowturtle), 0, 0, 0};
  stringToCmd[k.st()] = {ifGUI(&Kernel::excShowturtle), 0, 0, 0};
  // stringToCmd["SETXYZ"]         = {&Kernel::excSetXYZ, 3,3,3};
  stringToCmd[k.setxy()] = {ifGUI(&Kernel::excSetXY), 2, 2, 2};
  stringToCmd[k.setx()] = {ifGUI(&Kernel::excSetX), 1, 1, 1};
  stringToCmd[k.sety()] = {ifGUI(&Kernel::excSetY), 1, 1, 1};
  // stringToCmd["SETZ"]           = {&Kernel::excSetZ, 1,1,1};
  stringToCmd[k.setpos()] = {ifGUI(&Kernel::excSetpos), 1, 1, 1};
  stringToCmd[k.pos()] = {ifGUI(&Kernel::excPos), 0, 0, 1};
  stringToCmd[k.home()] = {ifGUI(&Kernel::excHome), 0, 0, 0};
  stringToCmd[k.heading()] = {ifGUI(&Kernel::excHeading), 0, 0, 1};
  stringToCmd[k.setheading()] = {ifGUI(&Kernel::excSetheading), 1, 1, 2};
  stringToCmd[k.seth()] = {ifGUI(&Kernel::excSetheading), 1, 1, 2};
  stringToCmd[k.arc()] = {ifGUI(&Kernel::excArc), 2, 2, 2};
  stringToCmd[k.towards()] = {ifGUI(&Kernel::excTowards), 1, 1, 1};
  stringToCmd[k.scrunch()] = {ifGUI(&Kernel::excScrunch), 0, 0, 0};
  stringToCmd[k.setscrunch()] = {ifGUI(&Kernel::excSetscrunch), 2, 2, 2};
  stringToCmd[k.label()] = {ifGUI(&Kernel::excLabel), 1, 1, 1};
  stringToCmd[k.labelheight()] = {ifGUI(&Kernel::excLabelheight), 0, 0, 0};
  stringToCmd[k.setlabelheight()] = {ifGUI(&Kernel::excSetlabelheight), 1, 1, 1};
  stringToCmd[k.shownp()] = {ifGUI(&Kernel::excShownp), 0, 0, 0};
  stringToCmd[k.shownq()] = {ifGUI(&Kernel::excShownp), 0, 0, 0};
  stringToCmd[k.setpencolor()] = {ifGUI(&Kernel::excSetpencolor), 1, 1, 1};
  stringToCmd[k.setpc()] = {ifGUI(&Kernel::excSetpencolor), 1, 1, 1};
  stringToCmd[k.pencolor()] = {ifGUI(&Kernel::excPencolor), 0, 0, 0};
  stringToCmd[k.pc()] = {ifGUI(&Kernel::excPencolor), 0, 0, 0};
  stringToCmd[k.setpalette()] = {ifGUI(&Kernel::excSetpalette), 2, 2, 2};
  stringToCmd[k.palette()] = {ifGUI(&Kernel::excPalette), 1, 1, 1};
  stringToCmd[k.background()] = {ifGUI(&Kernel::excBackground), 0, 0, 0};
  stringToCmd[k.bg()] = {ifGUI(&Kernel::excBackground), 0, 0, 0};
  stringToCmd[k.setbackground()] = {ifGUI(&Kernel::excSetbackground), 1, 1, 1};
  stringToCmd[k.setbg()] = {ifGUI(&Kernel::excSetbackground), 1, 1, 1};
  stringToCmd[k.savepict()] = {ifGUI(&Kernel::excSavepict), 1, 1, 1};

  stringToCmd[k.penpaint()] = {ifGUI(&Kernel::excPenpaint), 0, 0, 0};
  stringToCmd[k.ppt()] = {ifGUI(&Kernel::excPenpaint), 0, 0, 0};
  stringToCmd[k.penerase()] = {ifGUI(&Kernel::excPenerase), 0, 0, 0};
  stringToCmd[k.pe()] = {ifGUI(&Kernel::excPenerase), 0, 0, 0};
  stringToCmd[k.penreverse()] = {ifGUI(&Kernel::excPenreverse), 0, 0, 0};
  stringToCmd[k.px()] = {ifGUI(&Kernel::excPenreverse), 0, 0, 0};
  stringToCmd[k.penmode()] = {ifGUI(&Kernel::excPenmode), 0, 0, 0};
  stringToCmd[k.setpensize()] = {ifGUI(&Kernel::excSetpensize), 1, 1, 1};
  stringToCmd[k.pensize()] = {ifGUI(&Kernel::excPensize), 0, 0, 0};
  stringToCmd[k.filled()] = {ifGUI(&Kernel::excFilled), 2, 2, 2};

  stringToCmd[k.cwrap()] = {ifGUI(&Kernel::excWrap), 0, 0, 0};
  stringToCmd[k.cfence()] = {ifGUI(&Kernel::excFence), 0, 0, 0};
  stringToCmd[k.cwindow()] = {ifGUI(&Kernel::excWindow), 0, 0, 0};
  stringToCmd[k.turtlemode()] = {ifGUI(&Kernel::excTurtlemode), 0, 0, 0};

  stringToCmd[k.mousepos()] = {ifGUI(&Kernel::excMousepos), 0, 0, 0};
  stringToCmd[k.clickpos()] = {ifGUI(&Kernel::excClickpos), 0, 0, 0};
  stringToCmd[k.bounds()] = {ifGUI(&Kernel::excBounds), 0, 0, 0};
  stringToCmd[k.setbounds()] = {ifGUI(&Kernel::excSetbounds), 2, 2, 2};

  stringToCmd[k.ctextscreen()] = {ifGUI(&Kernel::excTextscreen), 0, 0, 0};
  stringToCmd[k.ts()] = {ifGUI(&Kernel::excTextscreen), 0, 0, 0};
  stringToCmd[k.cfullscreen()] = {ifGUI(&Kernel::excFullscreen), 0, 0, 0};
  stringToCmd[k.fs()] = {ifGUI(&Kernel::excFullscreen), 0, 0, 0};
  stringToCmd[k.csplitscreen()] = {ifGUI(&Kernel::excSplitscreen), 0, 0, 0};
  stringToCmd[k.ss()] = {ifGUI(&Kernel::excSplitscreen), 0, 0, 0};
  stringToCmd[k.screenmode()] = {ifGUI(&Kernel::excScreenmode), 0, 0, 0};

  stringToCmd[k.buttonp()] = {ifGUI(&Kernel::excButtonp), 0, 0, 0};
  stringToCmd[k.buttonq()] = {ifGUI(&Kernel::excButtonp), 0, 0, 0};
  stringToCmd[k.button()] = {ifGUI(&Kernel::excButton), 0, 0, 0};

  stringToCmd[k.matrix()] = {ifGUI(&Kernel::excMatrix), 0, 0, 0}; // for debugging

  stringToCmd[k.sum()] = {&Kernel::excSum, 0, 2, -1};
  stringToCmd[k.difference()] = {&Kernel::excDifference, 2, 2, 2};
  stringToCmd[k.product()] = {&Kernel::excProduct, 0, 2, -1};
  stringToCmd[k.quotient()] = {&Kernel::excQuotient, 1, 2, 2};
  stringToCmd[k.remainder()] = {&Kernel::excRemainder, 2, 2, 2};
  stringToCmd[k.modulo()] = {&Kernel::excModulo, 2, 2, 2};
  stringToCmd[k.kint()] = {&Kernel::excInt, 1, 1, 1};
  stringToCmd[k.exp()] = {&Kernel::excExp, 1, 1, 1};
  stringToCmd[k.log10()] = {&Kernel::excLog10, 1, 1, 1};
  stringToCmd[k.ln()] = {&Kernel::excLn, 1, 1, 1};
  stringToCmd[k.sin()] = {&Kernel::excSin, 1, 1, 1};
  stringToCmd[k.radsin()] = {&Kernel::excRadsin, 1, 1, 1};
  stringToCmd[k.cos()] = {&Kernel::excCos, 1, 1, 1};
  stringToCmd[k.radcos()] = {&Kernel::excRadcos, 1, 1, 1};
  stringToCmd[k.arctan()] = {&Kernel::excArctan, 1, 1, 2};
  stringToCmd[k.radarctan()] = {&Kernel::excRadarctan, 1, 1, 2};
  stringToCmd[k.round()] = {&Kernel::excRound, 1, 1, 1};
  stringToCmd[k.power()] = {&Kernel::excPower, 2, 2, 2};
  stringToCmd[k.kbitand()] = {&Kernel::excBitand, 0, 2, -1};
  stringToCmd[k.kbitor()] = {&Kernel::excBitor, 0, 2, -1};
  stringToCmd[k.bitxor()] = {&Kernel::excBitxor, 0, 2, -1};
  stringToCmd[k.bitnot()] = {&Kernel::excBitnot, 1, 1, 1};
  stringToCmd[k.ashift()] = {&Kernel::excAshift, 2, 2, 2};
  stringToCmd[k.lshift()] = {&Kernel::excLshift, 2, 2, 2};
  stringToCmd[k.kand()] = {&Kernel::excAnd, 0, 2, -1};
  stringToCmd[k.kor()] = {&Kernel::excOr, 0, 2, -1};
  stringToCmd[k.knot()] = {&Kernel::excNot, 1, 1, 1};

  stringToCmd[k.form()] = {&Kernel::excForm, 3, 3, 3};

  stringToCmd[k.lessp()] = {&Kernel::excLessp, 2, 2, 2};
  stringToCmd[k.lessq()] = {&Kernel::excLessp, 2, 2, 2};
  stringToCmd[k.greaterp()] = {&Kernel::excGreaterp, 2, 2, 2};
  stringToCmd[k.greaterq()] = {&Kernel::excGreaterp, 2, 2, 2};
  stringToCmd[k.lessequalp()] = {&Kernel::excLessequalp, 2, 2, 2};
  stringToCmd[k.lessequalq()] = {&Kernel::excLessequalp, 2, 2, 2};
  stringToCmd[k.greaterequalp()] = {&Kernel::excGreaterequalp, 2, 2, 2};
  stringToCmd[k.greaterequalq()] = {&Kernel::excGreaterequalp, 2, 2, 2};

  stringToCmd[k.define()] = {&Kernel::excDefine, 2, 2, 2};
  stringToCmd[k.text()] = {&Kernel::excText, 1, 1, 1};
  stringToCmd[k.fulltext()] = {&Kernel::excFulltext, 1, 1, 1};
  stringToCmd[k.copydef()] = {&Kernel::excCopydef, 2, 2, 2};
  stringToCmd[k.local()] = {&Kernel::excLocal, 1, 1, -1};
  stringToCmd[k.global()] = {&Kernel::excGlobal, 1, 1, -1};

  stringToCmd[k.pprop()] = {&Kernel::excPprop, 3, 3, 3};
  stringToCmd[k.gprop()] = {&Kernel::excGprop, 2, 2, 2};
  stringToCmd[k.remprop()] = {&Kernel::excRemprop, 2, 2, 2};
  stringToCmd[k.plist()] = {&Kernel::excPlist, 1, 1, 1};

  stringToCmd[k.procedurep()] = {&Kernel::excProcedurep, 1, 1, 1};
  stringToCmd[k.procedureq()] = {&Kernel::excProcedurep, 1, 1, 1};
  stringToCmd[k.primitivep()] = {&Kernel::excPrimitivep, 1, 1, 1};
  stringToCmd[k.primitiveq()] = {&Kernel::excPrimitivep, 1, 1, 1};
  stringToCmd[k.definedp()] = {&Kernel::excDefinedp, 1, 1, 1};
  stringToCmd[k.definedq()] = {&Kernel::excDefinedp, 1, 1, 1};
  stringToCmd[k.namep()] = {&Kernel::excNamep, 1, 1, 1};
  stringToCmd[k.nameq()] = {&Kernel::excNamep, 1, 1, 1};
  stringToCmd[k.plistp()] = {&Kernel::excPlistp, 1, 1, 1};
  stringToCmd[k.plistq()] = {&Kernel::excPlistp, 1, 1, 1};

  stringToCmd[k.contents()] = {&Kernel::excContents, 0, 0, 0};
  stringToCmd[k.buried()] = {&Kernel::excBuried, 0, 0, 0};
  stringToCmd[k.traced()] = {&Kernel::excTraced, 0, 0, 0};
  stringToCmd[k.stepped()] = {&Kernel::excStepped, 0, 0, 0};
  stringToCmd[k.procedures()] = {&Kernel::excProcedures, 0, 0, 0};
  stringToCmd[k.primitives()] = {&Kernel::excPrimitives, 0, 0, 0};
  stringToCmd[k.names()] = {&Kernel::excNames, 0, 0, 0};
  stringToCmd[k.plists()] = {&Kernel::excPlists, 0, 0, 0};
  stringToCmd[k.arity()] = {&Kernel::excArity, 1, 1, 1};
  stringToCmd[k.nodes()] = {&Kernel::excNodes, 0, 0, 0};

  stringToCmd[k.printout()] = {&Kernel::excPrintout, 1, 1, 1};
  stringToCmd[k.po()] = {&Kernel::excPrintout, 1, 1, 1};
  stringToCmd[k.pot()] = {&Kernel::excPot, 1, 1, 1};

  stringToCmd[k.cerase()] = {&Kernel::excErase, 1, 1, 1};
  stringToCmd[k.er()] = {&Kernel::excErase, 1, 1, 1};
  stringToCmd[k.erall()] = {&Kernel::excErall, 0, 0, 0};
  stringToCmd[k.erps()] = {&Kernel::excErps, 0, 0, 0};
  stringToCmd[k.erns()] = {&Kernel::excErns, 0, 0, 0};
  stringToCmd[k.erpls()] = {&Kernel::excErpls, 0, 0, 0};
  stringToCmd[k.bury()] = {&Kernel::excBury, 1, 1, 1};
  stringToCmd[k.unbury()] = {&Kernel::excUnbury, 1, 1, 1};
  stringToCmd[k.buriedp()] = {&Kernel::excBuriedp, 1, 1, 1};
  stringToCmd[k.buriedq()] = {&Kernel::excBuriedp, 1, 1, 1};
  stringToCmd[k.trace()] = {&Kernel::excTrace, 1, 1, 1};
  stringToCmd[k.untrace()] = {&Kernel::excUntrace, 1, 1, 1};
  stringToCmd[k.tracedp()] = {&Kernel::excTracedp, 1, 1, 1};
  stringToCmd[k.tracedq()] = {&Kernel::excTracedp, 1, 1, 1};
  stringToCmd[k.step()] = {&Kernel::excStep, 1, 1, 1};
  stringToCmd[k.unstep()] = {&Kernel::excUnstep, 1, 1, 1};
  stringToCmd[k.steppedp()] = {&Kernel::excSteppedp, 1, 1, 1};
  stringToCmd[k.steppedq()] = {&Kernel::excSteppedp, 1, 1, 1};
  stringToCmd[k.edit()] = {&Kernel::excEdit, 0, -1, 1};
  stringToCmd[k.ed()] = {&Kernel::excEdit, 0, -1, 1};
  stringToCmd[k.editfile()] = {&Kernel::excEditfile, 1, 1, 1};
  stringToCmd[k.save()] = {&Kernel::excSave, 0, -1, 1};
  stringToCmd[k.load()] = {&Kernel::excLoad, 1, 1, 1};
  stringToCmd[k.help()] = {&Kernel::excHelp, 0, -1, -1};

  // CONTROL STRUCTURES

  stringToCmd[k.run()] = {&Kernel::excRun, 1, 1, 1};
  stringToCmd[k.runresult()] = {&Kernel::excRunresult, 1, 1, 1};
  stringToCmd[k.kforever()] = {&Kernel::excForever, 1, 1, 1};
  stringToCmd[k.repcount()] = {&Kernel::excRepcount, 0, 0, 0};
  stringToCmd[k.kif()] = {&Kernel::excIf, 2, 2, 2};
  stringToCmd[k.ifelse()] = {&Kernel::excIfelse, 3, 3, 3};
  stringToCmd[k.test()] = {&Kernel::excTest, 1, 1, 1};
  stringToCmd[k.iftrue()] = {&Kernel::excIftrue, 1, 1, 1};
  stringToCmd[k.ift()] = {&Kernel::excIftrue, 1, 1, 1};
  stringToCmd[k.iffalse()] = {&Kernel::excIffalse, 1, 1, 1};
  stringToCmd[k.iff()] = {&Kernel::excIffalse, 1, 1, 1};
  stringToCmd[k.stop()] = {&Kernel::excStop, 0, 0, 1};
  stringToCmd[k.output()] = {&Kernel::excOutput, 1, 1, 1};
  stringToCmd[k.op()] = {&Kernel::excOutput, 1, 1, 1};
  stringToCmd[k.kcatch()] = {&Kernel::excCatch, 2, 2, 2};
  stringToCmd[k.kthrow()] = {&Kernel::excThrow, 1, 1, 2};
  stringToCmd[k.error()] = {&Kernel::excError, 0, 0, 0};
  stringToCmd[k.pause()] = {&Kernel::excPause, 0, 0, 0};
  stringToCmd[k.kcontinue()] = {&Kernel::excContinue, 0, -1, 1};
  stringToCmd[k.co()] = {&Kernel::excContinue, 0, -1, 1};
  stringToCmd[k.bye()] = {&Kernel::excBye, 0, 0, 0};
  stringToCmd[k.dmaybeoutput()] = {&Kernel::excDotMaybeoutput, 1, 1, 1};
  stringToCmd[k.tag()] = {&Kernel::excTag, 1, 1, 1};
  stringToCmd[k.kgoto()] = {&Kernel::excGoto, 1, 1, 1};

  stringToCmd[k.apply()] = {&Kernel::excApply, 2, 2, 2};
  stringToCmd["?"] = {&Kernel::excNamedSlot, 0, 0, 1};

  stringToCmd[k.cto()] = {&Kernel::excTo, -1, -1, -1};
  stringToCmd[k.dcMacro()] = {&Kernel::excTo, -1, -1, -1};
  stringToCmd[k.dDefmacro()] = {&Kernel::excDefine, 2, 2, 2};
  stringToCmd[k.macrop()] = {&Kernel::excMacrop, 1, 1, 1};
  stringToCmd[k.macroq()] = {&Kernel::excMacrop, 1, 1, 1};

  stringToCmd[k.gc()] = {&Kernel::excNoop, 0, 0, -1};
  stringToCmd[k.dsetsegmentsize()] = {&Kernel::excNoop, 1, 1, 1};
  stringToCmd[k.setpenpattern()] = {&Kernel::excNoop, 1, 1, 1};
  stringToCmd[k.penpattern()] = {&Kernel::excNoop, 1, 1, 1};
  stringToCmd[k.refresh()] = {&Kernel::excNoop, 0, 0, 0};
  stringToCmd[k.norefresh()] = {&Kernel::excNoop, 0, 0, 0};

  stringToCmd["+"] = {&Kernel::excSum, 0, 2, -1};
  stringToCmd["*"] = {&Kernel::excProduct, 0, 2, -1};
  stringToCmd["/"] = {&Kernel::excQuotient, 1, 2, 2};
  stringToCmd[">"] = {&Kernel::excGreaterp, 2, 2, 2};
  stringToCmd["<"] = {&Kernel::excLessp, 2, 2, 2};
  stringToCmd["="] = {&Kernel::excEqualp, 2, 2, 2};
  stringToCmd[">="] = {&Kernel::excGreaterequalp, 2, 2, 2};
  stringToCmd["<="] = {&Kernel::excLessequalp, 2, 2, 2};
  stringToCmd["<>"] = {&Kernel::excNotequal, 2, 2, 2};

}


void Procedure::addToPool()
{
  instructionList = nothing;
  requiredInputs.clear();
  optionalInputs.clear();
  optionalDefaults.clear();
  tagToLine.clear();
  sourceText = nothing;
  pool.dealloc(this);
}
