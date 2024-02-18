
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
/// responsible for parsing text, lists, and arrays.
///
//===----------------------------------------------------------------------===//

#include "parser.h"
#include "logocontroller.h"
#include "error.h"
#include "kernel.h"
#include "datum_word.h"
#include "datum_astnode.h"
//#include "datum_array.h"
#include <qdatetime.h>
#include <qdebug.h>
#include "stringconstants.h"

// TODO: we could implement this into something a little faster.
const QString specialChars("+-()*%/<>=");



// firstline is ASTNode
void Parser::inputProcedure(DatumPtr nodeP, TextStream *readStream) {
  ASTNode *node = nodeP.astnodeValue();

  DatumPtr to = node->nodeName;
  if (node->countOfChildren() == 0)
    Error::notEnough(to);
  DatumPtr procnameP = node->childAtIndex(0);
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

  if (mainProcedures()->isProcedure(procname))
    Error::procDefined(procnameP);

  DatumPtr textP = DatumPtr(List::alloc());
  DatumPtr firstLine = DatumPtr(List::alloc());
  for (int i = 1; i < node->countOfChildren(); ++i) {
    firstLine.listValue()->append(node->childAtIndex(i));
  }
  textP.listValue()->append(firstLine);

  // Now read in the body
  forever {
    DatumPtr line = readStream->readlistWithPrompt("> ", true, true);
    if ( ! line.isList()) // this must be the end of the input
        break;
    if (line.listValue()->size() == 0)
      continue;
    DatumPtr first = line.listValue()->first();
    if (first.isWord()) {
      QString firstWord = first.wordValue()->keyValue();
      if (firstWord == k.end())
        break;
    }
    textP.listValue()->append(line);
  }

  DatumPtr sourceText = readStream->recentHistory();
  mainProcedures()->defineProcedure(to, procnameP, textP, sourceText);

  mainKernel()->sysPrint(procnameP.wordValue()->printValue());
  mainKernel()->sysPrint(k._defined());
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
  runparseRetval->append(DatumPtr(retval));
}

void Parser::runparseString() {
  QString retval = "";

  if (*runparseCIter == '?') {
    retval = "?";
    ++runparseCIter;
    DatumPtr number = runparseNumber();
    if (number != nothing) {
      runparseRetval->append(DatumPtr(QString("(")));
      runparseRetval->append(DatumPtr(QString("?")));
      runparseRetval->append(number);
      runparseRetval->append(DatumPtr(QString(")")));
      return;
    }
  }

  while ((runparseCIter != runparseCEnd) &&
         (!specialChars.contains(*runparseCIter))) {
    retval += *runparseCIter;
    ++runparseCIter;
  }
  runparseRetval->append(DatumPtr(retval, isRunparseSourceSpecial));
}

void Parser::runparseMinus() {
  QString::iterator nextCharIter = runparseCIter;
  ++nextCharIter;
  if (nextCharIter == runparseCEnd) {
    runparseSpecialchars();
    return;
  }

  DatumPtr number = runparseNumber();
  if (number != nothing) {
    runparseRetval->append(number);
    return;
  }

  // This is a minus function
  runparseRetval->append(DatumPtr(QString("0")));
  runparseRetval->append(DatumPtr(QString("--")));
  // discard the minus
  ++runparseCIter;
}

DatumPtr Parser::runparseNumber() {
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
  return DatumPtr(value);
}

void Parser::runparseQuotedWord() {
  QString retval = "";
  while ((runparseCIter != runparseCEnd) && (*runparseCIter != '(') &&
         (*runparseCIter != ')')) {
    retval += *runparseCIter;
    ++runparseCIter;
  }
  runparseRetval->append(DatumPtr(retval, isRunparseSourceSpecial));
}

/*
     * RUNPARSE wordorlist

        outputs the list that would result if the input word or list were
        entered as an instruction line; characters such as infix operators
        and parentheses are separate members of the output.  Note that
        sublists of a runparsed list are not themselves runparsed.
     */
DatumPtr Parser::runparse(DatumPtr src) {
  if (src.isWord()) {
    QString text = src.wordValue()->rawValue();
    QTextStream srcStream(&text, QIODevice::ReadOnly);
    TextStream stream(&srcStream);
    src = stream.readlistWithPrompt("", false);
  }
  runparseRetval = List::alloc();
  ListIterator iter = src.listValue()->newIterator();

  while (iter.elementExists()) {
    DatumPtr element = iter.element();
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

        DatumPtr number = runparseNumber();
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
  return DatumPtr(runparseRetval);
}

QList<DatumPtr> *Parser::astFromList(List *aList) {
    if (aList->astParseTimeStamp <= mainProcedures()->timeOfLastProcedureCreation()) {
    aList->astParseTimeStamp = QDateTime::currentMSecsSinceEpoch();

    DatumPtr runParsedList = runparse(aList);

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

DatumPtr Parser::parseExp() {
  DatumPtr left = parseSumexp();
  while ((currentToken.isa() == Datum::wordType) &&
         ((currentToken.wordValue()->printValue() == "=") ||
          (currentToken.wordValue()->printValue() == "<>") ||
          (currentToken.wordValue()->printValue() == ">") ||
          (currentToken.wordValue()->printValue() == "<") ||
          (currentToken.wordValue()->printValue() == ">=") ||
          (currentToken.wordValue()->printValue() == "<="))) {
    DatumPtr op = currentToken;
    advanceToken();
    DatumPtr right = parseSumexp();

    DatumPtr node = DatumPtr(ASTNode::alloc(op));
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

DatumPtr Parser::parseSumexp() {
  DatumPtr left = parseMulexp();
  while ((currentToken.isa() == Datum::wordType) &&
         ((currentToken.wordValue()->printValue() == "+") ||
          (currentToken.wordValue()->printValue() == "-"))) {
    DatumPtr op = currentToken;
    advanceToken();
    DatumPtr right = parseMulexp();

    DatumPtr node = DatumPtr(ASTNode::alloc(op));
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

DatumPtr Parser::parseMulexp() {
  DatumPtr left = parseminusexp();
  while ((currentToken.isa() == Datum::wordType) &&
         ((currentToken.wordValue()->printValue() == "*") ||
          (currentToken.wordValue()->printValue() == "/") ||
          (currentToken.wordValue()->printValue() == "%"))) {
    DatumPtr op = currentToken;
    advanceToken();
    DatumPtr right = parseminusexp();

    DatumPtr node = DatumPtr(ASTNode::alloc(op));
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

DatumPtr Parser::parseminusexp() {
  DatumPtr left = parseTermexp();
  while ((currentToken.isa() == Datum::wordType) &&
         ((currentToken.wordValue()->printValue() == "--"))) {
    DatumPtr op = currentToken;
    advanceToken();
    DatumPtr right = parseTermexp();

    DatumPtr node = DatumPtr(ASTNode::alloc(op));
    if (right == nothing)
      Error::notEnough(op);

    node.astnodeValue()->kernel = &Kernel::excDifference;
    node.astnodeValue()->addChild(left);
    node.astnodeValue()->addChild(right);
    left = node;
  }
  return left;
}

DatumPtr Parser::parseTermexp() {
  if (currentToken == nothing)
    return nothing;

  if (currentToken.isa() == Datum::listType) {
    DatumPtr node(ASTNode::alloc(k.word()));
    node.astnodeValue()->kernel = &Kernel::executeLiteral;
    node.astnodeValue()->addChild(currentToken);
    advanceToken();
    return node;
  }

  if (currentToken.isa() == Datum::arrayType) {
    DatumPtr node(ASTNode::alloc(k.array()));
    node.astnodeValue()->kernel = &Kernel::executeLiteral;
    node.astnodeValue()->addChild(currentToken);
    advanceToken();
    return node;
  }

  Q_ASSERT(currentToken.isa() == Datum::wordType);

  // See if it's an open paren
  if (currentToken.wordValue()->printValue() == "(") {
    // This may be an expression or a vararg function
    DatumPtr retval;

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
      DatumPtr node(ASTNode::alloc(k.quotedname()));
      node.astnodeValue()->kernel = &Kernel::executeLiteral;
      node.astnodeValue()->addChild(
          DatumPtr(DatumPtr(name, currentToken.wordValue()->isForeverSpecial)));
      advanceToken();
      return node;
    } else {
      DatumPtr node(ASTNode::alloc(k.valueof()));
      node.astnodeValue()->kernel = &Kernel::executeValueOf;
      node.astnodeValue()->addChild(DatumPtr(name));
      advanceToken();
      return node;
    }
  }

  // See if it's a number
  double number = currentToken.wordValue()->numberValue();
  if (currentToken.wordValue()->didNumberConversionSucceed()) {
    DatumPtr node(ASTNode::alloc(k.number()));
    node.astnodeValue()->kernel = &Kernel::executeLiteral;
    node.astnodeValue()->addChild(DatumPtr(number));
    advanceToken();
    return node;
  }

  // If all else fails, it must be a function with the default number of params
  return parseStopIfExists(parseCommand(false));
}

// First, check to see that the next token is indeed the STOP command.
// If it is, create a new node for STOP, and add the command node
// as a child to the STOP node.
DatumPtr Parser::parseStopIfExists(DatumPtr command)
{
    if ((currentToken != nothing) && currentToken.isWord()
            && (currentToken.wordValue()->keyValue() == k.stop())) {
        // Consume and create the STOP node
        DatumPtr stopCmd = parseCommand(false);
        stopCmd.astnodeValue()->addChild(command);
        return stopCmd;
    }
    return command;
}

DatumPtr Parser::parseCommand(bool isVararg) {
  if (currentToken == nothing)
    return nothing;
  DatumPtr cmdP = currentToken;
  QString cmdString = cmdP.wordValue()->keyValue();

  if (cmdString == ")")
    Error::unexpectedCloseParen();

  int defaultParams = 0;
  int minParams = 0;
  int maxParams = 0;

  DatumPtr node = mainProcedures()->astnodeFromCommand(cmdP, minParams, defaultParams, maxParams);

  advanceToken();

  int countOfChildren = 0;
  // isVararg: read all parameters until ')'
  if (isVararg) {
    while ((currentToken != nothing) &&
           ((!currentToken.isWord()) ||
            (currentToken.wordValue()->printValue() != ")"))) {
      DatumPtr child;
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
      DatumPtr child;
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
      DatumPtr child = parseExp();
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
