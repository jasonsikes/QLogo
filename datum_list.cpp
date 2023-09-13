
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

#include "datum_list.h"
#include "datum_array.h"
#include "datum_iterator.h"
#include "stringconstants.h"
#include <qdebug.h>

QList<void *> listVisited;
QList<void *> otherListVisited;


List::List() {
    astParseTimeStamp = 0;
    listSize = 0;
}


List *emptyList()
{
    return new List;
}


DatumP emptyListP()
{
    return DatumP(emptyList());
}


List::List(Array *source) {
  astParseTimeStamp = 0;
  listSize = source->size();
  auto aryIter = source->newIterator();
  DatumP prev;
  if (aryIter.elementExists()) {
      head = new ListNode;
      lastNode = head;
      head.listNodeValue()->item = aryIter.element();
      prev = head;
  }
  while (aryIter.elementExists()) {
      lastNode = new ListNode;
      prev.listNodeValue()->next = lastNode;
      lastNode.listNodeValue()->item = aryIter.element();
      prev = lastNode;
  }
}

List::~List() {}

List::List(List *source) {
  astParseTimeStamp = 0;
  head = source->head;
  lastNode = source->lastNode;
  listSize = source->size();
}

Datum::DatumType List::isa() { return listType; }

QString List::name() {
  return k.list();
}

QString List::printValue(bool fullPrintp, int printDepthLimit,
                         int printWidthLimit) {
  DatumP iter = head;
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

bool List::isEqual(DatumP other, bool ignoreCase) {
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
    DatumP value = iter.element();
    DatumP otherValue = otherIter.element();
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

DatumP List::first() {
  Q_ASSERT(head != nothing);
  return head.listNodeValue()->item;
}

bool List::isIndexInRange(int anIndex) {
    return (anIndex >= 1) && (anIndex <= listSize);
}

void List::setItem(int anIndex, DatumP aValue) {
    DatumP ptr = head;
    while (anIndex > 1) {
        --anIndex;
        ptr = ptr.listNodeValue()->next;
    }
  ptr.listNodeValue()->item = aValue;
  astParseTimeStamp = 0;
}

void List::setButfirstItem(DatumP aValue) {
  Q_ASSERT(head != nothing);
  Q_ASSERT(aValue.isList());
    head.listNodeValue()->next = aValue.listValue()->head;
    astParseTimeStamp = 0;
    listSize = aValue.listValue()->size() + 1;
}

void List::setFirstItem(DatumP aValue) {
    Q_ASSERT(head != nothing);
    head.listNodeValue()->item = aValue;
  astParseTimeStamp = 0;
}

// TODO: Check for cyclic list structures.
bool List::containsDatum(DatumP aDatum, bool ignoreCase) {
    ListIterator iter = newIterator();
    while (iter.elementExists()) {
        DatumP e = iter.element();
        if (e == aDatum)
            return true;
        if (e.datumValue()->containsDatum(aDatum, ignoreCase))
          return true;
    }
  return false;
}

bool List::isMember(DatumP aDatum, bool ignoreCase) {
    ListIterator iter = newIterator();
    while (iter.elementExists()) {
        if (aDatum.isEqual(iter.element(), ignoreCase))
            return true;
    }
  return false;
}

DatumP List::fromMember(DatumP aDatum, bool ignoreCase) {
  List *retval = emptyList();
  DatumP ptr = head;
  while (ptr != nothing) {
      DatumP e = ptr.listNodeValue()->item;
      if (e.isEqual(aDatum, ignoreCase)) {
          retval->head = ptr;
          retval->lastNode = lastNode;
          break;
      }
      ptr = ptr.listNodeValue()->next;
  }
  retval->setListSize();
  return DatumP(retval);
}

DatumP List::datumAtIndex(int anIndex) {
  Q_ASSERT(isIndexInRange(anIndex));
    DatumP ptr = head;
    while (anIndex > 1) {
        --anIndex;
        ptr = ptr.listNodeValue()->next;
    }
  return ptr.listNodeValue()->item;
}

DatumP List::butfirst() {
    Q_ASSERT(head != nothing);
  List *retval = emptyList();
    retval->head = head.listNodeValue()->next;
    retval->listSize = listSize - 1;
    retval->lastNode = lastNode;
  return DatumP(retval);
}

void List::clear() {
  head = nothing;
  lastNode = nothing;
  listSize = 0;
  astList.clear();
  astParseTimeStamp = 0;
}

// This should NOT be used in cases where a list may be shared
void List::append(DatumP element) {
    ListNode *newNode = new ListNode;
    ++listSize;
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

DatumP List::last() {
    Q_ASSERT(lastNode != nothing);
    return lastNode.listNodeValue()->item;
}

DatumP List::butlast() {
  List *retval = emptyList();
  retval->listSize = listSize - 1;
  if (head.listNodeValue()->next != nothing) {
      DatumP src = head;
      while (src.listNodeValue()->next != nothing) {
          ListNode *newnode = new ListNode;
          newnode->item = src.listNodeValue()->item;
          if (retval->head == nothing) {
              retval->head = newnode;
              retval->lastNode = newnode;
          } else {
            retval->lastNode.listNodeValue()->next = newnode;
            retval->lastNode = newnode;
          }
          src = src.listNodeValue()->next;
      }
  }
  return DatumP(retval);
}

void List::prepend(DatumP element) {
    ListNode *newnode = new ListNode;
    newnode->item = element;
    newnode->next = head;
    head = newnode;
    ++listSize;
  astParseTimeStamp = 0;
}

DatumP List::fput(DatumP item)
{
    ListNode *newnode = new ListNode;
    List *retval = emptyList();
    newnode->item = item;
    newnode->next = head;
    retval->head = newnode;
    retval->listSize = listSize + 1;
    return retval;
}

void List::setListSize()
{
    listSize = 0;
    DatumP ptr = head;
    while (ptr != nothing) {
        ++listSize;
        ptr = ptr.listNodeValue()->next;
    }
}

ListIterator List::newIterator() { return ListIterator(head); }
