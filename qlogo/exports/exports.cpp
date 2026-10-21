// ===-- qlogo/exports/exports.cpp - Exports for the QLogo language -------*- C++ -*-===//
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
/// This file contains the C library routines that are used by the QLogo compiler.
///
//===----------------------------------------------------------------------===//

#include "astnode.h"
#include "compiler_types.h"
#include "datum_types.h"
#include "flowcontrol.h"
#include "interface/streammanager.h"
#include "workspace/evaluator.h"
#include "workspace/exports_internal.h"
#include "workspace/kernel.h"
#include "workspace/specialvars.h"

#include <QObject>

#include <algorithm>
#include <cstdint>
#include <cstdlib>

/// Print an integer to the console.
/// @param p the integer to print.
/// @note For debugging.
// cppcheck-suppress unusedFunction
EXPORTC void printInt(int32_t p) noexcept
{
    qWarning() << "int = " << p;
}

/// @brief Run ERRACT to recover from @p err, or return @p err if ERRACT is unbound.
/// @param eAddr a pointer to the Evaluator object
/// @param err the error that just occurred
/// @param rejected the value the primitive rejected, if any; nothing() if there was none
/// @param datatypeTest true when ERRACT's output can replace the failed expression
/// @param errorFromOutput builds the next error when ERRACT outputs an unusable datum
/// @return the replacement value, or an error
addr_t recoverWithErract(addr_t eAddr,
                         DatumPtr err,
                         DatumPtr rejected,
                         std::function<bool(DatumPtr)> datatypeTest,
                         std::function<DatumPtr(DatumPtr)> errorFromOutput)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    Kernel &kernel = Kernel::get();
    const QString &erractName = SpecialVars::name(SpecialVars::Id::ErrAct);

    if (kernel.datumForName(erractName).isNothing())
    {
        e->watch(err.datumValue());
        return reinterpret_cast<addr_t>(err.datumValue());
    }

    // The compiler calls back immediately when the output is still the wrong kind of
    // datum. That is an ERRACT loop. A later, different input is a new error.
    if (kernel.isErractReturnRetry(rejected))
    {
        kernel.clearErractLoop();
        DatumPtr toplevel = {FCError::custom(DatumPtr(QObject::tr("TOPLEVEL")))};
        e->watch(toplevel.datumValue());
        return printToTerminalOrError(
            e, reinterpret_cast<addr_t>(toplevel.datumValue()), QObject::tr("Erract loop") + "\n");
    }
    kernel.clearErractReturnGuard();

    while (true)
    {
        if (kernel.isErractLoop(static_cast<int>(err.errValue()->code_)))
        {
            kernel.clearErractLoop();
            DatumPtr toplevel = {FCError::custom(DatumPtr(QObject::tr("TOPLEVEL")))};
            e->watch(toplevel.datumValue());
            return printToTerminalOrError(
                e, reinterpret_cast<addr_t>(toplevel.datumValue()), QObject::tr("Erract loop") + "\n");
        }

        kernel.armErractLoop(static_cast<int>(err.errValue()->code_));
        kernel.armErractMessage(err.toString());

        DatumPtr instruction = kernel.datumForName(erractName);
        uint32_t pausesBefore = kernel.pauseCount();
        DatumPtr retval = kernel.runErractInstruction(instruction);
        bool messageStillPending = kernel.takeErractMessagePending();

        if (datatypeTest(retval))
        {
            kernel.noteErractReturn(retval.datumValue(), kernel.pauseCount() != pausesBefore);
            e->watch(retval.datumValue());
            return reinterpret_cast<addr_t>(retval.datumValue());
        }
        if (retval.isErr())
        {
            kernel.clearErractLoop();
            e->watch(retval.datumValue());
            return reinterpret_cast<addr_t>(retval.datumValue());
        }
        // The instruction did not output. If PAUSE already printed the error, leave
        // the pause the way CONTINUE without an input does. Otherwise propagate the
        // original error so toplevel can print it.
        if ((retval.isa() & Datum::typeDataMask) == 0)
        {
            kernel.clearErractLoop();
            if (!messageStillPending)
            {
                retval = {FCError::custom(DatumPtr(QObject::tr("TOPLEVEL")))};
                e->watch(retval.datumValue());
                return reinterpret_cast<addr_t>(retval.datumValue());
            }
            e->watch(err.datumValue());
            return reinterpret_cast<addr_t>(err.datumValue());
        }

        err = errorFromOutput(retval);
    }
}

/// @brief Handle a bad value. If ERRACT has a value, run it as an instruction.
/// Otherwise, return the error.
/// @param eAddr a pointer to the Evaluator object
/// @param parentAddr a pointer to the parent node
/// @param value the value to handle
/// @param datatypeTest a function that tests if the value is of the correct type
/// @return a pointer to the result
/// @details This is the recoverable "doesn't like" path. When ERRACT's instruction
/// outputs a value that passes @p datatypeTest, that value replaces the bad input.
/// PAUSE prints the error before its prompt. The same error twice, with no line
/// typed in between, is an ERRACT loop and returns to toplevel.
addr_t handleBadValue(addr_t eAddr, addr_t parentAddr, DatumPtr value, std::function<bool(DatumPtr)> datatypeTest)
{
    auto *parent = reinterpret_cast<ASTNode *>(parentAddr);
    DatumPtr err = {FCError::doesntLike(parent->nodeName_, DatumPtr(value))};
    return recoverWithErract(eAddr, err, value, datatypeTest, [parent](DatumPtr output) {
        return DatumPtr(FCError::doesntLike(parent->nodeName_, output));
    });
}

/// Parse a datum as a double. Writes *out and returns whether it is a valid number.
/// @param eAddr a pointer to the Evaluator object
/// @param datumAddr a pointer to a Datum object
/// @param out destination for the parsed double (0.0 if not a number)
/// @return true iff the datum is a word with a valid number
EXPORTC bool parseDoubleForDatum(addr_t eAddr, addr_t datumAddr, double *out) noexcept
{
    Q_UNUSED(eAddr);
    auto *w = reinterpret_cast<Word *>(datumAddr);
    if (!w->isWord())
    {
        *out = 0.0;
        return false;
    }
    *out = w->numberValue();
    return w->numberIsValid;
}

/// Parse a datum as a bool. Writes *out and returns whether it is a valid boolean word.
/// @param eAddr a pointer to the Evaluator object
/// @param datumAddr a pointer to a Datum object
/// @param out destination for the parsed bool (false if not a boolean word)
/// @return true iff the datum is a word with a valid boolean
EXPORTC bool parseBoolForDatum(addr_t eAddr, addr_t datumAddr, bool *out) noexcept
{
    Q_UNUSED(eAddr);
    auto *w = reinterpret_cast<Word *>(datumAddr);
    if (!w->isWord())
    {
        *out = false;
        return false;
    }
    *out = w->boolValue();
    return w->boolIsValid;
}

/// Lookup the var name and return the value as a QLogo object (Word, List, etc)
/// @param wordAddr a pointer to a Word object which contains the name of the variable
/// @return the stored value as a QLogo object
EXPORTC addr_t getDatumForVarname(addr_t wordAddr) noexcept
{
    auto name = reinterpret_cast<Word *>(wordAddr)->toString(Datum::ToStringFlags_Key);
    Datum *val = Kernel::get().datumForName(name).datumValue();

    return reinterpret_cast<addr_t>(val);
}

/// Create a QLogo Word object using a double value
/// @param eAddr a pointer to the Evaluator object
/// @param val a value stored as a double
/// @return the given value as a QLogo object
EXPORTC addr_t getWordForDouble(addr_t eAddr, double val) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *w = new Word(val);
    e->watch(w);

    return reinterpret_cast<addr_t>(w);
}

/// Create a QLogo Word object using a bool value
/// @param eAddr a pointer to the Evaluator object
/// @param val a value stored as a bool
/// @return the given value as a QLogo object
EXPORTC addr_t getWordForBool(addr_t eAddr, bool val) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *w = new Word(val ? QObject::tr("true") : QObject::tr("false"));
    e->watch(w);

    return reinterpret_cast<addr_t>(w);
}

/// MAKE / LOCALMAKE: store the given datum using the given word as a variable name.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param datumAddr a pointer to a QLogo object which is the value to be stored
/// @param wordAddr a pointer to a Word object which contains the name of the variable
/// @param makeLocal true for LOCALMAKE: localize first, then assign
/// @return the owning ASTNode, or an Error if a TRACE MAKE line cannot be written
EXPORTC addr_t setDatumForWord(addr_t eAddr, addr_t astnodeAddr, addr_t datumAddr, addr_t wordAddr, bool makeLocal) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto d = DatumPtr(reinterpret_cast<Datum *>(datumAddr));
    auto *w = reinterpret_cast<Word *>(wordAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        const QString name = w->toString(Datum::ToStringFlags_Key);
        if (makeLocal)
            Kernel::get().currentCallFrame()->bindLocal(name, d);
        else
            Kernel::get().setDatumForName(d, name);
        Kernel::get().traceMake(DatumPtr(w), d);
    });
}

/// Create and return Error: "SYSTEM"
/// @param eAddr a pointer to the Evaluator object context.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorSystem(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    FCError *err = FCError::custom(DatumPtr(QObject::tr("SYSTEM")));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return Error: "TOPLEVEL"
/// @param eAddr a pointer to the Evaluator object context.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorToplevel(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    FCError *err = FCError::custom(DatumPtr(QObject::tr("TOPLEVEL")));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return Error: "X didn't like Y as input"
/// @param eAddr a pointer to the Evaluator object context.
/// @param whoAddr a pointer to the Datum object which rejected the input.
/// @param whatAddr a pointer to the Datum object that was rejected.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoLike(addr_t eAddr, addr_t whoAddr, addr_t whatAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *who = reinterpret_cast<Datum *>(whoAddr);
    auto *what = reinterpret_cast<Datum *>(whatAddr);
    FCError *err = FCError::doesntLike(who, what);
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return Error: "You don't say what to do with X"
/// @param eAddr a pointer to the Evaluator object context.
/// @param whatAddr a pointer to the Datum object that was orphaned.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoSay(addr_t eAddr, addr_t whatAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *what = reinterpret_cast<Datum *>(whatAddr);
    FCError *err = FCError::dontSay(DatumPtr(what->toString(Datum::ToStringFlags_Show)));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return Error: "X without TEST"
/// @param eAddr a pointer to the Evaluator object context.
/// @param xAddr a pointer to the Datum object that was called without a TEST.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoTest(addr_t eAddr, addr_t whoAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *who = reinterpret_cast<Datum *>(whoAddr);
    FCError *err = FCError::noTest(DatumPtr(who->toString(Datum::ToStringFlags_Show)));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return Error: "X didn't output to Y"
/// @param eAddr a pointer to the Evaluator object context.
/// @param xAddr a pointer to the Datum object which didn't output.
/// @param yAddr a pointer to the Datum object that expected input.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoOutput(addr_t eAddr, addr_t xAddr, addr_t yAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *x = reinterpret_cast<Datum *>(xAddr);
    auto *y = reinterpret_cast<Datum *>(yAddr);
    // If the thing that didn't output is an ASTNode, use the name of the ASTNode.
    if (x->isa_ == Datum::typeASTNode)
    {
        x = reinterpret_cast<ASTNode *>(x)->nodeName_.datumValue();
    }
    FCError *err = FCError::didntOutput(DatumPtr(x->toString(Datum::ToStringFlags_Show)),
                                        DatumPtr(y->toString(Datum::ToStringFlags_Show)));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return Error: "not enough inputs to X"
/// @param eAddr a pointer to the Evaluator object context.
/// @param xAddr a pointer to the Datum object that is deprived of inputs.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNotEnoughInputs(addr_t eAddr, addr_t xAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *x = reinterpret_cast<Datum *>(xAddr);
    FCError *err = FCError::notEnoughInputs(DatumPtr(x->toString(Datum::ToStringFlags_Show)));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return Error: "X has no value"
/// @param eAddr a pointer to the Evaluator object context.
/// @param whatAddr a pointer to the Word that was looked up.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoValue(addr_t eAddr, addr_t whatAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *what = reinterpret_cast<Datum *>(whatAddr);
    FCError *err = FCError::noValue(DatumPtr(what->toString(Datum::ToStringFlags_Show)));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return Error: "Graphics not initialized"
/// @param eAddr a pointer to the Evaluator object context.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoGraphics(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    FCError *err = FCError::noGraphics();
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return an error generated by THROW
/// @param eAddr a pointer to the Evaluator object context.
/// @param tagAddr a pointer to the Datum object which is the tag of the error.
/// @param outputAddr a pointer to the Datum object which is the output of the error.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorCustom(addr_t eAddr, addr_t tagAddr, addr_t outputAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *tag = reinterpret_cast<Datum *>(tagAddr);
    auto *output = reinterpret_cast<Datum *>(outputAddr);
    FCError *err = FCError::custom(DatumPtr(tag), nothing(), DatumPtr(output));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

// Functions moved from compiler*.cpp files

/// @brief Handle a bad double value. If ERRACT has a value, run it. Otherwise, return an error.
/// @param eAddr a pointer to the Evaluator object
/// @param parentAddr a pointer to the parent node
/// @param value the value to handle
/// @return a pointer to the result
EXPORTC addr_t handleBadDouble(addr_t eAddr, addr_t parentAddr, double value) noexcept
{
    return handleBadValue(eAddr, parentAddr, DatumPtr(value), [](DatumPtr p) {
        if (p.isWord())
        {
            Word *word = p.wordValue();
            word->numberValue();
            return word->numberIsValid;
        }
        return false;
    });
}

/// @brief Handle a bad datum value. If ERRACT has a value, run it. Otherwise, return an error.
/// @param eAddr a pointer to the Evaluator object
/// @param parentAddr a pointer to the parent node
/// @param valueAddr a pointer to a Datum object
/// @return a pointer to the result
EXPORTC addr_t handleBadDatum(addr_t eAddr, addr_t parentAddr, addr_t valueAddr) noexcept
{
    auto value = reinterpret_cast<Datum *>(valueAddr);
    return handleBadValue(
        eAddr, parentAddr, DatumPtr(value), [](DatumPtr p) { return (p.isa() & Datum::typeDataMask) != 0; });
}

/// @brief Handle an unbound variable. If ERRACT has a value, run it. Otherwise, return an error.
/// @param eAddr a pointer to the Evaluator object
/// @param whatAddr a pointer to the Word that was looked up
/// @return the replacement datum, or an error
EXPORTC addr_t handleNoValue(addr_t eAddr, addr_t whatAddr) noexcept
{
    auto *what = reinterpret_cast<Datum *>(whatAddr);
    DatumPtr name(what->toString(Datum::ToStringFlags_Show));
    DatumPtr err = {FCError::noValue(name)};
    return recoverWithErract(
        eAddr,
        err,
        nothing(),
        [](DatumPtr p) { return (p.isa() & Datum::typeDataMask) != 0; },
        [name](DatumPtr) { return DatumPtr(FCError::noValue(name)); });
}

/// @brief Handle a command that did not output. If ERRACT has a value, run it.
/// Otherwise, return an error.
/// @param eAddr a pointer to the Evaluator object
/// @param xAddr a pointer to the Datum that did not output
/// @param yAddr a pointer to the Datum that expected an input
/// @return the replacement datum, or an error
EXPORTC addr_t handleDidntOutput(addr_t eAddr, addr_t xAddr, addr_t yAddr) noexcept
{
    auto *x = reinterpret_cast<Datum *>(xAddr);
    auto *y = reinterpret_cast<Datum *>(yAddr);
    if (x->isa_ == Datum::typeASTNode)
        x = reinterpret_cast<ASTNode *>(x)->nodeName_.datumValue();
    DatumPtr who(x->toString(Datum::ToStringFlags_Show));
    DatumPtr what(y->toString(Datum::ToStringFlags_Show));
    DatumPtr err = {FCError::didntOutput(who, what)};
    return recoverWithErract(
        eAddr,
        err,
        nothing(),
        [](DatumPtr p) { return (p.isa() & Datum::typeDataMask) != 0; },
        [who, what](DatumPtr) { return DatumPtr(FCError::didntOutput(who, what)); });
}

/// @brief Allocate memory with a specified alignment.
/// @param eAddr a pointer to the Evaluator object
/// @param size the size of the memory to allocate
/// @param align the alignment of the memory to allocate
/// @return a pointer to the allocated memory
EXPORTC addr_t q_malloc(addr_t eAddr, uint32_t size, uint32_t align) noexcept
{
    align = std::max(align, static_cast<uint32_t>(alignof(std::max_align_t)));
    const uint64_t rounded = (static_cast<uint64_t>(size) + align - 1) / align * align;
    if (rounded == 0 || rounded > UINT32_MAX)
        FCError::fatalInternal();
    size = static_cast<uint32_t>(rounded);
#ifdef _WIN32
    void *ptr = _aligned_malloc(size, align);
#else
    void *ptr = aligned_alloc(align, size);
#endif
    if (ptr == nullptr)
        FCError::fatalInternal();
    return reinterpret_cast<addr_t>(ptr);
}

/// @brief Free memory allocated with q_malloc.
/// @param eAddr a pointer to the Evaluator object
/// @param ptr a pointer to the memory to free
EXPORTC void q_free(addr_t eAddr, addr_t ptr) noexcept
{
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}
