#ifndef DATUM_ASTNODE_H
#define DATUM_ASTNODE_H


//===-- qlogo/datum_astnode.h - ASTNode class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the ASTNode class, which is an
/// abstract syntax tree node.
///
//===----------------------------------------------------------------------===//



#include "datum_datump.h"
#include <QVector>


/// \brief A node of QLogo's Abstract Syntax Tree.
///
/// Before execution, a list is parsed into a list of executable nodes. Each node
/// contains a name, a pointer to the KernelMethod that does the actual execution,
/// and an array of zero or more children.
class ASTNode : public Datum {
protected:
    QVector<DatumP> children;

    void addToPool();

public:

    /// Initialize a node.
    void init(DatumP aName);

    /// A human-readable string. Usually the command name.
    DatumP nodeName;

    // TODO: This is badly misnamed! Should be called "method".
    // (This got caught in the mass renaming.)
    /// A pointer to the kernel method that should be called when executing this node.
    KernelMethod kernel;

    /// Add a child to the node.
    void addChild(DatumP aChild);

    /// Returns the child at the specified index.
    DatumP childAtIndex(unsigned index);

    /// Returns the number of children that this node owns.
    int countOfChildren();


    /// Create an ASTNode with no name.
    ASTNode() {}

    /// Create an ASTNode with the node's name.
    ASTNode(DatumP aNodeName);

    /// Create an ASTNode with the node's name.
    ASTNode(QString aNodeName);

    ~ASTNode();
    DatumType isa();

    /// For debugging. To be used when printing out the AST.
    QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                       int printWidthLimit = -1);

    /// For debugging. To be used when printing out the AST.
    QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                      int printWidthLimit = -1);

};

ASTNode * astNodeWithName(const QString aName);

ASTNode * astNodeWithName(DatumP aName);

class ASTNodePool : public DatumPool
{
    void createNewDatums(QVector<Datum*> &box);
};



#endif // DATUM_ASTNODE_H
