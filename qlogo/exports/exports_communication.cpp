// ===-- qlogo/exports/exports_communication.cpp - Exports for the QLogo language -------*- C++ -*-===//
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
/// for file and terminal input and output.
///
//===----------------------------------------------------------------------===//

#include "interface/streammanager.h"
#include "interface/logointerface.h"
#include "workspace/exports_internal.h"
#include "datum_types.h"
#include "workspace/kernel.h"
#include "flowcontrol.h"
#include "sharedconstants.h"

#include <QFile>
#include <QObject>
#include <QStringBuilder>

/// Write a Datum object to the standard output device.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param datumAddr a pointer to a Datum object to print.
/// @param useShow set to true to generate output for SHOW, false for PRINT
/// @return a pointer to the owning ASTNode object or a pointer to the Error object if an error occurs.
EXPORTC addr_t stdWriteDatum(addr_t eAddr, addr_t astnodeAddr, addr_t datumAddr, bool useShow)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    Datum::ToStringFlags writeFlags = useShow ? Datum::ToStringFlags_Show : Datum::ToStringFlags_None;
    auto *d = reinterpret_cast<Datum *>(datumAddr);
    QString output = d->toString(writeFlags) + "\n";
    try {
        StreamManager::get().print(output);
    } catch (FCError *err) {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(astnodeAddr);
}

/// Write an array of Datum objects to the standard output device.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param datumAddr a pointer to an array of pointers to Datum objects to print.
/// @param count the number of Datum objects in the array
/// @param useShow set to true to generate output for SHOW, false for PRINT
/// @param addWhitespace set to true to add a newline to the end of the output and spaces between datums.
EXPORTC addr_t stdWriteDatumAry(addr_t eAddr, addr_t astnodeAddr, addr_t datumAddr, uint32_t count, bool useShow, bool addWhitespace)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    Datum::ToStringFlags writeFlags = useShow ? Datum::ToStringFlags_Show : Datum::ToStringFlags_None;
    auto **datumAry = reinterpret_cast<Datum **>(datumAddr);
    QString output;
    for (uint32_t i = 0; i < count; ++i)
    {
        Datum *d = datumAry[i];
        if ((i != 0) && addWhitespace)
            output = output % " ";
        output = output % d->toString(writeFlags);
    }
    if (addWhitespace)
        output = output % "\n";
        try {
            StreamManager::get().print(output);
        } catch (FCError *err) {
            e->watch(err);
            return reinterpret_cast<addr_t>(err);
        }
        return reinterpret_cast<addr_t>(astnodeAddr);
    }

/// Read a list from the current reader (READLIST / RL).
/// @param eAddr a pointer to the Evaluator object
/// @return a list, the empty word at end of input, or an Error on I/O failure.
EXPORTC addr_t stdReadList(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    try
    {
        DatumPtr retval = StreamManager::get().readList(QString(), false);
        if (retval.isNothing())
        {
            auto *emptyWord = new Word(QStringLiteral(""));
            e->watch(emptyWord);
            return reinterpret_cast<addr_t>(emptyWord);
        }
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// Read a word from the current reader (READWORD / RW).
/// @param eAddr a pointer to the Evaluator object
/// @return a word, the empty list at end of input, or an Error on I/O failure.
EXPORTC addr_t stdReadWord(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    try
    {
        DatumPtr retval = StreamManager::get().readWord(QString());
        if (retval.isNothing())
        {
            List *empty = EmptyList::instance();
            e->watch(empty);
            return reinterpret_cast<addr_t>(empty);
        }
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// Read a raw line from the current reader (READRAWLINE).
/// @param eAddr a pointer to the Evaluator object
/// @return a word, the empty list at end of input, or an Error on I/O failure.
EXPORTC addr_t stdReadRawline(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    try
    {
        DatumPtr retval = StreamManager::get().readRawline(QString());
        if (retval.isNothing())
        {
            List *empty = EmptyList::instance();
            e->watch(empty);
            return reinterpret_cast<addr_t>(empty);
        }
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// Read characters from the current reader (READCHAR / RC / READCHARS / RCS).
/// @param eAddr a pointer to the Evaluator object
/// @param count the number of characters to read
/// @return a word of the characters read, the empty list at end of input, or an Error on I/O failure.
EXPORTC addr_t stdReadChars(addr_t eAddr, int32_t count)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    try
    {
        DatumPtr retval = StreamManager::get().readChars(count);
        if (retval.isNothing())
        {
            List *empty = EmptyList::instance();
            e->watch(empty);
            return reinterpret_cast<addr_t>(empty);
        }
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// Present a modal file dialog (FILEDIALOG). GUI only.
/// @param eAddr a pointer to the Evaluator object
/// @return a word containing the selected path, the empty list if cancelled, or an Error.
EXPORTC addr_t stdFileDialog(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    try
    {
        QString path = Config::get().mainInterface()->fileDialogModal();
        if (path.isEmpty())
        {
            List *empty = EmptyList::instance();
            e->watch(empty);
            return reinterpret_cast<addr_t>(empty);
        }
        auto *word = new Word(path);
        e->watch(word);
        return reinterpret_cast<addr_t>(word);
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// Print the QLogo copyright message to the current writer (COPYRIGHT).
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @return the owning ASTNode, or an Error on I/O failure.
EXPORTC addr_t stdCopyright(addr_t eAddr, addr_t astnodeAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    QString printString = QStringLiteral(
        "QLogo and Psychi source Copyright 2017-2026 Jason Sikes\n"
        "Help and library texts Copyright (C) 1993 by the Regents of the University of California\n");
    try
    {
        StreamManager::get().print(printString);
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(node);
}

