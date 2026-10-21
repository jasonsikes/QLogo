#ifndef SPECIALVARS_H
#define SPECIALVARS_H

//===-- qlogo/specialvars.h - Special variable names -------*- C++ -*-===//
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
/// Logo spellings of interpreter-owned variables. Each name is defined once
/// in QLOGO_SPECIAL_VARS. specialvars.cpp is the only translation unit that
/// leaves that macro defined, so it can build the QString table from the
/// same list.
///
//===----------------------------------------------------------------------===//

#include <QString>

class Word;

#define QLOGO_SPECIAL_VARS(X)             \
    X(AllowGetSet, "ALLOWGETSET")         \
    X(CaseIgnoredP, "CASEIGNOREDP")       \
    X(Argv, "ARGV")                       \
    X(CommandLine, "COMMAND.LINE")        \
    X(ErrAct, "ERRACT")                   \
    X(LogoPlatform, "LOGOPLATFORM")       \
    X(LogoVersion, "LOGOVERSION")         \
    X(QlogoVersion, "QLOGOVERSION")       \
    X(ButtonAct, "BUTTONACT")             \
    X(FullPrintP, "FULLPRINTP")           \
    X(KeyAct, "KEYACT")                   \
    X(LoadNoisily, "LOADNOISILY")         \
    X(PrintDepthLimit, "PRINTDEPTHLIMIT") \
    X(PrintWidthLimit, "PRINTWIDTHLIMIT") \
    X(Startup, "STARTUP")                 \
    X(UnburyOnEdit, "UNBURYONEDIT")

/// @brief Names of variables owned by the interpreter.
namespace SpecialVars
{

/// @brief Identifiers for interpreter-owned variables.
enum class Id : int
{
#define QLOGO_SV_ENUM(id, spelling) id,
    QLOGO_SPECIAL_VARS(QLOGO_SV_ENUM)
#undef QLOGO_SV_ENUM
    Count
};

/// @brief Logo spelling of @p id.
/// @param id The special variable.
/// @return The variable name used in the workspace.
const QString &name(Id id);

/// @brief Interned name word for @p id.
/// @param id The special variable.
/// @return The persistent word used as the variable-table key.
Word *word(Id id);

} // namespace SpecialVars

#ifndef QLOGO_SPECIAL_VARS_IMPLEMENTATION
#undef QLOGO_SPECIAL_VARS
#endif

#endif // SPECIALVARS_H
