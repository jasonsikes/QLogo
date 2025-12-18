#ifndef PARSER_H
#define PARSER_H

//===-- qlogo/parser.h - Parser class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Parser class, which is responsible
/// for parsing a QLogo list into Abstract Syntax Trees and for reading user-defined
/// procedures from a text stream.
///
//===----------------------------------------------------------------------===//

#include "datum_ptr.h"

#include <QHash>

class Procedures;
class Kernel;
class TextStream;

/// @brief The Parser class is responsible for parsing a QLogo list into Abstract
/// Syntax Trees and for reading user-defined procedures from a text stream.
class Parser
{
    DatumPtr currentToken;

    static QHash<List *, QList<QList<DatumPtr>>> astListTable;

    void advanceToken();
    List *listIter;

    DatumPtr parseRootExp();
    DatumPtr parseExp();
    DatumPtr parseSumexp();
    DatumPtr parseMulexp();
    DatumPtr parseminusexp();
    DatumPtr parseTermexp();
    DatumPtr parseCommand(bool isVararg);

  public:

    /// @brief Parse a QLogo list into a list of Abstract Syntax Trees.
    /// @param aList The list to parse.
    /// @returns A list of AST nodes.
    /// @note A QLogo list may hold multiple trees. For example,
    /// `[HOME FORWARD 100]` is a list containing two trees, one for the `HOME`
    /// command and one for the `FORWARD` command.
    QList<QList<DatumPtr>> astFromList(List *aList);

    /// @brief Read a user-defined procedure from a text stream.
    /// @param node An AST node that contains the first line of the procedure.
    /// @param readStream The text stream to read the procedure from.
    /// @note The first line will have already been read and assigned to the children nodes of `node`.
    /// The procedure is read until the end of the text stream or until a line
    /// starting with `END` is encountered, and the text is read into the body of
    /// the procedure.
    void inputProcedure(ASTNode *node, TextStream *readStream);

    /// @brief Destroy the AST built from a list, if it exists.
    /// @param aList The list to destroy the AST for.
    /// @note This should only be called by a list destructor.
    /// @note This will, in turn, destroy the compiled function for the AST.
    static void destroyAstForList(List *aList);
};

#endif // PARSER_H
