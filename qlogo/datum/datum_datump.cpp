
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

#include "datum.h"
#include <qdebug.h>
#include <QObject>

DatumPtr::DatumPtr() : d(&notADatum) {}

DatumPtr::DatumPtr(Datum *other) {
  d = other;
  if (d) {
    d->retainCount++;
  }
}

DatumPtr::DatumPtr(const DatumPtr &other) noexcept {
  d = other.d;
  if (d) {
      d->retainCount++;
  }
}

DatumPtr::DatumPtr(bool b) {
  d = new Word(b ? QObject::tr("true") : QObject::tr("false"));
    d->retainCount++;
}


DatumPtr::DatumPtr(double n)
{
  d = new Word(n);
    d->retainCount++;
}


DatumPtr::DatumPtr(int n)
{
  d = new Word((double)n);
    d->retainCount++;
}


DatumPtr::DatumPtr(QString n, bool isVBarred)
{
  d = new Word(n, isVBarred);
    d->retainCount++;
}

DatumPtr::DatumPtr(const char* n)
{
  d = new Word(QString(n));
    d->retainCount++;
}

void DatumPtr::destroy() {
  if (d != &notADatum) {
        d->retainCount--;
      if (d->retainCount <= 0) {
            if (d->alertOnDelete) {
              qDebug() <<"DELETING: " <<d <<" " <<d->showValue();
          }
            delete d;
      }
  }
}

DatumPtr::~DatumPtr() { destroy(); }

DatumPtr &DatumPtr::operator=(const DatumPtr &other) noexcept {
  if (&other != this) {
    destroy();
    d = other.d;
    if (d) {
        d->retainCount++;
    }
  }
  return *this;
}

DatumPtr &DatumPtr::operator=(DatumPtr *other) noexcept {
  if (other != this) {
    destroy();
    d = other->d;
    d->retainCount++;
  }
  return *this;
}

bool DatumPtr::operator==(DatumPtr *other) { return d == other->d; }

bool DatumPtr::operator==(const DatumPtr &other) { return d == other.d; }

bool DatumPtr::operator!=(DatumPtr *other) { return d != other->d; }

bool DatumPtr::operator!=(const DatumPtr &other) { return d != other.d; }

bool DatumPtr::isASTNode() { return d->isa() == Datum::astnodeType; }

bool DatumPtr::isList() { return d->isa() == Datum::listType; }

bool DatumPtr::isArray() { return d->isa() == Datum::arrayType; }

bool DatumPtr::isWord() { return d->isa() == Datum::wordType; }

bool DatumPtr::isError() { return d->isa() == Datum::errorType; }

bool DatumPtr::isNothing() { return d->isa() == Datum::noType; }

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
