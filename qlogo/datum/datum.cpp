
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

#include "datum_core.h"
#include <QObject>
#include <qdebug.h>
#include <unistd.h>
#include "sharedconstants.h"

/// @brief The number of Datum objects in use.
int countOfNodes = 0;
/// @brief The maximum number of Datum objects that have ever been in use.
int maxCountOfNodes = 0;


Datum::Datum() : retainCount(0), isa(Datum::DatumType(typeNothing | typePersistentMask))
{
    ++countOfNodes;
    if (countOfNodes > maxCountOfNodes)
        maxCountOfNodes = countOfNodes;
    if (Config::get().showCON)
        qDebug() <<this << " con++: " << countOfNodes;
}

Datum *Datum::getInstance()
{
    static Datum singleton;
    return &singleton;
}

Datum::~Datum()
{
    --countOfNodes;
    if (Config::get().showCON)
        qDebug() <<this << " --con: " << countOfNodes;
}

QString Datum::toString( ToStringFlags flags, int printDepthLimit, int printWidthLimit, VisitedSet *visited) const
{
    return QObject::tr("nothing");
}
