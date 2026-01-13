//===-- op_strings.cpp - String constants implementation -------*- C++ -*-===//
//
// Copyright 2017-2025 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of string constants.
///
//===----------------------------------------------------------------------===//

#include "op_strings.h"
#include <QObject>

namespace StringConstants
{
//=== Operator Strings ===//

const QString &opEqual()
{
    static const QString str = QStringLiteral("=");
    return str;
}

const QString &opNotEqual()
{
    static const QString str = QStringLiteral("<>");
    return str;
}

const QString &opLessThan()
{
    static const QString str = QStringLiteral("<");
    return str;
}

const QString &opGreaterThan()
{
    static const QString str = QStringLiteral(">");
    return str;
}

const QString &opLessEqual()
{
    static const QString str = QStringLiteral("<=");
    return str;
}

const QString &opGreaterEqual()
{
    static const QString str = QStringLiteral(">=");
    return str;
}

const QString &opPlus()
{
    static const QString str = QStringLiteral("+");
    return str;
}

const QString &opMinus()
{
    static const QString str = QStringLiteral("-");
    return str;
}

const QString &opMultiply()
{
    static const QString str = QStringLiteral("*");
    return str;
}

const QString &opDivide()
{
    static const QString str = QStringLiteral("/");
    return str;
}

const QString &opModulo()
{
    static const QString str = QStringLiteral("%");
    return str;
}

const QString &opDoubleMinus()
{
    static const QString str = QStringLiteral("--");
    return str;
}

const QString &opOpenParen()
{
    static const QString str = QStringLiteral("(");
    return str;
}

const QString &opCloseParen()
{
    static const QString str = QStringLiteral(")");
    return str;
}

const QString &opQuote()
{
    static const QString str = QStringLiteral("\"");
    return str;
}

const QString &opColon()
{
    static const QString str = QStringLiteral(":");
    return str;
}

//=== Keyword Strings (Translatable) ===//

// const QString &keywordStop()
// {
//     static const QString str = QObject::tr("STOP");
//     return str;
// }

const QString &keywordNoop()
{
    static const QString str = QObject::tr("NOOP");
    return str;
}

const QString &astNodeTypeList()
{
    static const QString str = QObject::tr("List");
    return str;
}

const QString &astNodeTypeArray()
{
    static const QString str = QObject::tr("Array");
    return str;
}

const QString &astNodeTypeQuotedWord()
{
    static const QString str = QObject::tr("QuotedWord");
    return str;
}

const QString &astNodeTypeValueOf()
{
    static const QString str = QObject::tr("ValueOf");
    return str;
}

const QString &astNodeTypeNumber()
{
    static const QString str = QObject::tr("number");
    return str;
}
} // namespace StringConstants
