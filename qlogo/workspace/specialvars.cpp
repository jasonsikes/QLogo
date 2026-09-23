//===-- qlogo/specialvars.cpp - Special variable names -------*- C++ -*-===//
//
// Copyright 2017-2026 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// QString table for interpreter-owned variable names. The spellings come
/// from QLOGO_SPECIAL_VARS in specialvars.h.
///
//===----------------------------------------------------------------------===//

#define QLOGO_SPECIAL_VARS_IMPLEMENTATION
#include "workspace/specialvars.h"
#undef QLOGO_SPECIAL_VARS_IMPLEMENTATION

#include <QObject>
#include <cstddef>

const QString &SpecialVars::name(Id id)
{
    static const QString names[] = {
#define QLOGO_SV_STRING(id, spelling) QObject::tr(spelling),
        QLOGO_SPECIAL_VARS(QLOGO_SV_STRING)
#undef QLOGO_SV_STRING
    };
    static_assert(sizeof(names) / sizeof(names[0]) == static_cast<std::size_t>(Id::Count));
    return names[static_cast<std::size_t>(id)];
}

#undef QLOGO_SPECIAL_VARS
