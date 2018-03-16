
//===-- qlogo/parser.cpp - Parser class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Parser class, which is
/// responsible for parsing text and keeping user-defined functions.
///
//===----------------------------------------------------------------------===//

#include "parser.h"
#include "error.h"
#include "kernel.h"
#include <qdatetime.h>
#include <qdebug.h>

#include CONTROLLER_HEADER

static const QString specialChars("+-()*%/<>=");

char lastNonSpaceChar(const QString &line) {
  char retval = ' ';
  for (int i = line.length() - 1; i >= 0; --i) {
    retval = line[i].toLatin1();
    if (retval != ' ')
      break;
  }
  return retval;
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
  Procedure *body = new Procedure;
  DatumP bodyP(body);

  QString cmdString = cmd.wordValue()->keyValue();
  bool isMacro = ((cmdString == ".MACRO") || (cmdString == ".DEFMACRO"));

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
      if (d.isWord() && (d.wordValue()->keyValue() == "TAG") &&
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

  List *retval = new List;

  List *inputs = new List;

  for (auto &i : body->requiredInputs) {
    inputs->append(DatumP(new Word(i)));
  }

  QList<DatumP>::iterator d = body->optionalDefaults.begin();
  for (auto &i : body->optionalInputs) {
    List *optInput = new List(d->listValue());
    optInput->prepend(DatumP(new Word(i)));
    ++d;
    inputs->append(DatumP(optInput));
  }

  if (body->restInput != "") {
    List *restInput = new List;
    restInput->append(DatumP(new Word(body->restInput)));
    inputs->append(DatumP(restInput));
  }

  if (body->defaultNumber != body->requiredInputs.size()) {
    inputs->append(DatumP(new Word(body->defaultNumber)));
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

    List *retval = new List;

    if (body->sourceText == nothing) {
      retval->append(DatumP(new Word(procedureTitle(procnameP))));

      ListIterator b = body->instructionList.listValue()->newIterator();

      while (b.elementExists()) {
        retval->append(new Word(unreadList(b.element().listValue(), false)));
      }

      DatumP end(new Word("end"));
      retval->append(end);
    } else {
      return body->sourceText;
    }
    return DatumP(retval);
  } else if (shouldValidate) {
    Error::noHow(procnameP);
  }
  List *retval = new List;
  retval->append(
      DatumP(new Word(QString("to ") + procnameP.wordValue()->printValue())));
  retval->append(DatumP(new Word("end")));
  return DatumP(retval);
}

QString Parser::procedureTitle(DatumP procnameP) {
  QString procname = procnameP.wordValue()->keyValue();

  if (stringToCmd.contains(procname))
    Error::isPrimative(procnameP);
  if (!procedures.contains(procname))
    Error::noHow(procnameP);

  Procedure *body = procedures[procname].procedureValue();

  List *firstLine = new List;

  if (body->isMacro)
    firstLine->append(DatumP(new Word(".macro")));
  else
    firstLine->append(DatumP(new Word("to")));
  firstLine->append(procnameP);

  QString paramName;

  for (auto &i : body->requiredInputs) {
    paramName = i;
    paramName.prepend(':');
    firstLine->append(DatumP(new Word(paramName)));
  }

  QList<DatumP>::iterator d = body->optionalDefaults.begin();
  for (auto &i : body->optionalInputs) {
    paramName = i;
    paramName.push_front(':');
    List *optInput = new List(d->listValue());
    optInput->prepend(DatumP(new Word(paramName)));
    firstLine->append(DatumP(optInput));
    ++d;
  }

  paramName = body->restInput;
  if (paramName != "") {
    paramName.push_front(':');
    List *restInput = new List;
    restInput->append(DatumP(new Word(paramName)));
    firstLine->append(DatumP(restInput));
  }

  if (body->defaultNumber != body->requiredInputs.size()) {
    firstLine->append(DatumP(new Word(body->defaultNumber)));
  }

  QString retval = unreadList(firstLine, false);
  delete firstLine;
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

  DatumP textP(new List);
  DatumP sourceText = lastReadListSource();
  DatumP firstLine(new List);
  for (int i = 1; i < node->countOfChildren(); ++i) {
    firstLine.listValue()->append(node->childAtIndex(i));
  }
  textP.listValue()->append(firstLine);

  // Now read in the body
  forever {
    DatumP line = readlistWithPrompt("> ", true, readStream);
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
      if (firstWord == "END")
        break;
    }
    textP.listValue()->append(line);
  }

  defineProcedure(to, procnameP, textP, sourceText);

  kernel->sysPrint(procnameP.wordValue()->printValue());
  kernel->sysPrint(" defined\n");
  lastProcedureCreatedTimestamp = QDateTime::currentMSecsSinceEpoch();
}

DatumP Parser::readrawlineWithPrompt(const QString &prompt,
                                     QTextStream *readStream) {
  DatumP retval;
  if (readStream == NULL) {
    retval = mainController()->readrawlineWithPrompt(prompt);
  } else {
    if (readStream->atEnd()) {
      return nothing;
    }
    QString str = readStream->readLine();
    if (readStream->status() != QTextStream::Ok)
      Error::fileSystem();
    retval = DatumP(new Word(str));
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
      return DatumP(new Word(retval));

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
      retval.push_back("\n");
      line = readrawlineWithPrompt("\\ ", readStream);
      continue;
    }
    if (isVbarred) {
      retval.push_back(charToRaw('\n'));
      line = readrawlineWithPrompt("| ", readStream);
      continue;
    }
    if (lastNonSpaceChar(t) == '~') {
      retval.push_back("\n");
      line = readrawlineWithPrompt("~ ", readStream);
      continue;
    }

    return DatumP(new Word(retval));
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
  List *retval = new List;
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
          retval->append(DatumP(new Word(currentWord, isCurrentWordVbarred)));
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
          Array *ary = new Array(origin, retval);
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
      retval->append(DatumP(new Word(currentWord, isCurrentWordVbarred)));
      currentWord = "";
      isCurrentWordVbarred = false;
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
      Array *ary = new Array(1, retval);
      return DatumP(ary);
    }
    return retvalP;
  }
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
  listSourceText = new List;
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
  runparseRetval->append(new Word(retval));
}

void Parser::runparseString() {
  QString retval = "";

  if (*runparseCIter == '?') {
    retval = "?";
    ++runparseCIter;
    DatumP number = runparseNumber();
    if (number != nothing) {
      runparseRetval->append(new Word("("));
      runparseRetval->append(new Word("?"));
      runparseRetval->append(number);
      runparseRetval->append(new Word(")"));
      return;
    }
  }

  while ((runparseCIter != runparseCEnd) &&
         (!specialChars.contains(*runparseCIter))) {
    retval += *runparseCIter;
    ++runparseCIter;
  }
  runparseRetval->append(new Word(retval, isRunparseSourceSpecial));
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
  runparseRetval->append(DatumP(new Word("0")));
  runparseRetval->append(DatumP(new Word("--")));
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
  return DatumP(new Word(value));
}

void Parser::runparseQuotedWord() {
  QString retval = "";
  while ((runparseCIter != runparseCEnd) && (*runparseCIter != '(') &&
         (*runparseCIter != ')')) {
    retval += *runparseCIter;
    ++runparseCIter;
  }
  runparseRetval->append(new Word(retval, isRunparseSourceSpecial));
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
  runparseRetval = new List;
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

    while (currentToken != nothing) {
      aList->astList.push_back(parseExp());
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

    DatumP node = DatumP(new ASTNode(op));
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

    DatumP node = DatumP(new ASTNode(op));
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

    DatumP node = DatumP(new ASTNode(op));
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

    DatumP node = DatumP(new ASTNode(op));
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
    DatumP node(new ASTNode("List"));
    node.astnodeValue()->kernel = &Kernel::executeLiteral;
    node.astnodeValue()->addChild(currentToken);
    advanceToken();
    return node;
  }

  if (currentToken.isa() == Datum::arrayType) {
    DatumP node(new ASTNode("Array"));
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
      DatumP node(new ASTNode("QuotedName"));
      node.astnodeValue()->kernel = &Kernel::executeLiteral;
      node.astnodeValue()->addChild(
          DatumP(new Word(name, currentToken.wordValue()->isForeverSpecial)));
      advanceToken();
      return node;
    } else {
      DatumP node(new ASTNode("ValueOf"));
      node.astnodeValue()->kernel = &Kernel::executeValueOf;
      node.astnodeValue()->addChild(DatumP(new Word(name)));
      advanceToken();
      return node;
    }
  }

  // See if it's a number
  currentToken.wordValue()->numberValue();
  if (currentToken.wordValue()->didNumberConversionSucceed()) {
    DatumP node(new ASTNode("number"));
    node.astnodeValue()->kernel = &Kernel::executeLiteral;
    node.astnodeValue()->addChild(currentToken);
    advanceToken();
    return node;
  }

  // If all else fails, it must be a function with the default number of params
  return parseCommand(false);
}

DatumP Parser::astnodeWithLiterals(DatumP cmd, DatumP params) {
  int minParams, maxParams, defaultParams;
  DatumP node = astnodeFromCommand(cmd, minParams, defaultParams, maxParams);

  int countOfChildren = params.listValue()->size();
  if (countOfChildren < minParams)
    Error::notEnough(cmd);
  if ((countOfChildren > maxParams) && (maxParams != -1))
    Error::tooMany(cmd);

  ListIterator iter = params.listValue()->newIterator();
  while (iter.elementExists()) {
    DatumP p = iter.element();
    DatumP a = DatumP(new ASTNode("literal"));
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
  DatumP node = DatumP(new ASTNode(cmdP));
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
  } else if (cmdString.startsWith("SET") && (cmdString.size() > 3) &&
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

  int defaultParams;
  int minParams;
  int maxParams;

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
        qDebug() << "Adding child:" << currentToken.printValue();
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
  List *retval = new List;

  for (auto &name : procedures.keys()) {

    if (shouldInclude(showWhat, name))
      retval->append(DatumP(new Word(name)));
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
  List *retval = new List;

  for (auto name : stringToCmd.keys()) {
    retval->append(DatumP(new Word(name)));
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

  List *retval = new List;
  retval->append(DatumP(new Word(minParams)));
  retval->append(DatumP(new Word(defParams)));
  retval->append(DatumP(new Word(maxParams)));
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
    retval.append("]");
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
  retval.append("}");
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
    retval.append("|");
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
  listSourceText = new List;
  if (stringToCmd.size() > 0)
    return;

  // DATA STRUCTURE PRIMITIVES (MIN, default, MAX)
  // (MIN = -1)     = All parameters are read as list, e.g. "TO PROC :p1"
  // becomes ["TO", "PROC", ":p1"] (default = -1) = All parameters are consumed
  // until end of line (MAX = -1)     = All parameters are consumed within
  // parens
  stringToCmd["SAVE"] = {&Kernel::excShow, 0, -1, 1};

  stringToCmd["WORD"] = {&Kernel::excWord, 0, 2, -1};
  stringToCmd["LIST"] = {&Kernel::excList, 0, 2, -1};
  stringToCmd["SENTENCE"] = {&Kernel::excSentence, 0, 2, -1};
  stringToCmd["SE"] = stringToCmd["SENTENCE"];
  stringToCmd["FPUT"] = {&Kernel::excFput, 2, 2, 2};
  stringToCmd["LPUT"] = {&Kernel::excLput, 2, 2, 2};
  stringToCmd["ARRAY"] = {&Kernel::excArray, 1, 1, 2};
  stringToCmd["LISTTOARRAY"] = {&Kernel::excListtoarray, 1, 1, 2};
  stringToCmd["ARRAYTOLIST"] = {&Kernel::excArraytolist, 1, 1, 1};
  stringToCmd["READLIST"] = {&Kernel::excReadlist, 0, 0, 0};
  stringToCmd["RL"] = stringToCmd["READLIST"];
  stringToCmd["READWORD"] = {&Kernel::excReadword, 0, 0, 0};
  stringToCmd["RW"] = stringToCmd["READWORD"];
  stringToCmd["READRAWLINE"] = {&Kernel::excReadrawline, 0, 0, 0};
  stringToCmd["READCHAR"] = {&Kernel::excReadchar, 0, 0, 0};
  stringToCmd["RC"] = stringToCmd["READCHAR"];
  stringToCmd["READCHARS"] = {&Kernel::excReadchars, 1, 1, 1};
  stringToCmd["RCS"] = stringToCmd["READCHARS"];
  stringToCmd["SHELL"] = {&Kernel::excShell, 1, 1, 2};

  stringToCmd["SETPREFIX"] = {&Kernel::excSetprefix, 1, 1, 1};
  stringToCmd["PREFIX"] = {&Kernel::excPrefix, 0, 0, 0};
  stringToCmd["OPENREAD"] = {&Kernel::excOpenread, 1, 1, 1};
  stringToCmd["OPENWRITE"] = {&Kernel::excOpenwrite, 1, 1, 1};
  stringToCmd["OPENAPPEND"] = {&Kernel::excOpenappend, 1, 1, 1};
  stringToCmd["OPENUPDATE"] = {&Kernel::excOpenupdate, 1, 1, 1};
  stringToCmd["ALLOPEN"] = {&Kernel::excAllopen, 0, 0, 0};
  stringToCmd["SETREAD"] = {&Kernel::excSetread, 1, 1, 1};
  stringToCmd["SETWRITE"] = {&Kernel::excSetwrite, 1, 1, 1};
  stringToCmd["READER"] = {&Kernel::excReader, 0, 0, 0};
  stringToCmd["WRITER"] = {&Kernel::excWriter, 0, 0, 0};
  stringToCmd["READPOS"] = {&Kernel::excReadpos, 0, 0, 0};
  stringToCmd["WRITEPOS"] = {&Kernel::excWritepos, 0, 0, 0};
  stringToCmd["SETREADPOS"] = {&Kernel::excSetreadpos, 1, 1, 1};
  stringToCmd["SETWRITEPOS"] = {&Kernel::excSetwritepos, 1, 1, 1};
  stringToCmd["EOFP"] = {&Kernel::excEofp, 0, 0, 0};
  stringToCmd["EOF?"] = stringToCmd["EOFP"];
  stringToCmd["KEYP"] = {&Kernel::excKeyp, 0, 0, 0};
  stringToCmd["KEY?"] = stringToCmd["KEYP"];
  stringToCmd["DRIBBLE"] = {&Kernel::excDribble, 1, 1, 1};
  stringToCmd["NODRIBBLE"] = {&Kernel::excNodribble, 0, 0, 0};

  stringToCmd["CLEARTEXT"] = {&Kernel::excCleartext, 0, 0, 0};
  stringToCmd["CT"] = stringToCmd["CLEARTEXT"];
  stringToCmd["CURSORINSERT"] = {&Kernel::excCursorInsert, 0, 0, 0};
  stringToCmd["CURSOROVERWRITE"] = {&Kernel::excCursorOverwrite, 0, 0, 0};
  stringToCmd["CURSORMODE"] = {&Kernel::excCursorMode, 0, 0, 0};

  stringToCmd["CLOSE"] = {&Kernel::excClose, 1, 1, 1};
  stringToCmd["CLOSEALL"] = {&Kernel::excCloseall, 0, 0, 0};
  stringToCmd["ERASEFILE"] = {&Kernel::excErasefile, 1, 1, 1};
  stringToCmd["ERF"] = stringToCmd["ERASEFILE"];

  stringToCmd["FIRST"] = {&Kernel::excFirst, 1, 1, 1};
  stringToCmd["LAST"] = {&Kernel::excLast, 1, 1, 1};
  stringToCmd["BUTFIRST"] = {&Kernel::excButfirst, 1, 1, 1};
  stringToCmd["BF"] = stringToCmd["BUTFIRST"];
  stringToCmd["FIRSTS"] = {&Kernel::excFirsts, 1, 1, 1};
  stringToCmd["BUTFIRSTS"] = {&Kernel::excButfirsts, 1, 1, 1};
  stringToCmd["BFS"] = stringToCmd["BUTFIRSTS"];
  stringToCmd["BUTLAST"] = {&Kernel::excButlast, 1, 1, 1};
  stringToCmd["BL"] = stringToCmd["BUTLAST"];
  stringToCmd["ITEM"] = {&Kernel::excItem, 2, 2, 2};

  stringToCmd["SETITEM"] = {&Kernel::excSetitem, 3, 3, 3};
  stringToCmd[".SETFIRST"] = {&Kernel::excDotSetfirst, 2, 2, 2};
  stringToCmd[".SETBF"] = {&Kernel::excDotSetbf, 2, 2, 2};
  stringToCmd[".SETITEM"] = {&Kernel::excDotSetitem, 3, 3, 3};

  stringToCmd["WORDP"] = {&Kernel::excWordp, 1, 1, 1};
  stringToCmd["WORD?"] = stringToCmd["WORDP"];
  stringToCmd["LISTP"] = {&Kernel::excListp, 1, 1, 1};
  stringToCmd["LIST?"] = stringToCmd["LISTP"];
  stringToCmd["ARRAYP"] = {&Kernel::excArrayp, 1, 1, 1};
  stringToCmd["ARRAY?"] = stringToCmd["ARRAYP"];
  stringToCmd["EMPTYP"] = {&Kernel::excEmptyp, 1, 1, 1};
  stringToCmd["EMPTY?"] = stringToCmd["EMPTYP"];
  stringToCmd["EQUALP"] = {&Kernel::excEqualp, 2, 2, 2};
  stringToCmd["EQUAL?"] = stringToCmd["EQUALP"];
  stringToCmd["NOTEQUALP"] = {&Kernel::excNotequal, 2, 2, 2};
  stringToCmd["NOTEQUAL?"] = stringToCmd["NOTEQUALP"];
  stringToCmd["BEFOREP"] = {&Kernel::excBeforep, 2, 2, 2};
  stringToCmd["BEFORE?"] = stringToCmd["BEFOREP"];
  stringToCmd[".EQ"] = {&Kernel::excDotEq, 2, 2, 2};
  stringToCmd["MEMBERP"] = {&Kernel::excMemberp, 2, 2, 2};
  stringToCmd["MEMBER?"] = stringToCmd["MEMBERP"];
  stringToCmd["SUBSTRINGP"] = {&Kernel::excSubstringp, 2, 2, 2};
  stringToCmd["SUBSTRING?"] = stringToCmd["SUBSTRINGP"];
  stringToCmd["NUMBERP"] = {&Kernel::excNumberp, 1, 1, 1};
  stringToCmd["NUMBER?"] = stringToCmd["NUMBERP"];
  stringToCmd["VBARREDP"] = {&Kernel::excVbarredp, 1, 1, 1};
  stringToCmd["VBARRED?"] = stringToCmd["VBARREDP"];

  stringToCmd["COUNT"] = {&Kernel::excCount, 1, 1, 1};
  stringToCmd["ASCII"] = {&Kernel::excAscii, 1, 1, 1};
  stringToCmd["RAWASCII"] = {&Kernel::excRawascii, 1, 1, 1};
  stringToCmd["CHAR"] = {&Kernel::excChar, 1, 1, 1};
  stringToCmd["MEMBER"] = {&Kernel::excMember, 2, 2, 2};
  stringToCmd["LOWERCASE"] = {&Kernel::excLowercase, 1, 1, 1};
  stringToCmd["UPPERCASE"] = {&Kernel::excUppercase, 1, 1, 1};
  stringToCmd["STANDOUT"] = {&Kernel::excStandout, 1, 1, 1};
  stringToCmd["PARSE"] = {&Kernel::excParse, 1, 1, 1};
  stringToCmd["RUNPARSE"] = {&Kernel::excRunparse, 1, 1, 1};

  stringToCmd["MINUS"] = {&Kernel::excMinus, 1, 1, 1};
  stringToCmd["-"] = {&Kernel::excMinus, 1, 1, 1};
  stringToCmd["--"] = stringToCmd["-"];

  stringToCmd["PRINT"] = {&Kernel::excPrint, 0, 1, -1};
  stringToCmd["PR"] = stringToCmd["PRINT"];
  stringToCmd["TYPE"] = {&Kernel::excType, 0, 1, -1};
  stringToCmd["SHOW"] = {&Kernel::excShow, 0, 1, -1};
  stringToCmd["MAKE"] = {&Kernel::excMake, 2, 2, 2};
  stringToCmd["REPEAT"] = {&Kernel::excRepeat, 2, 2, 2};
  stringToCmd["SQRT"] = {&Kernel::excSqrt, 1, 1, 1};
  stringToCmd["RANDOM"] = {&Kernel::excRandom, 1, 1, 2};
  stringToCmd["RERANDOM"] = {&Kernel::excRerandom, 0, 0, 1};
  stringToCmd["THING"] = {&Kernel::excThing, 1, 1, 1};
  stringToCmd["WAIT"] = {&Kernel::excWait, 1, 1, 1};
  stringToCmd["SETCURSOR"] = {&Kernel::excSetcursor, 1, 1, 1};
  stringToCmd["CURSOR"] = {&Kernel::excCursor, 0, 0, 0};
  stringToCmd["SETTEXTCOLOR"] = {&Kernel::excSettextcolor, 1, 2, 2};
  stringToCmd["SETTC"] = stringToCmd["SETTEXTCOLOR"];
  stringToCmd["SETTEXTSIZE"] = {&Kernel::excSettextsize, 1, 1, 1};
  stringToCmd["INCREASEFONT"] = {&Kernel::excIncreasefont, 0, 0, 0};
  stringToCmd["DECREASEFONT"] = {&Kernel::excDecreasefont, 0, 0, 0};
  stringToCmd["SETTEXTSIZE"] = {&Kernel::excSettextsize, 1, 1, 1};
  stringToCmd["TEXTSIZE"] = {&Kernel::excTextsize, 0, 0, 0};
  stringToCmd["SETFONT"] = {&Kernel::excSetfont, 1, 1, 1};
  stringToCmd["FONT"] = {&Kernel::excFont, 0, 0, 0};
  stringToCmd["ALLFONTS"] = {&Kernel::excAllfonts, 0, 0, 0};

  stringToCmd["FORWARD"] = {&Kernel::excForward, 1, 1, 1};
  stringToCmd["FD"] = stringToCmd["FORWARD"];
  stringToCmd["BACK"] = {&Kernel::excBack, 1, 1, 1};
  stringToCmd["BK"] = stringToCmd["BACK"];
  stringToCmd["RIGHT"] = {&Kernel::excRight, 1, 1, 1};
  stringToCmd["RT"] = stringToCmd["RIGHT"];
  stringToCmd["LEFT"] = {&Kernel::excLeft, 1, 1, 1};
  stringToCmd["LT"] = stringToCmd["LEFT"];
  stringToCmd["CLEARSCREEN"] = {&Kernel::excClearscreen, 0, 0, 0};
  stringToCmd["CS"] = stringToCmd["CLEARSCREEN"];
  stringToCmd["CLEAN"] = {&Kernel::excClean, 0, 0, 0};
  stringToCmd["PENUP"] = {&Kernel::excPenup, 0, 0, 0};
  stringToCmd["PU"] = stringToCmd["PENUP"];
  stringToCmd["PENDOWN"] = {&Kernel::excPendown, 0, 0, 0};
  stringToCmd["PD"] = stringToCmd["PENDOWN"];
  stringToCmd["PENDOWNP"] = {&Kernel::excPendownp, 0, 0, 0};
  stringToCmd["PENDOWN?"] = stringToCmd["PENDOWNP"];
  stringToCmd["HIDETURTLE"] = {&Kernel::excHideturtle, 0, 0, 0};
  stringToCmd["HT"] = stringToCmd["HIDETURTLE"];
  stringToCmd["SHOWTURTLE"] = {&Kernel::excShowturtle, 0, 0, 0};
  stringToCmd["ST"] = stringToCmd["SHOWTURTLE"];
  // stringToCmd["SETXYZ"]         = {&Kernel::excSetXYZ, 3,3,3};
  stringToCmd["SETXY"] = {&Kernel::excSetXY, 2, 2, 2};
  stringToCmd["SETX"] = {&Kernel::excSetX, 1, 1, 1};
  stringToCmd["SETY"] = {&Kernel::excSetY, 1, 1, 1};
  // stringToCmd["SETZ"]           = {&Kernel::excSetZ, 1,1,1};
  stringToCmd["SETPOS"] = {&Kernel::excSetpos, 1, 1, 1};
  stringToCmd["POS"] = {&Kernel::excPos, 0, 0, 1};
  stringToCmd["HOME"] = {&Kernel::excHome, 0, 0, 0};
  stringToCmd["HEADING"] = {&Kernel::excHeading, 0, 0, 1};
  stringToCmd["SETHEADING"] = {&Kernel::excSetheading, 1, 1, 2};
  stringToCmd["SETH"] = stringToCmd["SETHEADING"];
  stringToCmd["ARC"] = {&Kernel::excArc, 2, 2, 2};
  stringToCmd["TOWARDS"] = {&Kernel::excTowards, 1, 1, 1};
  stringToCmd["SCRUNCH"] = {&Kernel::excScrunch, 0, 0, 0};
  stringToCmd["SETSCRUNCH"] = {&Kernel::excSetscrunch, 2, 2, 2};
  stringToCmd["LABEL"] = {&Kernel::excLabel, 1, 1, 1};
  stringToCmd["LABELHEIGHT"] = {&Kernel::excLabelheight, 0, 0, 0};
  stringToCmd["SETLABELHEIGHT"] = {&Kernel::excSetlabelheight, 1, 1, 1};
  stringToCmd["SHOWNP"] = {&Kernel::excShownp, 0, 0, 0};
  stringToCmd["SHOWN?"] = stringToCmd["SHOWNP"];
  stringToCmd["SETPENCOLOR"] = {&Kernel::excSetpencolor, 1, 1, 1};
  stringToCmd["SETPC"] = stringToCmd["SETPENCOLOR"];
  stringToCmd["PENCOLOR"] = {&Kernel::excPencolor, 0, 0, 0};
  stringToCmd["PC"] = stringToCmd["PENCOLOR"];
  stringToCmd["SETPALETTE"] = {&Kernel::excSetpalette, 2, 2, 2};
  stringToCmd["PALETTE"] = {&Kernel::excPalette, 1, 1, 1};
  stringToCmd["BACKGROUND"] = {&Kernel::excBackground, 0, 0, 0};
  stringToCmd["BG"] = stringToCmd["BACKGROUND"];
  stringToCmd["SETBACKGROUND"] = {&Kernel::excSetbackground, 1, 1, 1};
  stringToCmd["SETBG"] = stringToCmd["SETBACKGROUND"];
  stringToCmd["SAVEPICT"] = {&Kernel::excSavepict, 1, 1, 1};

  stringToCmd["PENPAINT"] = {&Kernel::excPenpaint, 0, 0, 0};
  stringToCmd["PPT"] = stringToCmd["PENPAINT"];
  stringToCmd["PENERASE"] = {&Kernel::excPenerase, 0, 0, 0};
  stringToCmd["PE"] = stringToCmd["PENERASE"];
  stringToCmd["PENREVERSE"] = {&Kernel::excPenreverse, 0, 0, 0};
  stringToCmd["PX"] = stringToCmd["PENREVERSE"];
  stringToCmd["PENMODE"] = {&Kernel::excPenmode, 0, 0, 0};
  stringToCmd["SETPENSIZE"] = {&Kernel::excSetpensize, 1, 1, 1};
  stringToCmd["PENSIZE"] = {&Kernel::excPensize, 0, 0, 0};
  stringToCmd["FILLED"] = {&Kernel::excFilled, 2, 2, 2};

  stringToCmd["WRAP"] = {&Kernel::excWrap, 0, 0, 0};
  stringToCmd["FENCE"] = {&Kernel::excFence, 0, 0, 0};
  stringToCmd["WINDOW"] = {&Kernel::excWindow, 0, 0, 0};
  stringToCmd["TURTLEMODE"] = {&Kernel::excTurtlemode, 0, 0, 0};

  stringToCmd["MOUSEPOS"] = {&Kernel::excMousepos, 0, 0, 0};
  stringToCmd["CLICKPOS"] = {&Kernel::excClickpos, 0, 0, 0};
  stringToCmd["BOUNDS"] = {&Kernel::excBounds, 0, 0, 0};
  stringToCmd["SETBOUNDS"] = {&Kernel::excSetbounds, 2, 2, 2};

  stringToCmd["TEXTSCREEN"] = {&Kernel::excTextscreen, 0, 0, 0};
  stringToCmd["TS"] = stringToCmd["TEXTSCREEN"];
  stringToCmd["FULLSCREEN"] = {&Kernel::excFullscreen, 0, 0, 0};
  stringToCmd["FS"] = stringToCmd["FULLSCREEN"];
  stringToCmd["SPLITSCREEN"] = {&Kernel::excSplitscreen, 0, 0, 0};
  stringToCmd["SS"] = stringToCmd["SPLITSCREEN"];
  stringToCmd["SCREENMODE"] = {&Kernel::excScreenmode, 0, 0, 0};

  stringToCmd["BUTTONP"] = {&Kernel::excButtonp, 0, 0, 0};
  stringToCmd["BUTTON?"] = stringToCmd["BUTTONP"];
  stringToCmd["BUTTON"] = {&Kernel::excButton, 0, 0, 0};

  stringToCmd["MATRIX"] = {&Kernel::excMatrix, 0, 0, 0}; // for debugging

  stringToCmd["SUM"] = {&Kernel::excSum, 0, 2, -1};
  stringToCmd["DIFFERENCE"] = {&Kernel::excDifference, 2, 2, 2};
  stringToCmd["PRODUCT"] = {&Kernel::excProduct, 0, 2, -1};
  stringToCmd["QUOTIENT"] = {&Kernel::excQuotient, 1, 2, 2};
  stringToCmd["REMAINDER"] = {&Kernel::excRemainder, 2, 2, 2};
  stringToCmd["MODULO"] = {&Kernel::excModulo, 2, 2, 2};
  stringToCmd["INT"] = {&Kernel::excInt, 1, 1, 1};
  stringToCmd["EXP"] = {&Kernel::excExp, 1, 1, 1};
  stringToCmd["LOG10"] = {&Kernel::excLog10, 1, 1, 1};
  stringToCmd["LN"] = {&Kernel::excLn, 1, 1, 1};
  stringToCmd["SIN"] = {&Kernel::excSin, 1, 1, 1};
  stringToCmd["RADSIN"] = {&Kernel::excRadsin, 1, 1, 1};
  stringToCmd["COS"] = {&Kernel::excCos, 1, 1, 1};
  stringToCmd["RADCOS"] = {&Kernel::excRadcos, 1, 1, 1};
  stringToCmd["ARCTAN"] = {&Kernel::excArctan, 1, 1, 2};
  stringToCmd["RADARCTAN"] = {&Kernel::excRadarctan, 1, 1, 2};
  stringToCmd["ROUND"] = {&Kernel::excRound, 1, 1, 1};
  stringToCmd["POWER"] = {&Kernel::excPower, 2, 2, 2};
  stringToCmd["BITAND"] = {&Kernel::excBitand, 0, 2, -1};
  stringToCmd["BITOR"] = {&Kernel::excBitor, 0, 2, -1};
  stringToCmd["BITXOR"] = {&Kernel::excBitxor, 0, 2, -1};
  stringToCmd["BITNOT"] = {&Kernel::excBitnot, 1, 1, 1};
  stringToCmd["ASHIFT"] = {&Kernel::excAshift, 2, 2, 2};
  stringToCmd["LSHIFT"] = {&Kernel::excLshift, 2, 2, 2};
  stringToCmd["AND"] = {&Kernel::excAnd, 0, 2, -1};
  stringToCmd["OR"] = {&Kernel::excOr, 0, 2, -1};
  stringToCmd["NOT"] = {&Kernel::excNot, 1, 1, 1};

  stringToCmd["FORM"] = {&Kernel::excForm, 3, 3, 3};

  stringToCmd["LESSP"] = {&Kernel::excLessp, 2, 2, 2};
  stringToCmd["LESS?"] = stringToCmd["LESSP"];
  stringToCmd["GREATERP"] = {&Kernel::excGreaterp, 2, 2, 2};
  stringToCmd["GREATER?"] = stringToCmd["GREATERP"];
  stringToCmd["LESSEQUALP"] = {&Kernel::excLessequalp, 2, 2, 2};
  stringToCmd["LESSEQUAL?"] = stringToCmd["LESSEQUALP"];
  stringToCmd["GREATEREQUALP"] = {&Kernel::excGreaterequalp, 2, 2, 2};
  stringToCmd["GREATEREQUAL?"] = stringToCmd["GREATEREQUALP"];

  stringToCmd["DEFINE"] = {&Kernel::excDefine, 2, 2, 2};
  stringToCmd["TEXT"] = {&Kernel::excText, 1, 1, 1};
  stringToCmd["FULLTEXT"] = {&Kernel::excFulltext, 1, 1, 1};
  stringToCmd["COPYDEF"] = {&Kernel::excCopydef, 2, 2, 2};
  stringToCmd["LOCAL"] = {&Kernel::excLocal, 1, 1, -1};
  stringToCmd["GLOBAL"] = {&Kernel::excGlobal, 1, 1, -1};

  stringToCmd["PPROP"] = {&Kernel::excPprop, 3, 3, 3};
  stringToCmd["GPROP"] = {&Kernel::excGprop, 2, 2, 2};
  stringToCmd["REMPROP"] = {&Kernel::excRemprop, 2, 2, 2};
  stringToCmd["PLIST"] = {&Kernel::excPlist, 1, 1, 1};

  stringToCmd["PROCEDUREP"] = {&Kernel::excProcedurep, 1, 1, 1};
  stringToCmd["PROCEDURE?"] = stringToCmd["PROCEDUREP"];
  stringToCmd["PRIMITIVEP"] = {&Kernel::excPrimitivep, 1, 1, 1};
  stringToCmd["PRIMITIVE?"] = stringToCmd["PRIMITIVEP"];
  stringToCmd["DEFINEDP"] = {&Kernel::excDefinedp, 1, 1, 1};
  stringToCmd["DEFINED?"] = stringToCmd["DEFINEDP"];
  stringToCmd["NAMEP"] = {&Kernel::excNamep, 1, 1, 1};
  stringToCmd["NAME?"] = stringToCmd["NAMEP"];
  stringToCmd["PLISTP"] = {&Kernel::excPlistp, 1, 1, 1};
  stringToCmd["PLIST?"] = stringToCmd["PLISTP"];

  stringToCmd["CONTENTS"] = {&Kernel::excContents, 0, 0, 0};
  stringToCmd["BURIED"] = {&Kernel::excBuried, 0, 0, 0};
  stringToCmd["TRACED"] = {&Kernel::excTraced, 0, 0, 0};
  stringToCmd["STEPPED"] = {&Kernel::excStepped, 0, 0, 0};
  stringToCmd["PROCEDURES"] = {&Kernel::excProcedures, 0, 0, 0};
  stringToCmd["PRIMITIVES"] = {&Kernel::excPrimitives, 0, 0, 0};
  stringToCmd["NAMES"] = {&Kernel::excNames, 0, 0, 0};
  stringToCmd["PLISTS"] = {&Kernel::excPlists, 0, 0, 0};
  stringToCmd["ARITY"] = {&Kernel::excArity, 1, 1, 1};
  stringToCmd["NODES"] = {&Kernel::excNodes, 0, 0, 0};

  stringToCmd["PRINTOUT"] = {&Kernel::excPrintout, 1, 1, 1};
  stringToCmd["PO"] = stringToCmd["PRINTOUT"];
  stringToCmd["POT"] = {&Kernel::excPot, 1, 1, 1};

  stringToCmd["ERASE"] = {&Kernel::excErase, 1, 1, 1};
  stringToCmd["ER"] = stringToCmd["ERASE"];
  stringToCmd["ERALL"] = {&Kernel::excErall, 0, 0, 0};
  stringToCmd["ERPS"] = {&Kernel::excErps, 0, 0, 0};
  stringToCmd["ERNS"] = {&Kernel::excErns, 0, 0, 0};
  stringToCmd["ERPLS"] = {&Kernel::excErpls, 0, 0, 0};
  stringToCmd["BURY"] = {&Kernel::excBury, 1, 1, 1};
  stringToCmd["UNBURY"] = {&Kernel::excUnbury, 1, 1, 1};
  stringToCmd["BURIEDP"] = {&Kernel::excBuriedp, 1, 1, 1};
  stringToCmd["BURIED?"] = stringToCmd["BURIEDP"];
  stringToCmd["TRACE"] = {&Kernel::excTrace, 1, 1, 1};
  stringToCmd["UNTRACE"] = {&Kernel::excUntrace, 1, 1, 1};
  stringToCmd["TRACEDP"] = {&Kernel::excTracedp, 1, 1, 1};
  stringToCmd["TRACED?"] = stringToCmd["TRACEDP"];
  stringToCmd["STEP"] = {&Kernel::excStep, 1, 1, 1};
  stringToCmd["UNSTEP"] = {&Kernel::excUnstep, 1, 1, 1};
  stringToCmd["STEPPEDP"] = {&Kernel::excSteppedp, 1, 1, 1};
  stringToCmd["STEPPED?"] = stringToCmd["STEPPEDP"];
  stringToCmd["EDIT"] = {&Kernel::excEdit, 0, -1, 1};
  stringToCmd["ED"] = stringToCmd["EDIT"];
  stringToCmd["EDITFILE"] = {&Kernel::excEditfile, 1, 1, 1};
  stringToCmd["SAVE"] = {&Kernel::excSave, 0, -1, 1};
  stringToCmd["LOAD"] = {&Kernel::excLoad, 1, 1, 1};
  stringToCmd["HELP"] = {&Kernel::excHelp, 0, -1, 1};

  // CONTROL STRUCTURES

  stringToCmd["RUN"] = {&Kernel::excRun, 1, 1, 1};
  stringToCmd["RUNRESULT"] = {&Kernel::excRunresult, 1, 1, 1};
  stringToCmd["FOREVER"] = {&Kernel::excForever, 1, 1, 1};
  stringToCmd["REPCOUNT"] = {&Kernel::excRepcount, 0, 0, 0};
  stringToCmd["IF"] = {&Kernel::excIf, 2, 2, 2};
  stringToCmd["IFELSE"] = {&Kernel::excIfelse, 3, 3, 3};
  stringToCmd["TEST"] = {&Kernel::excTest, 1, 1, 1};
  stringToCmd["IFTRUE"] = {&Kernel::excIftrue, 1, 1, 1};
  stringToCmd["IFT"] = stringToCmd["IFTRUE"];
  stringToCmd["IFFALSE"] = {&Kernel::excIffalse, 1, 1, 1};
  stringToCmd["IFF"] = stringToCmd["IFFALSE"];
  stringToCmd["STOP"] = {&Kernel::excStop, 0, 0, 0};
  stringToCmd["OUTPUT"] = {&Kernel::excOutput, 1, 1, 1};
  stringToCmd["OP"] = stringToCmd["OUTPUT"];
  stringToCmd["CATCH"] = {&Kernel::excCatch, 2, 2, 2};
  stringToCmd["THROW"] = {&Kernel::excThrow, 1, 1, 2};
  stringToCmd["ERROR"] = {&Kernel::excError, 0, 0, 0};
  stringToCmd["PAUSE"] = {&Kernel::excPause, 0, 0, 0};
  stringToCmd["CONTINUE"] = {&Kernel::excContinue, 0, -1, 1};
  stringToCmd["CO"] = stringToCmd["CONTINUE"];
  stringToCmd["BYE"] = {&Kernel::excBye, 0, 0, 0};
  stringToCmd[".MAYBEOUTPUT"] = {&Kernel::excDotMaybeoutput, 1, 1, 1};
  stringToCmd["TAG"] = {&Kernel::excTag, 1, 1, 1};
  stringToCmd["GOTO"] = {&Kernel::excGoto, 1, 1, 1};

  stringToCmd["APPLY"] = {&Kernel::excApply, 2, 2, 2};
  stringToCmd["?"] = {&Kernel::excNamedSlot, 0, 0, 1};

  stringToCmd["TO"] = {&Kernel::excTo, -1, -1, -1};
  stringToCmd[".MACRO"] = stringToCmd["TO"];
  stringToCmd[".DEFMACRO"] = stringToCmd["DEFINE"];
  stringToCmd["MACROP"] = {&Kernel::excMacrop, 1, 1, 1};
  stringToCmd["MACRO?"] = stringToCmd["MACROP"];

  stringToCmd["GC"] = {&Kernel::excNoop, 0, 0, -1};
  stringToCmd[".SETSEGMENTSIZE"] = {&Kernel::excNoop, 1, 1, 1};
  stringToCmd["SETPENPATTERN"] = {&Kernel::excNoop, 1, 1, 1};
  stringToCmd["PENPATTERN"] = {&Kernel::excNoop, 1, 1, 1};
  stringToCmd["REFRESH"] = {&Kernel::excNoop, 0, 0, 0};
  stringToCmd["NOREFRESH"] = {&Kernel::excNoop, 0, 0, 0};

  stringToCmd["+"] = stringToCmd["SUM"]; // {&Kernel::executeWrongUseOf, 0,0,0};
  stringToCmd["*"] =
      stringToCmd["PRODUCT"]; // {&Kernel::executeWrongUseOf, 0,0,0};
  stringToCmd["/"] =
      stringToCmd["QUOTIENT"]; // {&Kernel::executeWrongUseOf, 0,0,0};
  stringToCmd[">"] =
      stringToCmd["GREATERP"]; // {&Kernel::executeWrongUseOf, 0,0,0};
  stringToCmd["<"] =
      stringToCmd["LESSP"]; // {&Kernel::executeWrongUseOf, 0,0,0};
  stringToCmd["="] =
      stringToCmd["EQUALP"]; // {&Kernel::executeWronstringToCmd["PRODUCT"]; //
                             // gUseOf, 0,0,0};
  stringToCmd[">="] =
      stringToCmd["GREATEREQUALP"]; // {&Kernel::executeWrongUseOf, 0,0,0};
  stringToCmd["<="] =
      stringToCmd["LESSEQUALP"]; // {&Kernel::executeWrongUseOf, 0,0,0};
  stringToCmd["<>"] =
      stringToCmd["NOTEQUALP"]; // {&Kernel::executeWrongUseOf, 0,0,0};
}
