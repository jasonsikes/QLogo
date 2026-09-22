// ===-- qlogo/exports/exports_workspacemanagement.cpp - Exports for the QLogo language -------*- C++ -*-===//
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
/// for workspace management.
///
//===----------------------------------------------------------------------===//

#include "workspace/exports_internal.h"
#include "datum_types.h"
#include "workspace/kernel.h"
#include "workspace/procedures.h"
#include "astnode.h"
#include "flowcontrol.h"
#include "compiler_types.h"
#include "workspace/callframe.h"

#include <QStringList>

/// @brief input a procedure using the system read stream.
/// @param eAddr a pointer to the Evaluator object
/// @param nodeAddr a pointer to the node calling this procedure.
/// @return ASTNode on success, else Err.
EXPORTC addr_t inputProcedure(addr_t eAddr, addr_t nodeAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(nodeAddr);
    const CallFrame *currentFrame = Kernel::get().currentCallFrame();
    DatumPtr currentProc = currentFrame->sourceNode_;

    // We don't allow inputting a procedure while in a procedure.
    if (currentProc.isASTNode())
    {
        FCError *err = FCError::toInProc(node->nodeName_);
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }

    Datum *retval = Kernel::get().inputProcedure(node);

    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

/// DEFINE: install a procedure from a list of lists.
/// The first sublist is the parameter list. The rest are instruction lines.
/// There is no END line, and no source-text history is stored.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param nameAddr a word naming the procedure
/// @param textAddr a list whose members are lists
/// @return the owning ASTNode, or an Error if the name or text is rejected.
EXPORTC addr_t stdDefine(addr_t eAddr, addr_t astnodeAddr, addr_t nameAddr, addr_t textAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *name = reinterpret_cast<Datum *>(nameAddr);
    auto *text = reinterpret_cast<Datum *>(textAddr);
    try
    {
        Procedures::get().defineProcedure(node->nodeName_, DatumPtr(name), DatumPtr(text), QList<DatumPtr>());
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(node);
}

/// TEXT: output the DEFINE-form text of a user procedure.
/// The result is the stored instruction list: a parameter list followed by
/// one sublist per body line. There is no END line.
/// @param eAddr a pointer to the Evaluator object
/// @param nameAddr a word naming the procedure
/// @return the procedure text as a list of lists, or an Error if the name is a primitive or undefined.
EXPORTC addr_t stdText(addr_t eAddr, addr_t nameAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *name = reinterpret_cast<Datum *>(nameAddr);
    try
    {
        DatumPtr text = Procedures::get().procedureText(DatumPtr(name));
        e->watch(text);
        return reinterpret_cast<addr_t>(text.datumValue());
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// FULLTEXT: output the source form of a user procedure.
/// TO and .MACRO keep the raw source lines, so the result is a list of words
/// including END. DEFINE and .DEFMACRO have no source lines, so the result is
/// a list of lists: a generated title, the body, and [end].
/// @param eAddr a pointer to the Evaluator object
/// @param nameAddr a word naming the procedure
/// @return the procedure's full text, or an Error if the name is a primitive or undefined.
EXPORTC addr_t stdFulltext(addr_t eAddr, addr_t nameAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *name = reinterpret_cast<Datum *>(nameAddr);
    try
    {
        DatumPtr text = Procedures::get().procedureFulltext(DatumPtr(name));
        e->watch(text);
        return reinterpret_cast<addr_t>(text.datumValue());
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// COPYDEF: make newname a procedure identical to oldname.
/// A user procedure is copied. A primitive is aliased, and the new name is
/// then a primitive itself. The new name must not already name a primitive.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param newnameAddr a word naming the procedure to create
/// @param oldnameAddr a word naming the procedure to copy
/// @return the owning ASTNode, or an Error if either name is rejected.
EXPORTC addr_t stdCopydef(addr_t eAddr, addr_t astnodeAddr, addr_t newnameAddr, addr_t oldnameAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *newname = reinterpret_cast<Datum *>(newnameAddr);
    auto *oldname = reinterpret_cast<Datum *>(oldnameAddr);
    try
    {
        Procedures::get().copyProcedure(node->nodeName_, DatumPtr(newname), DatumPtr(oldname));
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(node);
}

/// THING: output the value of the variable named by the input word.
/// Dynamic scope selects the innermost binding. An unbound name is an error.
/// @param eAddr a pointer to the Evaluator object
/// @param nameAddr a word naming the variable
/// @return the variable's value, or an Error if it has no value.
EXPORTC addr_t stdThing(addr_t eAddr, addr_t nameAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *name = reinterpret_cast<Datum *>(nameAddr);
    DatumPtr val = Kernel::get().datumForName(name->toString(Datum::ToStringFlags_Key));
    if (!val.isNothing())
        return reinterpret_cast<addr_t>(val.datumValue());

    FCError *err = FCError::noValue(DatumPtr(name));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

namespace
{

/// Words, or one list of words. A list combined with other inputs is rejected.
/// @return Key-form names. Throws before returning if any input is not a word.
QStringList varNamesFromInputs(const DatumPtr &who, Datum **inputs, uint32_t count)
{
    QStringList names;
    if (count > 1 && inputs[0]->isList())
        throw FCError::doesntLike(who, DatumPtr(inputs[0]));

    if (count == 1 && inputs[0]->isList())
    {
        ListIterator iter = inputs[0]->listValue()->newIterator();
        while (iter.elementExists())
        {
            DatumPtr member = iter.element();
            if (!member.isWord())
                throw FCError::doesntLike(who, member);
            names.append(member.wordValue()->toString(Datum::ToStringFlags_Key));
        }
        return names;
    }

    for (uint32_t i = 0; i < count; ++i)
    {
        if (!inputs[i]->isWord())
            throw FCError::doesntLike(who, DatumPtr(inputs[i]));
        names.append(inputs[i]->toString(Datum::ToStringFlags_Key));
    }
    return names;
}

} // namespace

/// GLOBAL: create each missing variable with no value.
/// A variable that already exists is left unchanged. Scope comes from LOCAL.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param inputsAddr a pointer to an array of input Datums
/// @param count the number of inputs
/// @return the owning ASTNode, or an Error if an input is not a word or a list of words.
EXPORTC addr_t stdGlobal(addr_t eAddr, addr_t astnodeAddr, addr_t inputsAddr, uint32_t count)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto **inputs = reinterpret_cast<Datum **>(inputsAddr);
    try
    {
        QStringList names = varNamesFromInputs(node->nodeName_, inputs, count);
        for (const QString &name : names)
        {
            if (!Kernel::get().doesExist(name))
                Kernel::get().setDatumForName(nothing(), name);
        }
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(node);
}

/// LOCAL: make each name local to the current procedure, with no value.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param inputsAddr a pointer to an array of input Datums
/// @param count the number of inputs
/// @return the owning ASTNode, or an Error if an input is not a word or a list of words.
EXPORTC addr_t stdLocal(addr_t eAddr, addr_t astnodeAddr, addr_t inputsAddr, uint32_t count)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto **inputs = reinterpret_cast<Datum **>(inputsAddr);
    try
    {
        QStringList names = varNamesFromInputs(node->nodeName_, inputs, count);
        CallFrame *currentFrame = Kernel::get().currentCallFrame();
        for (const QString &name : names)
            currentFrame->setVarAsLocal(name);
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(node);
}

