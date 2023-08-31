
//===-- qlogo/datum.cpp - Datum class and subclasses implementation -------*-
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
/// This file contains the implementation of the Datum class, which is the superclass
/// of Word, List, and Array.
///
//===----------------------------------------------------------------------===//

#include "datum.h"
#include <qdebug.h>

int countOfNodes = 0;
int maxCountOfNodes = 0;

DatumP nodes()
{
  int a = countOfNodes;
  int b = maxCountOfNodes;

  maxCountOfNodes = countOfNodes;

  List *retval = new List;
  retval->append(DatumP(new Word(a)));
  retval->append(DatumP(new Word(b)));
  return DatumP(retval);
}


Datum::Datum()
{
  retainCount = 0;
  ++countOfNodes;
  if (countOfNodes > maxCountOfNodes)
    maxCountOfNodes = countOfNodes;
}

Datum::~Datum()
{
  --countOfNodes;
}

QString Datum::printValue(bool, int, int) { return name(); }

QString Datum::name(void) {
  static QString retval("nothing");
  return retval;
}

QString Datum::showValue(bool, int, int) { return name(); }

bool Datum::isEqual(DatumP other, bool) {
  return (other.isa() == Datum::noType);
}

DatumP Datum::first() {
  Q_ASSERT(false);
  return nothing;
}

DatumP Datum::last() {
  Q_ASSERT(false);
  return nothing;
}

DatumP Datum::butlast() {
  Q_ASSERT(false);
  return nothing;
}

DatumP Datum::butfirst() {
  Q_ASSERT(false);
  return nothing;
}

void Datum::setItem(int, DatumP) { Q_ASSERT(false); }

void Datum::setButfirstItem(DatumP) { Q_ASSERT(false); }

void Datum::setFirstItem(DatumP) { Q_ASSERT(false); }

bool Datum::containsDatum(DatumP, bool) {
  Q_ASSERT(false);
  return false;
}

bool Datum::isMember(DatumP, bool) {
  Q_ASSERT(false);
  return false;
}

DatumP Datum::fromMember(DatumP, bool) {
  Q_ASSERT(false);
  return nothing;
}

DatumP Datum::datumAtIndex(int) {
  Q_ASSERT(false);
  return nothing;
}

bool Datum::isIndexInRange(int) {
  Q_ASSERT(false);
  return false;
}

Datum &Datum::operator=(const Datum &) {
  Q_ASSERT(false);
  return *this;
}

Datum::DatumType Datum::isa() { return noType; }

int Datum::size() {
  Q_ASSERT(false);
  return 0;
}



// Values to represent no data (NULL)
Datum notADatum;
DatumP nothing(&notADatum);
