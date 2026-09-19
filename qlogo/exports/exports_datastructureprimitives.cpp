// ===-- qlogo/exports/exports_datastructureprimitives.cpp - Exports for the QLogo language -------*- C++ -*-===//
//
// Copyright 2017-2026 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the C library routines that are used by the QLogo compiler
/// for data structure primitives.
///
//===----------------------------------------------------------------------===//

#include "interface/logointerface.h"
#include "interface/textstream.h"
#include "runparser.h"
#include "sharedconstants.h"
#include "workspace/visited.h"
#include "workspace/exports_internal.h"
#include "datum_types.h"
#include "workspace/kernel.h"

bool areDatumsEqual(VisitedMap &visited, Datum *d1, Datum *d2, Qt::CaseSensitivity cs);

bool isDatumInContainer(VisitedSet &visited, Datum *value, Datum *container, Qt::CaseSensitivity cs);

/// @brief Recursively check if a datum is in an array.
/// @param visited The set of visited nodes.
/// @param value The value to check for.
/// @param array The array to check.
/// @param cs The case sensitivity to use for the comparison.
/// @return True if the value is in the array, false otherwise.
bool isDatumInArray(VisitedSet &visited, Datum *value, Array *array, Qt::CaseSensitivity cs)
{
    VisitedMap searched;
    for (auto &item : array->array)
    {
        searched.clear();
        Datum *itemPtr = item.datumValue();
        if (areDatumsEqual(searched, itemPtr, value, cs))
            return true;
        if (itemPtr->isArray() || itemPtr->isList())
        {
            if ( ! visited.contains(itemPtr))
            {
                visited.add(itemPtr);
                if (isDatumInContainer(visited, itemPtr, value, cs))
                    return true;
            }
        }
    }
    return false;
}

/// @brief Recursively check if a datum is in a list.
/// @param visited The set of visited nodes.
/// @param value The value to check for.
/// @param list The list to check.
/// @param cs The case sensitivity to use for the comparison.
/// @return True if the value is in the list, false otherwise.
bool isDatumInList(VisitedSet &visited, Datum *value, List *list, Qt::CaseSensitivity cs)
{
    VisitedMap searched;
    while (list != EmptyList::instance())
    {
        Datum *itemPtr = list->head.datumValue();
        searched.clear();
        if (areDatumsEqual(searched, itemPtr, value, cs))
            return true;
        if (itemPtr->isArray() || itemPtr->isList())
        {
            if (!visited.contains(itemPtr))
            {
                visited.add(itemPtr);
                if (isDatumInContainer(visited, itemPtr, value, cs))
                    return true;
            }
        }
        list = list->tail.listValue();
    }
    return false;
}

/// @brief Recursively check if a datum is in a container.
/// @param visited The set of visited nodes.
/// @param value The value to check for.
/// @param container The container to check.
/// @param cs The case sensitivity to use for the comparison.
/// @return True if the value is in the container, false otherwise.
bool isDatumInContainer(VisitedSet &visited, Datum *value, Datum *container, Qt::CaseSensitivity cs)
{
    if (visited.contains(container))
        return false;
    visited.add(container);

    VisitedMap searched;

    if (container->isArray())
    {
        Array *a = container->arrayValue();
        return isDatumInArray(visited, value, a, cs);
    }

    // If it's not an Array then it must be a List.
    List *l = container->listValue();
    return isDatumInList(visited, value, l, cs);
}

/// @brief Get the number of elements in a list.
/// @param listAddr a pointer to the List object.
/// @return the number of elements in the list.
EXPORTC int32_t getCountOfList(addr_t listAddr)
{
    auto *list = reinterpret_cast<List *>(listAddr);
    return list->count();
}

/// @brief Get a double array from a list of doubles.
/// @param eAddr a pointer to the Evaluator object context.
/// @param listAddr a pointer to the List object source
/// @param destAddr a pointer to the double array to store the values.
/// @return 0 if the list is not a list of doubles, 1 if it is.
EXPORTC int32_t getNumberAryFromList(addr_t listAddr, addr_t destAddr)
{
    auto list = DatumPtr(reinterpret_cast<Datum *>(listAddr));
    auto *dest = reinterpret_cast<double *>(destAddr);
    // Presumably, getCountOfList() has already been called so the destination size is correct.
    while (list.isList() && !list.listValue()->isEmpty())
    {
        DatumPtr d = list.listValue()->head;
        if (!d.isWord())
        {
            return 0;
        }
        *dest = d.wordValue()->numberValue();
        if (!d.wordValue()->numberIsValid)
        {
            return 0;
        }
        ++dest;
        list = list.listValue()->tail;
    }
    return 1;
}

/// Compare a Datum with a bool.
/// @param d a Datum
/// @param b a bool
/// @returns true iff d is a bool that equals b
EXPORTC bool cmpDatumToBool(addr_t d, bool b)
{
    auto *dD = reinterpret_cast<Datum *>(d);
    if (dD->isa_ != Datum::typeWord)
        return false;
    auto *dW = reinterpret_cast<Word *>(dD);
    bool dB = dW->boolValue();
    if (!dW->boolIsValid)
        return false;
    return dB == b;
}

/// Compare a Datum with a double.
/// @param d a Datum
/// @param n a number
/// @returns true iff d is a number that equals n
EXPORTC bool cmpDatumToDouble(addr_t d, double n)
{
    auto *dD = reinterpret_cast<Datum *>(d);
    if (dD->isa_ != Datum::typeWord)
        return false;
    auto *dW = reinterpret_cast<Word *>(dD);
    double dN = dW->numberValue();
    if (!dW->numberIsValid)
        return false;
    return dN == n;
}

/// Compare a Datum with a Datum.
/// @param eAddr The address of the Evaluator.
/// @param d1 a Datum
/// @param d2 a Datum
/// @returns true iff d1 is equal to d2 according to the help text of EQUALP
EXPORTC bool cmpDatumToDatum(addr_t eAddr, addr_t d1, addr_t d2)
{
    auto *dD1 = reinterpret_cast<Datum *>(d1);
    auto *dD2 = reinterpret_cast<Datum *>(d2);
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    Qt::CaseSensitivity cs = e->varCASEIGNOREDP() ? Qt::CaseInsensitive : Qt::CaseSensitive;
    VisitedMap visited;
    return areDatumsEqual(visited, dD1, dD2, cs);
}

EXPORTC addr_t concatWord(addr_t eAddr, addr_t aryAddr, uint32_t count)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto **wordAry = reinterpret_cast<Word **>(aryAddr);
    QString retval;
    for (uint32_t i = 0; i < count; ++i)
    {
        Word *w = *(wordAry + i);
        retval = retval % w->toString(Datum::ToStringFlags_Raw);
    }
    return reinterpret_cast<addr_t>(e->watch(new Word(retval)));
}

EXPORTC bool isDatumEmpty(addr_t dAddr)
{
    auto *d = reinterpret_cast<Datum *>(dAddr);
    if (d->isWord())
    {
        return d->toString(Datum::ToStringFlags_Raw).isEmpty();
    }
    else if (d->isList())
    {
        return d->listValue()->isEmpty();
    }

    // If it's not a Word or a List then it must be an Array.
    return d->arrayValue()->array.isEmpty();
}

EXPORTC addr_t createList(addr_t eAddr, addr_t aryAddr, uint32_t count)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto **ary = reinterpret_cast<Datum **>(aryAddr);
    ListBuilder builder;
    for (uint32_t i = 0; i < count; ++i)
    {
        DatumPtr d = DatumPtr(ary[i]);
        builder.append(d);
    }
    DatumPtr retval = builder.finishedList();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC addr_t createSentence(addr_t eAddr, addr_t aryAddr, uint32_t count)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto **ary = reinterpret_cast<Datum **>(aryAddr);
    ListBuilder builder;
    for (uint32_t i = 0; i < count; ++i)
    {
        DatumPtr d = DatumPtr(ary[i]);
        if (d.isList())
        {
            ListIterator it = d.listValue()->newIterator();
            while (it.elementExists())
            {
                builder.append(it.element());
            }
        }
        else
        {
            builder.append(d);
        }
    }
    DatumPtr retval = builder.finishedList();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC addr_t fputList(addr_t eAddr, addr_t thingAddr, addr_t listAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    auto *list = reinterpret_cast<List *>(listAddr);

    auto *retval = new List(DatumPtr(thing), list);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t lputList(addr_t eAddr, addr_t thingAddr, addr_t listAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    auto *list = reinterpret_cast<List *>(listAddr);

    ListBuilder builder;
    ListIterator it = list->newIterator();

    while (it.elementExists())
    {
        builder.append(it.element());
    }
    builder.append(DatumPtr(thing));

    DatumPtr retval = builder.finishedList();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC addr_t createArray(addr_t eAddr, int32_t size, int32_t origin)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *retval = new Array(origin, size);
    for (int i = 0; i < size; ++i)
    {
        retval->array.append(emptyList());
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t listToArray(addr_t eAddr, addr_t listAddr, int32_t origin)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *list = reinterpret_cast<List *>(listAddr);
    auto *retval = new Array(origin, list->count());
    ListIterator it = list->newIterator();
    while (it.elementExists())
    {
        retval->array.append(it.element());
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t arrayToList(addr_t eAddr, addr_t arrayAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    const auto *array = reinterpret_cast<const Array *>(arrayAddr);
    ListBuilder builder;
    for (const auto &i : array->array)
    {
        builder.append(i);
    }
    DatumPtr retval = builder.finishedList();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC addr_t firstOfDatum(addr_t eAddr, addr_t thingAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    Datum *retval = nullptr;
    if (thing->isWord())
    {
        Word *w = thing->wordValue();
        retval = new Word(w->toString(Datum::ToStringFlags_Raw).left(1));
    }
    else if (thing->isList())
    {
        List *l = thing->listValue();
        retval = l->head.datumValue();
    }
    else
    {
        // If it's not a Word or a List then it must be an Array.
        Array *a = thing->arrayValue();
        retval = new Word(QString::number(a->origin));
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t lastOfDatum(addr_t eAddr, addr_t thingAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    Datum *retval = nullptr;
    if (thing->isWord())
    {
        Word *w = thing->wordValue();
        retval = new Word(w->toString(Datum::ToStringFlags_Raw).right(1));
    }
    else if (thing->isList())
    {
        List *l = thing->listValue();
        ListIterator iter = l->newIterator();
        DatumPtr lastElement;
        while (iter.elementExists())
        {
            lastElement = iter.element();
        }
        retval = lastElement.datumValue();
    }
    else
    {
        // If it's not a Word or a List then it must be an Array.
        Array *a = thing->arrayValue();
        retval = new Word(a->origin);
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t butFirstOfDatum(addr_t eAddr, addr_t thingAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    Datum *retval = nullptr;
    if (thing->isWord())
    {
        Word *w = thing->wordValue();
        QString rawValue = w->toString(Datum::ToStringFlags_Raw);
        retval = new Word(rawValue.sliced(1, rawValue.size() - 1));
    }
    else
    {
        // If it's not a Word then it must be a List.
        List *l = thing->listValue();
        retval = l->tail.datumValue();
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t butLastOfDatum(addr_t eAddr, addr_t thingAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);

    if (thing->isWord())
    {
        Word *w = thing->wordValue();
        QString rawValue = w->toString(Datum::ToStringFlags_Raw);
        Datum *retval = new Word(rawValue.sliced(0, rawValue.size() - 1));
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval);
    }

    // If it's not a Word then it must be a List.
    List *l = thing->listValue();
    ListIterator iter = l->newIterator();
    ListBuilder builder;
    while (iter.elementExists())
    {
        DatumPtr element = iter.element();
        if (iter.elementExists())
        {
            builder.append(element);
        }
    }
    DatumPtr retval = builder.finishedList();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC bool isDatumIndexValid(addr_t thingAddr, double dIndex, addr_t listItemPtrAddr)
{
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    auto **listItemPtr = reinterpret_cast<Datum **>(listItemPtrAddr);
    auto index = static_cast<qsizetype>(dIndex);
    if (index != dIndex)
        return false;

    if (thing->isWord())
    {
        Word *w = thing->wordValue();
        QString rawValue = w->toString(Datum::ToStringFlags_Raw);
        return (index >= 1) && (index <= rawValue.size());
    }
    else if (thing->isList())
    {
        List *l = thing->listValue();
        if (index < 1)
            return false;
        ListIterator iter = l->newIterator();
        while (iter.elementExists())
        {
            *listItemPtr = iter.element().datumValue();
            index--;
            if (index == 0)
                return true;
        }
        return false;
    }

    // If it's not a Word or a List then it must be an Array.
    Array *a = thing->arrayValue();
    auto size = static_cast<int32_t>(a->array.size());
    index = index - a->origin;
    return (index >= 0) && (index < size);
}

EXPORTC addr_t itemOfDatum(addr_t eAddr, addr_t thingAddr, double dIndex, addr_t listItemPtrAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    Datum *retval = nullptr;
    auto index = static_cast<qsizetype>(dIndex);

    if (thing->isWord())
    {
        Word *w = thing->wordValue();
        QString rawValue = w->toString(Datum::ToStringFlags_Raw);
        retval = new Word(rawValue[index - 1]);
    }
    else if (thing->isList())
    {
        auto **retvalPtr = reinterpret_cast<Datum **>(listItemPtrAddr);
        retval = *retvalPtr;
    }
    else
    {
        // If it's not a Word or a List then it must be an Array.
        Array *a = thing->arrayValue();
        index = index - a->origin;
        retval = a->array[index].datumValue();
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC bool isDatumContainerOrInContainer(addr_t eAddr, addr_t valueAddr, addr_t containerAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *value = reinterpret_cast<Datum *>(valueAddr);
    auto *container = reinterpret_cast<Datum *>(containerAddr);

    // If not a container then there's no container to search.
    if (container->isa_ == Datum::typeWord)
        return false;

    // If value and container are the same then it's in the container.
    if (value == container)
        return true;

    Qt::CaseSensitivity cs = e->varCASEIGNOREDP() ? Qt::CaseInsensitive : Qt::CaseSensitive;
    VisitedSet visited;

    return isDatumInContainer(visited, value, container, cs);
}

EXPORTC void setDatumAtIndexOfContainer(addr_t valueAddr, double dIndex, addr_t containerAddr)
{
    auto *container = reinterpret_cast<Datum *>(containerAddr);
    DatumPtr value(reinterpret_cast<Datum *>(valueAddr));
    auto index = static_cast<qsizetype>(dIndex);

    if (container->isList())
    {
        List *l = container->listValue();
        for (qsizetype i = 1; i < index; ++i)
        {
            l = l->tail.listValue();
        }
        l->head = value;
    }

    // If it's not a List then it must be an Array.
    Array *a = container->arrayValue();
    a->array[index - a->origin] = value;
}

EXPORTC void setFirstOfList(addr_t listAddr, addr_t valueAddr)
{
    auto *l = reinterpret_cast<List *>(listAddr);
    l->head = DatumPtr(reinterpret_cast<Datum *>(valueAddr));
    if (l->compileTimeStamp > 0)
    {
        l->compileTimeStamp = 1;
        l->compiledText_.reset();
    }
}

EXPORTC void setButfirstOfList(addr_t listAddr, addr_t valueAddr)
{
    auto *l = reinterpret_cast<List *>(listAddr);
    l->tail = DatumPtr(reinterpret_cast<Datum *>(valueAddr));
    if (l->compileTimeStamp > 0)
    {
        l->compileTimeStamp = 1;
        l->compiledText_.reset();
    }
}

EXPORTC bool isEmpty(addr_t thingAddr)
{
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    if (thing->isWord())
    {
        Word *word = thing->wordValue();
        return word->toString(Datum::ToStringFlags_Raw).isEmpty();
    }
    else if (thing->isList())
    {
        List *list = thing->listValue();
        return list->isEmpty();
    }
    return false;
}

EXPORTC bool isBefore(addr_t eAddr, addr_t word1Addr, addr_t word2Addr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *word1 = reinterpret_cast<Word *>(word1Addr);
    auto *word2 = reinterpret_cast<Word *>(word2Addr);

    Qt::CaseSensitivity cs = e->varCASEIGNOREDP() ? Qt::CaseInsensitive : Qt::CaseSensitive;

    QString value1 = word1->toString();
    QString value2 = word2->toString();
    return value1.compare(value2, cs) < 0;
}

EXPORTC bool isMember(addr_t eAddr, addr_t thingAddr, addr_t containerAddr)
{
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    auto *container = reinterpret_cast<Datum *>(containerAddr);

    if (container->isWord())
    {
        Word *word = container->wordValue();
        QString containerString = word->toString(Datum::ToStringFlags_Key);
        if (thing->isWord())
        {
            Word *word2 = thing->wordValue();
            QString thingString = word2->toString(Datum::ToStringFlags_Key);
            if (thingString.length() != 1)
            {
                return false;
            }
            return containerString.contains(thingString);
        }
        return false;
    }
    else if (container->isList())
    {
        List *list = container->listValue();
        ListIterator iter(list);
        while (iter.elementExists())
        {
            Datum *item = iter.element().datumValue();
            auto itemAddr = reinterpret_cast<addr_t>(item);
            if (cmpDatumToDatum(eAddr, thingAddr, itemAddr))
            {
                return true;
            }
        }
        return false;
    }

    // If it's not a Word or a List then it must be an Array.
    const Array *array = container->arrayValue();

    return std::any_of(array->array.begin(), array->array.end(), [eAddr, thingAddr](const auto &item) {
        return cmpDatumToDatum(eAddr, thingAddr, reinterpret_cast<addr_t>(item.datumValue()));
    });
}

EXPORTC bool isSubstring(addr_t thing1Addr, addr_t thing2Addr)
{
    auto *thing1 = reinterpret_cast<Datum *>(thing1Addr);
    auto *thing2 = reinterpret_cast<Datum *>(thing2Addr);

    if (thing1->isa_ == Datum::typeWord && thing2->isa_ == Datum::typeWord)
    {
        auto *word1 = reinterpret_cast<Word *>(thing1);
        auto *word2 = reinterpret_cast<Word *>(thing2);
        QString string1 = word1->toString(Datum::ToStringFlags_Key);
        QString string2 = word2->toString(Datum::ToStringFlags_Key);
        return string2.contains(string1);
    }
    return false;
}

EXPORTC bool isNumber(addr_t thingAddr)
{
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    if (thing->isa_ != Datum::typeWord)
    {
        return false;
    }
    Word *word = reinterpret_cast<Word *>(thing);
    word->numberValue();
    return word->numberIsValid;
}

EXPORTC bool isSingleCharWord(addr_t candidateAddr)
{
    auto *candidate = reinterpret_cast<Datum *>(candidateAddr);
    if (candidate->isa_ != Datum::typeWord)
    {
        return false;
    }
    auto *word = reinterpret_cast<Word *>(candidate);
    return word->toString(Datum::ToStringFlags_Key).length() == 1;
}

EXPORTC bool isVbarred(addr_t cAddr)
{
    auto *word = reinterpret_cast<Word *>(cAddr);

    // A character is vbarred IFF it's print value is different from its raw value.
    char16_t rawC = word->toString(Datum::ToStringFlags_Raw).front().unicode();
    char16_t c = word->toString().front().unicode();
    return rawC != c;
}

EXPORTC double datumCount(addr_t thingAddr)
{
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    if (thing->isWord())
    {
        Word *word = thing->wordValue();
        return word->toString(Datum::ToStringFlags_Raw).length();
    }
    else if (thing->isList())
    {
        List *list = thing->listValue();
        return list->count();
    }

    // If it's not a Word or a List then it must be an Array.
    Array *array = thing->arrayValue();
    return array->array.size();
}

EXPORTC double ascii(addr_t cAddr)
{
    auto *word = reinterpret_cast<Word *>(cAddr);
    return word->toString().front().unicode();
}

EXPORTC double rawascii(addr_t cAddr)
{
    auto *word = reinterpret_cast<Word *>(cAddr);
    return word->toString(Datum::ToStringFlags_Raw).front().unicode();
}

EXPORTC addr_t chr(addr_t eAddr, uint32_t c)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto qstr = QString(QChar(static_cast<uint16_t>(c)));
    auto *retval = new Word(qstr);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t member(addr_t eAddr, addr_t thing1Addr, addr_t thing2Addr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing1 = reinterpret_cast<Datum *>(thing1Addr);
    auto *thing2 = reinterpret_cast<Datum *>(thing2Addr);
    if (thing2->isWord())
    {
        Word *word1 = thing1->wordValue();
        Word *word2 = thing2->wordValue();
        QString retval = "";
        if (thing1->isWord())
        {
            QString thing2Str = word2->toString(Datum::ToStringFlags_Raw);
            QString thing1Str = word1->toString(Datum::ToStringFlags_Raw);
            if (!thing1Str.isEmpty())
            {
                int index = thing2Str.indexOf(thing1Str);
                if (index != -1)
                {
                    retval = thing2Str.sliced(index);
                }
            }
        }
        Word *retvalWord = new Word(retval);
        e->watch(retvalWord);
        return reinterpret_cast<addr_t>(retvalWord);
    }

    // If it's not a Word then it must be a list.
    List *list = thing2->listValue();
    while (!list->isEmpty())
    {
        auto listAddr = reinterpret_cast<addr_t>(list->head.datumValue());
        if (cmpDatumToDatum(eAddr, listAddr, thing1Addr))
        {
            return reinterpret_cast<addr_t>(list);
        }
        list = list->tail.listValue();
    }
    // If we get here, thing1 was not found in thing2.
    // Return an empty list.
    List *retval = EmptyList::instance();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t lowercase(addr_t eAddr, addr_t wordAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *word = reinterpret_cast<Word *>(wordAddr);
    QString retval = word->toString(Datum::ToStringFlags_Raw).toLower();
    auto *retvalWord = new Word(retval);
    e->watch(retvalWord);
    return reinterpret_cast<addr_t>(retvalWord);
}

EXPORTC addr_t uppercase(addr_t eAddr, addr_t wordAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *word = reinterpret_cast<Word *>(wordAddr);
    QString retval = word->toString(Datum::ToStringFlags_Raw).toUpper();
    auto *retvalWord = new Word(retval);
    e->watch(retvalWord);
    return reinterpret_cast<addr_t>(retvalWord);
}

EXPORTC addr_t standout(addr_t eAddr, addr_t thingAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    QString phrase = thing->toString();
    QString retval = Config::get().mainInterface()->addStandoutToString(phrase);
    auto *retvalWord = new Word(retval);
    e->watch(retvalWord);
    return reinterpret_cast<addr_t>(retvalWord);
}

EXPORTC addr_t parse(addr_t eAddr, addr_t wordAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *word = reinterpret_cast<Word *>(wordAddr);
    QString phrase = word->toString(Datum::ToStringFlags_Raw);
    QTextStream stream(&phrase, QIODevice::ReadOnly);
    TextStream ts(&stream);
    DatumPtr retvalPtr = ts.readListWithPrompt(QString(), false);
    List *retval = retvalPtr.listValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t runparseDatum(addr_t eAddr, addr_t wordorlistAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto wordorlist = DatumPtr(reinterpret_cast<Datum *>(wordorlistAddr));

    DatumPtr retvalPtr = runparse(wordorlist);
    List *retval = retvalPtr.listValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

