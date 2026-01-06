
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

void ASTNode::addChild(const DatumPtr &aChild)
{
    children.push_back(aChild);
}

int ASTNode::countOfChildren() const
{
    return (int)children.size();
}

DatumPtr ASTNode::childAtIndex(unsigned index) const
{
    return children.at(index);
}

ASTNode::ASTNode(const DatumPtr &aNodeName)
{
    isa = Datum::typeASTNode;
    children.clear();
    nodeName = aNodeName;
}

ASTNode::ASTNode(const QString &aNodeName)
{
    isa = Datum::typeASTNode;
    children.clear();
    nodeName = DatumPtr(aNodeName);
}

ASTNode::~ASTNode() = default;

// For debugging. Parameters are ignored.
QString ASTNode::toString(ToStringFlags, int, int, VisitedSet *) const
{
    QString retval = QString("( %1").arg(nodeName.toString());
    for (auto &iter : children)
    {
        retval.append(QString(" %2").arg(iter.toString()));
    }
    retval.append(" )");
    return retval;
}
