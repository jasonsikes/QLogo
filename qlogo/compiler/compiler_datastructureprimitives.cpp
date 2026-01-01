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

#include "astnode.h"
#include "compiler.h"
#include "compiler_private.h"
#include "controller/logocontroller.h"
#include "controller/textstream.h"
#include "exports.h"
#include "kernel.h"
#include "runparser.h"
#include "sharedconstants.h"
#include "visited.h"
#include "workspace/callframe.h"
#include <QIODevice>

using namespace llvm;
using namespace llvm::orc;
/// @brief Determine if the given Datums are equal, according to `EQUALP` help text.
/// @param visited The set of visited nodes.
/// @param d1 The first Datum to compare.
/// @param d2 The second Datum to compare.
/// @param cs The case sensitivity to use for the comparison.
/// @returns true if the Datums are equal, false otherwise.
bool areDatumsEqual(VisitedMap &visited, Datum *d1, Datum *d2, Qt::CaseSensitivity cs)
{
    if (d1 == d2)
        return true;
    if (d1->isa != d2->isa)
        return false;

    if (d1->isWord())
    {
        Word *w1 = d1->wordValue();
        Word *w2 = d2->wordValue();
        if (w1->isSourceNumber() || w2->isSourceNumber())
            return w1->numberValue() == w2->numberValue();

        return w1->toString().compare(w2->toString(), cs) == 0;
    }
    else if (d1->isList())
    {
        // If we have seen this list before,
        if (visited.contains(d1))
        {
            // Then return true if the comparison is the same as the previous comparison.
            return visited.get(d1) == d2;
        }

        List *l1 = d1->listValue();
        List *l2 = d2->listValue();

        while (l1 != EmptyList::instance() && l2 != EmptyList::instance())
        {
            if (!areDatumsEqual(visited, l1->head.datumValue(), l2->head.datumValue(), cs))
                return false;
            visited.add(l1, l2);
            l1 = l1->tail.listValue();
            l2 = l2->tail.listValue();
        }

        return l1 && l2 && l1->isEmpty() && l2->isEmpty();
    }
    else if (d1->isArray())
    {
        // Arrays are equal iff they are the same array, which would have
        // passed the "datum1 == datum2" test at the beginning.
        return false;
    }
    else
    {
        Q_ASSERT(false);
    }
    return false;
}
Value *Compiler::generateNotEmptyWordOrListFromDatum(ASTNode *parent, Value *src)
{
    auto validator = [this](Value *wordorlist) {
        BasicBlock *startBB = scaff->builder.GetInsertBlock();
        Function *theFunction = startBB->getParent();

        BasicBlock *wordOrListBB = BasicBlock::Create(*scaff->theContext, "wordOrListBlock", theFunction);
        BasicBlock *endBB = BasicBlock::Create(*scaff->theContext, "endBlock", theFunction);

        Value *wordOrListType = generateGetDatumIsa(wordorlist);
        Value *maskCalc =
            scaff->builder.CreateAnd(wordOrListType, CoInt32(Datum::typeWord | Datum::typeList), "maskCalc");
        Value *wordOrListCond = scaff->builder.CreateICmpNE(maskCalc, CoInt32(0), "wordOrListCond");
        scaff->builder.CreateCondBr(wordOrListCond, wordOrListBB, endBB);

        // Word or List block
        scaff->builder.SetInsertPoint(wordOrListBB);
        Value *isEmpty = generateCallExtern(TyBool, isDatumEmpty, PaAddr(evaluator), PaAddr(wordorlist));
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
        Value *isEmpty = generateCallExtern(TyBool, isDatumEmpty, PaAddr(evaluator), PaAddr(wordorlist));
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
Value *Compiler::genEqualp(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnBool);

    Value *thing1 = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *thing2 = generateChild(node.astnodeValue(), 1, RequestReturnDatum);

    Type *typeOfThing1 = thing1->getType();
    Type *typeOfThing2 = thing2->getType();

    // If one is bool and other is double, then they can't be equal.
    if ((typeOfThing1->isIntegerTy(1) && typeOfThing2->isDoubleTy()) ||
        (typeOfThing2->isIntegerTy(1) && typeOfThing1->isDoubleTy()))
    {
        return CoBool(false);
    }

    // Both double? Compare them
    if (typeOfThing1->isDoubleTy() && typeOfThing2->isDoubleTy())
    {
        return scaff->builder.CreateFCmpUEQ(thing1, thing2, "Fequalp");
    }

    // Both bool? Compare them
    if (typeOfThing1->isIntegerTy(1) && typeOfThing2->isIntegerTy(1))
    {
        return scaff->builder.CreateICmpEQ(thing1, thing2, "Bequalp");
    }

    // At this point we know at least one of the inputs is a Datum.
    // For simplicity, make thing1 the Datum, and thing2 can be whatever the other type was.
    if (!typeOfThing1->isPointerTy())
    {
        Value *t = thing1;
        thing1 = thing2;
        thing2 = t;
        typeOfThing2 = thing2->getType();
    }

    if (typeOfThing2->isIntegerTy(1))
    {
        return generateCallExtern(TyBool, cmpDatumToBool, PaAddr(evaluator), PaAddr(thing1), PaBool(thing2));
    }
    if (typeOfThing2->isDoubleTy())
    {
        return generateCallExtern(TyBool, cmpDatumToDouble, PaAddr(evaluator), PaAddr(thing1), PaDouble(thing2));
    }
    Q_ASSERT(typeOfThing2->isPointerTy());
    return generateCallExtern(TyBool, cmpDatumToDatum, PaAddr(evaluator), PaAddr(thing1), PaAddr(thing2));
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
Value *Compiler::genNotequalp(const DatumPtr &node, RequestReturnType returnType)
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
Value *Compiler::genWord(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    AllocaInst *ary = generateChildrenAlloca(node.astnodeValue(), RequestReturnDatum, "wordAry");
    return generateCallExtern(TyAddr, concatWord, PaAddr(evaluator), PaAddr(ary), PaInt32(ary->getArraySize()));
}

/***DOC LIST
LIST thing1 thing2
(LIST thing1 thing2 thing3 ...)

    outputs a list whose members are its inputs, which can be any
    Logo datum (word, list, or array).

COD***/
// CMD LIST 0 2 -1 d
Value *Compiler::genList(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    AllocaInst *ary = generateChildrenAlloca(node.astnodeValue(), RequestReturnDatum, "listAry");
    return generateCallExtern(TyAddr, createList, PaAddr(evaluator), PaAddr(ary), PaInt32(ary->getArraySize()));
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
Value *Compiler::genSentence(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    AllocaInst *ary = generateChildrenAlloca(node.astnodeValue(), RequestReturnDatum, "sentenceAry");
    return generateCallExtern(TyAddr, createSentence, PaAddr(evaluator), PaAddr(ary), PaInt32(ary->getArraySize()));
}
/***DOC FPUT
FPUT thing list

    outputs a list equal to its second input with one extra member,
    the first input, at the beginning.  If the second input is a word,
    then the first input must be a word, and FPUT is equivalent to WORD.

COD***/
// CMD FPUT 2 2 2 d
Value *Compiler::genFput(const DatumPtr &node, RequestReturnType returnType)
{
    return generateFputlput(node, returnType, false);
}

/***DOC LPUT
LPUT thing list

    outputs a list equal to its second input with one extra member,
    the first input, at the end.  If the second input is a word,
    then the first input must be a one-letter word, and LPUT is
    equivalent to WORD with its inputs in the other order.

COD***/
// CMD LPUT 2 2 2 d
Value *Compiler::genLput(const DatumPtr &node, RequestReturnType returnType)
{
    return generateFputlput(node, returnType, true);
}

Value *Compiler::generateFputlput(const DatumPtr &node, RequestReturnType returnType, bool isLput)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *list = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    Value *listWordTest = nullptr;

    auto wordVector = isLput ? std::vector<Value *>{list, thing} : std::vector<Value *>{thing, list};

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
        Value *mask = scaff->builder.CreateAnd(listType, CoInt32(Datum::typeList), "dataTypeMask");
        Value *listListTest = scaff->builder.CreateICmpNE(mask, CoInt32(0), "dataTypeMaskTest");
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
    Value *wordRetval =
        generateCallExtern(TyAddr, concatWord, PaAddr(evaluator), PaAddr(ary), PaInt32(ary->getArraySize()));
    scaff->builder.CreateBr(mergeBB);

    scaff->builder.SetInsertPoint(listBB);
    Value *listRetval = isLput ? generateCallExtern(TyAddr, lputList, PaAddr(evaluator), PaAddr(thing), PaAddr(list))
                               : generateCallExtern(TyAddr, fputList, PaAddr(evaluator), PaAddr(thing), PaAddr(list));
    scaff->builder.CreateBr(mergeBB);

    scaff->builder.SetInsertPoint(mergeBB);
    PHINode *phi = scaff->builder.CreatePHI(list->getType(), 2, "putRetval");
    phi->addIncoming(listRetval, listBB);
    phi->addIncoming(wordRetval, wordBB);
    return phi;
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
Value *Compiler::genArray(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *size = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *origin = nullptr;
    if (node.astnodeValue()->countOfChildren() == 2)
    {
        origin = generateChild(node.astnodeValue(), 1, RequestReturnReal);
        origin = generateInt32FromDouble(node.astnodeValue(), origin, true);
    }
    else
    {
        origin = CoInt32(1);
    }

    size = generateNotNegativeFromDouble(node.astnodeValue(), size);
    size = generateInt32FromDouble(node.astnodeValue(), size, true);

    return generateCallExtern(TyAddr, createArray, PaAddr(evaluator), PaInt32(size), PaInt32(origin));
}
/***DOC LISTTOARRAY
LISTTOARRAY list
(LISTTOARRAY list origin)

    outputs an array of the same size as the input list, whose members
    are the members of the input list.

COD***/
// CMD LISTTOARRAY 1 1 2 d
Value *Compiler::genListtoarray(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *list = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    list = generateListFromDatum(node.astnodeValue(), list);
    Value *origin = nullptr;
    if (node.astnodeValue()->countOfChildren() == 2)
    {
        origin = generateChild(node.astnodeValue(), 1, RequestReturnReal);
        origin = generateInt32FromDouble(node.astnodeValue(), origin, true);
    }
    else
    {
        origin = CoInt32(1);
    }

    return generateCallExtern(TyAddr, listToArray, PaAddr(evaluator), PaAddr(list), PaInt32(origin));
}
/***DOC ARRAYTOLIST
ARRAYTOLIST array

    outputs a list whose members are the members of the input array.
    The first member of the output is the first member of the array,
    regardless of the array's origin.

COD***/
// CMD ARRAYTOLIST 1 1 1 d
Value *Compiler::genArraytolist(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *array = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    array = generateArrayFromDatum(node.astnodeValue(), array);
    return generateCallExtern(TyAddr, arrayToList, PaAddr(evaluator), PaAddr(array));
}
/***DOC FIRST
FIRST thing

    if the input is a word, outputs the first character of the word.
    If the input is a list, outputs the first member of the list.
    If the input is an array, outputs the origin of the array (that
    is, the INDEX OF the first member of the array).

COD***/
// CMD FIRST 1 1 1 d
Value *Compiler::genFirst(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);

    auto validator = [this](Value *thing) {
        Value *isEmpty = generateCallExtern(TyBool, isDatumEmpty, PaAddr(evaluator), PaAddr(thing));
        return scaff->builder.CreateICmpEQ(isEmpty, CoBool(false), "isDatumEmptyCond");
    };
    thing = generateValidationDatum(node.astnodeValue(), thing, validator);

    return generateCallExtern(TyAddr, firstOfDatum, PaAddr(evaluator), PaAddr(thing));
}
/***DOC LAST
LAST wordorlist

    if the input is a word, outputs the last character of the word.
    If the input is a list, outputs the last member of the list.

COD***/
// CMD LAST 1 1 1 d
Value *Compiler::genLast(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *wordorlist = generateChild(node.astnodeValue(), 0, RequestReturnDatum);

    wordorlist = generateNotEmptyWordOrListFromDatum(node.astnodeValue(), wordorlist);

    return generateCallExtern(TyAddr, lastOfDatum, PaAddr(evaluator), PaAddr(wordorlist));
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
Value *Compiler::genButfirst(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *wordorlist = generateChild(node.astnodeValue(), 0, RequestReturnDatum);

    wordorlist = generateNotEmptyWordOrListFromDatum(node.astnodeValue(), wordorlist);

    return generateCallExtern(TyAddr, butFirstOfDatum, PaAddr(evaluator), PaAddr(wordorlist));
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
Value *Compiler::genButlast(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *wordorlist = generateChild(node.astnodeValue(), 0, RequestReturnDatum);

    wordorlist = generateNotEmptyWordOrListFromDatum(node.astnodeValue(), wordorlist);

    return generateCallExtern(TyAddr, butLastOfDatum, PaAddr(evaluator), PaAddr(wordorlist));
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
Value *Compiler::genItem(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *index = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *thing = generateChild(node.astnodeValue(), 1, RequestReturnDatum);

    // Instead of iterating a list twice, we'll save the list item if we find it when counting.
    Datum *listItem = nullptr;
    Datum **listItemPtr = &listItem;

    auto validator = [this, listItemPtr, thing](Value *index) {
        Value *isValid = generateCallExtern(
            TyBool, isDatumIndexValid, PaAddr(evaluator), PaAddr(thing), PaDouble(index), PaAddr(CoAddr(listItemPtr)));
        return scaff->builder.CreateICmpEQ(isValid, CoBool(true), "isDatumIndexValidCond");
    };
    index = generateValidationDouble(node.astnodeValue(), index, validator);

    return generateCallExtern(
        TyAddr, itemOfDatum, PaAddr(evaluator), PaAddr(thing), PaDouble(index), PaAddr(CoAddr(listItemPtr)));
}
/***DOC SETITEM
SETITEM index array value

    command.  Replaces the "index"th member of "array" with the new
    "value".  Ensures that the resulting array is not circular, i.e.,
    "value" may not be a list or array that contains "array".

COD***/
// CMD SETITEM 3 3 3 n
Value *Compiler::genSetitem(const DatumPtr &node, RequestReturnType returnType)
{
    return generateSetitem(node, returnType, false);
}

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
Value *Compiler::genDotSetitem(const DatumPtr &node, RequestReturnType returnType)
{
    return generateSetitem(node, returnType, true);
}

Value *Compiler::generateSetitem(const DatumPtr &node, RequestReturnType returnType, bool isDangerous)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *index = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *array = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    array = generateArrayFromDatum(node.astnodeValue(), array);

    auto indexValidator = [this, array](Value *index) {
        Value *isValid = generateCallExtern(
            TyBool, isDatumIndexValid, PaAddr(evaluator), PaAddr(array), PaDouble(index), PaAddr(CoAddr(0)));
        return scaff->builder.CreateICmpEQ(isValid, CoBool(true), "isDatumIndexValidCond");
    };
    index = generateValidationDouble(node.astnodeValue(), index, indexValidator);

    Value *value = generateChild(node.astnodeValue(), 2, RequestReturnDatum);

    if (!isDangerous)
    {
        auto valueValidator = [this, array](Value *value) {
            Value *isValid = generateCallExtern(
                TyBool, isDatumContainerOrInContainer, PaAddr(evaluator), PaAddr(array), PaAddr(value));
            return scaff->builder.CreateICmpEQ(isValid, CoBool(false), "isDatumInContainerCond");
        };
        value = generateValidationDatum(node.astnodeValue(), value, valueValidator);
    }

    generateCallExtern(
        TyVoid, setDatumAtIndexOfContainer, PaAddr(evaluator), PaAddr(value), PaDouble(index), PaAddr(array));
    return generateVoidRetval(node.astnodeValue());
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
Value *Compiler::genDotSetfirst(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *list = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *value = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    list = generateNotEmptyListFromDatum(node.astnodeValue(), list);
    generateCallExtern(TyVoid, setFirstOfList, PaAddr(evaluator), PaAddr(list), PaAddr(value));
    return generateVoidRetval(node.astnodeValue());
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
Value *Compiler::genDotSetbf(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *list = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *value = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    list = generateNotEmptyListFromDatum(node.astnodeValue(), list);
    value = generateListFromDatum(node.astnodeValue(), value);
    generateCallExtern(TyVoid, setButfirstOfList, PaAddr(evaluator), PaAddr(list), PaAddr(value));
    return generateVoidRetval(node.astnodeValue());
}
/***DOC WORDP WORD?
WORDP thing
WORD? thing

    outputs TRUE if the input is a word, FALSE otherwise.

COD***/
// CMD WORDP 1 1 1 b
// CMD WORD? 1 1 1 b
Value *Compiler::genWordp(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *thingType = generateGetDatumIsa(thing);
    Value *isType = scaff->builder.CreateICmpEQ(thingType, CoInt32(Datum::typeWord), "isDatumTypeCond");
    return scaff->builder.CreateSelect(isType, CoBool(true), CoBool(false), "isDatumTypeResult");
}

/***DOC ARRAYP ARRAY?
ARRAYP thing
ARRAY? thing

    outputs TRUE if the input is an array, FALSE otherwise.

COD***/
// CMD ARRAYP 1 1 1 b
// CMD ARRAY? 1 1 1 b
Value *Compiler::genArrayp(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *thingType = generateGetDatumIsa(thing);
    Value *isType = scaff->builder.CreateICmpEQ(thingType, CoInt32(Datum::typeArray), "isDatumTypeCond");
    return scaff->builder.CreateSelect(isType, CoBool(true), CoBool(false), "isDatumTypeResult");
}

/***DOC LISTP LIST?
LISTP thing
LIST? thing

    outputs TRUE if the input is a list, FALSE otherwise.

COD***/
// CMD LISTP 1 1 1 b
// CMD LIST? 1 1 1 b
Value *Compiler::genListp(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *thingType = generateGetDatumIsa(thing);
    Value *mask = scaff->builder.CreateAnd(thingType, CoInt32(Datum::typeList), "dataTypeMask");
    Value *cond = scaff->builder.CreateICmpNE(mask, CoInt32(0), "typeTest");
    return scaff->builder.CreateSelect(cond, CoBool(true), CoBool(false), "isDatumTypeResult");
}

/***DOC EMPTYP EMPTY?
EMPTYP thing
EMPTY? thing

    outputs TRUE if the input is the empty word or the empty list,
    FALSE otherwise.

COD***/
// CMD EMPTYP 1 1 1 b
// CMD EMPTY? 1 1 1 b
Value *Compiler::genEmptyp(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    return generateCallExtern(TyBool, isEmpty, PaAddr(evaluator), PaAddr(thing));
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
Value *Compiler::genBeforep(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *word1 = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *word2 = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    word1 = generateWordFromDatum(node.astnodeValue(), word1);
    word2 = generateWordFromDatum(node.astnodeValue(), word2);
    return generateCallExtern(TyBool, isBefore, PaAddr(evaluator), PaAddr(word1), PaAddr(word2));
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
Value *Compiler::genDotEq(const DatumPtr &node, RequestReturnType returnType)
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
Value *Compiler::genMemberp(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing1 = generateChild(node.astnodeValue(), 0, RequestReturnDatum); // thing
    Value *thing2 = generateChild(node.astnodeValue(), 1, RequestReturnDatum); // container
    return generateCallExtern(TyBool, isMember, PaAddr(evaluator), PaAddr(thing1), PaAddr(thing2));
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
Value *Compiler::genSubstringp(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing1 = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *thing2 = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    return generateCallExtern(TyBool, isSubstring, PaAddr(evaluator), PaAddr(thing1), PaAddr(thing2));
}
/***DOC NUMBERP NUMBER?
NUMBERP thing
NUMBER? thing

    outputs TRUE if the input is a number, FALSE otherwise.

COD***/
// CMD NUMBERP 1 1 1 b
// CMD NUMBER? 1 1 1 b
Value *Compiler::genNumberp(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    return generateCallExtern(TyBool, isNumber, PaAddr(evaluator), PaAddr(thing));
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
Value *Compiler::genVbarredp(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *c = generateChild(node.astnodeValue(), 0, RequestReturnDatum);

    auto validator = [this](Value *candidate) {
        Value *isGoodChar = generateCallExtern(TyBool, isSingleCharWord, PaAddr(evaluator), PaAddr(candidate));
        return scaff->builder.CreateICmpEQ(isGoodChar, CoBool(true), "isGoodCond");
    };
    c = generateValidationDatum(node.astnodeValue(), c, validator);

    return generateCallExtern(TyBool, isVbarred, PaAddr(evaluator), PaAddr(c));
}
/***DOC COUNT
COUNT thing

    outputs the number of characters in the input, if the input is a word;
    outputs the number of members in the input, if it is a list
    or an array.  (For an array, this may or may not be the index of the
    last member, depending on the array's origin.)

COD***/
// CMD COUNT 1 1 1 n
Value *Compiler::genCount(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    return generateCallExtern(TyDouble, datumCount, PaAddr(evaluator), PaAddr(thing));
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
Value *Compiler::genAscii(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *c = generateChild(node.astnodeValue(), 0, RequestReturnDatum);

    auto validator = [this](Value *candidate) {
        Value *isGoodChar = generateCallExtern(TyBool, isSingleCharWord, PaAddr(evaluator), PaAddr(candidate));
        return scaff->builder.CreateICmpEQ(isGoodChar, CoBool(true), "isGoodCond");
    };
    c = generateValidationDatum(node.astnodeValue(), c, validator);

    return generateCallExtern(TyDouble, ascii, PaAddr(evaluator), PaAddr(c));
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
Value *Compiler::genRawascii(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *c = generateChild(node.astnodeValue(), 0, RequestReturnDatum);

    auto validator = [this](Value *candidate) {
        Value *isGoodChar = generateCallExtern(TyBool, isSingleCharWord, PaAddr(evaluator), PaAddr(candidate));
        return scaff->builder.CreateICmpEQ(isGoodChar, CoBool(true), "isGoodCond");
    };
    c = generateValidationDatum(node.astnodeValue(), c, validator);

    return generateCallExtern(TyDouble, rawascii, PaAddr(evaluator), PaAddr(c));
}
/***DOC CHAR
CHAR int

    outputs the character represented in Unicode by the input,
    which must be an integer between 0 and 65535.

    See ASCII for discussion of Unicode characters.

COD***/
// CMD CHAR 1 1 1 d
Value *Compiler::genChar(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *c = generateChild(node.astnodeValue(), 0, RequestReturnReal);

    Value *retval = nullptr;
    auto validator = [this, &retval](Value *candidate) {
        retval = scaff->builder.CreateFPToUI(candidate, TyInt32, "FpToInt");
        retval = scaff->builder.CreateAnd(retval, CoInt32(65535), "intMask");
        Value *retvalCheck = scaff->builder.CreateUIToFP(retval, TyDouble, "FpToIntCheck");
        return scaff->builder.CreateFCmpOEQ(candidate, retvalCheck, "isValidTest");
    };
    generateValidationDouble(node.astnodeValue(), c, validator);
    return generateCallExtern(TyAddr, chr, PaAddr(evaluator), PaInt32(retval));
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
Value *Compiler::genMember(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing1 = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *thing2 = generateChild(node.astnodeValue(), 1, RequestReturnDatum);

    thing2 = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), thing2);
    return generateCallExtern(TyAddr, member, PaAddr(evaluator), PaAddr(thing1), PaAddr(thing2));
}
/***DOC LOWERCASE
LOWERCASE word

    outputs a copy of the input word, but with all uppercase letters
    changed to the corresponding lowercase letter.

COD***/
// CMD LOWERCASE 1 1 1 d
Value *Compiler::genLowercase(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *word = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    word = generateFromDatum(Datum::typeWord, node.astnodeValue(), word);
    return generateCallExtern(TyAddr, lowercase, PaAddr(evaluator), PaAddr(word));
}
/***DOC UPPERCASE
UPPERCASE word

    outputs a copy of the input word, but with all lowercase letters
    changed to the corresponding uppercase letter.

COD***/
// CMD UPPERCASE 1 1 1 d
Value *Compiler::genUppercase(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *word = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    word = generateFromDatum(Datum::typeWord, node.astnodeValue(), word);
    return generateCallExtern(TyAddr, uppercase, PaAddr(evaluator), PaAddr(word), PaBool(CoBool(false)));
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
Value *Compiler::genStandout(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    return generateCallExtern(TyAddr, standout, PaAddr(evaluator), PaAddr(thing));
}
/***DOC PARSE
PARSE word

    outputs the list that would result if the input word were entered
    in response to a READLIST operation.  That is, PARSE READWORD has
    the same value as READLIST for the same characters read.

COD***/
// CMD PARSE 1 1 1 d
Value *Compiler::genParse(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *word = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    word = generateFromDatum(Datum::typeWord, node.astnodeValue(), word);
    return generateCallExtern(TyAddr, parse, PaAddr(evaluator), PaAddr(word));
}
/***DOC RUNPARSE
RUNPARSE wordorlist

    outputs the list that would result if the input word or list were
    entered as an instruction line; characters such as infix operators
    and parentheses are separate members of the output.  Note that
    sublists of a runparsed list are not themselves runparsed.


COD***/
// CMD RUNPARSE 1 1 1 d
Value *Compiler::genRunparse(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnDatum);
    Value *wordorlist = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    wordorlist = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), wordorlist);
    return generateCallExtern(TyAddr, runparseDatum, PaAddr(evaluator), PaAddr(wordorlist));
}
