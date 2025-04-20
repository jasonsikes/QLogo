
//===-- qlogo/datum_astNode.cpp - AstNode class implementation --*- C++ -*-===//
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
/// This file contains the implementation of the AstNode class, which is a node
/// in the Abstract Syntax Tree.
///
//===----------------------------------------------------------------------===//

#include "astnode.h"
#include <qdebug.h>

void ASTNode::addChild(DatumPtr aChild)
{
    children.push_back(aChild);
}

int ASTNode::countOfChildren()
{
    return (int)children.size();
}

DatumPtr ASTNode::childAtIndex(unsigned index)
{
    return children.at(index);
}

ASTNode::ASTNode(DatumPtr aNodeName)
{
    isa = Datum::typeASTNode;
    children.clear();
    nodeName = aNodeName;
    genExpression = nullptr;
    returnType = RequestReturnVoid;
}

ASTNode::ASTNode(QString aNodeName)
{
    isa = Datum::typeASTNode;
    children.clear();
    nodeName = DatumPtr(aNodeName);
    genExpression = nullptr;
    returnType = RequestReturnVoid;
}

ASTNode::~ASTNode()
{
}

// For debugging. Parameters are ignored.
QString ASTNode::printValue(bool, int, int)
{
    QString retval = QString("( %1").arg(nodeName.showValue());
    for (auto &iter : children)
    {
        retval.append(QString(" %2").arg(iter.showValue()));
    }
    retval.append(" )");
    return retval;
}

// For debugging. Parameters are ignored.
QString ASTNode::showValue(bool, int, int)
{
    return printValue();
}
