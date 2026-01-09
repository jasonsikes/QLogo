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
/// for treeifying a QLogo list into Abstract Syntax Trees and for reading user-defined
/// procedures from a text stream.
///
//===----------------------------------------------------------------------===//

#include "datum_ptr.h"

#include <QHash>

class Procedures;
class Kernel;
class TextStream;

/// @brief The Treeifier class is responsible for treeifying a QLogo list into Abstract
/// Syntax Trees and for reading user-defined procedures from a text stream.
class Treeifier
{
    DatumPtr currentToken;

    static QHash<List *, QList<QList<DatumPtr>>> astListTable;

    void advanceToken();
    List *listIter;

    DatumPtr treeifyRootExp();
    DatumPtr treeifyExp();
    DatumPtr treeifySumexp();
    DatumPtr treeifyMulexp();
    DatumPtr treeifyminusexp();
    DatumPtr treeifyTermexp();
    DatumPtr treeifyCommand(bool isVararg);

  public:
    /// @brief Treeify a QLogo list into a list of Abstract Syntax Trees.
    /// @param aList The list to treeify.
    /// @returns A list of AST nodes.
    /// @note A QLogo list may hold multiple trees. For example,
    /// `[HOME FORWARD 100]` is a list containing two trees, one for the `HOME`
    /// command and one for the `FORWARD` command.
    QList<QList<DatumPtr>> astFromList(List *aList);
};

#endif // TREEIFIER_H
