
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
#include "sharedconstants.h"
#include <QObject>
#include <algorithm>

namespace
{
int g_countOfNodes = 0;
int g_maxCountOfNodes = 0;
}

Datum::Datum()
{
    ++g_countOfNodes;
    g_maxCountOfNodes = std::max(g_maxCountOfNodes, g_countOfNodes);
    if (Config::get().showCON_)
        qInfo() << this << " con++: " << g_countOfNodes;
}

Datum *Datum::notADatum()
{
    static Datum singleton;
    return &singleton;
}

int Datum::countOfNodes()
{
    return g_countOfNodes;
}

int Datum::maxCountOfNodes()
{
    return g_maxCountOfNodes;
}

void Datum::resetMaxCountOfNodes()
{
    g_maxCountOfNodes = g_countOfNodes;
}

Datum::~Datum()
{
    --g_countOfNodes;
    if (Config::get().showCON_)
        qInfo() << this << " --con: " << g_countOfNodes;
}

QString Datum::toString(ToStringFlags flags, int printDepthLimit, int printWidthLimit, VisitedSet *visited) const
{
    return QObject::tr("nothing");
}
