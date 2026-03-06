
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

#include "astnode.h"
#include "datum_types.h"
#include "workspace/procedures.h"
#include <QObject>
#include <qdebug.h>

bool isNotPersistent(Datum *d)
{
    return (d != nullptr) && ((d->isa_ & Datum::typePersistentMask) == 0);
}

DatumPtr::DatumPtr() : datumValue_(Datum::notADatum())
{
}

DatumPtr::DatumPtr(Datum *other) noexcept
{
    datumValue_ = other;
    if (isNotPersistent(datumValue_))
    {
        ++(datumValue_->retainCount_);
    }
}

DatumPtr::DatumPtr(const DatumPtr &other) noexcept
{
    datumValue_ = other.datumValue_;
    if (isNotPersistent(datumValue_))
    {
        ++(datumValue_->retainCount_);
    }
}

DatumPtr::DatumPtr(bool b)
{
    datumValue_ = new Word(b ? QObject::tr("true") : QObject::tr("false"));
    ++(datumValue_->retainCount_);
}

DatumPtr::DatumPtr(double n)
{
    datumValue_ = new Word(n);
    ++(datumValue_->retainCount_);
}

DatumPtr::DatumPtr(int n)
{
    datumValue_ = new Word((double)n);
    ++(datumValue_->retainCount_);
}

DatumPtr::DatumPtr(const QString &n, bool isVBarred)
{
    datumValue_ = new Word(n, isVBarred);
    ++(datumValue_->retainCount_);
}

DatumPtr::DatumPtr(const char *n)
{
    datumValue_ = new Word(QString(n));
    ++(datumValue_->retainCount_);
}

void DatumPtr::destroy()
{
    if (isNotPersistent(datumValue_))
    {
        --(datumValue_->retainCount_);
        if (datumValue_->retainCount_ <= 0)
        {
            if (datumValue_->alertOnDelete_)
            {
                qInfo() << "DELETING: " << datumValue_ << " " << datumValue_->toString(Datum::ToStringFlags_Show);
            }
            delete datumValue_;
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
        datumValue_ = other.datumValue_;
        if (isNotPersistent(datumValue_))
        {
            ++(datumValue_->retainCount_);
        }
    }
    return *this;
}

bool DatumPtr::operator==(const DatumPtr &other) const
{
    return datumValue_ == other.datumValue_;
}

bool DatumPtr::operator!=(const DatumPtr &other) const
{
    return datumValue_ != other.datumValue_;
}

Word *DatumPtr::wordValue() const
{
    Q_ASSERT(datumValue_->isa_ == Datum::typeWord);
    return reinterpret_cast<Word *>(datumValue_);
}

List *DatumPtr::listValue() const
{
    Q_ASSERT(datumValue_ && (datumValue_->isa_ & Datum::typeList) != 0);
    return reinterpret_cast<List *>(datumValue_);
}

Array *DatumPtr::arrayValue() const
{
    Q_ASSERT(datumValue_->isa_ == Datum::typeArray);
    return reinterpret_cast<Array *>(datumValue_);
}

FlowControl *DatumPtr::flowControlValue() const
{
    Q_ASSERT((datumValue_->isa_ & Datum::typeFlowControlMask) != 0);
    return reinterpret_cast<FlowControl *>(datumValue_);
}

Procedure *DatumPtr::procedureValue() const
{
    Q_ASSERT(datumValue_->isa_ == Datum::typeProcedure);
    return reinterpret_cast<Procedure *>(datumValue_);
}

ASTNode *DatumPtr::astnodeValue() const
{
    Q_ASSERT(datumValue_->isa_ == Datum::typeASTNode);
    return reinterpret_cast<ASTNode *>(datumValue_);
}

FCError *DatumPtr::errValue() const
{
    Q_ASSERT(datumValue_->isa_ == Datum::typeError);
    return reinterpret_cast<FCError *>(datumValue_);
}

QString DatumPtr::toString(Datum::ToStringFlags flags,
                           int printDepthLimit,
                           int printWidthLimit,
                           VisitedSet *visited) const
{
    return datumValue_->toString(flags, printDepthLimit, printWidthLimit, visited);
}

// Value to represent nothing (similar to nullptr)
// Use function-local static to avoid exceptions during global static initialization
const DatumPtr &nothing()
{
    static const DatumPtr instance(Datum::notADatum());
    return instance;
}
