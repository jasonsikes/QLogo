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

#include "datum.h"
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
    auto iter = source->newIterator();
    origin = aOrigin;
    VisitedSet visited;
    while (iter.elementExists())
    {
        DatumPtr item = iter.element();
        if (visited.contains(item.datumValue()))
        {
            // We have a cycle, so stop copying the list.
            break;
        }
        visited.add(item.datumValue());
        array.append(item);
    }
}

Array::~Array()
{
}

// TODO: Make element access protected to properly enforce origin addition and bounds checking.

QString Array::toString( ToStringFlags flags, int printDepthLimit, int printWidthLimit, VisitedSet *visited)
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

    if ( (printDepthLimit == 0) || (visited->contains(this)))
    {
        return "{...}";
    }

    visited->add(this);
    auto iter = array.begin();
    int printWidth = printWidthLimit;

    // Any words within a collection don't need to be formatted as source code.
    flags = (Datum::ToStringFlags)(flags & ~(Datum::ToStringFlags_Source));
    // Any lists within a collection need to show their brackets.
    flags = (Datum::ToStringFlags)(flags | Datum::ToStringFlags_Show);

    QString retval = "{";
    do
    {
        if (iter != array.begin())
            retval.append(' ');
        if (printWidth == 0)
        {
            retval.append("...");
            break;
        }
        retval.append(iter->toString(flags, printDepthLimit - 1, printWidthLimit, visited));
        --printWidth;
    } while (++iter != array.end());
    retval.append("}");
    if ((origin != 1) && ((flags & Datum::ToStringFlags_FullPrint) != 0))
    {
        retval.append("@" + QString::number(origin));
    }
    visited->remove(this);
    return retval;
}
