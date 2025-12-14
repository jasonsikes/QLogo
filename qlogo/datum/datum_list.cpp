
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
#include "parser.h"
#include "compiler.h"
#include "visited.h"
#include <QObject>
#include <qdebug.h>

QList<void *> listVisited;
QList<void *> otherListVisited;

List::List(DatumPtr item, List *srcList)
{
    isa = Datum::typeList;
    head = item;
    tail = DatumPtr(srcList);
    astParseTimeStamp = 0;
    //qDebug() <<this << " new++ list";
}

List::~List()
{
    clear();
    //qDebug() <<this << " --del list";
}

QString List::printValue(bool fullPrintp, int printDepthLimit, int printWidthLimit)
{
    if (head.isNothing())
        return "";
    if ((printDepthLimit == 0) || (printWidthLimit == 0))
    {
        return "...";
    }
    int printWidth = printWidthLimit - 1;
    QString retval = head.showValue(fullPrintp, printDepthLimit - 1, printWidthLimit);
    List* iter = tail.listValue();
    while ( ! iter->head.isNothing())
    {
        retval.append(' ');
        if (printWidth == 0)
        {
            retval.append("...");
            break;
        }
        retval.append(iter->head.showValue(fullPrintp, printDepthLimit - 1, printWidthLimit));
        --printWidth;
        iter = iter->tail.listValue();
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
    return this == EmptyList::instance();
}

void List::setButfirstItem(DatumPtr aValue)
{
    Q_ASSERT(! isEmpty());
    Q_ASSERT(aValue.isList());
    tail = aValue;
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
    Parser::destroyAstForList(this);
    astParseTimeStamp = 0;
}

int List::count()
{
    int retval = 0;
    List* iter = this;
    VisitedSet visited;
    while ( ! iter->isEmpty())
    {
        ++retval;
        if (visited.contains(iter))
        {
            // TODO: How should we report a cycle? -1? MAX_INT?
            return retval;
        }
        visited.add(iter);
        iter = iter->tail.listValue();
    }
    return retval;
}

ListIterator List::newIterator()
{
    return ListIterator(this);
}

// EmptyList singleton implementation
EmptyList *EmptyList::instance_ = nullptr;

EmptyList::EmptyList()
    : List(nothing, nullptr)
{
    isa = (DatumType)(Datum::typeList | Datum::typePersistentMask);
}

EmptyList *EmptyList::instance()
{
    if (instance_ == nullptr)
    {
        instance_ = new EmptyList();
    }
    return instance_;
}

void EmptyList::clear()
{
    // EmptyList is immutable - do nothing
    Q_ASSERT(false && "Attempted to modify immutable EmptyList");
}

void EmptyList::setButfirstItem(DatumPtr /* aValue */)
{
    Q_ASSERT(false && "Attempted to modify immutable EmptyList");
}

// Value to represent an empty list
DatumPtr emptyList(EmptyList::instance());
