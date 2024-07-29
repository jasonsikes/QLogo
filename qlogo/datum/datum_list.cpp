
//===-- qlogo/datum_list.cpp - List class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the List class, which is the general
/// container of sequence data. A list may contain words, lists or arrays.
/// Is is implemented by using a linked list.
///
//===----------------------------------------------------------------------===//

#include "datum.h"
#include <QObject>
#include <qdebug.h>

QList<void *> listVisited;
QList<void *> otherListVisited;

List::List()
{
    astParseTimeStamp = 0;
}

List::List(Array *source)
{
    for (auto &aryIter : source->array)
    {
        append(aryIter);
    }
}

List::List(DatumPtr item, List *srcList)
{
    head = item;
    if (!srcList->head.isNothing())
    {
        tail = DatumPtr(srcList);
        lastNode = srcList->lastNode;
    }
    else
    {
        lastNode = DatumPtr(this);
    }
    astParseTimeStamp = 0;
}

List::~List()
{
}

List::List(List *source)
{
    astParseTimeStamp = 0;
    head = source->head;
    lastNode = source->lastNode;
}

Datum::DatumType List::isa()
{
    return listType;
}

QString List::printValue(bool fullPrintp, int printDepthLimit, int printWidthLimit)
{
    if (head.isNothing())
        return "";
    DatumPtr iter = DatumPtr(this);
    if ((printDepthLimit == 0) || (printWidthLimit == 0))
    {
        return "...";
    }
    int printWidth = printWidthLimit - 1;
    QString retval = iter.listValue()->head.showValue(fullPrintp, printDepthLimit - 1, printWidthLimit);
    while (iter.listValue()->tail != nothing)
    {
        iter = iter.listValue()->tail;
        retval.append(' ');
        if (printWidth == 0)
        {
            retval.append("...");
            break;
        }
        retval.append(iter.listValue()->head.showValue(fullPrintp, printDepthLimit - 1, printWidthLimit));
        --printWidth;
    }
    return retval;
}

QString List::showValue(bool fullPrintp, int printDepthLimit, int printWidthLimit)
{
    if (!listVisited.contains(this))
    {
        listVisited.push_back(this);
        QString retval = "[";
        retval.append(printValue(fullPrintp, printDepthLimit, printWidthLimit));
        retval.append(']');
        listVisited.removeOne(this);
        return retval;
    }
    return "...";
}

bool List::isEmpty()
{
    return head.isNothing();
}

void List::setButfirstItem(DatumPtr aValue)
{
    Q_ASSERT(head != nothing);
    Q_ASSERT(aValue.isList());
    tail = aValue;
    lastNode = aValue.listValue()->lastNode;
    astParseTimeStamp = 0;
}

DatumPtr List::itemAtIndex(int anIndex)
{
    DatumPtr ptr(this);
    while (anIndex > 1)
    {
        --anIndex;
        ptr = ptr.listValue()->tail;
    }
    return ptr.listValue()->head;
}

void List::clear()
{
    head = nothing;
    tail = nothing;
    lastNode = nothing;
    astList.clear();
    astParseTimeStamp = 0;
}

// This should only be used when initializing a list. It should not be used
// after list has been made available to the user.
void List::append(DatumPtr element)
{
    astParseTimeStamp = 0;

    if (head == nothing)
    {
        head = element;
        tail = nothing;
        lastNode = nothing;
        return;
    }

    List *l = new List();
    l->head = element;
    l->tail = nothing;
    l->lastNode = nothing;
    DatumPtr lP(l);

    if (tail == nothing)
    {
        tail = lP;
        lastNode = tail;
        return;
    }

    lastNode.listValue()->tail = lP;
    lastNode = lP;
}

int List::count()
{
    int retval = 0;
    DatumPtr ptr(this);
    while (ptr.isList() && (!ptr.listValue()->head.isNothing()))
    {
        ++retval;
        ptr = ptr.listValue()->tail;
    }
    return retval;
}

ListIterator List::newIterator()
{
    return ListIterator(this);
}
