
//===-- qlogo/datum_astNode.cpp - AstNode class implementation -------*-
// C++ -*-===//
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
/// This file contains the implementation of the AstNode class, which is the basic
/// unit of the Abstract Syntax Tree.
///
//===----------------------------------------------------------------------===//

#include "datum_astnode.h"
#include <qdebug.h>


static ASTNodePool pool;

void ASTNode::addChild(DatumP aChild) { children.push_back(aChild); }

int ASTNode::countOfChildren() { return (int)children.size(); }

DatumP ASTNode::childAtIndex(unsigned index) { return children.at(index); }


ASTNode * ASTNode::alloc(DatumP aNodeName) {
    ASTNode *retval = (ASTNode *)pool.alloc();
    retval->children.clear();
    retval->nodeName = aNodeName;
    retval->kernel = NULL;
    return retval;
}


ASTNode * ASTNode::alloc(QString aNodeName) {
    return alloc(DatumP(aNodeName));
}

ASTNode::~ASTNode() {
}

void ASTNode::addToPool()
{
  children.clear();
  pool.dealloc(this);
}

Datum::DatumType ASTNode::isa() { return astnodeType; }


QString ASTNode::printValue(bool, int, int) {
  QString retval = QString("( %1").arg(nodeName.showValue());
  for (auto &iter : children) {
    retval.append(QString(" %2").arg(iter.showValue()));
  }
  retval.append(" )");
  return retval;
}

QString ASTNode::showValue(bool, int, int) { return printValue(); }



void ASTNodePool::createNewDatums(QVector<Datum*> &box)
{
  const int count = 40;

  // This block is never deleted.
  ASTNode *block = new ASTNode[count];

  box.reserve(count);
  for (int i = 0; i < count; ++i) {
    box.push_back(&block[i]);
  }
}


