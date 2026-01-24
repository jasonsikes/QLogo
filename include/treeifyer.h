#ifndef TREEIFIER_H
#define TREEIFIER_H

//===-- qlogo/treeifyer.h - Treeifier class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Treeifier class, which is responsible
/// for treeifying a QLogo list into an Abstract Syntax Tree.
///
//===----------------------------------------------------------------------===//

#include "datum_ptr.h"

#include <QHash>

class Procedures;
class TextStream;

/// @brief The Treeifier class contains a singleton class with a single public method,
/// astFromList(), which is used to treeify a QLogo list into an Abstract Syntax Tree.
class Treeifier
{
    DatumPtr currentToken;

    QList<QList<DatumPtr>> retval;

    void advanceToken();
    List *listIter;

    DatumPtr treeifyRootExp();
    DatumPtr treeifyExp();
    DatumPtr treeifySumexp();
    DatumPtr treeifyMulexp();
    DatumPtr treeifyMinusexp();
    DatumPtr treeifyTermexp();
    DatumPtr treeifyCommand(bool isVararg);

    /// @brief Private constructor for singleton pattern.
    Treeifier() = default;

    ~Treeifier() = default;
    Treeifier(const Treeifier &) = delete;
    Treeifier(Treeifier &&) = delete;
    Treeifier &operator=(const Treeifier &) = delete;
    Treeifier &operator=(Treeifier &&) = delete;

  public:

    /// @brief Treeify a QLogo list into a list of Abstract Syntax Trees.
    /// @param aList The list to treeify.
    /// @returns A list of AST nodes.
    /// @note A QLogo list may hold multiple trees. For example,
    /// `[HOME FORWARD 100]` is a list containing two trees, one for the `HOME`
    /// command and one for the `FORWARD` command.
    static const QList<QList<DatumPtr>> &astFromList(List *aList);
};

/// @brief Convenience function to check if a node is a tag.
/// @param node The node to check.
/// @return True if the node is a tag, false otherwise.
bool isTag(const DatumPtr &node);

#endif // TREEIFIER_H
