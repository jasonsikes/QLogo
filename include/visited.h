#ifndef VISITED_H
#define VISITED_H

//===-- qlogo/visited.h - VisitedSet and VisitedMap class definitions -------*- C++ -*-===//
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
/// This file contains the declaration of the VisitedSet and VisitedMap classes, which are
/// used to track visited nodes during graph traversal, in order to prevent cycles.
///
//===----------------------------------------------------------------------===//


#include <QSet>
#include <QMap>

class Datum;

/// @brief A set of visited nodes.
/// @details The VisitedSet class is used to track visited nodes during Datum graph traversal,
/// in order to prevent cycles when comparing Datum objects. The node is the Datum object that has been visited.
class VisitedSet
{
  protected:
    QSet<const Datum *> visited;

  public:
    /// @brief Create an empty VisitedSet.
    VisitedSet() {}

    /// @brief Add a node to the visited set.
    /// @param node The node to mark as visited.
    void add(const Datum *node);

    /// @brief Remove a node from the visited set.
    /// @param node The node to UNmark as visited.
    void remove(const Datum *node);

    /// @brief Check if a node has been visited.
    /// @param node The node to check.
    /// @return True if the node has been visited, false otherwise.
    bool contains(const Datum *node) const;

    /// @brief Clear all visited nodes from the set.
    void clear();

    /// @brief Return the number of visited nodes.
    /// @return The number of visited nodes.
    int size() const;
};

/// @brief A map of visited nodes.
/// @details The VisitedMap class is used to track visited nodes during Datum graph traversal,
/// in order to prevent cycles when comparing Datum objects. The key is the node that has been visited, and the value is
/// the node of the object being compared.
class VisitedMap
{
  protected:
    QMap<const Datum *, const Datum *> visited;

  public:
    /// @brief Create an empty VisitedMap.
    VisitedMap() {}

    /// @brief Add a node to the visited map.
    /// @param key The key of the node to mark as visited.
    /// @param value The value of the node to mark as visited.
    void add(const Datum *key, const Datum *value);

    /// @brief Remove a node from the visited map.
    /// @param key The key of the node to unmark as visited.
    void remove(const Datum *key);

    /// @brief Get the value associated with a key in the visited map.
    /// @param key The key of the node to get the value of.
    /// @return The value associated with the key.
    const Datum *get(const Datum *key) const;

    /// @brief Check if a node has been visited.
    /// @param key The key of the node to check.
    /// @return True if the node has been visited, false otherwise.
    bool contains(const Datum *key) const;

    /// @brief Clear all visited nodes from the map.
    void clear();

    /// @brief Return the number of visited nodes.
    /// @return The number of visited nodes.
    int size() const;
};

#endif // VISITEDSET_H