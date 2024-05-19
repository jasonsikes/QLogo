
//===-- qlogo/datum_DatumPtr.cpp - DatumPtr class implementation -------*-
// C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// QLogo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with QLogo.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the DatumPtr class, which is simply
/// a pointer to a Datum. The DatumPtr class automatically maintains retain counts
/// to the referred Datum.
///
//===----------------------------------------------------------------------===//

#include "datum/datump.h"
#include "datum/word.h"
#include "datum/list.h"
#include <qdebug.h>
#include <QObject>

DatumPtr::DatumPtr() { d = &notADatum; }

DatumPtr::DatumPtr(Datum *other) {
  d = other;
  if (d) {
    d->retain();
  }
}

DatumPtr::DatumPtr(const DatumPtr &other) noexcept {
  d = other.d;
  if (d) {
    d->retain();
  }
}

DatumPtr::DatumPtr(bool b) {
  d = new Word(b ? QObject::tr("true") : QObject::tr("false"));
  d -> retain();
}


DatumPtr::DatumPtr(double n)
{
  d = new Word(n);
  d->retain();
}


DatumPtr::DatumPtr(int n)
{
  d = new Word((double)n);
  d->retain();
}


DatumPtr::DatumPtr(QString n, bool isVBarred)
{
  d = new Word(n, isVBarred);
  d->retain();
}

DatumPtr::DatumPtr(const char* n)
{
  d = new Word(QString(n));
  d->retain();
}

void DatumPtr::destroy() {
  if (d != &notADatum) {
    d->release();
  }
}

DatumPtr::~DatumPtr() { destroy(); }

DatumPtr &DatumPtr::operator=(const DatumPtr &other) noexcept {
  if (&other != this) {
    destroy();
    d = other.d;
    if (d) {
      d->retain();
    }
  }
  return *this;
}

DatumPtr &DatumPtr::operator=(DatumPtr *other) noexcept {
  if (other != this) {
    destroy();
    d = other->d;
    d->retain();
  }
  return *this;
}

bool DatumPtr::operator==(DatumPtr *other) { return d == other->d; }

bool DatumPtr::operator==(const DatumPtr &other) { return d == other.d; }

bool DatumPtr::operator!=(DatumPtr *other) { return d != other->d; }

bool DatumPtr::operator!=(const DatumPtr &other) { return d != other.d; }

// This is true IFF EQUALP is true
bool DatumPtr::isEqual(DatumPtr other, bool ignoreCase) {
  if (d->isa() != other.isa())
    return false;
  if (d == other.d)
    return true;
  return d->isEqual(other, ignoreCase);
}

bool DatumPtr::isDotEqual(DatumPtr other) { return (d == other.d); }

bool DatumPtr::isASTNode() { return d->isa() == Datum::astnodeType; }

bool DatumPtr::isList() { return d->isa() == Datum::listType; }

bool DatumPtr::isArray() { return d->isa() == Datum::arrayType; }

bool DatumPtr::isWord() { return d->isa() == Datum::wordType; }

bool DatumPtr::isError() { return d->isa() == Datum::errorType; }

bool DatumPtr::isNothing() { return d == &notADatum; }

Word *DatumPtr::wordValue() {
  Q_ASSERT(d->isa() == Datum::wordType);
  return (Word *)d;
}

List *DatumPtr::listValue() {
  if (d->isa() != Datum::listType) {
    qDebug() << "Hello";
  }
  Q_ASSERT(d->isa() == Datum::listType);
  return (List *)d;
}

ListNode *DatumPtr::listNodeValue() {
  if (d->isa() != Datum::listNodeType) {
    qDebug() << "Hello";
  }
  Q_ASSERT(d->isa() == Datum::listNodeType);
  return (ListNode *)d;
}

Array *DatumPtr::arrayValue() {
  Q_ASSERT(d->isa() == Datum::arrayType);
  return (Array *)d;
}

Procedure *DatumPtr::procedureValue() {
  if (d->isa() != Datum::procedureType) {
    qDebug() << "Hello";
  }
  Q_ASSERT(d->isa() == Datum::procedureType);
  return (Procedure *)d;
}

ASTNode *DatumPtr::astnodeValue() {
  if (d->isa() != Datum::astnodeType) {
    qDebug() << "Error here";
  }
  return (ASTNode *)d;
}

Error *DatumPtr::errorValue() {
  Q_ASSERT(d->isa() == Datum::errorType);
  return (Error *)d;
}

Datum::DatumType DatumPtr::isa() { return d->isa(); }

QString DatumPtr::printValue(bool fullPrintp, int printDepthLimit,
                           int printWidthLimit) {
  return d->printValue(fullPrintp, printDepthLimit, printWidthLimit);
}

QString DatumPtr::showValue(bool fullPrintp, int printDepthLimit,
                          int printWidthLimit) {
  return d->showValue(fullPrintp, printDepthLimit, printWidthLimit);
}
