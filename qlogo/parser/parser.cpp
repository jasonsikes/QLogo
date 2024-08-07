
//===-- parser.cpp - Parser class implementation -------*- C++ -*-===//
//
// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the Parser class, which is
/// responsible for parsing text, lists, and arrays.
///
//===----------------------------------------------------------------------===//

#include "parser.h"
#include "astnode.h"
#include "controller/logocontroller.h"
#include "datum.h"
#include "error.h"
#include "kernel.h"
#include "runparser.h"
#include <qdatetime.h>
#include <qdebug.h>

// TODO: we could reimplement this into something a little faster.
const QString specialChars("+-()*%/<>=");

void Parser::inputProcedure(DatumPtr nodeP, TextStream *readStream)
{
    ASTNode *node = nodeP.astnodeValue();

    // to is the command name that initiated inputProcedure(), it is the first
    // word in the input line, 'TO' or '.MACRO'.
    DatumPtr to = node->nodeName;
    if (node->countOfChildren() == 0)
        Error::notEnough(to);

    // procnameP is the name of the procedure, the second word in the input line,
    // following 'TO' or '.MACRO'.
    DatumPtr procnameP = node->childAtIndex(0);
    if (!procnameP.isWord())
        Error::doesntLike(to, procnameP);

    if (!std::isnan(procnameP.wordValue()->numberValue()))
        Error::doesntLike(to, procnameP);

    QString procname = procnameP.wordValue()->keyValue();

    QChar firstChar = (procname)[0];
    if ((firstChar == '"') || (firstChar == ':') || (firstChar == '(') || (firstChar == ')'))
        Error::doesntLike(to, procnameP);

    if (Config::get().mainProcedures()->isProcedure(procname))
        Error::procDefined(procnameP);

    // Assign the procedure's parameter names and default values.
    DatumPtr textP = DatumPtr(new List());
    DatumPtr firstLine = DatumPtr(new List());
    for (int i = 1; i < node->countOfChildren(); ++i)
    {
        firstLine.listValue()->append(node->childAtIndex(i));
    }
    textP.listValue()->append(firstLine);

    // Now read in the body
    forever
    {
        DatumPtr line = readStream->readlistWithPrompt("> ", true, true);
        if (!line.isList()) // this must be the end of the input
            break;
        if (line.listValue()->isEmpty())
            continue;
        DatumPtr first = line.listValue()->head;
        if (first.isWord())
        {
            QString firstWord = first.wordValue()->keyValue();
            if (firstWord == QObject::tr("END"))
                break;
        }
        textP.listValue()->append(line);
    }

    // The sourcetext is the raw text from which the procedure was defined.
    // We save it in case user executes `FULLTEXT`.
    DatumPtr sourceText = readStream->recentHistory();
    Config::get().mainProcedures()->defineProcedure(to, procnameP, textP, sourceText);

    Config::get().mainKernel()->sysPrint(procnameP.wordValue()->printValue());
    Config::get().mainKernel()->sysPrint(QObject::tr(" defined\n"));
}

QList<DatumPtr> *Parser::astFromList(List *aList)
{
    if (aList->astParseTimeStamp <= Config::get().mainProcedures()->timeOfLastProcedureCreation())
    {
        aList->astParseTimeStamp = QDateTime::currentMSecsSinceEpoch();

        DatumPtr runParsedList = runparse(aList);

        listIter = runParsedList.listValue()->newIterator();
        aList->astList.clear();
        advanceToken();

        try
        {
            while (currentToken != nothing)
            {
                aList->astList.push_back(parseExp());
            }
        }
        catch (Error *e)
        {
            // The AST is invalid, so clear it and rethrow the error.
            aList->astList.clear();
            aList->astParseTimeStamp = 0;
            throw e;
        }
    }
    return &aList->astList;
}

// The remaining methods parse into AST nodes.

DatumPtr Parser::parseExp()
{
    DatumPtr left = parseSumexp();
    while ((currentToken.isa() == Datum::wordType) &&
           ((currentToken.wordValue()->printValue() == "=") || (currentToken.wordValue()->printValue() == "<>") ||
            (currentToken.wordValue()->printValue() == ">") || (currentToken.wordValue()->printValue() == "<") ||
            (currentToken.wordValue()->printValue() == ">=") || (currentToken.wordValue()->printValue() == "<=")))
    {
        DatumPtr op = currentToken;
        advanceToken();
        DatumPtr right = parseSumexp();

        DatumPtr node = DatumPtr(new ASTNode(op));
        if (right == nothing)
            Error::notEnough(op);

        if (op.wordValue()->printValue() == "=")
        {
            node.astnodeValue()->kernel = &Kernel::excEqualp;
        }
        else if (op.wordValue()->printValue() == "<>")
        {
            node.astnodeValue()->kernel = &Kernel::excNotequalp;
        }
        else if (op.wordValue()->printValue() == "<")
        {
            node.astnodeValue()->kernel = &Kernel::excLessp;
        }
        else if (op.wordValue()->printValue() == ">")
        {
            node.astnodeValue()->kernel = &Kernel::excGreaterp;
        }
        else if (op.wordValue()->printValue() == "<=")
        {
            node.astnodeValue()->kernel = &Kernel::excLessequalp;
        }
        else
        {
            node.astnodeValue()->kernel = &Kernel::excGreaterequalp;
        }
        node.astnodeValue()->addChild(left);
        node.astnodeValue()->addChild(right);
        left = node;
    }
    return left;
}

DatumPtr Parser::parseSumexp()
{
    DatumPtr left = parseMulexp();
    while ((currentToken.isa() == Datum::wordType) &&
           ((currentToken.wordValue()->printValue() == "+") || (currentToken.wordValue()->printValue() == "-")))
    {
        DatumPtr op = currentToken;
        advanceToken();
        DatumPtr right = parseMulexp();

        DatumPtr node = DatumPtr(new ASTNode(op));
        if (right == nothing)
            Error::notEnough(op);

        if (op.wordValue()->printValue() == "+")
        {
            node.astnodeValue()->kernel = &Kernel::excSum;
        }
        else
        {
            node.astnodeValue()->kernel = &Kernel::excDifference;
        }
        node.astnodeValue()->addChild(left);
        node.astnodeValue()->addChild(right);
        left = node;
    }
    return left;
}

DatumPtr Parser::parseMulexp()
{
    DatumPtr left = parseminusexp();
    while ((currentToken.isa() == Datum::wordType) &&
           ((currentToken.wordValue()->printValue() == "*") || (currentToken.wordValue()->printValue() == "/") ||
            (currentToken.wordValue()->printValue() == "%")))
    {
        DatumPtr op = currentToken;
        advanceToken();
        DatumPtr right = parseminusexp();

        DatumPtr node = DatumPtr(new ASTNode(op));
        if (right == nothing)
            Error::notEnough(op);

        if (op.wordValue()->printValue() == "*")
        {
            node.astnodeValue()->kernel = &Kernel::excProduct;
        }
        else if (op.wordValue()->printValue() == "/")
        {
            node.astnodeValue()->kernel = &Kernel::excQuotient;
        }
        else
        {
            node.astnodeValue()->kernel = &Kernel::excRemainder;
        }
        node.astnodeValue()->addChild(left);
        node.astnodeValue()->addChild(right);
        left = node;
    }
    return left;
}

DatumPtr Parser::parseminusexp()
{
    DatumPtr left = parseTermexp();
    while ((currentToken.isa() == Datum::wordType) && ((currentToken.wordValue()->printValue() == "--")))
    {
        DatumPtr op = currentToken;
        advanceToken();
        DatumPtr right = parseTermexp();

        DatumPtr node = DatumPtr(new ASTNode(op));
        if (right == nothing)
            Error::notEnough(op);

        node.astnodeValue()->kernel = &Kernel::excDifference;
        node.astnodeValue()->addChild(left);
        node.astnodeValue()->addChild(right);
        left = node;
    }
    return left;
}

DatumPtr Parser::parseTermexp()
{
    if (currentToken == nothing)
        return nothing;

    if (currentToken.isa() == Datum::listType)
    {
        DatumPtr node(new ASTNode(QObject::tr("Word")));
        node.astnodeValue()->kernel = &Kernel::executeLiteral;
        node.astnodeValue()->addChild(currentToken);
        advanceToken();
        return node;
    }

    if (currentToken.isa() == Datum::arrayType)
    {
        DatumPtr node(new ASTNode(QObject::tr("Array")));
        node.astnodeValue()->kernel = &Kernel::executeLiteral;
        node.astnodeValue()->addChild(currentToken);
        advanceToken();
        return node;
    }

    Q_ASSERT(currentToken.isa() == Datum::wordType);

    // See if it's an open paren
    if (currentToken.wordValue()->printValue() == "(")
    {
        // This may be an expression or a vararg function
        DatumPtr retval;

        advanceToken();
        if ((currentToken != nothing) && currentToken.isWord())
        {
            QString cmdString = currentToken.wordValue()->keyValue();
            QChar firstChar = (cmdString)[0];
            if ((firstChar != '"') && (firstChar != ':') && ((firstChar < '0') || (firstChar > '9')) &&
                !specialChars.contains(firstChar))
            {
                retval = parseCommand(true);
            }
            else
            {
                retval = parseExp();
            }
        }
        else
        {
            retval = parseExp();
        }

        // Make sure there is a closing paren
        if ((!currentToken.isWord()) || (currentToken.wordValue()->printValue() != ")"))
        {

            Error::parenNf();
        }

        advanceToken();
        retval = parseStopIfExists(retval);
        return retval;
    }

    QChar firstChar = currentToken.wordValue()->rawValue().at(0);
    if ((firstChar == '"') || (firstChar == ':'))
    {
        QString name = currentToken.wordValue()->rawValue().right(currentToken.wordValue()->rawValue().size() - 1);
        if (!currentToken.wordValue()->isForeverSpecial)
        {
            rawToChar(name);
        }
        if (firstChar == '"')
        {
            DatumPtr node(new ASTNode(QObject::tr("QuotedName")));
            node.astnodeValue()->kernel = &Kernel::executeLiteral;
            node.astnodeValue()->addChild(DatumPtr(DatumPtr(name, currentToken.wordValue()->isForeverSpecial)));
            advanceToken();
            return node;
        }
        else
        {
            DatumPtr node(new ASTNode(QObject::tr("ValueOf")));
            node.astnodeValue()->kernel = &Kernel::executeValueOf;
            node.astnodeValue()->addChild(DatumPtr(name));
            advanceToken();
            return node;
        }
    }

    // See if it's a number
    double number = currentToken.wordValue()->numberValue();
    if (!std::isnan(number))
    {
        DatumPtr node(new ASTNode(QObject::tr("number")));
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
    if ((currentToken != nothing) && currentToken.isWord() &&
        (currentToken.wordValue()->keyValue() == QObject::tr("STOP")))
    {
        // Consume and create the STOP node
        DatumPtr stopCmd = parseCommand(false);
        stopCmd.astnodeValue()->addChild(command);
        return stopCmd;
    }
    return command;
}

DatumPtr Parser::parseCommand(bool isVararg)
{
    if (currentToken == nothing)
        return nothing;
    DatumPtr cmdP = currentToken;
    QString cmdString = cmdP.wordValue()->keyValue();

    if (cmdString == ")")
        Error::unexpectedCloseParen();

    int defaultParams = 0;
    int minParams = 0;
    int maxParams = 0;

    DatumPtr node = Config::get().mainProcedures()->astnodeFromCommand(cmdP, minParams, defaultParams, maxParams);

    advanceToken();

    int countOfChildren = 0;
    // isVararg: read all parameters until ')'
    if (isVararg)
    {
        while ((currentToken != nothing) &&
               ((!currentToken.isWord()) || (currentToken.wordValue()->printValue() != ")")))
        {
            DatumPtr child;
            if (minParams < 0)
            {
                child = currentToken;
                advanceToken();
            }
            else
            {
                child = parseExp();
            }
            node.astnodeValue()->addChild(child);
            ++countOfChildren;
        }
    }
    else if (defaultParams < 0)
    { // "Special form": read all parameters until EOL
        while (currentToken != nothing)
        {
            DatumPtr child;
            if (minParams < 0)
            {
                child = currentToken;
                advanceToken();
            }
            else
            {
                child = parseExp();
            }
            node.astnodeValue()->addChild(child);
            ++countOfChildren;
        }
    }
    else
    { // Read in the default number of params
        for (int i = defaultParams; i > 0; --i)
        {
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

void Parser::advanceToken()
{
    if (listIter.elementExists())
    {
        currentToken = listIter.element();
    }
    else
    {
        currentToken = nothing;
    }
}
