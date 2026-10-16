#ifndef WORKSPACE_EXPORTS_INTERNAL_H
#define WORKSPACE_EXPORTS_INTERNAL_H

//===-- include/workspace/exports_internal.h - EXPORTC host ABI -*- C++ -*-===//
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
/// JIT host ABI. Every EXPORTC definition is `noexcept`; Logo errors are
/// returned as Datum pointers. catchError and printOrError convert a thrown
/// FCError* into that return value.
///
//===----------------------------------------------------------------------===//

#include "compiler_types.h"
#include "flowcontrol.h"
#include "interface/streammanager.h"
#include "workspace/evaluator.h"

#include <utility>

#ifdef _WIN32
#define EXPORTC extern "C" __declspec(dllexport)
#else
#define EXPORTC extern "C"
#endif

/// Run @p fn and return its Datum pointer. A thrown FCError* is watched and returned.
template <typename Fn>
addr_t catchError(Evaluator *e, Fn &&fn)
{
    try
    {
        return std::forward<Fn>(fn)();
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// Run a void host operation. Return @p success, or a watched FCError*.
template <typename Fn>
addr_t catchError(Evaluator *e, addr_t success, Fn &&fn)
{
    try
    {
        std::forward<Fn>(fn)();
        return success;
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// Print to the current writer. Return @p success, or a watched file-system error.
inline addr_t printOrError(Evaluator *e, addr_t success, const QString &text)
{
    return catchError(e, success, [&] { StreamManager::get().print(text); });
}

/// Print to the terminal. Return @p success, or a watched file-system error.
inline addr_t printToTerminalOrError(Evaluator *e, addr_t success, const QString &text)
{
    return catchError(e, success, [&] { StreamManager::get().printToTerminal(text); });
}

#endif
