//===-- op_strings.h - String constants -------*- C++ -*-===//
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
/// This file contains declarations for string constants used throughout
/// the codebase. This provides a single source of truth for all
/// string literals used in the codebase.
///
//===----------------------------------------------------------------------===//

#ifndef OP_STRINGS_H
#define OP_STRINGS_H

#include <QString>

/// @brief Namespace containing string constants used throughout the codebase.
///
/// This namespace provides a centralized location for all string constants
/// used in the codebase.
namespace StringConstants
{
//=== Operator Strings ===//
// These return QString references for non-translatable operators

/// @brief Equality operator string.
/// @return Reference to the "=" string.
const QString &opEqual();

/// @brief Not-equal operator string.
/// @return Reference to the "<>" string.
const QString &opNotEqual();

/// @brief Less-than operator string.
/// @return Reference to the "<" string.
const QString &opLessThan();

/// @brief Greater-than operator string.
/// @return Reference to the ">" string.
const QString &opGreaterThan();

/// @brief Less-than-or-equal operator string.
/// @return Reference to the "<=" string.
const QString &opLessEqual();

/// @brief Greater-than-or-equal operator string.
/// @return Reference to the ">=" string.
const QString &opGreaterEqual();

/// @brief Addition operator string.
/// @return Reference to the "+" string.
const QString &opPlus();

/// @brief Subtraction operator string.
/// @return Reference to the "-" string.
const QString &opMinus();

/// @brief Multiplication operator string.
/// @return Reference to the "*" string.
const QString &opMultiply();

/// @brief Division operator string.
/// @return Reference to the "/" string.
const QString &opDivide();

/// @brief Modulo operator string.
/// @return Reference to the "%" string.
const QString &opModulo();

/// @brief Double-minus operator string.
/// @return Reference to the "--" string.
const QString &opDoubleMinus();

/// @brief Open parenthesis operator string.
/// @return Reference to the "(" string.
const QString &opOpenParen();

/// @brief Close parenthesis operator string.
/// @return Reference to the ")" string.
const QString &opCloseParen();

/// @brief Double quote operator string.
/// @return Reference to the "\"" string.
const QString &opQuote();

/// @brief Colon operator string (used for variable references).
/// @return Reference to the ":" string.
const QString &opColon();

/// @brief Question operator string.
/// @return Reference to the "?" string.
const QString &opQuestion();

/// @brief Zero operator string.
/// @return Reference to the "0" string.
const QString &opNumberZero();

//=== Keyword Strings (Translatable) ===//
// These return QString references for translatable keywords

/// @brief "NOOP" keyword string.
/// @return Reference to the translated "NOOP" string.
const QString &keywordNoop();

/// @brief "List" AST node type string.
/// @return Reference to the translated "List" string.
const QString &astNodeTypeList();

/// @brief "Array" AST node type string.
/// @return Reference to the translated "Array" string.
const QString &astNodeTypeArray();

/// @brief "QuotedWord" AST node type string.
/// @return Reference to the translated "QuotedWord" string.
const QString &astNodeTypeQuotedWord();

/// @brief "ValueOf" AST node type string.
/// @return Reference to the translated "ValueOf" string.
const QString &astNodeTypeValueOf();

/// @brief "number" AST node type string.
/// @return Reference to the translated "number" string.
const QString &astNodeTypeNumber();
} // namespace StringConstants

#endif // OP_STRINGS_H
