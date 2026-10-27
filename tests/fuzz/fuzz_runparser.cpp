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
#include <vector>

// libFuzzer parses flags after LLVMFuzzerInitialize. If the command line
// has no -max_len=, append -max_len=4096.
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    for (int i = 1; i < *argc; ++i)
    {
        if (std::strncmp((*argv)[i], "-max_len=", 9) == 0)
            return 0;
    }

    static char kMaxLenFlag[] = "-max_len=4096";
    static std::vector<char *> args;
    args.assign(*argv, *argv + *argc);
    args.push_back(kMaxLenFlag);
    args.push_back(nullptr);
    *argc = static_cast<int>(args.size() - 1);
    *argv = args.data();
    return 0;
}

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
