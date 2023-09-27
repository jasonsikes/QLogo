
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

#include "datum_datump.h"
#include "datum_list.h"
#include "stringconstants.h"
#include <qdebug.h>
#include <unistd.h>

// TODO: is this necessary?
int countOfNodes = 0;
int maxCountOfNodes = 0;

DatumP nodes()
{
  int a = countOfNodes;
  int b = maxCountOfNodes;

  maxCountOfNodes = countOfNodes;

  List *retval = List::alloc();
  retval->append(DatumP(a));
  retval->append(DatumP(b));
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


void Datum::addToPool()
{
  // Base class does nothing. Subclasses should add themselves to their pools.
}

QString Datum::printValue(bool, int, int) { return name(); }

QString Datum::name(void) {
  return k.nothing();
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






Datum * DatumPool::alloc()
{
  if (top == NULL) {
    fillPool();
  }
  Datum * item = top;
  top = item->nextInPool;
  --allocCount;
  ++poolCount;
  return item;
}


void DatumPool::dealloc(Datum *item)
{
  item->nextInPool = top;
  top = item;
  ++allocCount;
  --poolCount;
}


void DatumPool::fillPool()
{
  QVector<Datum*> box;

  // Ask subclass to create block of new objects.
  createNewDatums(box);

  poolCount += box.size();

  // Set the list pointers for each item in box
  for (int i = box.size() - 1; i > 0; --i) {
    box[i]->nextInPool = box[i - 1];
  }
  box[0]->nextInPool = NULL;
  top = box[box.size() - 1];
}


int DatumPool::getPageSize()
{
  return getpagesize();
}


void DatumPool::createNewDatums(QVector<Datum*> &)
{
  Q_ASSERT(false);
}


// Values to represent no data (NULL)
Datum notADatum;
DatumP nothing(&notADatum);
