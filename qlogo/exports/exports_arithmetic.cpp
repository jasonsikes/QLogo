// ===-- qlogo/exports/exports_arithmetic.cpp - Exports for the QLogo language -------*- C++ -*-===//
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
/// for arithmetic operations.
///
//===----------------------------------------------------------------------===//

#include "workspace/exports_internal.h"
#include "datum_types.h"
#include "workspace/kernel.h"

#include <QRandomGenerator>

/// Get a reference to the random number generator instance.
static QRandomGenerator &randomGenerator()
{
    static QRandomGenerator randomGeneratorInstance;
    return randomGeneratorInstance;
}

/// Generate a random nonnegative integer less than num given
/// @param num an upper bound (exclusive) to the random number.
/// @return the random number generated.
EXPORTC double random1(int32_t num) noexcept
{
    int result = randomGenerator().bounded(num);
    return (double)result;
}

/// Generate a random integer between start and end (both inclusive)
/// @param start a lower bound (inclusive) to the random number.
/// @param end an upper bound (inclusive) to the random number.
/// @return the random number generated.
EXPORTC double random2(int32_t start, int32_t end) noexcept
{
    int result = randomGenerator().bounded(start, end + 1);
    return (double)result;
}

/// Set the seed for the random number generator
/// @param seed the seed.
/// @return nothing.
EXPORTC addr_t setRandomWithSeed(int32_t seed) noexcept
{
    randomGenerator().seed(seed);
    return nullptr;
}

/// Set the seed for the random number generator using the system seed
/// @return nothing.
EXPORTC addr_t setRandom() noexcept
{
    QRandomGenerator *s = QRandomGenerator::system();
    quint32 seed = s->generate();
    randomGenerator().seed(seed);
    return nullptr;
}

/// Generate a Word(string) from a number that is formatted according to the
/// other parameters.
/// @param eAddr a pointer to the Evaluator object
/// @param num the number to apply formatting to.
/// @param width the minimum number of characters to use. Spaces may be added.
/// @param precision the number of digits to add after the decimal point.
/// @return A Word(string) with formatting applied.
EXPORTC addr_t getFormForNumber(addr_t eAddr, double num, int32_t width, int32_t precision) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QString retval = QString("%1").arg(num, width, 'f', precision);
    auto *w = new Word(retval);
    e->watch(w);

    return reinterpret_cast<addr_t>(w);
}

