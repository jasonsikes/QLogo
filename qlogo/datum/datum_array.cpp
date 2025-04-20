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
    while (iter.elementExists())
    {
        array.append(iter.element());
    }
}

Array::~Array()
{
}

QString Array::printValue(bool fullPrintp, int printDepthLimit, int printWidthLimit)
{
    // This is used to prevent infinite recursion when printing arrays.
    static QList<void *> aryVisited;

    // If this array has already been printed, don't print it again.
    if (!aryVisited.contains(this))
    {
        aryVisited.push_back(this);
        auto iter = array.begin();
        if (iter == array.end())
        {
            return "{}";
        }
        if ((printDepthLimit == 0) || (printWidthLimit == 0))
        {
            return "{...}";
        }
        int printWidth = printWidthLimit;
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
            retval.append(iter->showValue(fullPrintp, printDepthLimit - 1, printWidthLimit));
            --printWidth;
        } while (++iter != array.end());
        retval += "}";
        aryVisited.removeOne(this);
        return retval;
    }
    return "...";
}

QString Array::showValue(bool fullPrintp, int printDepthLimit, int printWidthLimit)
{
    return printValue(fullPrintp, printDepthLimit, printWidthLimit);
}
