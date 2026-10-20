//===-- tests/fuzz/fuzz_runparser.cpp - libFuzzer harness for Runparser -*- C++ -*-===//
//
// Copyright 2026 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//

#include "datum_types.h"
#include "runparser.h"
#include <QCoreApplication>
#include <cstdint>
#include <cstring>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static int argc = 1;
    static char argv0[] = "fuzz_runparser";
    static char *argv[] = {argv0, nullptr};
    static QCoreApplication app(argc, argv);

    const QString text = QString::fromUtf8(reinterpret_cast<const char *>(data), static_cast<qsizetype>(size));

    ListBuilder builder;
    builder.append(DatumPtr(text));
    (void)runparse(builder.finishedList());
    return 0;
}
