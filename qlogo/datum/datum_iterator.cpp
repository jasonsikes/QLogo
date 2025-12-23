
//===-- qlogo/datum_iterator.cpp - class implementation --*- C++ -*-===//
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
/// This file contains the implementation of the ListIterator class.
/// It is a minimalist iterator over QLogo List items.
///
//===----------------------------------------------------------------------===//

#include "datum_types.h"
#include <qdebug.h>

ListIterator::ListIterator() = default;

ListIterator::ListIterator(const DatumPtr &aList) : iterator(aList.listValue())
{
}

DatumPtr ListIterator::element()
{
    DatumPtr retval = iterator->head;
    iterator = iterator->tail.listValue();
    return retval;
}

bool ListIterator::elementExists() const
{
    return iterator != EmptyList::instance();
}
