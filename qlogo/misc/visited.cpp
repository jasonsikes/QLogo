//===-- qlogo/misc/visited.cpp - VisitedSet and VisitedMap class implementations --*- C++ -*-===//
//
// Copyright 2025 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the VisitedSet and VisitedMap classes, which are
/// used to track visited nodes during graph traversal, in order to prevent cycles.
///
//===----------------------------------------------------------------------===//

#include "visited.h"


// VisitedSet implementation

void VisitedSet::add(Datum *node)
{
    visited.insert(node);
}

void VisitedSet::remove(Datum *node)
{
    visited.remove(node);
}

bool VisitedSet::contains(Datum *node) const
{
    return visited.contains(node);
}

void VisitedSet::clear()
{
    visited.clear();
}

int VisitedSet::size() const
{
    return visited.size();
}


// VisitedMap implementation

void VisitedMap::add(Datum *key, Datum *value)
{
    visited.insert(key, value);
}

void VisitedMap::remove(Datum *key)
{
    visited.remove(key);
}

Datum *VisitedMap::get(Datum *key) const
{
    return visited.value(key);
}

bool VisitedMap::contains(Datum *key) const
{
    return visited.contains(key);
}

void VisitedMap::clear()
{
    visited.clear();
}

int VisitedMap::size() const
{
    return visited.size();
}