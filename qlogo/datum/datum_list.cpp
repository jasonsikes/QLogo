
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
#include <QObject>
#include <qdebug.h>

QList<void *> listVisited;
QList<void *> otherListVisited;

List::List()
{
    isa = Datum::typeList;
    astParseTimeStamp = 0;
    lastNode = this;
    //qDebug() <<this << " new++ list";
}

List::List(DatumPtr item, List *srcList)
{
    isa = Datum::typeList;
    head = item;
    tail = DatumPtr(srcList);
    lastNode = srcList->lastNode;
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
    Parser::destroyAstForList(this);
    astParseTimeStamp = 0;
}

int List::count()
{
    int retval = 0;
    List* iter = this;
    while ( ! iter->head.isNothing())
    {
        ++retval;
        iter = iter->tail.listValue();
    }
    return retval;
}

ListIterator List::newIterator()
{
    return ListIterator(this);
}
