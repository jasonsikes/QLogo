//===-- qlogo/compiler_specialvariables.cpp - Special variables implementations -------*- C++ -*-===//
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
/// This file contains the implementation of the special variables of the
/// Compiler class.
///
//===----------------------------------------------------------------------===//

#include "compiler_private.h"
#include "compiler.h"
#include "sharedconstants.h"
#include "kernel.h"

#include <QObject>

using namespace llvm;
using namespace llvm::orc;

/***DOC ERRACT
ERRACT                          (variable)

    When set to a value that is not FALSE nor an empty string nor an empty list,
    the command interpreter will execute PAUSE to enable the user to
    inspect the state of the program.

COD***/

/// @brief Get the value of the ERRORACT variable.
/// @param eAddr the address of the evaluator.
/// @return the value of the ERRORACT variable as a boolean.
/// @note In QLogo, ERRACT is a pseudo-boolean variable. For compatibility with
/// UCBLogo, we accept any word or list. However, in QLogo, ERRACT is considered TRUE
/// only if the following conditions are met:
/// 1. The value EXISTS, and:
/// 2 a. The value is a word AND the word is not "FALSE" or the empty string, or
/// 2 b. The value is a list AND the list is not empty.
EXPORTC bool getvarErroract(addr_t eAddr)
{
    QString name = QObject::tr("ERRACT");
    DatumPtr val = Config::get().mainKernel()->callStack.datumForName(name);
    if (val.isWord())
    {
        QString word = val.wordValue()->keyValue();
        return (word != "FALSE") && (word != "");
    }
    if (val.isList())
    {
        return ! val.listValue()->isEmpty();
    }
    return false;
}
