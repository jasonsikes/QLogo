#ifndef PARSER_H
#define PARSER_H

//===-- qlogo/parser.h - Parser class definition -------*- C++ -*-===//
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
