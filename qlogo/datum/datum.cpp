
//===-- qlogo/datum.cpp - Datum class and subclasses implementation --*- C++ -*-===//
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
/// This file contains the implementation of the Datum class, which is the superclass
/// of Word, List, and Array.
///
//===----------------------------------------------------------------------===//

#include "datum.h"
#include <QObject>
#include <qdebug.h>
#include <unistd.h>
#include "sharedconstants.h"

/// @brief The number of Datum objects in use.
int countOfNodes = 0;
/// @brief The maximum number of Datum objects that have ever been in use.
int maxCountOfNodes = 0;

/// @brief Get the number of Datum objects in use and the maximum number of Datum objects
/// that have ever been in use.
/// @return A List of two Words, the first contains the number of Datum objects in use and the
/// second contains the maximum number of Datum objects that have ever been in use.
DatumPtr nodes()
{
    int a = countOfNodes;
    int b = maxCountOfNodes;

    maxCountOfNodes = countOfNodes;

    List *retval = new List();
    retval = new List(DatumPtr(b), retval);
    retval = new List(DatumPtr(a), retval);
    return DatumPtr(retval);
}

Datum::Datum() : retainCount(0)
{
    ++countOfNodes;
    if (countOfNodes > maxCountOfNodes)
        maxCountOfNodes = countOfNodes;
    if (Config::get().showCON)
        qDebug() <<this << " con++: " << countOfNodes;
}

Datum::~Datum()
{
    --countOfNodes;
    if (Config::get().showCON)
        qDebug() <<this << " --con: " << countOfNodes;
}

QString Datum::printValue(bool, int, int)
{
    return QObject::tr("nothing");
}

QString Datum::showValue(bool, int, int)
{
    return printValue();
}

// Values to represent no data (nullptr)
Datum notADatum;
DatumPtr nothing(&notADatum);
