
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


static DatumPool<List> listPool(40);

static DatumPool<ListNode> nodePool(60);


List::List() {
    astParseTimeStamp = 0;
    listSize = 0;
}


List * List::alloc()
{
    List * retval = (List *) listPool.alloc();
    retval->clear();
    retval->astParseTimeStamp = 0;
    retval->listSize = 0;
    return retval;
}


List * List::alloc(Array *source)
{
    List *retval = alloc();
    auto aryIter = source->newIterator();
    while (aryIter.elementExists()) {
        retval->append(aryIter.element());
    }
    return retval;
}

List::~List() {}


void List::addToPool()
{
    clear();
    listPool.dealloc(this);
}


List * List::alloc(List *source) {
    List *retval = alloc();
    retval->astParseTimeStamp = 0;
    retval->head = source->head;
    retval->lastNode = source->lastNode;
    retval->listSize = source->size();
    return retval;
}

Datum::DatumType List::isa() { return listType; }

QString List::name() {
  return k.list();
}

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
    return (anIndex >= 1) && (anIndex <= listSize);
}

void List::setItem(int anIndex, DatumPtr aValue) {
    DatumPtr ptr = head;
    while (anIndex > 1) {
        --anIndex;
        ptr = ptr.listNodeValue()->next;
    }
  ptr.listNodeValue()->item = aValue;
  astParseTimeStamp = 0;
}

void List::setButfirstItem(DatumPtr aValue) {
  Q_ASSERT(head != nothing);
  Q_ASSERT(aValue.isList());
    head.listNodeValue()->next = aValue.listValue()->head;
    astParseTimeStamp = 0;
    listSize = aValue.listValue()->size() + 1;
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
  List *retval = List::alloc();
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
  retval->setListSize();
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
  List *retval = List::alloc();
    retval->head = head.listNodeValue()->next;
    retval->listSize = listSize - 1;
    retval->lastNode = lastNode;
  return DatumPtr(retval);
}

void List::clear() {
  head = nothing;
  lastNode = nothing;
  listSize = 0;
  astList.clear();
  astParseTimeStamp = 0;
}

// This should NOT be used in cases where a list may be shared
void List::append(DatumPtr element) {
  ListNode *newNode = ListNode::alloc();
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

DatumPtr List::last() {
    Q_ASSERT(lastNode != nothing);
    return lastNode.listNodeValue()->item;
}

DatumPtr List::butlast() {
  List *retval = List::alloc();
  retval->listSize = listSize - 1;
  if (head.listNodeValue()->next != nothing) {
      DatumPtr src = head;
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
  return DatumPtr(retval);
}

void List::prepend(DatumPtr element) {
    ListNode *newnode = ListNode::alloc();
    newnode->item = element;
    newnode->next = head;
    head = newnode;
    ++listSize;
  astParseTimeStamp = 0;
}

DatumPtr List::fput(DatumPtr item)
{
    ListNode *newnode = ListNode::alloc();
    List *retval = List::alloc();
    newnode->item = item;
    newnode->next = head;
    retval->head = newnode;
    retval->listSize = listSize + 1;
    return retval;
}

void List::setListSize()
{
    listSize = 0;
    DatumPtr ptr = head;
    while (ptr != nothing) {
        ++listSize;
        ptr = ptr.listNodeValue()->next;
    }
}

ListIterator List::newIterator() { return ListIterator(head); }

ListNode * ListNode::alloc()
{
    ListNode * retval = nodePool.alloc();
    return retval;
}


void ListNode::addToPool()
{
    item = nothing;
    next = nothing;
    nodePool.dealloc(this);
}

