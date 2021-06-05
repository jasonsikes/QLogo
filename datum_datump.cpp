
//===-- qlogo/datum_datumP.cpp - DatumP class implementation -------*-
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
/// This file contains the implementation of the DatumP class, which is simply
/// a pointer to a Datum. The DatumP class automatically maintains retain counts
/// to the referred Datum.
///
//===----------------------------------------------------------------------===//

#include "datum.h"
#include <qdebug.h>

Word trueWord("true", false, false);
Word falseWord("false", false, false);


DatumP::DatumP() { d = &notADatum; }

DatumP::DatumP(Datum *other) {
  d = other;
  if (d) {
    d->retain();
  }
}

DatumP::DatumP(const DatumP &other) noexcept {
  d = other.d;
  if (d) {
    d->retain();
  }
}

DatumP::DatumP(bool b) { d = b ? &trueWord : &falseWord; }

void DatumP::destroy() {
  if (d != &notADatum) {
    d->release();
    if (d->shouldDelete()) {
      delete d;
    }
  }
}

DatumP::~DatumP() { destroy(); }

DatumP &DatumP::operator=(const DatumP &other) noexcept {
  if (&other != this) {
    destroy();
    d = other.d;
    if (d) {
      d->retain();
    }
  }
  return *this;
}

DatumP &DatumP::operator=(DatumP *other) noexcept {
  if (other != this) {
    destroy();
    d = other->d;
    d->retain();
  }
  return *this;
}

bool DatumP::operator==(DatumP *other) { return d == other->d; }

bool DatumP::operator==(const DatumP &other) { return d == other.d; }

bool DatumP::operator!=(DatumP *other) { return d != other->d; }

bool DatumP::operator!=(const DatumP &other) { return d != other.d; }

// This is true IFF EQUALP is true
bool DatumP::isEqual(DatumP other, bool ignoreCase) {
  if (d->isa() != other.isa())
    return false;
  if (d == other.d)
    return true;
  return d->isEqual(other, ignoreCase);
}

bool DatumP::isDotEqual(DatumP other) { return (d == other.d); }

bool DatumP::isASTNode() { return d->isa() == Datum::astnodeType; }

bool DatumP::isList() { return d->isa() == Datum::listType; }

bool DatumP::isArray() { return d->isa() == Datum::arrayType; }

bool DatumP::isWord() { return d->isa() == Datum::wordType; }

bool DatumP::isError() { return d->isa() == Datum::errorType; }

Word *DatumP::wordValue() {
  Q_ASSERT(d->isa() == Datum::wordType);
  return (Word *)d;
}

List *DatumP::listValue() {
  if (d->isa() != Datum::listType) {
    qDebug() << "Hello";
  }
  Q_ASSERT(d->isa() == Datum::listType);
  return (List *)d;
}

ListNode *DatumP::listNodeValue() {
  if (d->isa() != Datum::listNodeType) {
    qDebug() << "Hello";
  }
  Q_ASSERT(d->isa() == Datum::listNodeType);
  return (ListNode *)d;
}

Array *DatumP::arrayValue() {
  Q_ASSERT(d->isa() == Datum::arrayType);
  return (Array *)d;
}

Procedure *DatumP::procedureValue() {
  if (d->isa() != Datum::procedureType) {
    qDebug() << "Hello";
  }
  Q_ASSERT(d->isa() == Datum::procedureType);
  return (Procedure *)d;
}

ASTNode *DatumP::astnodeValue() {
  if (d->isa() != Datum::astnodeType) {
    qDebug() << "Error here";
  }
  return (ASTNode *)d;
}

Error *DatumP::errorValue() {
  Q_ASSERT(d->isa() == Datum::errorType);
  return (Error *)d;
}

Datum::DatumType DatumP::isa() { return d->isa(); }

QString DatumP::printValue(bool fullPrintp, int printDepthLimit,
                           int printWidthLimit) {
  return d->printValue(fullPrintp, printDepthLimit, printWidthLimit);
}

QString DatumP::showValue(bool fullPrintp, int printDepthLimit,
                          int printWidthLimit) {
  return d->showValue(fullPrintp, printDepthLimit, printWidthLimit);
}
