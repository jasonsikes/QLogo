
//===-- qlogo/propertylists.cpp - PropertyLists class implementation --*- C++ -*-===//
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
/// This file contains the implementation of the PropertyLists class, which
/// provides property list functionality for the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "workspace/propertylists.h"
#include "datum_types.h"

PropertyLists::PropertyLists() = default;

void PropertyLists::addProperty(const QString &plistname, const QString &propname, const DatumPtr &value)
{
    if (!plists_.contains(plistname))
    {
        plists_.insert(plistname, QHash<QString, DatumPtr>());
    }

    plists_[plistname][propname] = value; 
}

DatumPtr PropertyLists::getProperty(const QString &plistname, const QString &propname) const
{
    if (plists_.contains(plistname) && plists_[plistname].contains(propname))
        return plists_[plistname][propname];
    return emptyList();
}

void PropertyLists::removeProperty(const QString &plistname, const QString &propname)
{
    if (plists_.contains(plistname))
    {
        plists_[plistname].remove(propname);
        if (plists_[plistname].isEmpty())
            plists_.remove(plistname);
    }
}

DatumPtr PropertyLists::getPropertyList(const QString &plistname) const
{
    ListBuilder builder;
    const auto propertyList = plists_.find(plistname);
    if (propertyList != plists_.end())
    {
        for (const auto [key, value] : propertyList->asKeyValueRange())
        {
            builder.append(DatumPtr(key));
            builder.append(value);
        }
    }
    return builder.finishedList();
}

void PropertyLists::erasePropertyList(const QString &plistname)
{
    plists_.remove(plistname);
}

bool PropertyLists::isPropertyList(const QString &plistname) const
{
    return plists_.contains(plistname);
}

DatumPtr PropertyLists::allPLists() const
{
    ListBuilder builder;
    for (const auto name : plists_.keys())
    {
        builder.append(DatumPtr(name));
    }
    return builder.finishedList();
}
