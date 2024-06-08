
//===-- qlogo/datum_list.cpp - List class implementation -------*-
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
/// This file contains the implementation of the List class, which is the general
/// container of sequence data. A list may contain words, lists or arrays.
/// Is is implemented by using a linked list.
///
//===----------------------------------------------------------------------===//

#include "datum/list.h"
#include "datum/array.h"
#include "datum/iterator.h"
#include <qdebug.h>
#include <QObject>

QList<void *> listVisited;
QList<void *> otherListVisited;


List::List() {
    astParseTimeStamp = 0;
}


List::List(Array *source)
{
    auto aryIter = source->newIterator();
    while (aryIter.elementExists()) {
        append(aryIter.element());
    }
}

List::List(DatumPtr item, List *srcList)
{
    head = item;
    if (! srcList->head.isNothing()) {
        tail = DatumPtr(srcList);
        lastNode = srcList->lastNode;
    } else {
        lastNode = DatumPtr(this);
    }
    astParseTimeStamp = 0;
}

List::~List() {}


List::List(List *source) {
    astParseTimeStamp = 0;
    head = source->head;
    lastNode = source->lastNode;
}

Datum::DatumType List::isa() { return listType; }

QString List::printValue(bool fullPrintp, int printDepthLimit,
                         int printWidthLimit) {
  if (head.isNothing())
    return "";
  DatumPtr iter = DatumPtr(this);
  if ((printDepthLimit == 0) || (printWidthLimit == 0)) {
    return "...";
  }
  int printWidth = printWidthLimit - 1;
  QString retval = iter.listValue()->head.showValue(fullPrintp, printDepthLimit - 1, printWidthLimit);
  while (iter.listValue()->tail != nothing) {
      iter = iter.listValue()->tail;
    retval.append(' ');
    if (printWidth == 0) {
      retval.append("...");
      break;
    }
    retval.append(
        iter.listValue()->head.showValue(fullPrintp, printDepthLimit - 1, printWidthLimit));
    --printWidth;
  }
  return retval;
}

QString List::showValue(bool fullPrintp, int printDepthLimit,
                        int printWidthLimit) {
  if (!listVisited.contains(this)) {
    listVisited.push_back(this);
    QString retval = "[";
    retval.append(printValue(fullPrintp, printDepthLimit, printWidthLimit));
    retval.append(']');
    listVisited.removeOne(this);
    return retval;
  }
  return "...";
}

bool List::isEqual(DatumPtr other, bool ignoreCase) {
  ListIterator iter;
  ListIterator otherIter;
  List *o = other.listValue();
  int myIndex = listVisited.indexOf(this);
  int otherIndex = otherListVisited.indexOf(o);
  if (myIndex != otherIndex)
    goto exit_false;

  if (myIndex > -1)
    return true;

  iter = newIterator();
  otherIter = o->newIterator();
  listVisited.push_back(this);
  otherListVisited.push_back(o);

  while (iter.elementExists()) {
    DatumPtr value = iter.element();
    DatumPtr otherValue = otherIter.element();
    if (!value.isEqual(otherValue, ignoreCase))
      goto exit_false;
  }

  listVisited.pop_back();
  otherListVisited.pop_back();
  return true;

exit_false:
  listVisited.clear();
  otherListVisited.clear();
  return false;
}

DatumPtr List::first() {
  Q_ASSERT(head != nothing);
  return head;
}

bool List::isEmpty() {
    return head.isNothing();
}

bool List::isIndexInRange(int anIndex) {
    if (head.isNothing()) return false;
    if (anIndex < 1) return false;
    DatumPtr ptr(this);
    while (ptr.isList()) {
        if (--anIndex < 1) return true;
        ptr = ptr.listValue()->tail;
    }
    return false;
}

void List::setButfirstItem(DatumPtr aValue) {
  Q_ASSERT(head != nothing);
  Q_ASSERT(aValue.isList());
  tail = aValue;
  lastNode = aValue.listValue()->lastNode;
    astParseTimeStamp = 0;
}

DatumPtr List::itemAtIndex(int anIndex) {
  Q_ASSERT(isIndexInRange(anIndex));
    DatumPtr ptr(this);
    while (anIndex > 1) {
        --anIndex;
        ptr = ptr.listValue()->tail;
    }
  return ptr.listValue()->head;
}

void List::clear() {
  head = nothing;
  tail = nothing;
  lastNode = nothing;
  astList.clear();
  astParseTimeStamp = 0;
}

// This should only be used when initializing a list. It should not be used
// afterwards.
void List::append(DatumPtr element) {
    astParseTimeStamp = 0;

    if (head == nothing) {
        head = element;
        tail = nothing;
        lastNode = nothing;
        return;
    }

    List *l = new List();
    l->head = element;
    l->tail = nothing;
    l->lastNode = nothing;
    DatumPtr lP(l);

    if (tail == nothing) {
        tail = lP;
        lastNode = tail;
        return;
    }

    lastNode.listValue()->tail = lP;
    lastNode = lP;
}

DatumPtr List::last() {
    Q_ASSERT(lastNode != nothing);
    return lastNode.listValue()->head;
}

int List::count()
{
    int retval = 0;
    DatumPtr ptr(this);
    while (ptr.isList() && ( ! ptr.listValue()->head.isNothing())) {
        ++retval;
        ptr = ptr.listValue()->tail;
    }
    return retval;
}

ListIterator List::newIterator() { return ListIterator(this); }

