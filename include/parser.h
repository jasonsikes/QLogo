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

#include "datum.h"
#include <QHash>

class Procedures;
class Kernel;
class TextStream;

/// @brief The Parser class is responsible for parsing a QLogo list into Abstract
/// Syntax Trees and for reading user-defined procedures from a text stream.
class Parser
{
    DatumPtr currentToken;

    void advanceToken();
    ListIterator listIter;

    DatumPtr parseExp();
    DatumPtr parseSumexp();
    DatumPtr parseMulexp();
    DatumPtr parseminusexp();
    DatumPtr parseTermexp();
    DatumPtr parseCommand(bool isVararg);
    DatumPtr parseStopIfExists(DatumPtr command);
    DatumPtr astnodeFromCommand(DatumPtr command, int &minParams, int &defaultParams, int &maxParams);

  public:

    /// @brief Parse a QLogo list into a list of Abstract Syntax Trees.
    /// @param aList The list to parse.
    /// @returns A list of AST nodes.
    /// @note A QLogo list may hold multiple trees. For example,
    /// `[HOME FORWARD 100]` is a list containing two trees, one for the `HOME`
    /// command and one for the `FORWARD` command.
    QList<DatumPtr> *astFromList(List *aList);

    /// @brief Read a user-defined procedure from a text stream.
    /// @param nodeP An AST node that contains the first line of the procedure.
    /// @param readStream The text stream to read the procedure from.
    /// @note The first line will have already been read and assigned to the children nodes of `nodeP`.
    /// The procedure is read until the end of the text stream or until a line
    /// starting with `END` is encountered, and the text is read into the body of
    /// the procedure.
    void inputProcedure(DatumPtr nodeP, TextStream *readStream);
};

#endif // PARSER_H
