//===-- qlogo/datum_array.cpp - Array class implementation --*- C++ -*-===//
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
/// This file contains the implementation of the Array class.
/// An array may contain words, lists or arrays.
/// Is is implemented using QVector.
///
//===----------------------------------------------------------------------===//

#include "datum_types.h"
#include "visited.h"

Array::Array(int aOrigin, int aSize)
{
    isa = Datum::typeArray;
    origin = aOrigin;
    array.reserve(aSize);
}

Array::Array(int aOrigin, List *source)
{
    isa = Datum::typeArray;
    origin = aOrigin;
    VisitedSet visited;
    while (source != EmptyList::instance())
    {
        if (visited.contains(source))
        {
            // We have a cycle, so stop copying the list.
            break;
        }
        visited.add(source);
        array.append(source->head);
        source = source->tail.listValue();
    }
}

Array::~Array() = default;

// TODO: Make element access protected to properly enforce origin addition and bounds checking.

QString Array::toString(ToStringFlags flags, int printDepthLimit, int printWidthLimit, VisitedSet *visited) const
{
    if (array.isEmpty())
    {
        return "{}";
    }

    std::unique_ptr<VisitedSet> localVisited;
    if (visited == nullptr)
    {
        localVisited = std::make_unique<VisitedSet>();
        visited = localVisited.get();
    }

    if ((printDepthLimit == 0) || (visited->contains(this)))
    {
        return "{...}";
    }

    visited->add(this);
    auto iter = array.constBegin();
    int printWidth = printWidthLimit;

    // Any words within a collection don't need to be formatted as source code.
    flags = (Datum::ToStringFlags)(flags & ~(Datum::ToStringFlags_Source));
    // Any lists within a collection need to show their brackets.
    flags = (Datum::ToStringFlags)(flags | Datum::ToStringFlags_Show);

    QString retval = "{";
    do
    {
        if (iter != array.constBegin())
            retval.append(' ');
        if (printWidth == 0)
        {
            retval.append("...");
            break;
        }
        retval.append(iter->toString(flags, printDepthLimit - 1, printWidthLimit, visited));
        --printWidth;
    } while (++iter != array.constEnd());
    retval.append("}");
    if ((origin != 1) && ((flags & Datum::ToStringFlags_FullPrint) != 0))
    {
        retval.append("@" + QString::number(origin));
    }
    visited->remove(this);
    return retval;
}
