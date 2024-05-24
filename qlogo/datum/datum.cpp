
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

#include "datum/datump.h"
#include "datum/list.h"
#include <qdebug.h>
#include <unistd.h>
#include <QObject>

// TODO: is this necessary?
int countOfNodes = 0;
int maxCountOfNodes = 0;

DatumPtr nodes()
{
  int a = countOfNodes;
  int b = maxCountOfNodes;

  maxCountOfNodes = countOfNodes;

  List *retval = new List();
  retval->append(DatumPtr(a));
  retval->append(DatumPtr(b));
  return DatumPtr(retval);
}


Datum::Datum() : retainCount(0)
{
  ++countOfNodes;
  if (countOfNodes > maxCountOfNodes)
    maxCountOfNodes = countOfNodes;
}

Datum::~Datum()
{
  --countOfNodes;
}


QString Datum::printValue(bool, int, int) { return QObject::tr("nothing"); }

QString Datum::showValue(bool, int, int) { return printValue(); }

bool Datum::isEqual(DatumPtr other, bool) {
  return (other.isa() == Datum::noType);
}

DatumPtr Datum::first() {
  Q_ASSERT(false);
  return nothing;
}

DatumPtr Datum::last() {
  Q_ASSERT(false);
  return nothing;
}

DatumPtr Datum::butfirst() {
  Q_ASSERT(false);
  return nothing;
}

void Datum::setButfirstItem(DatumPtr) { Q_ASSERT(false); }

bool Datum::containsDatum(DatumPtr, bool) {
  Q_ASSERT(false);
  return false;
}

bool Datum::isMember(DatumPtr, bool) {
  Q_ASSERT(false);
  return false;
}

DatumPtr Datum::fromMember(DatumPtr, bool) {
  Q_ASSERT(false);
  return nothing;
}

DatumPtr Datum::datumAtIndex(int) {
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
DatumPtr nothing(&notADatum);
