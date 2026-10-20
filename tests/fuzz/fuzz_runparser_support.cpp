//===-- tests/fuzz/fuzz_runparser_support.cpp - stubs for the Runparser fuzzer -*- C++ -*-===//
//
// Copyright 2026 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Provides symbols that `runparser.cpp` references but that would otherwise
/// pull StreamManager, Kernel, and Treeifier into the fuzz binary.
/// The harness feeds `runparse` a list of words, so TextStream is never used.
///
//===----------------------------------------------------------------------===//

#include "datum_types.h"
#include "interface/textstream.h"
#include <QString>

const QString &specialChars()
{
    static const QString specialCharsInstance("+-()*/<>=");
    return specialCharsInstance;
}

RawStream::RawStream(QTextStream *aStream) : stream_(aStream)
{
}

WordReader::WordReader(RawStream *rawStream) : rawStream_(rawStream)
{
}

ListReader::ListReader(WordReader *wordReader) : wordReader_(wordReader)
{
}

TextStream::TextStream(QTextStream *aStream) : rawStream_(aStream), wordReader_(&rawStream_), listReader_(&wordReader_)
{
}

DatumPtr TextStream::readListWithPrompt(const QString &, bool)
{
    return emptyList();
}
