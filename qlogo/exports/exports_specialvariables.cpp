// ===-- qlogo/exports/exports_specialvariables.cpp - Exports for the QLogo language -------*- C++ -*-===//
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
/// This file contains the C library routines that are used by the QLogo compiler
/// to access special variables.
///
//===----------------------------------------------------------------------===//

#include "workspace/exports_internal.h"
#include "datum_types.h"
#include "workspace/kernel.h"
#include "workspace/specialvars.h"

/// @brief Get the value of the ERRORACT variable.
/// @param eAddr the address of the evaluator.
/// @return the value of the ERRORACT variable as a boolean.
/// @note In QLogo, ERRACT is a pseudo-boolean variable. For compatibility with
/// UCBLogo, we accept any word or list. However, in QLogo, ERRACT is considered TRUE
/// only if the following conditions are met:
/// 1. The value EXISTS, and:
/// 2 a. The value is a word AND the word is not "FALSE" or the empty string, or
/// 2 b. The value is a list AND the list is not empty.
EXPORTC bool getvarErroract(void)
{
    const QString &name = SpecialVars::name(SpecialVars::Id::ErrAct);
    DatumPtr val = Kernel::get().datumForName(name);
    if (val.isWord())
    {
        QString word = val.toString(Datum::ToStringFlags_Key);
        return (word != "FALSE") && (word != "");
    }
    if (val.isList())
    {
        return !val.listValue()->isEmpty();
    }
    return false;
}

