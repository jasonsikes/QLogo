
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

#include "datum_types.h"
#include "parser.h"
#include "compiler.h"
#include "visited.h"
#include <QObject>
#include <qdebug.h>

List::List(DatumPtr item, List *srcList)
{
    isa = Datum::typeList;
    head = item;
    tail = DatumPtr(srcList);
    astParseTimeStamp = 0;
}

List::~List()
{
    try
    {
        clear();
    }
    catch (...)
    {
        // Destructors must not throw exceptions. Log the error and continue.
        // The collection removal may have failed, but we still need to complete
        // the destruction of this object.
        qDebug() << "Exception caught in List destructor during cleanup";
    }
}

QString List::toString( ToStringFlags flags, int printDepthLimit, int printWidthLimit, VisitedSet *visited) const
{
    bool shouldShowBrackets = (flags & ToStringFlags_Show) != 0;
    QString retval = shouldShowBrackets ? "[" : "";
    std::unique_ptr<VisitedSet> localVisited;
    int printWidth = printWidthLimit - 1;
    const List* l = this;

    if (this == EmptyList::instance())
        goto exit;
    if (printDepthLimit == 0)
    {
        retval.append("...");
        goto exit;
    }

    if (visited == nullptr)
    {
        localVisited = std::make_unique<VisitedSet>();
        visited = localVisited.get();
    }

    // Any words within a collection don't need to be formatted as source code.
    flags = (Datum::ToStringFlags)(flags & ~(Datum::ToStringFlags_Source));
    // Any lists within a collection need to show their brackets.
    flags = (Datum::ToStringFlags)(flags | Datum::ToStringFlags_Show);

    while ( ! l->isEmpty())
    {
        if (l != this)
        {
            retval.append(' ');
        }
        if ( (printWidth == 0) || (visited->contains(const_cast<List *>(l))))
        {
            // We have reached the print width limit or have a cycle, so stop traversing the list.
            retval.append("...");
            goto exit;
        }
        visited->add(const_cast<List *>(l));
        retval.append(l->head.toString(flags, printDepthLimit - 1, printWidthLimit, visited));
        --printWidth;
        l = l->tail.listValue();
    }

exit:
    visited->remove(const_cast<List *>(this));
    if (shouldShowBrackets)
    {
        retval.append(']');
    }
    return retval;
}

bool List::isEmpty() const
{
    return this == EmptyList::instance();
}

void List::setButfirstItem(DatumPtr aValue)
{
    Q_ASSERT(this != EmptyList::instance());
    Q_ASSERT(aValue.isList());
    tail = aValue;
    astParseTimeStamp = 0;
}

DatumPtr List::itemAtIndex(int anIndex) const
{
    DatumPtr ptr(const_cast<List *>(this));
    while (anIndex > 1)
    {
        --anIndex;
        ptr = ptr.listValue()->tail;
    }
    return ptr.listValue()->head;
}

void List::clear()
{
    Q_ASSERT(this != EmptyList::instance());
    head = nothing;
    tail = nothing;
    Parser::destroyAstForList(this);
    astParseTimeStamp = 0;
}

int List::count() const
{
    int retval = 0;
    const List* iter = this;
    VisitedSet visited;
    while (iter != EmptyList::instance())
    {
        ++retval;
        if (visited.contains(const_cast<List *>(iter)))
        {
            // TODO: How should we report a cycle? -1? 0?
            //  Not MAX_INT because that's a positive number.
            return retval;
        }
        visited.add(const_cast<List *>(iter));
        iter = iter->tail.listValue();
    }
    return retval;
}

ListIterator List::newIterator() const
{
    return ListIterator(const_cast<List *>(this));
}

// EmptyList singleton implementation
EmptyList *EmptyList::instance_ = nullptr;

EmptyList::EmptyList()
    : List(nothing, nullptr)
{
    isa = Datum::typeEmptyList;
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

QString EmptyList::toString( ToStringFlags flags, int /* printDepthLimit */, int /* printWidthLimit */, VisitedSet * /* visited */) const
{
    bool shouldShowBrackets = (flags & ToStringFlags_Show) != 0;
    return shouldShowBrackets ? "[]" : "";
}

// Value to represent an empty list
DatumPtr emptyList(EmptyList::instance());
