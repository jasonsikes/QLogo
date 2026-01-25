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

#include "compiler.h"
#include "compiler_private.h"
#include "datum_types.h"
#include "workspace/kernel.h"
#include "sharedconstants.h"


/***DOC ERRACT
ERRACT                          (variable)

    When set to a value that is not FALSE nor an empty string nor an empty list,
    the command interpreter will execute PAUSE to enable the user to
    inspect the state of the program.

COD***/

/***DOC LOGOPLATFORM
LOGOPLATFORM						(variable)

    one of the following words: OSX, WINDOWS, or UNIX.


COD***/

/***DOC LOGOVERSION
LOGOVERSION						(variable)

    a real number indicating the Logo version number, e.g., 5.5

COD***/

/***DOC COMMANDLINE
COMMANDLINE						(variable)

    contains all text on the command line used to start Logo.

COD***/

