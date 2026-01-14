//===-- treeifyer.cpp - Treeifier class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Treeifier class, which is
/// responsible for treeifying a list into an Abstract Syntax Tree.
///
/// \section Operator Precedence Hierarchy
///
/// The parser uses a recursive descent structure where each function handles
/// operators at a specific precedence level. The precedence hierarchy (from
/// lowest to highest) is:
///
/// 1. Comparison operators (lowest precedence)
///    - Operators: ==, !=, <, >, <=, >=
///    - Function: treeifyExp()
///    - Associates: left-to-right
///
/// 2. Addition and subtraction
///    - Operators: +, -
///    - Function: treeifySumexp()
///    - Associates: left-to-right
///
/// 3. Multiplication, division, and modulo
///    - Operators: *, /, %
///    - Function: treeifyMulexp()
///    - Associates: left-to-right
///
/// 4. Double minus operator
///    - Operator: --
///    - Function: treeifyMinusexp()
///    - Associates: left-to-right
///
/// 5. Terminals (highest precedence)
///    - Includes: numbers, literals, quoted words, variable references (:var),
///      parentheses (for grouping), function calls, lists, arrays
///    - Function: treeifyTermexp()
///
//===----------------------------------------------------------------------===//

#include "treeifyer.h"
#include "astnode.h"
#include "cmd_strings.h"
#include "compiler.h"
#include "controller/logocontroller.h"
#include "controller/textstream.h"
#include "datum_types.h"
#include "flowcontrol.h"
#include "kernel.h"
#include "op_strings.h"
#include "runparser.h"
#include "workspace/procedures.h"
#include <qdatetime.h>
#include <qdebug.h>

using namespace StringConstants;

// TODO: we could reimplement this into something a little faster.
/// A string of special characters that are used in the treeifyer.
const QString &specialChars()
{
    static const QString specialCharsInstance("+-()*%/<>=");
    return specialCharsInstance;
}

bool isTag(const DatumPtr &node)
{
    return node.astnodeValue()->genExpression == &Compiler::genTag;
}

QList<QList<DatumPtr>> Treeifier::astFromList(List *aList)
{
    QList<QList<DatumPtr>> retval;
    // Mark the list with current timestamp to track compilation time
    aList->compileTimeStamp = QDateTime::currentMSecsSinceEpoch();

    DatumPtr runParsedList = runparse(aList);

    listIter = runParsedList.listValue();
    QList<DatumPtr> astFlatList;

    advanceToken();

    // Build a flat list of AST nodes by treeifying each root expression
    try
    {
        while (!currentToken.isNothing())
        {
            astFlatList.push_back(treeifyRootExp());
        }
    }
    catch (FCError *e)
    {
        // Reset timestamp on error to indicate failed compilation
        aList->compileTimeStamp = 0;
        throw;
    }
    // If the last ASTNode is a tag, generate a NOOP expression after it
    // to ensure that there is an instruction to jump to.
    Q_ASSERT(!astFlatList.isEmpty());
    if (isTag(astFlatList.last()))
    {
        auto *noopNode = new ASTNode(DatumPtr(keywordNoop()));
        noopNode->genExpression = &Compiler::genNoop;
        noopNode->returnType = RequestReturnNothing;
        astFlatList.append(DatumPtr(noopNode));
    }

    // Now create AST sublists for tag/block pairs.
    // Group consecutive nodes of the same type (tag or non-tag) together
    QList<DatumPtr> currentBlock;
    for (auto &node : astFlatList)
    {
        if (currentBlock.isEmpty())
            // Start a new block with the first node
            currentBlock.append(node);
        else
        {
            if (isTag(node) == isTag(currentBlock.last()))
                // Same type as previous node: add to current block
                currentBlock.append(node);
            else
            {
                // Type changed: save current block and start a new one
                retval.append(currentBlock);
                currentBlock = {node};
            }
        }
    }
    // Append the final block
    retval.append(currentBlock);
    return retval;
}

// The remaining methods treeify into AST nodes.

/// @brief Parse a root expression, handling optional STOP command.
/// @return AST node representing the root expression.
DatumPtr Treeifier::treeifyRootExp()
{
    DatumPtr node = treeifyExp();

    // The STOP command is a special case that is handled here.
    // The reason is that STOP terminates a procedure, but it may be preceded by an expression,
    // and because of tail recursion optimization, we must trampoline the expression when terminating the procedure.
    // Thus, the expression becomes a child of the STOP node.
    // e.g. [PRINT 2+2 STOP] becomes [STOP [PRINT 2+2]]
    if ((currentToken.isa() == Datum::typeWord) &&
        (currentToken.toString(Datum::ToStringFlags_Key) == cmdStrSTOP()))
    {
        auto newNode = DatumPtr(new ASTNode(currentToken));
        newNode.astnodeValue()->genExpression = &Compiler::genStop;
        newNode.astnodeValue()->returnType = RequestReturnNothing;
        newNode.astnodeValue()->addChild(node);
        node = newNode;
        advanceToken();
    }
    return node;
}

/// @brief Parse comparison expressions (lowest precedence level).
/// @details Handles comparison operators: ==, !=, <, >, <=, >=
///          These operators have the lowest precedence and associate left-to-right.
///          Example: `a < b == c` parses as `(a < b) == c`
/// @return AST node representing the comparison expression.
DatumPtr Treeifier::treeifyExp()
{
    DatumPtr left = treeifySumexp();
    while ((currentToken.isa() == Datum::typeWord) && ((currentToken.toString() == opEqual()) ||
                                                       (currentToken.toString() == opNotEqual()) ||
                                                       (currentToken.toString() == opGreaterThan()) ||
                                                       (currentToken.toString() == opLessThan()) ||
                                                       (currentToken.toString() == opGreaterEqual()) ||
                                                       (currentToken.toString() == opLessEqual())))
    {
        DatumPtr op = currentToken;
        advanceToken();
        DatumPtr right = treeifySumexp();

        auto node = DatumPtr(new ASTNode(op));
        if (!right.isASTNode())
            throw FCError::notEnoughInputs(op);

        if (op.toString() == opEqual())
        {
            node.astnodeValue()->genExpression = &Compiler::genEqualp;
            node.astnodeValue()->returnType = RequestReturnBool;
        }
        else if (op.toString() == opNotEqual())
        {
            node.astnodeValue()->genExpression = &Compiler::genNotequalp;
            node.astnodeValue()->returnType = RequestReturnBool;
        }
        else if (op.toString() == opLessThan())
        {
            node.astnodeValue()->genExpression = &Compiler::genLessp;
            node.astnodeValue()->returnType = RequestReturnBool;
        }
        else if (op.toString() == opGreaterThan())
        {
            node.astnodeValue()->genExpression = &Compiler::genGreaterp;
            node.astnodeValue()->returnType = RequestReturnBool;
        }
        else if (op.toString() == opLessEqual())
        {
            node.astnodeValue()->genExpression = &Compiler::genLessequalp;
            node.astnodeValue()->returnType = RequestReturnBool;
        }
        else
        {
            node.astnodeValue()->genExpression = &Compiler::genGreaterequalp;
            node.astnodeValue()->returnType = RequestReturnBool;
        }
        node.astnodeValue()->addChild(left);
        node.astnodeValue()->addChild(right);
        left = node;
    }
    return left;
}

/// @brief Parse addition and subtraction expressions.
/// @details Handles operators: +, -
///          These operators have higher precedence than comparisons and associate left-to-right.
///          Example: `a + b - c` parses as `(a + b) - c`
/// @return AST node representing the sum/difference expression.
DatumPtr Treeifier::treeifySumexp()
{
    DatumPtr left = treeifyMulexp();
    while ((currentToken.isa() == Datum::typeWord) && ((currentToken.toString() == opPlus()) ||
                                                       (currentToken.toString() == opMinus())))
    {
        DatumPtr op = currentToken;
        advanceToken();
        DatumPtr right = treeifyMulexp();

        auto node = DatumPtr(new ASTNode(op));
        if (!right.isASTNode())
            throw FCError::notEnoughInputs(op);

        if (op.toString() == opPlus())
        {
            node.astnodeValue()->genExpression = &Compiler::genSum;
            node.astnodeValue()->returnType = RequestReturnReal;
        }
        else
        {
            node.astnodeValue()->genExpression = &Compiler::genDifference;
            node.astnodeValue()->returnType = RequestReturnReal;
        }
        node.astnodeValue()->addChild(left);
        node.astnodeValue()->addChild(right);
        left = node;
    }
    return left;
}

/// @brief Parse multiplication, division, and modulo expressions.
/// @details Handles operators: *, /, %
///          These operators have higher precedence than addition/subtraction and associate left-to-right.
///          Example: `a * b / c` parses as `(a * b) / c`
/// @return AST node representing the product/quotient/remainder expression.
DatumPtr Treeifier::treeifyMulexp()
{
    DatumPtr left = treeifyMinusexp();
    while ((currentToken.isa() == Datum::typeWord) && ((currentToken.toString() == opMultiply()) ||
                                                       (currentToken.toString() == opDivide()) ||
                                                       (currentToken.toString() == opModulo())))
    {
        DatumPtr op = currentToken;
        advanceToken();
        DatumPtr right = treeifyMinusexp();

        auto node = DatumPtr(new ASTNode(op));
        if (!right.isASTNode())
            throw FCError::notEnoughInputs(op);

        if (op.toString() == opMultiply())
        {
            node.astnodeValue()->genExpression = &Compiler::genProduct;
            node.astnodeValue()->returnType = RequestReturnReal;
        }
        else if (op.toString() == opDivide())
        {
            node.astnodeValue()->genExpression = &Compiler::genQuotient;
            node.astnodeValue()->returnType = RequestReturnReal;
        }
        else
        {
            node.astnodeValue()->genExpression = &Compiler::genRemainder;
            node.astnodeValue()->returnType = RequestReturnReal;
        }
        node.astnodeValue()->addChild(left);
        node.astnodeValue()->addChild(right);
        left = node;
    }
    return left;
}

/// @brief Parse double-minus expressions.
/// @details Handles operator: --
///          This operator has higher precedence than multiplication/division/modulo and associates left-to-right.
///          Example: `a -- b` parses as subtraction (difference operation).
/// @return AST node representing the difference expression.
DatumPtr Treeifier::treeifyMinusexp()
{
    DatumPtr left = treeifyTermexp();
    while ((currentToken.isa() == Datum::typeWord) && ((currentToken.toString() == opDoubleMinus())))
    {
        DatumPtr op = currentToken;
        advanceToken();
        DatumPtr right = treeifyTermexp();

        auto node = DatumPtr(new ASTNode(op));
        if (!right.isASTNode())
            throw FCError::notEnoughInputs(op);

        node.astnodeValue()->genExpression = &Compiler::genDifference;
        node.astnodeValue()->addChild(left);
        node.astnodeValue()->addChild(right);
        left = node;
    }
    return left;
}

/// @brief Parse terminal expressions (highest precedence level).
/// @details Handles terminals including:
///          - Numbers (literals)
///          - Lists and arrays (literals)
///          - Quoted words (string literals)
///          - Variable references (colon-prefixed, e.g., :var)
///          - Parenthesized expressions (for grouping/overriding precedence)
///          - Function/procedure calls
///          Terminals have the highest precedence and are evaluated first.
///          Parentheses allow explicit grouping to override the default precedence.
/// @return AST node representing the terminal expression.
DatumPtr Treeifier::treeifyTermexp()
{
    if (currentToken.isNothing())
        return nothing();

    if (currentToken.isList())
    {
        DatumPtr node(new ASTNode(astNodeTypeList()));
        node.astnodeValue()->genExpression = &Compiler::genLiteral;
        node.astnodeValue()->returnType = RequestReturnDatum;
        node.astnodeValue()->addChild(currentToken);
        advanceToken();
        return node;
    }

    if (currentToken.isa() == Datum::typeArray)
    {
        DatumPtr node(new ASTNode(astNodeTypeArray()));
        node.astnodeValue()->genExpression = &Compiler::genLiteral;
        node.astnodeValue()->returnType = RequestReturnDatum;
        node.astnodeValue()->addChild(currentToken);
        advanceToken();
        return node;
    }

    Q_ASSERT(currentToken.isa() == Datum::typeWord);

    // See if it's an open paren
    if (currentToken.toString() == opOpenParen())
    {
        // This may be an expression or a vararg function
        DatumPtr retval;

        advanceToken();
        if (currentToken.isWord())
        {
            QString cmdString = currentToken.toString(Datum::ToStringFlags_Key);
            QChar firstChar = (cmdString)[0];
            if ((firstChar != '"') && (firstChar != ':') && ((firstChar < '0') || (firstChar > '9')) &&
                !specialChars().contains(firstChar))
            {
                retval = treeifyCommand(true);
            }
            else
            {
                retval = treeifyExp();
            }
        }
        else
        {
            retval = treeifyExp();
        }

        // Make sure there is a closing paren
        if ((!currentToken.isWord()) || (currentToken.toString() != opCloseParen()))
            throw FCError::parenNf();

        advanceToken();
        return retval;
    }

    QChar firstChar = currentToken.toString(Datum::ToStringFlags_Raw).at(0);
    if ((firstChar == opQuote().at(0)) || (firstChar == opColon().at(0)))
    {
        QString name = currentToken.toString(Datum::ToStringFlags_Raw).mid(1);
        if (!currentToken.wordValue()->isForeverSpecial)
        {
            rawToChar(name);
        }
        if (firstChar == opQuote().at(0))
        {
            DatumPtr node(new ASTNode(astNodeTypeQuotedWord()));
            node.astnodeValue()->genExpression = &Compiler::genLiteral;
            node.astnodeValue()->returnType = RequestReturnDatum;
            node.astnodeValue()->addChild(DatumPtr(DatumPtr(name, currentToken.wordValue()->isForeverSpecial)));
            advanceToken();
            return node;
        }
        else
        {
            DatumPtr node(new ASTNode(astNodeTypeValueOf()));
            node.astnodeValue()->genExpression = &Compiler::genValueOf;
            node.astnodeValue()->returnType = RequestReturnDatum;
            node.astnodeValue()->addChild(DatumPtr(name));
            advanceToken();
            return node;
        }
    }

    // See if it's a number
    double number = currentToken.wordValue()->numberValue();
    if (currentToken.wordValue()->numberIsValid)
    {
        DatumPtr node(new ASTNode(astNodeTypeNumber()));
        node.astnodeValue()->genExpression = &Compiler::genLiteral;
        node.astnodeValue()->returnType = RequestReturnDatum;
        node.astnodeValue()->addChild(DatumPtr(number));
        advanceToken();
        return node;
    }

    // If all else fails, it must be a function with the default number of params
    return treeifyCommand(false);
}

DatumPtr Treeifier::treeifyCommand(bool isVararg)
{
    if (currentToken.isNothing())
        return nothing();
    DatumPtr cmdP = currentToken;
    QString cmdString = cmdP.toString(Datum::ToStringFlags_Key);

    if (cmdString == opCloseParen())
        throw FCError::unexpectedCloseParen();

    // Retrieve command configuration:
    // - minParams: minimum required parameters (-1 means no minimum, raw token mode)
    // - defaultParams: default number of parameters to read (-1 means special form, read until EOL)
    // - maxParams: maximum allowed parameters (-1 means unlimited)
    auto [node, minParams, defaultParams, maxParams] = Config::get().mainProcedures()->astnodeFromCommand(cmdP);

    advanceToken();

    int countOfChildren = 0;
    
    // Strategy 1: Variable-argument function (e.g., function calls in parentheses)
    // Read all parameters until encountering a closing parenthesis ')'
    if (isVararg)
    {
        // Continue reading while there are tokens and we haven't seen the closing paren
        // Condition: token exists AND (it's not a word OR it's not the closing paren)
        while ((!currentToken.isNothing()) &&
               ((!currentToken.isWord()) || (currentToken.toString() != opCloseParen())))
        {
            DatumPtr child;
            // If minParams < 0, this command takes raw tokens (no expression parsing)
            // Otherwise, recursively parse the token as an expression
            if (minParams < 0)
            {
                // Raw token mode: use token as-is without parsing
                child = currentToken;
                advanceToken();
            }
            else
            {
                // Expression mode: recursively parse the parameter as an expression
                child = treeifyExp();
            }
            node.astnodeValue()->addChild(child);
            ++countOfChildren;
        }
    }
    // Strategy 2: Special form (e.g., TO or HELP)
    // Read all parameters until end of line (EOL)
    else if (defaultParams < 0)
    {
        // Special forms consume all remaining tokens until nothing is left
        while (!currentToken.isNothing())
        {
            DatumPtr child;
            // Same raw token vs expression logic as vararg case
            if (minParams < 0)
            {
                // Raw token mode: use token as-is
                child = currentToken;
                advanceToken();
            }
            else
            {
                // Expression mode: parse parameter as expression
                child = treeifyExp();
            }
            node.astnodeValue()->addChild(child);
            ++countOfChildren;
        }
    }
    // Strategy 3: Fixed-parameter function (most common case)
    // Read exactly the default number of parameters
    else
    {
        // Read exactly defaultParams number of parameters
        for (int i = defaultParams; i > 0; --i)
        {
            // Check for premature end of input
            if (currentToken.isNothing())
                throw FCError::notEnoughInputs(cmdP);
            // Always parse as expression for fixed-parameter functions
            DatumPtr child = treeifyExp();
            node.astnodeValue()->addChild(child);
            ++countOfChildren;
        }
    }

    // Validate parameter count against constraints
    // Ensure we have at least the minimum required parameters
    if (countOfChildren < minParams)
        throw FCError::notEnoughInputs(node.astnodeValue()->nodeName);
    // Ensure we don't exceed the maximum allowed parameters (if maxParams is set)
    if ((countOfChildren > maxParams) && (maxParams > -1))
        throw FCError::tooManyInputs(node.astnodeValue()->nodeName);

    return node;
}

void Treeifier::advanceToken()
{
    if (listIter != EmptyList::instance())
    {
        currentToken = listIter->head;
        listIter = listIter->tail.listValue();
    }
    else
    {
        currentToken = nothing();
    }
}
