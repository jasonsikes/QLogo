//===-- qlogo/workspace/visited.cpp - VisitedSet and VisitedMap class implementations --*- C++ -*-===//
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

#include "workspace/visited.h"

// VisitedSet implementation

void VisitedSet::add(const Datum *node)
{
    visited_.insert(node);
}

void VisitedSet::remove(const Datum *node)
{
    visited_.remove(node);
}

bool VisitedSet::contains(const Datum *node) const
{
    return visited_.contains(node);
}

void VisitedSet::clear()
{
    visited_.clear();
}

// VisitedMap implementation

void VisitedMap::add(const Datum *key, const Datum *value)
{
    visited_.insert(key, value);
}

void VisitedMap::remove(const Datum *key)
{
    visited_.remove(key);
}

const Datum *VisitedMap::get(const Datum *key) const
{
    return visited_.value(key);
}

bool VisitedMap::contains(const Datum *key) const
{
    return visited_.contains(key);
}

void VisitedMap::clear()
{
    visited_.clear();
}
