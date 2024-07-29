#ifndef WORKSPACE_H
#define WORKSPACE_H

//===-- qlogo/workspace.h - Workspace class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Workspace class, which is the
/// superclass for classes that need QLogo language workspace functionality
/// (Variables, PropertyLists, Procedures).
///
//===----------------------------------------------------------------------===//

#include <QSet>
#include <QString>

/// @brief A type to represent the query option for a workspace item.
enum showContents_t
{
    showUnburied,
    showBuried,
    showTraced,
    showStepped
};

/// @brief A class to provide workspace functionality for the Workspace subclasses.
/// @note Items in a Workspace subclass can be "buried", "stepped", and/or "traced".
/// "Buried" means that the item exists, but is hidden from "showall" queries.
/// "Stepped" and "traced" have slightly different meanings depending on the
/// Workspace subclass. For more information, consult the HELP text for TRACE, BURY,
/// or STEP.
class Workspace
{
    QSet<QString> buriedNames;
    QSet<QString> steppedNames;
    QSet<QString> tracedNames;

  public:
    /// @brief Constructor.
    Workspace();

    /// @brief Bury a workspace item.
    /// @param aName The name of the workspace item to bury.
    void bury(const QString &aName);

    /// @brief Check if a workspace item is buried.
    /// @param aName The name of the workspace item to check.
    /// @return True if the workspace item is buried, false otherwise.
    bool isBuried(const QString &aName);

    /// @brief Unbury a workspace item.
    /// @param aName The name of the workspace item to unbury.
    void unbury(const QString &aName);

    /// @brief Step a workspace item.
    /// @param aName The name of the workspace item to step.
    void step(const QString &aName);

    /// @brief Check if a workspace item is stepped.
    /// @param aName The name of the workspace item to check.
    /// @return True if the workspace item is stepped, false otherwise.
    bool isStepped(const QString &aName);

    /// @brief Unstep a workspace item.
    /// @param aName The name of the workspace item to unstep.
    void unstep(const QString &aName);

    /// @brief Trace a workspace item.
    /// @param aName The name of the workspace item to trace.
    void trace(const QString &aName);

    /// @brief Check if a workspace item is traced.
    /// @param aName The name of the workspace item to check.
    /// @return True if the workspace item is traced, false otherwise.
    bool isTraced(const QString &aName);

    /// @brief Untrace a workspace item.
    /// @param aName The name of the workspace item to untrace.
    void untrace(const QString &aName);

    /// @brief Check if a workspace item should be included in a query.
    /// @param showWhat The query option to check.
    /// @param name The name of the workspace item to check.
    /// @return True if the workspace item should be included, false otherwise.
    bool shouldInclude(showContents_t showWhat, const QString &name);
};

#endif // WORKSPACE_H
