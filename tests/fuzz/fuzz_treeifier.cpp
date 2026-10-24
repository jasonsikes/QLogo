//===-- tests/fuzz/fuzz_treeifier.cpp - libFuzzer harness for Treeifier -*- C++ -*-===//
//
// Copyright 2026 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//

#include "datum_types.h"
#include "flowcontrol.h"
#include "interface/logointerface.h"
#include "sharedconstants.h"
#include "treeifyer.h"
#include "workspace/kernel.h"
#include "workspace/library.h"
#include "workspace/procedures.h"
#include <QCoreApplication>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    bool hasMaxLen = false;
    for (int i = 1; i < *argc; ++i)
    {
        if (std::strncmp((*argv)[i], "-max_len=", 9) == 0)
        {
            hasMaxLen = true;
            break;
        }
    }
    if (!hasMaxLen)
    {
        static char kMaxLenFlag[] = "-max_len=4096";
        static std::vector<char *> args;
        args.assign(*argv, *argv + *argc);
        args.push_back(kMaxLenFlag);
        args.push_back(nullptr);
        *argc = static_cast<int>(args.size() - 1);
        *argv = args.data();
    }

    static QCoreApplication app(*argc, *argv);

    for (int i = 0; i < *argc; ++i)
        Config::get().ARGV_.push_back(QString::fromLocal8Bit((*argv)[i]));

    Config::get().paramLibraryDatabaseFilepath_ = QStringLiteral(QLOGO_FUZZ_LIBRARY_DB);

    static LogoInterface iface;
    (void)Kernel::get();
    (void)Procedures::get();
    (void)Library::get().allProcedureNames();

    // qlogo closes these while QCoreApplication is still alive. Function-local
    // statics would otherwise destroy Library after the app and crash in Qt SQL.
    std::atexit([] {
        Library::get().closeConnection();
        Help::get().closeConnection();
    });
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    constexpr size_t kMaxInputLen = 4096;
    if (size > kMaxInputLen)
        size = kMaxInputLen;

    const QString text = QString::fromUtf8(reinterpret_cast<const char *>(data), static_cast<qsizetype>(size));

    ListBuilder builder;
    builder.append(DatumPtr(text));
    try
    {
        (void)Treeifier::astFromList(builder.finishedList().listValue());
    }
    catch (FCError *err)
    {
        DatumPtr owner(err);
    }
    return 0;
}
