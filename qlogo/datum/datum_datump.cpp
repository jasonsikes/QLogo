
//===-- qlogo/datum_DatumPtr.cpp - DatumPtr class implementation --*- C++ -*-===//
//
// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the DatumPtr class, which is simply
/// a pointer to a Datum. The DatumPtr class automatically maintains retain counts
/// to the referred Datum.
///
//===----------------------------------------------------------------------===//

#include "datum.h"
#include "workspace/procedures.h"
#include "astnode.h"
#include <QObject>
#include <qdebug.h>

bool isNotPersistent(Datum *d)
{
    return (d != nullptr) && ((d->isa & Datum::typePersistentMask) == 0);
}

DatumPtr::DatumPtr() : d(Datum::getInstance())
{
}

DatumPtr::DatumPtr(Datum *other)
{
    d = other;
    if (isNotPersistent(d))
    {
        ++(d->retainCount);
    }
}

DatumPtr::DatumPtr(const DatumPtr &other) noexcept
{
    d = other.d;
    if (isNotPersistent(d))
    {
        ++(d->retainCount);
    }
}

DatumPtr::DatumPtr(bool b)
{
    d = new Word(b ? QObject::tr("true") : QObject::tr("false"));
    ++(d->retainCount);
}

DatumPtr::DatumPtr(double n)
{
    d = new Word(n);
    ++(d->retainCount);
}

DatumPtr::DatumPtr(int n)
{
    d = new Word((double)n);
    ++(d->retainCount);
}

DatumPtr::DatumPtr(QString n, bool isVBarred)
{
    d = new Word(n, isVBarred);
    ++(d->retainCount);
}

DatumPtr::DatumPtr(const char *n)
{
    d = new Word(QString(n));
    ++(d->retainCount);
}

void DatumPtr::destroy()
{
    if (isNotPersistent(d))
    {
        --(d->retainCount);
        if (d->retainCount <= 0)
        {
            if (d->alertOnDelete)
            {
                qDebug() << "DELETING: " << d << " " << d->showValue();
            }
            delete d;
        }
    }
}

DatumPtr::~DatumPtr()
{
    destroy();
}

DatumPtr &DatumPtr::operator=(const DatumPtr &other) noexcept
{
    if (&other != this)
    {
        destroy();
        d = other.d;
        if (isNotPersistent(d))
        {
            ++(d->retainCount);
        }
    }
    return *this;
}

DatumPtr &DatumPtr::operator=(DatumPtr *other) noexcept
{
    if (other != this)
    {
        destroy();
        d = other->d;
        if (isNotPersistent(d))
        {
            ++(d->retainCount);
        }
    }
    return *this;
}

bool DatumPtr::operator==(DatumPtr *other)
{
    return d == other->d;
}

bool DatumPtr::operator==(const DatumPtr &other)
{
    return d == other.d;
}

bool DatumPtr::operator!=(DatumPtr *other)
{
    return d != other->d;
}

bool DatumPtr::operator!=(const DatumPtr &other)
{
    return d != other.d;
}

Word *DatumPtr::wordValue()
{
    Q_ASSERT(d->isa == Datum::typeWord);
    return static_cast<Word *>(d);
}

List *DatumPtr::listValue()
{
    Q_ASSERT(d && (d->isa & Datum::typeList) != 0);
    return static_cast<List *>(d);
}

Array *DatumPtr::arrayValue()
{
    Q_ASSERT(d->isa == Datum::typeArray);
    return static_cast<Array *>(d);
}

FlowControl *DatumPtr::flowControlValue()
{
    Q_ASSERT((d->isa & Datum::typeFlowControlMask) != 0);
    return reinterpret_cast<FlowControl *>(d);
}

Procedure *DatumPtr::procedureValue()
{
    Q_ASSERT(d->isa == Datum::typeProcedure);
    return static_cast<Procedure *>(d);
}

ASTNode *DatumPtr::astnodeValue()
{
    Q_ASSERT(d->isa == Datum::typeASTNode);
    return static_cast<ASTNode *>(d);
}

FCError *DatumPtr::errValue()
{
    Q_ASSERT(d->isa == Datum::typeError);
    return reinterpret_cast<FCError *>(d);
}

QString DatumPtr::printValue(bool fullPrintp, int printDepthLimit, int printWidthLimit)
{
    return d->printValue(fullPrintp, printDepthLimit, printWidthLimit);
}

QString DatumPtr::showValue(bool fullPrintp, int printDepthLimit, int printWidthLimit)
{
    return d->showValue(fullPrintp, printDepthLimit, printWidthLimit);
}
