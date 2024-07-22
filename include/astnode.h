#ifndef DATUM_ASTNODE_H
#define DATUM_ASTNODE_H

//===-- qlogo/datum/astnode.h - ASTNode class definition -------*- C++ -*-===//
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

#include "datum.h"

/// @brief A node of QLogo's Abstract Syntax Tree.
///
/// @details Before execution, a list is parsed into a QList of executable nodes. Each node
/// contains its name, a pointer to the KernelMethod that will perform the actual execution,
/// and an array of zero or more child ASTNodes.
class ASTNode : public Datum
{
  protected:
    QList<DatumPtr> children;

  public:
    /// @brief Allocate an ASTNode with the node's name as a Word.
    ASTNode(DatumPtr aNodeName);

    /// @brief Allocate an ASTNode with the node's name as a QString.
    ASTNode(QString aNodeName);

    /// @brief A human-readable string. Usually the command name.
    DatumPtr nodeName;

    /// @brief A pointer to the kernel method that should be called when executing this node.
    KernelMethod kernel;

    /// @brief Add a child to the node. Child will be added to the end of the children list.
    /// @param aChild The child to add.
    void addChild(DatumPtr aChild);

    /// @brief Returns the child at the specified index.
    /// @param index The index of the child to return.
    /// @return The child at the specified index.
    DatumPtr childAtIndex(unsigned index);

    /// @brief Returns the number of children that this node owns.
    /// @return The number of children that this node owns.
    int countOfChildren();

    /// @brief Create an invalid ASTNode.
    ASTNode()
    {
    }

    /// @brief Destructor.
    ~ASTNode();

    /// @brief Returns the type of this node.
    /// @return The type of this node.
    DatumType isa();

    /// @brief For debugging. To be used when printing out the AST. Parameters are ignored.
    /// @return A string representation of this node.
    QString printValue(bool = false, int printDepthLimit = -1, int printWidthLimit = -1);

    /// @brief For debugging. To be used when printing out the AST. Parameters are ignored.
    /// @return A string representation of this node.
    QString showValue(bool = false, int printDepthLimit = -1, int printWidthLimit = -1);
};

#endif // DATUM_ASTNODE_H
