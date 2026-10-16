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
#include "compiler.h"
#include "datum_types.h"
#include "workspace/kernel.h"
#include "workspace/library.h"
#include "workspace/procedures.h"
#include "workspace/specialvars.h"
#include "astnode.h"
#include "flowcontrol.h"
#include "compiler_types.h"
#include "workspace/callframe.h"
#include "interface/streammanager.h"

#include <QDebug>
#include <QObject>
#include <QStringBuilder>
#include <QStringList>

/// @brief input a procedure using the system read stream.
/// @param eAddr a pointer to the Evaluator object
/// @param nodeAddr a pointer to the node calling this procedure.
/// @return ASTNode on success, else Err.
EXPORTC addr_t inputProcedure(addr_t eAddr, addr_t nodeAddr) noexcept
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
/// The compiler rejects a bad name or a text that is not a non-empty list of lists.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param nameAddr a word naming the procedure
/// @param textAddr a non-empty list whose members are lists
/// @return the owning ASTNode, or an Error if the name is a primitive or the
/// parameter list is rejected.
EXPORTC addr_t stdDefine(addr_t eAddr, addr_t astnodeAddr, addr_t nameAddr, addr_t textAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *name = reinterpret_cast<Datum *>(nameAddr);
    auto *text = reinterpret_cast<Datum *>(textAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        Procedures::get().defineProcedure(node->nodeName_, DatumPtr(name), DatumPtr(text), QList<DatumPtr>());
    });
}

/// TEXT: output the DEFINE-form text of a user procedure.
/// The result is the stored instruction list: a parameter list followed by
/// one sublist per body line. There is no END line.
/// @param eAddr a pointer to the Evaluator object
/// @param nameAddr a word naming the procedure
/// @return the procedure text as a list of lists, or an Error if the name is a primitive or undefined.
EXPORTC addr_t stdText(addr_t eAddr, addr_t nameAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *name = reinterpret_cast<Datum *>(nameAddr);
    return catchError(e, [&]() -> addr_t {
        DatumPtr text = Procedures::get().procedureText(DatumPtr(name));
        e->watch(text);
        return reinterpret_cast<addr_t>(text.datumValue());
    });
}

/// FULLTEXT: output the source form of a user procedure.
/// TO and .MACRO keep the raw source lines, so the result is a list of words
/// including END. DEFINE and .DEFMACRO have no source lines, so the result is
/// a list of lists: a generated title, the body, and [end].
/// @param eAddr a pointer to the Evaluator object
/// @param nameAddr a word naming the procedure
/// @return the procedure's full text, or an Error if the name is a primitive or undefined.
EXPORTC addr_t stdFulltext(addr_t eAddr, addr_t nameAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *name = reinterpret_cast<Datum *>(nameAddr);
    return catchError(e, [&]() -> addr_t {
        DatumPtr text = Procedures::get().procedureFulltext(DatumPtr(name));
        e->watch(text);
        return reinterpret_cast<addr_t>(text.datumValue());
    });
}

/// COPYDEF: make newname a procedure identical to oldname.
/// A user procedure is copied. A primitive is aliased, and the new name is
/// then a primitive itself. The new name must not already name a primitive.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param newnameAddr a word naming the procedure to create
/// @param oldnameAddr a word naming the procedure to copy
/// @return the owning ASTNode, or an Error if either name is rejected.
EXPORTC addr_t stdCopydef(addr_t eAddr, addr_t astnodeAddr, addr_t newnameAddr, addr_t oldnameAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *newname = reinterpret_cast<Datum *>(newnameAddr);
    auto *oldname = reinterpret_cast<Datum *>(oldnameAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        Procedures::get().copyProcedure(node->nodeName_, DatumPtr(newname), DatumPtr(oldname));
    });
}

/// THING: output the value of the variable named by the input word.
/// Dynamic scope selects the innermost binding. An unbound name is an error.
/// @param eAddr a pointer to the Evaluator object
/// @param nameAddr a word naming the variable
/// @return the variable's value, or an Error if it has no value.
EXPORTC addr_t stdThing(addr_t eAddr, addr_t nameAddr) noexcept
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

/// FOO: output the value of the variable named by this command.
/// The name is the AST node's command word, not an input. A missing or
/// valueless name is "I don't know how", not "has no value".
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @return the variable's value, or an Error if it has no value.
EXPORTC addr_t stdFoo(addr_t eAddr, addr_t astnodeAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    QString name = node->nodeName_.toString(Datum::ToStringFlags_Key);
    DatumPtr val = Kernel::get().datumForName(name);
    if (!val.isNothing())
        return reinterpret_cast<addr_t>(val.datumValue());

    FCError *err = FCError::noHow(node->nodeName_);
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// SETFOO: assign to the variable named by this command without SET.
/// The variable must already exist. A missing name is "I don't know how".
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param valueAddr the value to store
/// @return the owning ASTNode, or an Error if the variable does not exist.
EXPORTC addr_t stdSetfoo(addr_t eAddr, addr_t astnodeAddr, addr_t valueAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *value = reinterpret_cast<Datum *>(valueAddr);
    QString cmd = node->nodeName_.toString(Datum::ToStringFlags_Key);
    QString name = cmd.mid(QObject::tr("SET").size());
    if (!Kernel::get().doesExist(name))
    {
        FCError *err = FCError::noHow(node->nodeName_);
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    Kernel::get().setDatumForName(DatumPtr(value), name);
    return reinterpret_cast<addr_t>(node);
}

namespace
{

/// Words, or one list of words. A list combined with other inputs is rejected.
/// The compiler rejects a top-level array, and rejects a non-word when there is
/// more than one input. A list of names may still contain a non-word member.
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
/// The compiler rejects a non-word, or (with one input) a non-word-or-list.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param inputsAddr a pointer to an array of input Datums
/// @param count the number of inputs
/// @return the owning ASTNode, or an Error if a list of names has a non-word member.
EXPORTC addr_t stdGlobal(addr_t eAddr, addr_t astnodeAddr, addr_t inputsAddr, uint32_t count) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto **inputs = reinterpret_cast<Datum **>(inputsAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        QStringList names = varNamesFromInputs(node->nodeName_, inputs, count);
        for (const QString &name : names)
        {
            if (!Kernel::get().doesExist(name))
                Kernel::get().setDatumForName(nothing(), name);
        }
    });
}

/// LOCAL: make each name local to the current procedure, with no value.
/// The compiler rejects a non-word, or (with one input) a non-word-or-list.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param inputsAddr a pointer to an array of input Datums
/// @param count the number of inputs
/// @return the owning ASTNode, or an Error if a list of names has a non-word member.
EXPORTC addr_t stdLocal(addr_t eAddr, addr_t astnodeAddr, addr_t inputsAddr, uint32_t count) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto **inputs = reinterpret_cast<Datum **>(inputsAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        QStringList names = varNamesFromInputs(node->nodeName_, inputs, count);
        CallFrame *currentFrame = Kernel::get().currentCallFrame();
        for (const QString &name : names)
            currentFrame->setVarAsLocal(name);
    });
}

namespace
{

QString propertyListKey(Datum *word)
{
    return word->toString(Datum::ToStringFlags_Key);
}

QString propertyNameKey(Datum *word)
{
    if (Kernel::get().specialVarIsTrue(SpecialVars::Id::CaseIgnoredP))
        return word->toString(Datum::ToStringFlags_Key);
    return word->toString();
}

} // namespace

/// PPROP: add or replace one property. Property-list names are always
/// case-insensitive. Property names follow CASEIGNOREDP.
/// @param plistnameAddr a word naming the property list
/// @param propnameAddr a word naming the property
/// @param valueAddr the value to store
EXPORTC void stdPprop(addr_t plistnameAddr, addr_t propnameAddr, addr_t valueAddr) noexcept
{
    auto *plistname = reinterpret_cast<Datum *>(plistnameAddr);
    auto *propname = reinterpret_cast<Datum *>(propnameAddr);
    auto *value = reinterpret_cast<Datum *>(valueAddr);
    Kernel::get().plists_.addProperty(propertyListKey(plistname), propertyNameKey(propname), DatumPtr(value));
}

/// GPROP: output a property value, or the empty list if it is absent.
/// @param eAddr a pointer to the Evaluator object
/// @param plistnameAddr a word naming the property list
/// @param propnameAddr a word naming the property
/// @return the stored value, or an empty list.
EXPORTC addr_t stdGprop(addr_t eAddr, addr_t plistnameAddr, addr_t propnameAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *plistname = reinterpret_cast<Datum *>(plistnameAddr);
    auto *propname = reinterpret_cast<Datum *>(propnameAddr);
    DatumPtr value = Kernel::get().plists_.getProperty(propertyListKey(plistname), propertyNameKey(propname));
    e->watch(value);
    return reinterpret_cast<addr_t>(value.datumValue());
}

/// REMPROP: remove one property. A missing name is ignored.
/// Removing the last property erases the property list.
/// @param plistnameAddr a word naming the property list
/// @param propnameAddr a word naming the property
EXPORTC void stdRemprop(addr_t plistnameAddr, addr_t propnameAddr) noexcept
{
    auto *plistname = reinterpret_cast<Datum *>(plistnameAddr);
    auto *propname = reinterpret_cast<Datum *>(propnameAddr);
    Kernel::get().plists_.removeProperty(propertyListKey(plistname), propertyNameKey(propname));
}

/// PLIST: output a copy of the property list as alternating names and values.
/// An unknown name outputs an empty list. Names in the result are the stored keys.
/// @param eAddr a pointer to the Evaluator object
/// @param plistnameAddr a word naming the property list
/// @return a list of alternating property names and values.
EXPORTC addr_t stdPlist(addr_t eAddr, addr_t plistnameAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *plistname = reinterpret_cast<Datum *>(plistnameAddr);
    DatumPtr plist = Kernel::get().plists_.getPropertyList(propertyListKey(plistname));
    e->watch(plist);
    return reinterpret_cast<addr_t>(plist.datumValue());
}

/// PROCEDUREP: TRUE if the word names a primitive or a user procedure.
/// An unknown name is FALSE. The compiler rejects a non-word before this runs.
/// @param nameAddr a word naming the candidate
/// @return true if the name is a procedure.
EXPORTC bool stdProcedurep(addr_t nameAddr) noexcept
{
    auto *name = reinterpret_cast<Datum *>(nameAddr);
    return Procedures::get().isProcedure(name->toString(Datum::ToStringFlags_Key));
}

/// MACROP: TRUE if the word names a user macro.
/// Primitives, ordinary procedures, and unknown names are FALSE.
/// @param nameAddr a word naming the candidate
/// @return true if the name is a macro.
EXPORTC bool stdMacrop(addr_t nameAddr) noexcept
{
    auto *name = reinterpret_cast<Datum *>(nameAddr);
    return Procedures::get().isMacro(name->toString(Datum::ToStringFlags_Key));
}

/// PRIMITIVEP: TRUE if the word names a built-in procedure.
/// A name created by COPYDEF from a primitive is itself a primitive.
/// User procedures, macros, and unknown names are FALSE.
/// @param nameAddr a word naming the candidate
/// @return true if the name is a primitive.
EXPORTC bool stdPrimitivep(addr_t nameAddr) noexcept
{
    auto *name = reinterpret_cast<Datum *>(nameAddr);
    return Procedures::get().isPrimitive(name->toString(Datum::ToStringFlags_Key));
}

/// DEFINEDP: TRUE if the word names a user-defined procedure.
/// TO, DEFINE, .MACRO, .DEFMACRO, and COPYDEF of one of those all count.
/// Primitives, names created by COPYDEF from a primitive, and unknown names are FALSE.
/// @param nameAddr a word naming the candidate
/// @return true if the name is a user-defined procedure.
EXPORTC bool stdDefinedp(addr_t nameAddr) noexcept
{
    auto *name = reinterpret_cast<Datum *>(nameAddr);
    return Procedures::get().isDefined(name->toString(Datum::ToStringFlags_Key));
}

/// NAMEP: TRUE if the word names a variable that has a value.
/// LOCAL and GLOBAL install a name with no value; that stays FALSE until MAKE.
/// An unknown name is FALSE. The compiler rejects a non-word before this runs.
/// @param nameAddr a word naming the candidate
/// @return true if the name has a value.
EXPORTC bool stdNamep(addr_t nameAddr) noexcept
{
    auto *name = reinterpret_cast<Datum *>(nameAddr);
    DatumPtr val = Kernel::get().datumForName(name->toString(Datum::ToStringFlags_Key));
    return !val.isNothing();
}

/// PLISTP: TRUE if the word names a nonempty property list.
/// A name with no properties is FALSE. Removing the last property erases the list.
/// The compiler rejects a non-word before this runs.
/// @param nameAddr a word naming the candidate
/// @return true if the name has at least one property.
EXPORTC bool stdPlistp(addr_t nameAddr) noexcept
{
    auto *name = reinterpret_cast<Datum *>(nameAddr);
    return Kernel::get().plists_.isPropertyList(name->toString(Datum::ToStringFlags_Key));
}

/// True if the datum is a contents list: a word, a list of words, or a list of
/// up to three lists of words. The compiler uses this as one ERRACT trip.
EXPORTC bool isContentsList(addr_t datumAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(datumAddr);
    return Kernel::get().isContentsList(DatumPtr(d));
}

/// True if the datum is a contents list that names at least one thing. Empty
/// ~[]~ / ~[[][]]~ / ~[[][][]]~ are false. The compiler uses this as one ERRACT trip.
EXPORTC bool isNamedContentsList(addr_t datumAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(datumAddr);
    return Kernel::get().isNamedContentsList(DatumPtr(d));
}

/// True if the datum is a DEFINE procedure name: a word that is not a number
/// and does not start with '"' or ':'. The compiler uses this as one ERRACT trip.
EXPORTC bool isDefinableProcName(addr_t nameAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(nameAddr);
    return Procedures::get().isDefinableProcName(DatumPtr(d));
}

/// True if the datum is DEFINE text: a non-empty list of lists. Parameter-list
/// grammar is not checked. The compiler uses this as one ERRACT trip.
EXPORTC bool isDefineText(addr_t textAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(textAddr);
    return Procedures::get().isDefineText(DatumPtr(d));
}

namespace
{

/// Add or remove each word in @p names for @p mark and @p kind.
/// A word is one name. A list is those names. A list or array member is rejected.
/// Adding a name already present leaves it. Removing a missing name does nothing.
/// @param clear When true, remove the names. When false, add them.
void markNames(Kernel::NameMark mark, Kernel::NameKind kind, Datum *names, const DatumPtr &who, bool clear)
{
    auto apply = clear ? &Kernel::removeName : &Kernel::addName;
    if (!names->isList())
    {
        if (!names->isWord())
            throw FCError::doesntLike(who, DatumPtr(names));
        (Kernel::get().*apply)(mark, kind, names->toString(Datum::ToStringFlags_Key));
        return;
    }

    ListIterator iter = names->listValue()->newIterator();
    while (iter.elementExists())
    {
        DatumPtr member = iter.element();
        if (!member.isWord())
            throw FCError::doesntLike(who, member);
        (Kernel::get().*apply)(mark, kind, member.wordValue()->toString(Datum::ToStringFlags_Key));
    }
}

/// UCBLogo contents list, same grammar as Kernel::contentsListFromDatum.
/// Apply @p mark to each name in the three sublists. Extra members past the
/// third list are rejected before this runs. Names need not already exist.
/// Names before a rejected member stay added or removed.
/// @param clear When true, remove the names. When false, add them.
void markContents(Kernel::NameMark mark, const DatumPtr &who, Datum *input, bool clear)
{
    DatumPtr three = Kernel::get().contentsListFromDatum(who, DatumPtr(input));
    List *contents = three.listValue();
    markNames(mark, Kernel::NameKind::Procedures, contents->head.datumValue(), who, clear);

    const List *rest = contents->tail.listValue();
    markNames(mark, Kernel::NameKind::Variables, rest->head.datumValue(), who, clear);

    rest = rest->tail.listValue();
    markNames(mark, Kernel::NameKind::PropertyLists, rest->head.datumValue(), who, clear);
}

addr_t markWorkspace(addr_t eAddr, addr_t astnodeAddr, addr_t contentsAddr, Kernel::NameMark mark, bool clear)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *contents = reinterpret_cast<Datum *>(contentsAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        markContents(mark, node->nodeName_, contents, clear);
        if (mark == Kernel::NameMark::Traced)
            Compiler::get().noteCodeChange();
    });
}

/// The first word in @p names, and whether that name carries @p mark.
/// An empty list sets @p found false. Later names are not examined.
/// The compiler has already required a named contents list of words.
bool queryFirstName(Kernel::NameMark mark, Kernel::NameKind kind, Datum *names, bool *found)
{
    *found = false;
    Q_ASSERT(names->isList());
    if (!names->isList())
        return false;

    ListIterator iter = names->listValue()->newIterator();
    if (!iter.elementExists())
        return false;

    DatumPtr member = iter.element();
    Q_ASSERT(member.isWord());
    if (!member.isWord())
        return false;
    *found = true;
    return Kernel::get().containsName(mark, kind, member.wordValue()->toString(Datum::ToStringFlags_Key));
}

/// TRUE if the first procedure, variable, or property-list name in @p input
/// carries @p mark. The compiler rejects a non-contents-list and an input that
/// names nothing before this runs.
bool queryContents(Kernel::NameMark mark, Datum *input)
{
    DatumPtr src(input);
    if (!Kernel::get().isNamedContentsList(src))
    {
        Q_ASSERT(false);
        return false;
    }
    DatumPtr three = Kernel::get().contentsListFromDatum(nothing(), src);
    bool found = false;
    bool marked = false;
    auto consider = [&](Kernel::NameKind kind, Datum *names) {
        if (!found)
            marked = queryFirstName(mark, kind, names, &found);
    };

    List *contents = three.listValue();
    consider(Kernel::NameKind::Procedures, contents->head.datumValue());
    const List *rest = contents->tail.listValue();
    consider(Kernel::NameKind::Variables, rest->head.datumValue());
    rest = rest->tail.listValue();
    consider(Kernel::NameKind::PropertyLists, rest->head.datumValue());
    Q_ASSERT(found);
    return marked;
}

bool keepDefinedProcedure(const QString &name)
{
    return Procedures::get().isDefined(name);
}

bool keepValuedVariable(const QString &name)
{
    return !Kernel::get().datumForName(name).isNothing();
}

bool keepPropertyList(const QString &name)
{
    return Kernel::get().plists_.isPropertyList(name);
}

/// Contents list of names carrying @p mark that currently exist.
/// Procedures must be user-defined. Variables must have a value.
/// Property lists must be nonempty. Buried primitives and names that
/// do not exist yet are omitted until they qualify.
DatumPtr contentsOfMark(Kernel::NameMark mark)
{
    auto names = [mark](Kernel::NameKind kind, bool (*keep)(const QString &)) {
        ListBuilder builder;
        for (const QString &name : Kernel::get().nameList(mark, kind))
        {
            if (keep(name))
                builder.append(DatumPtr(name));
        }
        return builder.finishedList();
    };

    ListBuilder contents;
    contents.append(names(Kernel::NameKind::Procedures, keepDefinedProcedure));
    contents.append(names(Kernel::NameKind::Variables, keepValuedVariable));
    contents.append(names(Kernel::NameKind::PropertyLists, keepPropertyList));
    return contents.finishedList();
}

bool queryWorkspace(addr_t contentsAddr, Kernel::NameMark mark)
{
    auto *contents = reinterpret_cast<Datum *>(contentsAddr);
    return queryContents(mark, contents);
}

} // namespace

/// BURY: mark the named procedures, variables, and property lists as buried.
/// The input is a contents list. See markContents for the accepted forms.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param contentsAddr a word or a contents list
/// @return the owning ASTNode, or an Error if a name is not a word.
EXPORTC addr_t stdBury(addr_t eAddr, addr_t astnodeAddr, addr_t contentsAddr) noexcept
{
    return markWorkspace(eAddr, astnodeAddr, contentsAddr, Kernel::NameMark::Buried, false);
}

/// TRACE: mark the named procedures, variables, and property lists as traced.
/// The input is a contents list. See markContents for the accepted forms.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param contentsAddr a word or a contents list
/// @return the owning ASTNode, or an Error if a name is not a word.
EXPORTC addr_t stdTrace(addr_t eAddr, addr_t astnodeAddr, addr_t contentsAddr) noexcept
{
    return markWorkspace(eAddr, astnodeAddr, contentsAddr, Kernel::NameMark::Traced, false);
}

namespace
{

QString traceArgToString(Datum *d, int printDepthLimit, int printWidthLimit)
{
    if (d->isWord())
    {
        Word *w = d->wordValue();
        w->numberValue();
        if (w->numberIsValid)
            return w->toString(Datum::ToStringFlags_None, printDepthLimit, printWidthLimit);
        w->boolValue();
        if (w->boolIsValid)
            return w->toString(Datum::ToStringFlags_None, printDepthLimit, printWidthLimit);
        Datum::ToStringFlags flags = Datum::ToStringFlags_Source;
        if (Kernel::get().specialVarIsTrue(SpecialVars::Id::FullPrintP))
            flags = static_cast<Datum::ToStringFlags>(flags | Datum::ToStringFlags_FullPrint);
        return w->toString(flags, printDepthLimit, printWidthLimit);
    }
    Datum::ToStringFlags flags = Datum::ToStringFlags_Show;
    if (Kernel::get().specialVarIsTrue(SpecialVars::Id::FullPrintP))
        flags = static_cast<Datum::ToStringFlags>(flags | Datum::ToStringFlags_FullPrint);
    return d->toString(flags, printDepthLimit, printWidthLimit);
}

bool isTracedOutputValue(Datum *d)
{
    return d != nullptr && (d->isa_ & Datum::typeDataMask) != 0;
}

} // namespace

/// Print a TRACE begin line: indent, then `( name input1 input2 ... )`.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the traced call's ASTNode (provides the name)
/// @param argsAddr an array of Datum pointers, the evaluated inputs
/// @param count the number of inputs
EXPORTC addr_t stdTraceBegin(addr_t eAddr, addr_t astnodeAddr, addr_t argsAddr, uint32_t count) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto **args = reinterpret_cast<Datum **>(argsAddr);
    const int printDepthLimit = Kernel::get().printLimitFromSpecialVar(SpecialVars::Id::PrintDepthLimit);
    const int printWidthLimit = Kernel::get().printLimitFromSpecialVar(SpecialVars::Id::PrintWidthLimit);

    QString output(Kernel::get().traceDepth(), QLatin1Char(' '));
    output = output % QLatin1String("( ") % node->nodeName_.toString();
    for (uint32_t i = 0; i < count; ++i)
        output = output % QLatin1Char(' ') % traceArgToString(args[i], printDepthLimit, printWidthLimit);
    output = output % QLatin1String(" )\n");

    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        StreamManager::get().print(output);
        Kernel::get().incrementTraceDepth();
    });
}

/// Print a TRACE exit line: indent, then `name outputs value` or `name stops`.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the traced call's ASTNode (provides the name)
/// @param resultAddr the output Datum, or null / non-data if the call stopped
EXPORTC addr_t stdTraceEnd(addr_t eAddr, addr_t astnodeAddr, addr_t resultAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *result = reinterpret_cast<Datum *>(resultAddr);
    Q_ASSERT(Kernel::get().traceDepth() > 0);
    Kernel::get().decrementTraceDepth();

    const int printDepthLimit = Kernel::get().printLimitFromSpecialVar(SpecialVars::Id::PrintDepthLimit);
    const int printWidthLimit = Kernel::get().printLimitFromSpecialVar(SpecialVars::Id::PrintWidthLimit);

    QString output(Kernel::get().traceDepth(), QLatin1Char(' '));
    output = output % node->nodeName_.toString();
    if (isTracedOutputValue(result))
        output = output % QObject::tr(" outputs ") % traceArgToString(result, printDepthLimit, printWidthLimit);
    else
        output = output % QObject::tr(" stops");
    output = output % QLatin1Char('\n');

    return printOrError(e, reinterpret_cast<addr_t>(node), output);
}

/// STEP: mark the named procedures, variables, and property lists as stepped.
/// The input is a contents list. See markContents for the accepted forms.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param contentsAddr a word or a contents list
/// @return the owning ASTNode, or an Error if a name is not a word.
EXPORTC addr_t stdStep(addr_t eAddr, addr_t astnodeAddr, addr_t contentsAddr) noexcept
{
    return markWorkspace(eAddr, astnodeAddr, contentsAddr, Kernel::NameMark::Stepped, false);
}

/// UNBURY: remove the buried mark from the named procedures, variables, and property lists.
/// A name that is not buried is ignored. The input is a contents list.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param contentsAddr a word or a contents list
/// @return the owning ASTNode, or an Error if a name is not a word.
EXPORTC addr_t stdUnbury(addr_t eAddr, addr_t astnodeAddr, addr_t contentsAddr) noexcept
{
    return markWorkspace(eAddr, astnodeAddr, contentsAddr, Kernel::NameMark::Buried, true);
}

/// UNTRACE: remove the traced mark from the named procedures, variables, and property lists.
/// A name that is not traced is ignored. The input is a contents list.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param contentsAddr a word or a contents list
/// @return the owning ASTNode, or an Error if a name is not a word.
EXPORTC addr_t stdUntrace(addr_t eAddr, addr_t astnodeAddr, addr_t contentsAddr) noexcept
{
    return markWorkspace(eAddr, astnodeAddr, contentsAddr, Kernel::NameMark::Traced, true);
}

/// UNSTEP: remove the stepped mark from the named procedures, variables, and property lists.
/// A name that is not stepped is ignored. The input is a contents list.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param contentsAddr a word or a contents list
/// @return the owning ASTNode, or an Error if a name is not a word.
EXPORTC addr_t stdUnstep(addr_t eAddr, addr_t astnodeAddr, addr_t contentsAddr) noexcept
{
    return markWorkspace(eAddr, astnodeAddr, contentsAddr, Kernel::NameMark::Stepped, true);
}

/// BURIED: output a contents list of buried procedures, variables, and property lists.
/// A buried name is included only while it exists: a user procedure, a variable
/// with a value, or a nonempty property list.
/// @param eAddr a pointer to the Evaluator object
/// @return a list of three lists, procedures then variables then property lists.
EXPORTC addr_t stdBuried(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr contents = contentsOfMark(Kernel::NameMark::Buried);
    e->watch(contents);
    return reinterpret_cast<addr_t>(contents.datumValue());
}

/// TRACED: output a contents list of traced procedures, variables, and property lists.
/// A traced name is included only while it exists: a user procedure, a variable
/// with a value, or a nonempty property list.
/// @param eAddr a pointer to the Evaluator object
/// @return a list of three lists, procedures then variables then property lists.
EXPORTC addr_t stdTraced(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr contents = contentsOfMark(Kernel::NameMark::Traced);
    e->watch(contents);
    return reinterpret_cast<addr_t>(contents.datumValue());
}

/// STEPPED: output a contents list of stepped procedures, variables, and property lists.
/// A stepped name is included only while it exists: a user procedure, a variable
/// with a value, or a nonempty property list.
/// @param eAddr a pointer to the Evaluator object
/// @return a list of three lists, procedures then variables then property lists.
EXPORTC addr_t stdStepped(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr contents = contentsOfMark(Kernel::NameMark::Stepped);
    e->watch(contents);
    return reinterpret_cast<addr_t>(contents.datumValue());
}

/// PROCEDURES: output the names of unburied user-defined procedures.
/// The result is a list of words, not a contents list. Primitives are omitted.
/// Buried procedures are omitted. Traced and stepped procedures stay.
/// @param eAddr a pointer to the Evaluator object
/// @return a list of procedure names.
EXPORTC addr_t stdProcedures(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr names = Procedures::get().allProcedureNames(true);
    e->watch(names);
    return reinterpret_cast<addr_t>(names.datumValue());
}

/// PRIMITIVES: output the names of all primitive procedures.
/// The result is a list of words, not a contents list. User-defined procedures
/// are omitted. Aliases are included. A name added with COPYDEF of a primitive
/// is included. Burying a primitive does not remove it.
/// @param eAddr a pointer to the Evaluator object
/// @return a list of primitive names.
EXPORTC addr_t stdPrimitives(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr names = Procedures::get().allPrimitiveProcedureNames();
    e->watch(names);
    return reinterpret_cast<addr_t>(names.datumValue());
}

/// NAMES: output a contents list of unburied variable names.
/// The first sublist is empty (no procedures). The second is the variable names.
/// A variable with no value is included. Procedures and property lists are not.
/// A buried variable is omitted until UNBURY. Traced and stepped variables stay.
/// @param eAddr a pointer to the Evaluator object
/// @return a list of two lists.
EXPORTC addr_t stdNames(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    ListBuilder contents;
    contents.append(emptyList());
    contents.append(Kernel::get().allVariables(true));
    DatumPtr names = contents.finishedList();
    e->watch(names);
    return reinterpret_cast<addr_t>(names.datumValue());
}

/// PLISTS: output a contents list of unburied property-list names.
/// The first two sublists are empty (no procedures, no variables). The third
/// is the property-list names. A property list is included only while it has
/// a property. A buried property list is omitted until UNBURY. Traced and
/// stepped property lists stay.
/// @param eAddr a pointer to the Evaluator object
/// @return a list of three lists.
EXPORTC addr_t stdPlists(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr names = Kernel::get().unburiedContents(false, false, true);
    e->watch(names);
    return reinterpret_cast<addr_t>(names.datumValue());
}

/// CONTENTS: output a contents list of every unburied named item.
/// The three sublists are user procedures, variables, and property lists,
/// the same names PROCEDURES, NAMES, and PLISTS report. A buried name is
/// omitted until UNBURY. Traced and stepped names stay. A variable with no
/// value is included. A property list is included only while it has a property.
/// @param eAddr a pointer to the Evaluator object
/// @return a list of three lists.
EXPORTC addr_t stdContents(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr names = Kernel::get().unburiedContents(true, true, true);
    e->watch(names);
    return reinterpret_cast<addr_t>(names.datumValue());
}

/// NODES: output the current and high-water Datum allocation counts.
/// The first number is live objects. The second is the most that have been
/// live at once since the previous NODES, or since startup if this is the
/// first call. After the snapshot, the high-water mark is reset to the
/// first number. The result list is not included in the snapshot.
/// @param eAddr a pointer to the Evaluator object
/// @return a list of two integers.
EXPORTC addr_t stdNodes(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    const int current = Datum::countOfNodes();
    const int peak = Datum::maxCountOfNodes();
    Datum::resetMaxCountOfNodes();
    ListBuilder builder;
    builder.append(DatumPtr(current));
    builder.append(DatumPtr(peak));
    DatumPtr result = builder.finishedList();
    e->watch(result);
    return reinterpret_cast<addr_t>(result.datumValue());
}

/// BURIEDP: TRUE if the first named procedure, variable, or property list is buried.
/// Only that first name is tested. The compiler rejects an input that names nothing.
/// @param contentsAddr a word or a named contents list
/// @return true if that first name is buried.
EXPORTC bool stdBuriedp(addr_t contentsAddr) noexcept
{
    return queryWorkspace(contentsAddr, Kernel::NameMark::Buried);
}

/// TRACEDP: TRUE if the first named procedure, variable, or property list is traced.
/// Only that first name is tested. The compiler rejects an input that names nothing.
/// @param contentsAddr a word or a named contents list
/// @return true if that first name is traced.
EXPORTC bool stdTracedp(addr_t contentsAddr) noexcept
{
    return queryWorkspace(contentsAddr, Kernel::NameMark::Traced);
}

/// STEPPEDP: TRUE if the first named procedure, variable, or property list is stepped.
/// Only that first name is tested. The compiler rejects an input that names nothing.
/// @param contentsAddr a word or a named contents list
/// @return true if that first name is stepped.
EXPORTC bool stdSteppedp(addr_t contentsAddr) noexcept
{
    return queryWorkspace(contentsAddr, Kernel::NameMark::Stepped);
}

/// ARITY: output the minimum, default, and maximum input counts of a procedure.
/// The name may be a primitive or a user procedure, including a macro.
/// An unknown name is an error. The compiler rejects a non-word before this runs.
/// @param eAddr a pointer to the Evaluator object
/// @param nameAddr a word naming the procedure
/// @return a list of three integers, or an Error if the name is not a procedure.
EXPORTC addr_t stdArity(addr_t eAddr, addr_t nameAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *name = reinterpret_cast<Datum *>(nameAddr);
    return catchError(e, [&]() -> addr_t {
        DatumPtr arity = Procedures::get().arity(DatumPtr(name));
        e->watch(arity);
        return reinterpret_cast<addr_t>(arity.datumValue());
    });
}

namespace
{

/// Parse a PRINTOUT/ERASE contents-list input, then run @p fn on the three lists.
/// A rejected input becomes the Error return. Success returns the AST node.
/// The compiler rejects a non-contents-list before this runs.
template <typename Fn>
addr_t withContentsList(addr_t eAddr, addr_t astnodeAddr, addr_t contentsAddr, Fn &&fn)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *contents = reinterpret_cast<Datum *>(contentsAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        fn(Kernel::get().contentsListFromDatum(node->nodeName_, DatumPtr(contents)));
    });
}

} // namespace

/// PRINTOUT / PO: print the definitions named in a contents list to the write stream.
/// A word is one procedure. A list of words is procedure names. A list of lists
/// is procedures, then variables, then property lists. A primitive or unknown
/// procedure, or a variable with no value, is an error. A missing property list
/// prints nothing.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param contentsAddr a word or a contents list
/// @return the owning ASTNode, or an Error if the input is rejected.
EXPORTC addr_t stdPrintout(addr_t eAddr, addr_t astnodeAddr, addr_t contentsAddr) noexcept
{
    return withContentsList(eAddr, astnodeAddr, contentsAddr, [](const DatumPtr &contentslist) {
        StreamManager::get().print(Kernel::get().printoutFromContentsList(contentslist));
    });
}

/// POT: print title lines of named procedures, MAKE of variables, and one Plist
/// line per nonempty property list. A word is one procedure. A list of words is
/// procedure names. A list of lists is procedures, then variables, then property
/// lists. A primitive or unknown procedure, or a variable with no value, is an
/// error. A missing property list prints nothing.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param contentsAddr a word or a contents list
/// @return the owning ASTNode, or an Error if the input is rejected.
EXPORTC addr_t stdPot(addr_t eAddr, addr_t astnodeAddr, addr_t contentsAddr) noexcept
{
    return withContentsList(eAddr, astnodeAddr, contentsAddr, [](const DatumPtr &contentslist) {
        StreamManager::get().print(Kernel::get().titlesFromContentsList(contentslist));
    });
}

/// LOAD: open a file as the commander and execute it.
/// TO in the file may replace a user procedure. STARTUP is run if this file
/// assigned it a list. Failure to open the file is an error. Errors inside
/// the file are printed and stop the rest of the file.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param filenameAddr a word naming the file
/// @return the owning ASTNode, or an Error if the file cannot be opened.
EXPORTC addr_t stdLoad(addr_t eAddr, addr_t astnodeAddr, addr_t filenameAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *filename = reinterpret_cast<Datum *>(filenameAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        DatumPtr file(filename);
        Kernel::get().loadFile(file);
        Kernel::get().rememberWorkspaceFile(file);
    });
}

/// SAVE: write unburied procedures, variables, and nonempty property lists to
/// a file, as PRINTOUT of CONTENTS. Does not change WRITER. PREFIX applies.
/// A name already in ALLOPEN is already-open. No input uses the most recent
/// LOAD or SAVE filename; none yet is not-enough-inputs.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param filenameAddr a word naming the file, or 0 to reuse the last LOAD/SAVE name
/// @return the owning ASTNode, or an Error if the file cannot be written.
EXPORTC addr_t stdSave(addr_t eAddr, addr_t astnodeAddr, addr_t filenameAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        DatumPtr filename;
        if (filenameAddr == 0)
        {
            filename = Kernel::get().lastWorkspaceFile();
            if (filename.isNothing())
                throw FCError::notEnoughInputs(node->nodeName_);
        }
        else
        {
            filename = DatumPtr(reinterpret_cast<Datum *>(filenameAddr));
        }
        Kernel::get().saveWorkspace(filename);
    });
}

/// HELP: print help text for a name, or list every documented name.
/// With no input, prints a SHOW-style list of aliases. With one word, looks
/// up that name. A missing name prints a sorry message rather than an error.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param nameAddr a word naming the topic, or 0 if HELP was given no input
/// @return the owning ASTNode, or an Error if printing fails.
EXPORTC addr_t stdHelp(addr_t eAddr, addr_t astnodeAddr, addr_t nameAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        QString output;
        if (nameAddr == 0)
        {
            QStringList names = Help::get().allCategories();
            output = QObject::tr("Help is available on the following:");
            for (const QString &category : names)
            {
                QStringList commands = Help::get().commandsForCategory(category);
                output += QObject::tr("\n\n%1:\n").arg(category);
                for (const QString &command : commands)
                    output += QObject::tr("  %1").arg(command);
            }
            output += QObject::tr("\n");
        }
        else
        {
            DatumPtr nameP(reinterpret_cast<Datum *>(nameAddr));
            const QString key = nameP.toString(Datum::ToStringFlags_Key);
            const QString text = Help::get().helpText(key);
            if (text.isEmpty())
            {
                output = QObject::tr("Sorry, no help available on %1\n")
                             .arg(nameP.toString(Datum::ToStringFlags_Show));
            }
            else
            {
                output = text;
                if (!output.endsWith(QLatin1Char('\n')))
                    output += QLatin1Char('\n');
            }
        }
        StreamManager::get().print(output);
    });
}

/// ERASE / ER: erase the procedures, variables, and property lists named in a
/// contents list. A word is one procedure. A list of words is procedure names.
/// A list of lists is procedures, then variables, then property lists. A missing
/// name is ignored. A primitive procedure is an error.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param contentsAddr a word or a contents list
/// @return the owning ASTNode, or an Error if the input is rejected.
EXPORTC addr_t stdErase(addr_t eAddr, addr_t astnodeAddr, addr_t contentsAddr) noexcept
{
    return withContentsList(eAddr, astnodeAddr, contentsAddr, [](const DatumPtr &contentslist) {
        Kernel::get().eraseFromContentsList(contentslist);
    });
}

/// ERALL / ERPS / ERNS / ERPLS: erase unburied procedures, variables, and/or
/// property lists. Buried names stay. The three flags select which kinds
/// CONTENTS would report. Special variables that CONTENTS still lists are
/// erased when variables are selected.
EXPORTC void stdEraseUnburied(bool procedures, bool variables, bool plists) noexcept
{
    Kernel::get().eraseUnburied(procedures, variables, plists);
}

/// EDIT / ED: write the named workspace items to a temp file, open the editor,
/// then LOAD. No input means unburied procedures. A cancelled Psychi edit
/// leaves the workspace unchanged. The compiler rejects a non-contents-list.
EXPORTC addr_t stdEdit(addr_t eAddr, addr_t astnodeAddr, addr_t contentsAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        DatumPtr contentslist = nothing();
        if (contentsAddr != 0)
        {
            contentslist = Kernel::get().contentsListFromDatum(
                node->nodeName_, DatumPtr(reinterpret_cast<Datum *>(contentsAddr)));
        }
        Kernel::get().editContents(contentslist);
    });
}

/// EDITFILE: open the named file in the editor, then LOAD it. A missing file
/// is created empty. The file is not deleted. A cancelled Psychi edit leaves
/// the file and workspace unchanged.
EXPORTC addr_t stdEditfile(addr_t eAddr, addr_t astnodeAddr, addr_t filenameAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *filename = reinterpret_cast<Datum *>(filenameAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        Kernel::get().editFile(DatumPtr(filename));
    });
}

/// SETEDITOR: store the editor command (a word, or a list of words).
EXPORTC void stdSeteditor(addr_t commandAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(commandAddr);
    Kernel::get().setEditor(DatumPtr(d));
}

/// EDITOR: return the current SETEDITOR command.
EXPORTC addr_t stdEditor(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr retval = Kernel::get().editor();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

/// SETTEMPLOC: store the EDIT temp directory (a word, or the empty list for
/// the system temp directory).
EXPORTC void stdSettemploc(addr_t pathAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(pathAddr);
    Kernel::get().setTempLoc(DatumPtr(d));
}

/// TEMPLOC: return the directory EDIT uses for its temporary file.
EXPORTC addr_t stdTemploc(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr retval = Kernel::get().tempLoc();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

