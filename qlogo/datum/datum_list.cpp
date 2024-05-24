
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

List::~List() {}


List::List(List *source) {
    astParseTimeStamp = 0;
    head = source->head;
    lastNode = source->lastNode;
}

Datum::DatumType List::isa() { return listType; }

QString List::printValue(bool fullPrintp, int printDepthLimit,
                         int printWidthLimit) {
  DatumPtr iter = head;
  if (iter == nothing) {
    return "";
  }
  if ((printDepthLimit == 0) || (printWidthLimit == 0)) {
    return "...";
  }
  int printWidth = printWidthLimit - 1;
  QString retval = iter.listNodeValue()->item.showValue(fullPrintp, printDepthLimit - 1, printWidthLimit);
  while (iter.listNodeValue()->next != nothing) {
      iter = iter.listNodeValue()->next;
    retval.append(' ');
    if (printWidth == 0) {
      retval.append("...");
      break;
    }
    retval.append(
        iter.listNodeValue()->item.showValue(fullPrintp, printDepthLimit - 1, printWidthLimit));
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

  if (size() != o->size())
    goto exit_false;

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
  return head.listNodeValue()->item;
}

bool List::isIndexInRange(int anIndex) {
    return (anIndex >= 1) && (anIndex <= size());
}

void List::setButfirstItem(DatumPtr aValue) {
  Q_ASSERT(head != nothing);
  Q_ASSERT(aValue.isList());
    head.listNodeValue()->next = aValue.listValue()->head;
    astParseTimeStamp = 0;
}

void List::setFirstItem(DatumPtr aValue) {
    Q_ASSERT(head != nothing);
    head.listNodeValue()->item = aValue;
  astParseTimeStamp = 0;
}

// TODO: Check for cyclic list structures.
bool List::containsDatum(DatumPtr aDatum, bool ignoreCase) {
    ListIterator iter = newIterator();
    while (iter.elementExists()) {
        DatumPtr e = iter.element();
        if (e == aDatum)
            return true;
        if (e.datumValue()->containsDatum(aDatum, ignoreCase))
          return true;
    }
  return false;
}

bool List::isMember(DatumPtr aDatum, bool ignoreCase) {
    ListIterator iter = newIterator();
    while (iter.elementExists()) {
        if (aDatum.isEqual(iter.element(), ignoreCase))
            return true;
    }
  return false;
}

DatumPtr List::fromMember(DatumPtr aDatum, bool ignoreCase) {
    List *retval = new List();
  DatumPtr ptr = head;
  while (ptr != nothing) {
      DatumPtr e = ptr.listNodeValue()->item;
      if (e.isEqual(aDatum, ignoreCase)) {
          retval->head = ptr;
          retval->lastNode = lastNode;
          break;
      }
      ptr = ptr.listNodeValue()->next;
  }
  return DatumPtr(retval);
}

DatumPtr List::datumAtIndex(int anIndex) {
  Q_ASSERT(isIndexInRange(anIndex));
    DatumPtr ptr = head;
    while (anIndex > 1) {
        --anIndex;
        ptr = ptr.listNodeValue()->next;
    }
  return ptr.listNodeValue()->item;
}

DatumPtr List::butfirst() {
    Q_ASSERT(head != nothing);
    List *retval = new List();
    retval->head = head.listNodeValue()->next;
    retval->lastNode = lastNode;
  return DatumPtr(retval);
}

void List::clear() {
  head = nothing;
  lastNode = nothing;
  astList.clear();
  astParseTimeStamp = 0;
}

// This should NOT be used in cases where a list may be shared
void List::append(DatumPtr element) {
    ListNode *newNode = new ListNode();
    newNode->item = element;
    if (head == nothing) {
        head = newNode;
        lastNode = newNode;
        return;
    }
    lastNode.listNodeValue()->next = newNode;
    lastNode = newNode;
  astParseTimeStamp = 0;
}

DatumPtr List::last() {
    Q_ASSERT(lastNode != nothing);
    return lastNode.listNodeValue()->item;
}

void List::prepend(DatumPtr element) {
    ListNode *newnode = new ListNode();
    newnode->item = element;
    newnode->next = head;
    head = newnode;
  astParseTimeStamp = 0;
}

DatumPtr List::fput(DatumPtr item)
{
    ListNode *newnode = new ListNode();
    List *retval = new List();
    newnode->item = item;
    newnode->next = head;
    retval->head = newnode;
    return retval;
}

int List::size()
{
    int retval = 0;
    DatumPtr ptr = head;
    while (ptr != nothing) {
        ++retval;
        ptr = ptr.listNodeValue()->next;
    }
    return retval;
}

ListIterator List::newIterator() { return ListIterator(head); }

