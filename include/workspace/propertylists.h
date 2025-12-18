#ifndef PROPERTYLISTS_H
#define PROPERTYLISTS_H

//===-- qlogo/propertylists.h - PropertyLists class definition --*- C++ -*-===//
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
/// This file contains the declaration of the PropertyLists class, which
/// provides property list functionality for the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "datum_ptr.h"

/// @brief A class to manage property lists.
/// @note This is a hash table of property lists, with each property list being a hash table of property names to property values.
class PropertyLists
{
    /// @brief The hash table of property lists.
    QHash<QString, QHash<QString, DatumPtr>> plists;

  public:
    /// @brief Constructor.
    PropertyLists();

    /// @brief Add a property to a property list.
    /// @param plistname The name of the property list.
    /// @param propname The name of the property.
    /// @param value The value of the property.
    void addProperty(const QString &plistname, const QString &propname, DatumPtr value);

    /// @brief Get a property from a property list.
    /// @param plistname The name of the property list.
    /// @param propname The name of the property.
    /// @return The value of the property.
    DatumPtr getProperty(const QString &plistname, const QString &propname);

    /// @brief Remove a property from a property list.
    /// @param plistname The name of the property list.
    /// @param propname The name of the property.
    void removeProperty(const QString &plistname, const QString &propname);

    /// @brief Get a property list.
    /// @param plistname The name of the property list.
    /// @return The property list.
    DatumPtr getPropertyList(const QString &plistname);

    /// @brief Remove a property list.
    /// @param plistname The name of the property list.
    void erasePropertyList(const QString &plistname);

    /// @brief Check if a property list exists.
    /// @param plistname The name of the property list.
    /// @return True if the property list exists, false otherwise.
    bool isPropertyList(const QString &plistname);

    /// @brief Get all property lists.
    /// @return A list of all property lists.
    DatumPtr allPLists();
};

#endif // PROPERTYLISTS_H
