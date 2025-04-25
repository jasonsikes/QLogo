//===-- qlogo/datastructureprimitives.cpp - Data structure implementations -------*- C++ -*-===//
//
// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the data structure methods of the
/// Compiler class.
///
//===----------------------------------------------------------------------===//

#include "compiler_private.h"
#include "astnode.h"
#include "compiler.h"
#include "sharedconstants.h"
#include "workspace/callframe.h"
#include "kernel.h"
#include "controller/logocontroller.h"
#include "controller/textstream.h"
#include "runparser.h"
#include <QIODevice>

using namespace llvm;
using namespace llvm::orc;

/// Compare a Datum with a bool.
/// @param d a Datum
/// @param b a bool
/// @returns true iff d is a bool that equals b
EXPORTC bool cmpDatumToBool(addr_t d, bool b)
{
    Datum *dD = reinterpret_cast<Datum*>(d);
    if (dD->isa != Datum::typeWord)
        return false;
    Word *dW = reinterpret_cast<Word*>(dD);
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
    Datum *dD = reinterpret_cast<Datum*>(d);
    if (dD->isa != Datum::typeWord)
        return false;
    Word *dW = reinterpret_cast<Word*>(dD);
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
    Datum *dD1 = reinterpret_cast<Datum*>(d1);
    Datum *dD2 = reinterpret_cast<Datum*>(d2);
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Qt::CaseSensitivity cs = e->varCASEIGNOREDP() ? Qt::CaseInsensitive : Qt::CaseSensitive;
    return e->areDatumsEqual(dD1, dD2, cs);
}


EXPORTC addr_t concatWord(addr_t eAddr, addr_t aryAddr, uint32_t count)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Word **wordAry = reinterpret_cast<Word**>(aryAddr);
    QString retval = "";
    for (int i = 0; i < count; ++i)
    {
        Word *w = *(wordAry + i);
        retval += w->rawValue();
    }
    return reinterpret_cast<addr_t >(e->watch(new Word(retval)));
}

Value *Compiler::generateNotEmptyWordOrListFromDatum(ASTNode *parent, Value *src)
{
    auto validator = [this](Value *wordorlist) {
        BasicBlock *startBB = scaff->builder.GetInsertBlock();
        Function *theFunction = startBB->getParent();

        BasicBlock *wordOrListBB = BasicBlock::Create(*scaff->theContext, "wordOrListBlock", theFunction);
        BasicBlock *endBB = BasicBlock::Create(*scaff->theContext, "endBlock", theFunction);

        Value *wordOrListType = generateGetDatumIsa(wordorlist);
        Value *maskCalc = scaff->builder.CreateAnd(wordOrListType, CoInt32(Datum::typeWord | Datum::typeList), "maskCalc");
        Value *wordOrListCond = scaff->builder.CreateICmpNE(maskCalc, CoInt32(0), "wordOrListCond");
        scaff->builder.CreateCondBr(wordOrListCond, wordOrListBB, endBB);

        // Word or List block
        scaff->builder.SetInsertPoint(wordOrListBB);
        Value *isEmpty = generateCallExtern(TyBool, "isDatumEmpty", {PaAddr(evaluator), PaAddr(wordorlist)});
        Value *isEmptyCond = scaff->builder.CreateICmpEQ(isEmpty, CoBool(false), "isDatumEmptyCond");
        scaff->builder.CreateBr(endBB);

        // Merge block
        scaff->builder.SetInsertPoint(endBB);
        PHINode *phi = scaff->builder.CreatePHI(wordOrListCond->getType(), 2, "lastOfDatumResult");
        phi->addIncoming(isEmptyCond, wordOrListBB);
        phi->addIncoming(wordOrListCond, startBB);
        return phi;
    };
    return generateValidationDatum(parent, src, validator);
}

// TODO: This is a near duplicate of generateNotEmptyWordOrListFromDatum. Refactor.
Value *Compiler::generateNotEmptyListFromDatum(ASTNode *parent, Value *src)
{
    auto validator = [this](Value *wordorlist) {
        BasicBlock *startBB = scaff->builder.GetInsertBlock();
        Function *theFunction = startBB->getParent();

        BasicBlock *listBB = BasicBlock::Create(*scaff->theContext, "listBlock", theFunction);
        BasicBlock *endBB = BasicBlock::Create(*scaff->theContext, "endBlock", theFunction);

        Value *wordOrListType = generateGetDatumIsa(wordorlist);
        Value *maskCalc = scaff->builder.CreateAnd(wordOrListType, CoInt32(Datum::typeList), "maskCalc");
        Value *wordOrListCond = scaff->builder.CreateICmpNE(maskCalc, CoInt32(0), "listCond");
        scaff->builder.CreateCondBr(wordOrListCond, listBB, endBB);

        // Word or List block
        scaff->builder.SetInsertPoint(listBB);
        Value *isEmpty = generateCallExtern(TyBool, "isDatumEmpty", {PaAddr(evaluator), PaAddr(wordorlist)});
        Value *isEmptyCond = scaff->builder.CreateICmpEQ(isEmpty, CoBool(false), "isDatumEmptyCond");
        scaff->builder.CreateBr(endBB);

        // Merge block
        scaff->builder.SetInsertPoint(endBB);
        PHINode *phi = scaff->builder.CreatePHI(wordOrListCond->getType(), 2, "lastOfDatumResult");
        phi->addIncoming(isEmptyCond, listBB);
        phi->addIncoming(wordOrListCond, startBB);
        return phi;
    };
    return generateValidationDatum(parent, src, validator);
}

EXPORTC bool isDatumEmpty(addr_t eAddr, addr_t dAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *d = reinterpret_cast<Datum*>(dAddr);
    switch (d->isa)
    {
        case Datum::typeWord:
        {
            Word *w = reinterpret_cast<Word*>(d);
            return w->rawValue().isEmpty();
        }
        case Datum::typeList:
        {
            List *l = reinterpret_cast<List*>(d);
            return l->isEmpty();
        }
        case Datum::typeArray:
        {
            return false;
        }
        default:
            Q_ASSERT(false);
    }
     return false;
}

/***DOC EQUALP EQUAL?
EQUALP thing1 thing2
EQUAL? thing1 thing2
thing1 = thing2

 outputs TRUE if the inputs are equal, FALSE otherwise.  Two numbers
 are equal if they have the same numeric value.  Two non-numeric words
 are equal if they contain the same characters in the same order.  If
 there is a variable named CASEIGNOREDP whose value is TRUE, then an
 upper case letter is considered the same as the corresponding lower
 case letter.  (This is the case by default.)  Two lists are equal if
 their members are equal.  An array is only equal to itself; two
 separately created arrays are never equal even if their members are
 equal.  (It is important to be able to know if two expressions have
 the same array as their value because arrays are mutable; if, for
 example, two variables have the same array as their values then
 performing SETITEM on one of them will also change the other.)

COD***/
// CMD EQUALP 2 2 2 b
// CMD EQUAL? 2 2 2 b
Value *Compiler::genEqualp(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnBool);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnRDB);

    Type *t0 = children[0]->getType();
    Type *t1 = children[1]->getType();

    // If one is bool and other is double, then they can't be equal.
    if ((t0->isIntegerTy(1) && t1->isDoubleTy()) ||(t1->isIntegerTy(1) && t0->isDoubleTy()))
    {
        return CoBool(false);
    }

    // Both double? Compare them
    if (t0->isDoubleTy() && t1->isDoubleTy())
    {
        return scaff->builder.CreateFCmpUEQ(children[0], children[1], "Fequalp");
    }

    // Both bool? Compare them
    if (t0->isIntegerTy(1) && t1->isIntegerTy(1))
    {
        return scaff->builder.CreateICmpEQ(children[0], children[1], "Bequalp");
    }

    // At this point we know at least one of the inputs is a Datum.
    // For simplicity, make the first child the Datum
    if (!t0->isPointerTy())
    {
        Value *t = children[0];
        children[0] = children[1];
        children[1] = t;
        t1 = children[1]->getType();
    }

    // If second child is bool:
    if (t1->isIntegerTy(1))
    {
        return generateCallExtern(TyBool, "cmpDatumToBool", {PaAddr(children[0]), PaBool(children[1])});
    }

    // If second child is number:
    if (t1->isDoubleTy())
    {
        return generateCallExtern(TyBool, "cmpDatumToDouble", {PaAddr(children[0]), PaDouble(children[1])});
    }

    // Both must be Datums
    Q_ASSERT(t1->isPointerTy());
    return generateCallExtern(TyBool, "cmpDatumToDatum", {PaAddr(evaluator), PaAddr(children[0]), PaAddr(children[1])});
}


/***DOC NOTEQUALP NOTEQUAL?
NOTEQUALP thing1 thing2
NOTEQUAL? thing1 thing2
thing1 <> thing2

 outputs FALSE if the inputs are equal, TRUE otherwise.  See EQUALP
 for the meaning of equality for different data types.

COD***/
// CMD NOTEQUALP 2 2 2 b
// CMD NOTEQUAL? 2 2 2 b
Value *Compiler::genNotequalp(DatumPtr node, RequestReturnType returnType)
{
    Value *eq = genEqualp(node, returnType);
    return scaff->builder.CreateSub(CoBool(1), eq, "noteq");
}


// CONSTRUCTORS

/***DOC WORD
WORD word1 word2
(WORD word1 word2 word3 ...)

    outputs a word formed by concatenating its inputs.

COD***/
// CMD WORD 0 2 -1 d
Value *Compiler::genWord(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    AllocaInst *ary = generateChildrenAlloca(node.astnodeValue(), RequestReturnDatum, "wordAry");
    return generateCallExtern(TyAddr, "concatWord", {PaAddr(evaluator), PaAddr(ary), PaInt32(ary->getArraySize())});
}

/***DOC LIST
LIST thing1 thing2
(LIST thing1 thing2 thing3 ...)

    outputs a list whose members are its inputs, which can be any
    Logo datum (word, list, or array).

COD***/
// CMD LIST 0 2 -1 d
Value *Compiler::genList(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    AllocaInst *ary = generateChildrenAlloca(node.astnodeValue(), RequestReturnDatum, "listAry");
    return generateCallExtern(TyAddr, "createList", {PaAddr(evaluator), PaAddr(ary), PaInt32(ary->getArraySize())});
}

EXPORTC addr_t createList(addr_t eAddr, addr_t aryAddr, uint32_t count)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum **ary = reinterpret_cast<Datum**>(aryAddr);
    List *retval = new List();
    ListBuilder builder(retval);
    for (int i = 0; i < count; ++i)
    {
        DatumPtr d = DatumPtr(ary[i]);
        builder.append(d);
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}


/***DOC SENTENCE SE
SENTENCE thing1 thing2
SE thing1 thing2
(SENTENCE thing1 thing2 thing3 ...)
(SE thing1 thing2 thing3 ...)

    outputs a list whose members are its inputs, if those inputs are
    not lists, or the members of its inputs, if those inputs are lists.

COD***/
// CMD SENTENCE 0 2 -1 d
// CMD SE 0 2 -1 d
Value *Compiler::genSentence(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    AllocaInst *ary = generateChildrenAlloca(node.astnodeValue(), RequestReturnDatum, "sentenceAry");
    return generateCallExtern(TyAddr, "createSentence", {PaAddr(evaluator), PaAddr(ary), PaInt32(ary->getArraySize())});
}

EXPORTC addr_t createSentence(addr_t eAddr, addr_t aryAddr, uint32_t count)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum **ary = reinterpret_cast<Datum**>(aryAddr);
    List *retval = new List();
    ListBuilder builder(retval);
    for (int i = 0; i < count; ++i)
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
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}


/***DOC FPUT
FPUT thing list

    outputs a list equal to its second input with one extra member,
    the first input, at the beginning.  If the second input is a word,
    then the first input must be a word, and FPUT is equivalent to WORD.

COD***/
/***DOC LPUT
LPUT thing list

    outputs a list equal to its second input with one extra member,
    the first input, at the end.  If the second input is a word,
    then the first input must be a one-letter word, and LPUT is
    equivalent to WORD with its inputs in the other order.

COD***/
// CMD LPUT 2 2 2 d
// CMD FPUT 2 2 2 d
Value *Compiler::genFputlput(DatumPtr node, RequestReturnType returnType)
{
    QChar nodeNameFirstLetter = node.astnodeValue()->nodeName.wordValue()->keyValue().front();
    bool isLput = nodeNameFirstLetter == 'L';

    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *list = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    Value *listWordTest = nullptr;

    auto wordVector = isLput ? std::vector<Value*>{list, thing} : std::vector<Value*>{thing, list};
    const char* listFunctionName = isLput ? "lputList" : "fputList";

    auto validator = [this, thing, &listWordTest](Value *list) {
        Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
        BasicBlock *wordBB = BasicBlock::Create(*scaff->theContext, "isWordBlock", theFunction);
        BasicBlock *listBB = BasicBlock::Create(*scaff->theContext, "isListBlock", theFunction);
        BasicBlock *endBB = BasicBlock::Create(*scaff->theContext, "endBlock", theFunction);

        Value *listType = generateGetDatumIsa(list);
        listWordTest = scaff->builder.CreateICmpEQ(listType, CoInt32(Datum::typeWord), "listWordTest");
        scaff->builder.CreateCondBr(listWordTest, wordBB, listBB);

        // Word block
        scaff->builder.SetInsertPoint(wordBB);
        Value *thingType = generateGetDatumIsa(thing);
        Value *thingWordTest = scaff->builder.CreateICmpEQ(thingType, CoInt32(Datum::typeWord), "thingWordTest");
        scaff->builder.CreateBr(endBB);

        // List block
        scaff->builder.SetInsertPoint(listBB);
        Value *listListTest = scaff->builder.CreateICmpEQ(listType, CoInt32(Datum::typeList), "listListTest");
        scaff->builder.CreateBr(endBB);

        scaff->builder.SetInsertPoint(endBB);
        PHINode *phi = scaff->builder.CreatePHI(listListTest->getType(), 2, "putResult");
        phi->addIncoming(listListTest, listBB);
        phi->addIncoming(thingWordTest, wordBB);
        return phi;
    };
    list = generateValidationDatum(node.astnodeValue(), list, validator);

    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    BasicBlock *wordBB = BasicBlock::Create(*scaff->theContext, "isWordBB", theFunction);
    BasicBlock *listBB = BasicBlock::Create(*scaff->theContext, "isListBB", theFunction);
    BasicBlock *mergeBB = BasicBlock::Create(*scaff->theContext, "mergeBB", theFunction);

    scaff->builder.CreateCondBr(listWordTest, wordBB, listBB);

    scaff->builder.SetInsertPoint(wordBB);
    AllocaInst *ary = generateAllocaAry(wordVector, "wordAry");
    Value *wordRetval = generateCallExtern(TyAddr, "concatWord", {PaAddr(evaluator), PaAddr(ary), PaInt32(ary->getArraySize())});
    scaff->builder.CreateBr(mergeBB);

    scaff->builder.SetInsertPoint(listBB);
    Value *listRetval = generateCallExtern(TyAddr, listFunctionName, {PaAddr(evaluator), PaAddr(thing), PaAddr(list)});
    scaff->builder.CreateBr(mergeBB);

    scaff->builder.SetInsertPoint(mergeBB);
    PHINode *phi = scaff->builder.CreatePHI(list->getType(), 2, "putRetval");
    phi->addIncoming(listRetval, listBB);
    phi->addIncoming(wordRetval, wordBB);
    return phi;
}

EXPORTC addr_t fputList(addr_t eAddr, addr_t thingAddr, addr_t listAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *thing = reinterpret_cast<Datum*>(thingAddr);
    List *list = reinterpret_cast<List*>(listAddr);

    List *retval = new List(DatumPtr(thing), list);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t lputList(addr_t eAddr, addr_t thingAddr, addr_t listAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *thing = reinterpret_cast<Datum*>(thingAddr);
    List *list = reinterpret_cast<List*>(listAddr);

    List *retval = new List();
    ListBuilder builder(retval);
    ListIterator it = list->newIterator();

    while (it.elementExists())
    {
        builder.append(it.element());
    }
    builder.append(DatumPtr(thing));

    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}



/***DOC ARRAY
ARRAY size
(ARRAY size origin)

    outputs an array of "size" members (must be a positive integer),
    each of which initially is an empty list.  Array members can be
    selected with ITEM and changed with SETITEM.  The first member of
    the array is member number 1 unless an "origin" input (must be an
    integer) is given, in which case the first member of the array has
    that number as its index.  (Typically 0 is used as the origin if
    anything.)  Arrays are printed by PRINT and friends, and can be
    typed in, inside curly braces; indicate an origin with {a b c}@0.

COD***/
// CMD ARRAY 1 1 2 d
Value *Compiler::genArray(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *size = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *origin = nullptr;
    if (node.astnodeValue()->countOfChildren() == 2)
    {
        origin = generateChild(node.astnodeValue(), 1, RequestReturnReal);
        origin = generateInt32FromDouble(node.astnodeValue(), origin, true);
    } else {
        origin = CoInt32(1);
    }

    size = generateNotNegativeFromDouble(node.astnodeValue(), size);
    size = generateInt32FromDouble(node.astnodeValue(), size, true);


    return generateCallExtern(TyAddr, "createArray", {PaAddr(evaluator), PaInt32(size), PaInt32(origin)});
}

EXPORTC addr_t createArray(addr_t eAddr, int32_t size, int32_t origin)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Array *retval = new Array(origin, size);
    for (int i = 0; i < size; ++i)
    {
        retval->array.append(DatumPtr(new List()));
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}



/***DOC LISTTOARRAY
LISTTOARRAY list
(LISTTOARRAY list origin)

    outputs an array of the same size as the input list, whose members
    are the members of the input list.

COD***/
// CMD LISTTOARRAY 1 1 2 d
Value *Compiler::genListtoarray(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *list = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    list = generateListFromDatum(node.astnodeValue(), list);
    Value *origin = nullptr;
    if (node.astnodeValue()->countOfChildren() == 2)
    {
        origin = generateChild(node.astnodeValue(), 1, RequestReturnReal);
        origin = generateInt32FromDouble(node.astnodeValue(), origin, true);
    } else {
        origin = CoInt32(1);
    }

    return generateCallExtern(TyAddr, "listToArray", {PaAddr(evaluator), PaAddr(list), PaInt32(origin)});
}

EXPORTC addr_t listToArray(addr_t eAddr, addr_t listAddr, int32_t origin)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    List *list = reinterpret_cast<List*>(listAddr);
    Array *retval = new Array(origin, list->count());
    ListIterator it = list->newIterator();
    while (it.elementExists())
    {
        retval->array.append(it.element());
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}


/***DOC ARRAYTOLIST
ARRAYTOLIST array

    outputs a list whose members are the members of the input array.
    The first member of the output is the first member of the array,
    regardless of the array's origin.

COD***/
// CMD ARRAYTOLIST 1 1 1 d
Value *Compiler::genArraytolist(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *array = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    array = generateArrayFromDatum(node.astnodeValue(), array);
    return generateCallExtern(TyAddr, "arrayToList", {PaAddr(evaluator), PaAddr(array)});
}

EXPORTC addr_t arrayToList(addr_t eAddr, addr_t arrayAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Array *array = reinterpret_cast<Array*>(arrayAddr);
    List *retval = new List();
    ListBuilder builder(retval);
    for (int i = 0; i < array->array.size(); ++i)
    {
        builder.append(array->array[i]);
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

/***DOC FIRST
FIRST thing

    if the input is a word, outputs the first character of the word.
    If the input is a list, outputs the first member of the list.
    If the input is an array, outputs the origin of the array (that
    is, the INDEX OF the first member of the array).

COD***/
// CMD FIRST 1 1 1 d
Value *Compiler::genFirst(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    
    auto validator = [this](Value *thing) {
        Value *isEmpty = generateCallExtern(TyBool, "isDatumEmpty", {PaAddr(evaluator), PaAddr(thing)});
        return scaff->builder.CreateICmpEQ(isEmpty, CoBool(false), "isDatumEmptyCond");
    };
    thing = generateValidationDatum(node.astnodeValue(), thing, validator);

    return generateCallExtern(TyAddr, "firstOfDatum", {PaAddr(evaluator), PaAddr(thing)});
}

EXPORTC addr_t firstOfDatum(addr_t eAddr, addr_t thingAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *thing = reinterpret_cast<Datum*>(thingAddr);
    Datum *retval = nullptr;
    switch (thing->isa)
    {
        case Datum::typeWord:
        {
            Word *w = reinterpret_cast<Word*>(thing);
            retval = new Word(w->rawValue().left(1));
            break;
        }
        case Datum::typeList:
        {
            List *l = reinterpret_cast<List*>(thing);
            retval = l->head.datumValue();
            break;
        }
        case Datum::typeArray:
        {
            Array *a = reinterpret_cast<Array*>(thing);
            retval = new Word(QString::number(a->origin));
            break;
        }
        default:
            Q_ASSERT(false);
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}


/***DOC LAST
LAST wordorlist

    if the input is a word, outputs the last character of the word.
    If the input is a list, outputs the last member of the list.

COD***/
// CMD LAST 1 1 1 d
Value *Compiler::genLast(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *wordorlist = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    
    wordorlist = generateNotEmptyWordOrListFromDatum(node.astnodeValue(), wordorlist);
    
    return generateCallExtern(TyAddr, "lastOfDatum", {PaAddr(evaluator), PaAddr(wordorlist)});
}

EXPORTC addr_t lastOfDatum(addr_t eAddr, addr_t thingAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *thing = reinterpret_cast<Datum*>(thingAddr);
    Datum *retval = nullptr;
    switch (thing->isa)
    {
        case Datum::typeWord:
        {
            Word *w = reinterpret_cast<Word*>(thing);
            retval = new Word(w->rawValue().right(1));
            break;
        }
        case Datum::typeList:
        {
            List *l = reinterpret_cast<List*>(thing);
            ListIterator iter = l->newIterator();
            DatumPtr lastElement;
            while (iter.elementExists())
            {
                lastElement = iter.element();
            }
            retval = lastElement.datumValue();
            break;
        }
        default:
            Q_ASSERT(false);
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}


/***DOC BUTFIRST BF
BUTFIRST wordorlist
BF wordorlist

    if the input is a word, outputs a word containing all but the first
    character of the input.  If the input is a list, outputs a list
    containing all but the first member of the input.

COD***/
// CMD BUTFIRST 1 1 1 d
// CMD BF 1 1 1 d
Value *Compiler::genButfirst(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *wordorlist = generateChild(node.astnodeValue(), 0, RequestReturnDatum);

    wordorlist = generateNotEmptyWordOrListFromDatum(node.astnodeValue(), wordorlist);
    
    return generateCallExtern(TyAddr, "butFirstOfDatum", {PaAddr(evaluator), PaAddr(wordorlist)});
}

EXPORTC addr_t butFirstOfDatum(addr_t eAddr, addr_t thingAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *thing = reinterpret_cast<Datum*>(thingAddr);
    Datum *retval = nullptr;
    switch (thing->isa)
    {
        case Datum::typeWord:
        {
            Word *w = reinterpret_cast<Word*>(thing);
            QString rawValue = w->rawValue();
            retval = new Word(rawValue.sliced(1, rawValue.size() - 1));
            break;
        }
        case Datum::typeList:
        {
            List *l = reinterpret_cast<List*>(thing);
            retval = l->tail.datumValue();
            break;
        }
        default:
            Q_ASSERT(false);
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}



/***DOC BUTLAST BL
BUTLAST wordorlist
BL wordorlist

    if the input is a word, outputs a word containing all but the last
    character of the input.  If the input is a list, outputs a list
    containing all but the last member of the input.

COD***/
// CMD BUTLAST 1 1 1 d
// CMD BL 1 1 1 d
Value *Compiler::genButlast(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *wordorlist = generateChild(node.astnodeValue(), 0, RequestReturnDatum);

    wordorlist = generateNotEmptyWordOrListFromDatum(node.astnodeValue(), wordorlist);

    return generateCallExtern(TyAddr, "butLastOfDatum", {PaAddr(evaluator), PaAddr(wordorlist)});
}

EXPORTC addr_t butLastOfDatum(addr_t eAddr, addr_t thingAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *thing = reinterpret_cast<Datum*>(thingAddr);
    Datum *retval = nullptr;
    switch (thing->isa)
    {
        case Datum::typeWord:
        {
            Word *w = reinterpret_cast<Word*>(thing);
            QString rawValue = w->rawValue();
            retval = new Word(rawValue.sliced(0, rawValue.size() - 1));
            break;
        }
        case Datum::typeList:
        {
            List *l = reinterpret_cast<List*>(thing);
            ListIterator iter = l->newIterator();
            retval = new List();
            ListBuilder builder(reinterpret_cast<List*>(retval));
            while (iter.elementExists())
            {
                DatumPtr element = iter.element();
                if (iter.elementExists())
                {
                    builder.append(element);
                }
            }
            break;
        }
        default:
            Q_ASSERT(false);
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}


/***DOC ITEM
ITEM index thing

    if the "thing" is a word, outputs the "index"th character of the
    word.  If the "thing" is a list, outputs the "index"th member of
    the list.  If the "thing" is an array, outputs the "index"th
    member of the array.  "Index" starts at 1 for words and lists;
    the starting index of an array is specified when the array is
    created.

COD***/
// CMD ITEM 2 2 2 d
Value *Compiler::genItem(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *index = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *thing = generateChild(node.astnodeValue(), 1, RequestReturnDatum);

    // Instead of iterating a list twice, we'll save the list item if we find it when counting.
    Datum *listItem = nullptr;
    Datum **listItemPtr = &listItem;

    auto validator = [this, listItemPtr, thing](Value *index) {
        Value *isValid = generateCallExtern(TyBool, "isDatumIndexValid",
                        {PaAddr(evaluator), PaAddr(thing), PaDouble(index), PaAddr(CoAddr(listItemPtr))});
        return scaff->builder.CreateICmpEQ(isValid, CoBool(true), "isDatumIndexValidCond");
    };
    index = generateValidationDouble(node.astnodeValue(), index, validator);

    return generateCallExtern(TyAddr, "itemOfDatum", {PaAddr(evaluator), PaAddr(thing), PaDouble(index), PaAddr(CoAddr(listItemPtr))});
}

EXPORTC bool isDatumIndexValid(addr_t eAddr, addr_t thingAddr, double dIndex, addr_t listItemPtrAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *thing = reinterpret_cast<Datum*>(thingAddr);
    Datum **listItemPtr = reinterpret_cast<Datum**>(listItemPtrAddr);
    qsizetype index = static_cast<qsizetype>(dIndex);
    if (index != dIndex) return false;

    switch (thing->isa) {
        case Datum::typeWord:
        {
            Word *w = reinterpret_cast<Word*>(thing);
            QString rawValue = w->rawValue();
            return (index >= 1) && (index <= rawValue.size());
        }
        case Datum::typeList:
        {
            List *l = reinterpret_cast<List*>(thing);
            if (index < 1) return false;
            ListIterator iter = l->newIterator();
            while (iter.elementExists()) {
                *listItemPtr = iter.element().datumValue();
                index--;
                if (index == 0) return true;
            }
            return false;
        }
        case Datum::typeArray:
        {
            Array *a = reinterpret_cast<Array*>(thing);
            int32_t size = static_cast<int32_t>(a->array.size());
            index = index - a->origin;
            return (index >= 0) && (index < size);
        }
        default:
            Q_ASSERT(false);
    }
    return false;
}

EXPORTC addr_t itemOfDatum(addr_t eAddr, addr_t thingAddr, double dIndex, addr_t listItemPtrAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *thing = reinterpret_cast<Datum*>(thingAddr);
    Datum *retval = nullptr;
    qsizetype index = static_cast<qsizetype>(dIndex);

    switch (thing->isa) {
        case Datum::typeWord:
        {
            Word *w = reinterpret_cast<Word*>(thing);
            QString rawValue = w->rawValue();
            retval = new Word(rawValue[index - 1]);
            break;
        }
        case Datum::typeList:
        {
            Datum **retvalPtr = reinterpret_cast<Datum**>(listItemPtrAddr);
            retval = *retvalPtr;
            break;
        }
        case Datum::typeArray:
        {
            Array *a = reinterpret_cast<Array*>(thing);
            index = index - a->origin;
            retval = a->array[index].datumValue();
            break;
        }
        default:
            Q_ASSERT(false);
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

/***DOC SETITEM
SETITEM index array value

    command.  Replaces the "index"th member of "array" with the new
    "value".  Ensures that the resulting array is not circular, i.e.,
    "value" may not be a list or array that contains "array".

COD***/
/***DOC .SETITEM
.SETITEM index array value

    command.  Changes the "index"th member of "array" to be "value",
    like SETITEM, but without checking for circularity.

    WARNING: Primitives whose names start with a period are DANGEROUS.
    Their use by non-experts is not recommended.  The use of .SETITEM
    can lead to circular arrays, which will get some Logo primitives into
    infinite loops.

COD***/
// CMD .SETITEM 3 3 3 n
// CMD SETITEM 3 3 3 n
Value *Compiler::genSetitem(DatumPtr node, RequestReturnType returnType)
{
    QChar nodeNameFirstLetter = node.astnodeValue()->nodeName.wordValue()->keyValue().front();
    bool isDangerous = nodeNameFirstLetter == '.';

    Q_ASSERT(returnType && RequestReturnDatum);
    Value *index = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *array = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    array = generateArrayFromDatum(node.astnodeValue(), array);
    
    auto indexValidator = [this, array](Value *index) {
        Value *isValid = generateCallExtern(TyBool, "isDatumIndexValid",
                        {PaAddr(evaluator), PaAddr(array), PaDouble(index), PaAddr(CoAddr(0))});
        return scaff->builder.CreateICmpEQ(isValid, CoBool(true), "isDatumIndexValidCond");
    };
    index = generateValidationDouble(node.astnodeValue(), index, indexValidator);

    Value *value = generateChild(node.astnodeValue(), 2, RequestReturnDatum);

    if ( ! isDangerous) {
        auto valueValidator = [this, array](Value *value) {
            Value *isValid = generateCallExtern(TyBool, "isDatumContainerOrInContainer",
                            {PaAddr(evaluator), PaAddr(array), PaAddr(value)});
            return scaff->builder.CreateICmpEQ(isValid, CoBool(false), "isDatumInContainerCond");
        };
        value = generateValidationDatum(node.astnodeValue(), value, valueValidator);
    }

    generateCallExtern(TyVoid, "setDatumAtIndexOfContainer", {PaAddr(evaluator), PaAddr(value), PaDouble(index), PaAddr(array)});
    return generateVoidRetval(node.astnodeValue());
}

EXPORTC bool isDatumContainerOrInContainer(addr_t eAddr, addr_t valueAddr, addr_t containerAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *value = reinterpret_cast<Datum*>(valueAddr);
    Datum *container = reinterpret_cast<Datum*>(containerAddr);

    // If not a container then there's no container to search.
    if (container->isa == Datum::typeWord)
        return false;

    // If value and container are the same then it's in the container.
    if (value == container)
        return true;

    Qt::CaseSensitivity cs = e->varCASEIGNOREDP() ? Qt::CaseInsensitive : Qt::CaseSensitive;

    return e->isDatumInContainer(value, container, cs);
}

EXPORTC void setDatumAtIndexOfContainer(addr_t eAddr, addr_t valueAddr, double dIndex, addr_t containerAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *container = reinterpret_cast<Datum*>(containerAddr);
    DatumPtr value(reinterpret_cast<Datum*>(valueAddr));
    qsizetype index = static_cast<qsizetype>(dIndex);

    switch (container->isa) {
        case Datum::typeList:
        {
            List *l = reinterpret_cast<List*>(container);
            for (qsizetype i = 1; i < index; ++i) {
                l = l->tail.listValue();
            }
            l->head = value;
            break;
        }
        case Datum::typeArray:
        {
            Array *a = reinterpret_cast<Array*>(container);
            a->array[index - a->origin] = value;
            break;
        }
        default:
            Q_ASSERT(false);
    }
}



/***DOC .SETFIRST
.SETFIRST list value

    command.  Changes the first member of "list" to be "value".

    WARNING:  Primitives whose names start with a period are DANGEROUS.
    Their use by non-experts is not recommended.  The use of .SETFIRST can
    lead to circular list structures, which will get some Logo primitives
    into infinite loops, and to unexpected changes to other data
    structures that share storage with the list being modified.

COD***/
// CMD .SETFIRST 2 2 2 n
Value *Compiler::genDotSetfirst(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *list = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *value = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    list = generateNotEmptyListFromDatum(node.astnodeValue(), list);
    generateCallExtern(TyVoid, "setFirstOfList", {PaAddr(evaluator), PaAddr(list), PaAddr(value)});
    return generateVoidRetval(node.astnodeValue());
}

EXPORTC void setFirstOfList(addr_t eAddr, addr_t listAddr, addr_t valueAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    List *l = reinterpret_cast<List*>(listAddr);
    l->head = DatumPtr(reinterpret_cast<Datum*>(valueAddr));
}



/***DOC .SETBF
.SETBF list value

    command.  Changes the butfirst of "list" to be "value".

    WARNING: Primitives whose names start with a period are DANGEROUS.
    Their use by non-experts is not recommended.  The use of .SETBF can
    lead to circular list structures, which will get some Logo primitives
    into infinite loops; unexpected changes to other data structures that
    share storage with the list being modified; or to Logo crashes and
    coredumps if the butfirst of a list is not itself a list.

COD***/
// CMD .SETBF 2 2 2 n
Value *Compiler::genDotSetbf(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *list = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *value = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    list = generateNotEmptyListFromDatum(node.astnodeValue(), list);
    value = generateListFromDatum(node.astnodeValue(), value);
    generateCallExtern(TyVoid, "setButfirstOfList", {PaAddr(evaluator), PaAddr(list), PaAddr(value)});
    return generateVoidRetval(node.astnodeValue());
}

EXPORTC void setButfirstOfList(addr_t eAddr, addr_t listAddr, addr_t valueAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    List *l = reinterpret_cast<List*>(listAddr);
    l->tail = DatumPtr(reinterpret_cast<Datum*>(valueAddr));
}



/***DOC WORDP WORD?
WORDP thing
WORD? thing

    outputs TRUE if the input is a word, FALSE otherwise.

COD***/
/***DOC LISTP LIST?
LISTP thing
LIST? thing

    outputs TRUE if the input is a list, FALSE otherwise.

COD***/
/***DOC ARRAYP ARRAY?
ARRAYP thing
ARRAY? thing

    outputs TRUE if the input is an array, FALSE otherwise.

COD***/
// CMD ARRAYP 1 1 1 b
// CMD ARRAY? 1 1 1 b
// CMD LISTP 1 1 1 b
// CMD LIST? 1 1 1 b
// CMD WORDP 1 1 1 b
// CMD WORD? 1 1 1 b
Value *Compiler::genWordListArrayp(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);

    int nodeNameFirstLetter = static_cast<int>(node.astnodeValue()->nodeName.wordValue()->keyValue().front().unicode());
    Datum::DatumType type;
    switch (nodeNameFirstLetter) {
        case 'W':
            type = Datum::typeWord;
            break;
        case 'L':
            type = Datum::typeList;
            break;
        case 'A':
            type = Datum::typeArray;
            break;
        default:
            Q_ASSERT(false);
    }

    Value *thingType = generateGetDatumIsa(thing);
    Value *isType = scaff->builder.CreateICmpEQ(thingType, CoInt32(type), "isDatumTypeCond");
    return scaff->builder.CreateSelect(isType, CoBool(true), CoBool(false), "isDatumTypeResult");
}


/***DOC BEFOREP BEFORE?
BEFOREP word1 word2
BEFORE? word1 word2

    outputs TRUE if word1 comes before word2 in ASCII collating sequence
    (for words of letters, in alphabetical order).  Case-sensitivity is
    determined by the value of CASEIGNOREDP.  Note that if the inputs are
    numbers, the result may not be the same as with LESSP; for example,
    BEFOREP 3 12 is false because 3 collates after 1.

COD***/
// CMD BEFOREP 2 2 2 b
// CMD BEFORE? 2 2 2 b
Value *Compiler::genBeforep(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *word1 = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *word2 = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    word1 = generateWordFromDatum(node.astnodeValue(), word1);
    word2 = generateWordFromDatum(node.astnodeValue(), word2);
    return generateCallExtern(TyBool, "isBefore", {PaAddr(evaluator), PaAddr(word1), PaAddr(word2)});
}

EXPORTC bool isBefore(addr_t eAddr, addr_t word1Addr, addr_t word2Addr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Word *word1 = reinterpret_cast<Word*>(word1Addr);
    Word *word2 = reinterpret_cast<Word*>(word2Addr);

    Qt::CaseSensitivity cs = e->varCASEIGNOREDP() ? Qt::CaseInsensitive : Qt::CaseSensitive;

    QString value1 = word1->printValue();
    QString value2 = word2->printValue();
    return value1.compare(value2, cs) < 0;
}


/***DOC .EQ
.EQ thing1 thing2

    outputs TRUE if its two inputs are the same datum, so that applying a
    mutator to one will change the other as well.  Outputs FALSE otherwise,
    even if the inputs are equal in value.
    WARNING: Primitives whose names start with a period are DANGEROUS.
    Their use by non-experts is not recommended.  The use of mutators
    can lead to circular data structures, infinite loops, or Logo crashes.

COD***/
// CMD .EQ 2 2 2 b
Value *Compiler::genDotEq(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing1 = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *thing2 = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    Value *isEqualCond = scaff->builder.CreateICmpEQ(thing1, thing2, "isEqualCond");
    return scaff->builder.CreateSelect(isEqualCond, CoBool(true), CoBool(false), "isEqualResult");
}


/***DOC MEMBERP MEMBER?
MEMBERP thing1 thing2
MEMBER? thing1 thing2

    if "thing2" is a list or an array, outputs TRUE if "thing1" is EQUALP
    to a member of "thing2", FALSE otherwise.  If "thing2" is
    a word, outputs TRUE if "thing1" is a one-character word EQUALP to a
    character of "thing2", FALSE otherwise.

COD***/
// CMD MEMBERP 2 2 2 b
// CMD MEMBER? 2 2 2 b
Value *Compiler::genMemberp(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing1 = generateChild(node.astnodeValue(), 0, RequestReturnDatum); // thing
    Value *thing2 = generateChild(node.astnodeValue(), 1, RequestReturnDatum); // container
    return generateCallExtern(TyBool, "isMember", {PaAddr(evaluator), PaAddr(thing1), PaAddr(thing2)});
}

EXPORTC bool isMember(addr_t eAddr, addr_t thingAddr, addr_t containerAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *thing = reinterpret_cast<Datum*>(thingAddr);
    Datum *container = reinterpret_cast<Datum*>(containerAddr);

    switch (container->isa) {
        case Datum::typeWord:
        {
            Word *word = reinterpret_cast<Word*>(container);
            QString containerString = word->keyValue();
            if (thing->isa == Datum::typeWord) {
                Word *word = reinterpret_cast<Word*>(thing);
                QString thingString = word->keyValue();
                if (thingString.length() != 1) {
                    return false;
                }
                return containerString.contains(thingString);
            }
            return false;
        }
        case Datum::typeList:
        {
            List *list = reinterpret_cast<List*>(container);
            ListIterator iter(list);
            while (iter.elementExists()) {
                Datum *item = iter.element().datumValue();
                addr_t itemAddr = reinterpret_cast<addr_t>(item);
                if (cmpDatumToDatum(eAddr, thingAddr, itemAddr)) {
                    return true;
                }
            }
            return false;
        }
        case Datum::typeArray:
        {
            Array *array = reinterpret_cast<Array*>(container);
            for (auto item : array->array) {
                if (cmpDatumToDatum(eAddr, thingAddr, reinterpret_cast<addr_t>(item.datumValue()))) {
                    return true;
                }
            }
            return false;
        }
        default:
            Q_ASSERT(false);
    }
    Q_ASSERT(false);
    return false;
}


/***DOC SUBSTRINGP SUBSTRING?
SUBSTRINGP thing1 thing2
SUBSTRING? thing1 thing2

    if "thing1" or "thing2" is a list or an array, outputs FALSE.  If
    "thing2" is a word, outputs TRUE if "thing1" is EQUALP to a
    substring of "thing2", FALSE otherwise.

COD***/
// CMD SUBSTRINGP 2 2 2 b
// CMD SUBSTRING? 2 2 2 b
Value *Compiler::genSubstringp(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing1 = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *thing2 = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    return generateCallExtern(TyBool, "isSubstring", {PaAddr(evaluator), PaAddr(thing1), PaAddr(thing2)});
}

EXPORTC bool isSubstring(addr_t eAddr, addr_t thing1Addr, addr_t thing2Addr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *thing1 = reinterpret_cast<Datum*>(thing1Addr);
    Datum *thing2 = reinterpret_cast<Datum*>(thing2Addr);

    if (thing1->isa == Datum::typeWord && thing2->isa == Datum::typeWord) {
        Word *word1 = reinterpret_cast<Word*>(thing1);
        Word *word2 = reinterpret_cast<Word*>(thing2);
        QString string1 = word1->keyValue();
        QString string2 = word2->keyValue();
        return string2.contains(string1);
    }
    return false;
}


/***DOC NUMBERP NUMBER?
NUMBERP thing
NUMBER? thing

    outputs TRUE if the input is a number, FALSE otherwise.

COD***/
// CMD NUMBERP 1 1 1 b
// CMD NUMBER? 1 1 1 b
Value *Compiler::genNumberp(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    return generateCallExtern(TyBool, "isNumber", {PaAddr(evaluator), PaAddr(thing)});
}

EXPORTC bool isNumber(addr_t eAddr, addr_t thingAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *thing = reinterpret_cast<Datum*>(thingAddr);
    if (thing->isa != Datum::typeWord) {
        return false;
    }
    Word *word = reinterpret_cast<Word*>(thing);
    word->numberValue();
    return word->numberIsValid;
}



/***DOC VBARREDP VBARRED? BACKSLASHEDP BACKSLASHED?
VBARREDP char
VBARRED? char
BACKSLASHEDP char                               (library procedure)
BACKSLASHED? char                               (library procedure)

    outputs TRUE if the input character was originally entered into Logo
    within vertical bars (|) to prevent its usual special syntactic
    meaning, FALSE otherwise.  (Outputs TRUE only if the character is a
    backslashed space, tab, newline, or one of ()[]+-/=*<>":;\~?| )

    The names BACKSLASHEDP and BACKSLASHED? are included in the Logo
    library for backward compatibility with the former names of this
    primitive, although it does *not* output TRUE for characters
    originally entered with backslashes.


COD***/
// CMD VBARREDP 1 1 1 b
// CMD VBARRED? 1 1 1 b
Value *Compiler::genVbarredp(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *c = generateChild(node.astnodeValue(), 0, RequestReturnDatum);

    auto validator = [this](Value *candidate) {
        Value *isGoodChar = generateCallExtern(TyBool, "isSingleCharWord", {PaAddr(evaluator), PaAddr(candidate)});
        return scaff->builder.CreateICmpEQ(isGoodChar, CoBool(true), "isGoodCond");
    };
    c = generateValidationDatum(node.astnodeValue(), c, validator);

    return generateCallExtern(TyBool, "isVbarred", {PaAddr(evaluator), PaAddr(c)});
}

EXPORTC bool isSingleCharWord(addr_t eAddr, addr_t candidateAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *candidate = reinterpret_cast<Datum*>(candidateAddr);
    if (candidate->isa != Datum::typeWord) {
        return false;
    }
    Word *word = reinterpret_cast<Word*>(candidate);
    return word->keyValue().length() == 1;
}

EXPORTC bool isVbarred(addr_t eAddr, addr_t cAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Word *word = reinterpret_cast<Word*>(cAddr);

    // A character is vbarred IFF it's print value is different from its raw value.
    char16_t rawC = word->rawValue().front().unicode();
    char16_t c = word->printValue().front().unicode();
    return rawC != c;
}


/***DOC COUNT
COUNT thing

    outputs the number of characters in the input, if the input is a word;
    outputs the number of members in the input, if it is a list
    or an array.  (For an array, this may or may not be the index of the
    last member, depending on the array's origin.)

COD***/
// CMD COUNT 1 1 1 n
Value *Compiler::genCount(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    return generateCallExtern(TyDouble, "count", {PaAddr(evaluator), PaAddr(thing)});
}

EXPORTC double count(addr_t eAddr, addr_t thingAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *thing = reinterpret_cast<Datum*>(thingAddr);
    switch (thing->isa) {
        case Datum::typeWord:
        {
            Word *word = reinterpret_cast<Word*>(thing);
            return word->rawValue().length();
        }
        case Datum::typeList:
        {
            List *list = reinterpret_cast<List*>(thing);
            return list->count();
        }
        case Datum::typeArray:
        {
            Array *array = reinterpret_cast<Array*>(thing);
            return array->array.size();
        }
        default:
            Q_ASSERT(false);
    }
    Q_ASSERT(false);
    return 0;
}


/***DOC ASCII
ASCII char

    outputs the integer (between 0 and 65535) that represents the input
    character in Unicode.  Interprets control characters as
    representing vbarred punctuation, and returns the character code
    for the corresponding punctuation character without vertical bars.
    (Compare RAWASCII.)

    Even though QLogo uses Unicode instead of ASCII, the primitives ASCII,
    RAWASCII, and CHAR are maintained for compatibility with UCBLogo and
    because ASCII is a proper subset of Unicode.

COD***/
// CMD ASCII 1 1 1 n
Value *Compiler::genAscii(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *c = generateChild(node.astnodeValue(), 0, RequestReturnDatum);

    auto validator = [this](Value *candidate) {
        Value *isGoodChar = generateCallExtern(TyBool, "isSingleCharWord", {PaAddr(evaluator), PaAddr(candidate)});
        return scaff->builder.CreateICmpEQ(isGoodChar, CoBool(true), "isGoodCond");
    };
    c = generateValidationDatum(node.astnodeValue(), c, validator);

    return generateCallExtern(TyDouble, "ascii", {PaAddr(evaluator), PaAddr(c)});
}

EXPORTC double ascii(addr_t eAddr, addr_t cAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Word *word = reinterpret_cast<Word*>(cAddr);
    return word->printValue().front().unicode();
}



/***DOC RAWASCII
RAWASCII char

    outputs the integer (between 0 and 65535) that represents the input
    character in Unicode.  Interprets control characters as
    representing themselves.  To find out the Unicode value of an arbitrary
    keystroke, use RAWASCII RC.

    See ASCII for discussion of Unicode characters.

COD***/
// CMD RAWASCII 1 1 1 n
Value *Compiler::genRawascii(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *c = generateChild(node.astnodeValue(), 0, RequestReturnDatum);

    auto validator = [this](Value *candidate) {
        Value *isGoodChar = generateCallExtern(TyBool, "isSingleCharWord", {PaAddr(evaluator), PaAddr(candidate)});
        return scaff->builder.CreateICmpEQ(isGoodChar, CoBool(true), "isGoodCond");
    };
    c = generateValidationDatum(node.astnodeValue(), c, validator);

    return generateCallExtern(TyDouble, "rawascii", {PaAddr(evaluator), PaAddr(c)});
}

EXPORTC double rawascii(addr_t eAddr, addr_t cAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Word *word = reinterpret_cast<Word*>(cAddr);
    return word->rawValue().front().unicode();
}


/***DOC CHAR
CHAR int

    outputs the character represented in Unicode by the input,
    which must be an integer between 0 and 65535.

    See ASCII for discussion of Unicode characters.

COD***/
// CMD CHAR 1 1 1 d
Value *Compiler::genChar(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *c = generateChild(node.astnodeValue(), 0, RequestReturnReal);

    auto validator = [this](Value *candidate) {
        BasicBlock *startBB = scaff->builder.GetInsertBlock();
        Function *theFunction = startBB->getParent();

        BasicBlock *geZeroBB = BasicBlock::Create(*scaff->theContext, "geZero", theFunction);
        BasicBlock *isIntBB = BasicBlock::Create(*scaff->theContext, "isInt", theFunction);
        BasicBlock *leMaxBB = BasicBlock::Create(*scaff->theContext, "leMax", theFunction);
        BasicBlock *endBB = BasicBlock::Create(*scaff->theContext, "end", theFunction);

        // Check if >= 0
        Value *geZero = scaff->builder.CreateFCmpOGE(candidate, CoDouble(0.0), "geZero");
        scaff->builder.CreateCondBr(geZero, geZeroBB, endBB);

        // Check if it's an integer (equals its floor)
        scaff->builder.SetInsertPoint(geZeroBB);
        Value *floor = generateCallExtern(TyDouble, "floor", {PaDouble(candidate)});
        Value *isInt = scaff->builder.CreateFCmpOEQ(candidate, floor, "isInt");
        scaff->builder.CreateCondBr(isInt, isIntBB, endBB);

        // Check if <= 65535
        scaff->builder.SetInsertPoint(isIntBB);
        Value *leMax = scaff->builder.CreateFCmpOLE(candidate, CoDouble((double)UINT16_MAX), "leMax");
        scaff->builder.CreateCondBr(leMax, leMaxBB, endBB);

        // If both checks pass, return true
        scaff->builder.SetInsertPoint(leMaxBB);
        scaff->builder.CreateBr(endBB);

        // Merge block
        scaff->builder.SetInsertPoint(endBB);
        PHINode *phi = scaff->builder.CreatePHI(Type::getInt1Ty(*scaff->theContext), 4, "isValid");
        phi->addIncoming(CoBool(false), startBB);
        phi->addIncoming(CoBool(false), geZeroBB);
        phi->addIncoming(CoBool(false), isIntBB);
        phi->addIncoming(CoBool(true), leMaxBB);
        return phi;
    };
    c = generateValidationDouble(node.astnodeValue(), c, validator);
    return generateCallExtern(TyAddr, "chr", {PaAddr(evaluator), PaDouble(c)});
}

EXPORTC addr_t chr(addr_t eAddr, double c)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    QChar qc = QChar(static_cast<uint16_t>(c));
    QString qstr = QString(qc);
    Word *retval = new Word(qstr);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}


/***DOC MEMBER
MEMBER thing1 thing2

    if "thing2" is a word or list and if MEMBERP with these inputs would
    output TRUE, outputs the portion of "thing2" from the first instance
    of "thing1" to the end.  If MEMBERP would output FALSE, outputs the
    empty word or list according to the type of "thing2".  It is an error
    for "thing2" to be an array.

COD***/
// CMD MEMBER 2 2 2 d
Value *Compiler::genMember(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing1 = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *thing2 = generateChild(node.astnodeValue(), 1, RequestReturnDatum);

    thing2 = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), thing2);
    return generateCallExtern(TyAddr, "member", {PaAddr(evaluator), PaAddr(thing1), PaAddr(thing2)});
}

EXPORTC addr_t member(addr_t eAddr, addr_t thing1Addr, addr_t thing2Addr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *thing1 = reinterpret_cast<Datum*>(thing1Addr);
    Datum *thing2 = reinterpret_cast<Datum*>(thing2Addr);
    switch (thing2->isa) {
        case Datum::typeWord:
        {
            Word *word1 = reinterpret_cast<Word*>(thing1);
            Word *word2 = reinterpret_cast<Word*>(thing2);
            QString retval = "";
            if (word1->isa == Datum::typeWord) {
                QString thing2Str = word2->rawValue();
                QString thing1Str = reinterpret_cast<Word*>(thing1)->rawValue();
                if ( ! thing1Str.isEmpty()) {
                    int index = thing2Str.indexOf(thing1Str);
                    if (index != -1) {
                        retval = thing2Str.sliced(index);
                    }
                }
            }
            Word *retvalWord = new Word(retval);
            e->watch(retvalWord);
            return reinterpret_cast<addr_t>(retvalWord);
        }
        case Datum::typeList:
        {
            List *list = reinterpret_cast<List*>(thing2);
            while ( ! list->isEmpty()) {
                addr_t listAddr = reinterpret_cast<addr_t>(list->head.datumValue());
                if (cmpDatumToDatum(eAddr, listAddr, thing1Addr)) {
                    return reinterpret_cast<addr_t>(list);
                }
                list = list->tail.listValue();
            }
            // If we get here, thing1 was not found in thing2.
            // Return an empty list.
            List *retval = new List();
            e->watch(retval);
            return reinterpret_cast<addr_t>(retval);
        }
        default:
            Q_ASSERT(false);
    }
    Q_ASSERT(false);
    return nullptr;
}



/***DOC LOWERCASE
LOWERCASE word

    outputs a copy of the input word, but with all uppercase letters
    changed to the corresponding lowercase letter.

COD***/
// CMD LOWERCASE 1 1 1 d
Value *Compiler::genLowercase(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *word = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    word = generateFromDatum(Datum::typeWord, node.astnodeValue(), word);
    return generateCallExtern(TyAddr, "lowercase", {PaAddr(evaluator), PaAddr(word)});
}

EXPORTC addr_t lowercase(addr_t eAddr, addr_t wordAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Word *word = reinterpret_cast<Word*>(wordAddr);
    QString retval = word->rawValue().toLower();
    Word *retvalWord = new Word(retval);
    e->watch(retvalWord);
    return reinterpret_cast<addr_t>(retvalWord);
}


/***DOC UPPERCASE
UPPERCASE word

    outputs a copy of the input word, but with all lowercase letters
    changed to the corresponding uppercase letter.

COD***/
// CMD UPPERCASE 1 1 1 d
Value *Compiler::genUppercase(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *word = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    word = generateFromDatum(Datum::typeWord, node.astnodeValue(), word);
    return generateCallExtern(TyAddr, "uppercase", {PaAddr(evaluator), PaAddr(word), PaBool(CoBool(false))});
}

EXPORTC addr_t uppercase(addr_t eAddr, addr_t wordAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Word *word = reinterpret_cast<Word*>(wordAddr);
    QString retval = word->rawValue().toUpper();
    Word *retvalWord = new Word(retval);
    e->watch(retvalWord);
    return reinterpret_cast<addr_t>(retvalWord);
}


/***DOC STANDOUT
STANDOUT thing

    outputs a word that, when printed, will appear like the input but
    displayed in standout mode (reverse video).  The word contains
    magic characters at the beginning and end; in between is the printed
    form (as if displayed using TYPE) of the input.  The output is always
    a word, even if the input is of some other type, but it may include
    spaces and other formatting characters.

COD***/
// CMD STANDOUT 1 1 1 d
Value *Compiler::genStandout(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    return generateCallExtern(TyAddr, "standout", {PaAddr(evaluator), PaAddr(thing)});
}

EXPORTC addr_t standout(addr_t eAddr, addr_t thingAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Datum *thing = reinterpret_cast<Datum*>(thingAddr);
    QString phrase = thing->printValue();
    QString retval = Config::get().mainController()->addStandoutToString(phrase);
    Word *retvalWord = new Word(retval);
    e->watch(retvalWord);
    return reinterpret_cast<addr_t>(retvalWord);
}


/***DOC PARSE
PARSE word

    outputs the list that would result if the input word were entered
    in response to a READLIST operation.  That is, PARSE READWORD has
    the same value as READLIST for the same characters read.

COD***/
// CMD PARSE 1 1 1 d
Value *Compiler::genParse(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *word = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    word = generateFromDatum(Datum::typeWord, node.astnodeValue(), word);
    return generateCallExtern(TyAddr, "parse", {PaAddr(evaluator), PaAddr(word)});
}

EXPORTC addr_t parse(addr_t eAddr, addr_t wordAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    Word *word = reinterpret_cast<Word*>(wordAddr);
    QString phrase = word->rawValue();
    QTextStream stream(&phrase, QIODevice::ReadOnly);
    TextStream ts(&stream);
    DatumPtr retvalPtr = ts.readlistWithPrompt("", false);
    List *retval = retvalPtr.listValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}


/***DOC RUNPARSE
RUNPARSE wordorlist

    outputs the list that would result if the input word or list were
    entered as an instruction line; characters such as infix operators
    and parentheses are separate members of the output.  Note that
    sublists of a runparsed list are not themselves runparsed.


COD***/
// CMD RUNPARSE 1 1 1 d
Value *Compiler::genRunparse(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *wordorlist = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    wordorlist = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), wordorlist);
    return generateCallExtern(TyAddr, "runparse", {PaAddr(evaluator), PaAddr(wordorlist)});
}

EXPORTC addr_t runparse(addr_t eAddr, addr_t wordorlistAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator*>(eAddr);
    DatumPtr wordorlist = DatumPtr(reinterpret_cast<Datum*>(wordorlistAddr));

    DatumPtr retvalPtr = runparse(wordorlist);
    List *retval = retvalPtr.listValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}