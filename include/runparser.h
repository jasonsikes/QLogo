#ifndef RUNPARSER_H
#define RUNPARSER_H

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
/// This file contains the declaration of the Runparser class, which is
/// responsible for parsing text and generating a list of tokens.
///
/// for example, `[PRINT 2+2]`, a list of two words, will be parsed into a list of
/// four words, `[PRINT 2 + 2]`. In QLogo a token is a word, list, or array.
//===----------------------------------------------------------------------===//

#include "datum_types.h"

class Procedures;
class Kernel;
class TextStream;

class Runparser
{
    ListBuilder *runparseBuilder = nullptr;
    QString::iterator runparseCIter;
    QString::iterator runparseCEnd;
    bool isRunparseSourceSpecial;
    void runparseSpecialchars();
    void runparseMinus();
    DatumPtr runparseNumber(); // returns a number if successful
    void runparseQuotedWord();
    void runparseString();

    ListIterator listIter;

  public:
    /// @brief Parse a QLogo word or list into a list of tokens.
    /// @param src A QLogo word or list to parse.
    /// @returns A list of tokens.
    DatumPtr doRunparse(DatumPtr src);
};

/// @brief Parse a QLogo word or list into a list of tokens.
/// @param src A QLogo word or list to parse.
/// @returns A list of tokens.
///
/// @note This function is a wrapper around the `Runparser` class.
DatumPtr runparse(const DatumPtr &src);

#endif // RUNPARSER_H
