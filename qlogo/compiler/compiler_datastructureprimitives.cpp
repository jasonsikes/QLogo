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
#include "compiler_internal.h"
#include "datum_types.h"
#include "workspace/exports.h"
#include "workspace/kernel.h"
#include "sharedconstants.h"
#include "workspace/callframe.h"

using namespace llvm;
using namespace llvm::orc;

Value *Compiler::generateNotEmptyFromDatum(ASTNode *parent, Value *src, Datum::DatumType typeMask)
{
    auto validator = [this, typeMask](Value *candidate) {
        BasicBlock *startBB = scaff_->builder_.GetInsertBlock();
        BasicBlock *okTypeBB = scaff_->createBasicBlock(DBG_NAME("okTypeBlock"));
        BasicBlock *endBB = scaff_->createBasicBlock(DBG_NAME("endBlock"));

        Value *isa = generateGetDatumIsa(candidate);
        Value *maskCalc = scaff_->builder_.CreateAnd(isa, CoInt32(typeMask), DBG_NAME("maskCalc"));
        Value *okType = scaff_->builder_.CreateICmpNE(maskCalc, CoInt32(0), DBG_NAME("okType"));
        scaff_->builder_.CreateCondBr(okType, okTypeBB, endBB);

        scaff_->builder_.SetInsertPoint(okTypeBB);
        Value *isEmpty = generateCallExtern(isDatumEmpty, PaAddr(candidate));
        Value *notEmpty = scaff_->builder_.CreateICmpEQ(isEmpty, CoBool(false), DBG_NAME("isDatumEmptyCond"));
        scaff_->builder_.CreateBr(endBB);

        scaff_->builder_.SetInsertPoint(endBB);
        PHINode *phi = scaff_->builder_.CreatePHI(okType->getType(), 2, DBG_NAME("notEmptyResult"));
        phi->addIncoming(notEmpty, okTypeBB);
        phi->addIncoming(okType, startBB);
        return phi;
    };
    return generateValidationDatum(parent, src, validator);
}

Value *Compiler::generateSingleCharWordFromDatum(ASTNode *parent, Value *src)
{
    return generateValidationDatum(parent, src, [this](Value *candidate) {
        Value *isGoodChar = generateCallExtern(isSingleCharWord, PaAddr(candidate));
        return scaff_->builder_.CreateICmpEQ(isGoodChar, CoBool(true), DBG_NAME("isGoodCond"));
    });
}

Value *Compiler::generateVariadicConstruct(const DatumPtr &node, addr_t (*ctor)(addr_t, addr_t, uint32_t),
                                           const std::string &ctorName, const std::string &aryName, bool requireWords)
{
    CallSite cs(this, node, TypeDatum);
    AllocaInst *ary;
    if (requireWords)
    {
        std::vector<Value *> words;
        words.reserve(cs.count());
        for (Value *child : cs.children())
            words.push_back(generateWordFromDatum(node.astnodeValue(), child));
        ary = generateAllocaAry(words, aryName);
    }
    else
    {
        ary = cs.allocaAry(aryName);
    }
    return cs.ret(generateCallExternTyped(ctor, ctorName,
                                          {PaAddr(scaff_->evaluator_), PaAddr(ary), PaInt32(ary->getArraySize())}));
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
Value *Compiler::generateEqualpCompare(Value *thing1, Value *thing2)
{
    Q_ASSERT(thing1->getType()->isPointerTy());
    Q_ASSERT(thing2->getType()->isPointerTy());
    return generateCallExtern(cmpDatumToDatum, PaAddr(scaff_->evaluator_), PaAddr(thing1), PaAddr(thing2));
}

Value *Compiler::genEqualp(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [thing1, thing2] = cs.children();
    return cs.ret(generateEqualpCompare(thing1, thing2));
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
Value *Compiler::genNotequalp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [thing1, thing2] = cs.children();
    Value *eq = generateEqualpCompare(thing1, thing2);
    return cs.ret(scaff_->builder_.CreateSub(CoBool(1), eq, "noteq"));
}

// CONSTRUCTORS

/***DOC WORD
WORD word1 word2
(WORD word1 word2 word3 ...)

    outputs a word formed by concatenating its inputs.

COD***/
// CMD WORD 0 2 -1 d
Value *Compiler::genWord(const DatumPtr &node, TypeMask expected)
{
    return generateVariadicConstruct(node, concatWord, "concatWord", DBG_NAME("wordAry"), true);
}

/***DOC LIST
LIST thing1 thing2
(LIST thing1 thing2 thing3 ...)

    outputs a list whose members are its inputs, which can be any
    Logo datum (word, list, or array).

COD***/
// CMD LIST 0 2 -1 d
Value *Compiler::genList(const DatumPtr &node, TypeMask expected)
{
    return generateVariadicConstruct(node, createList, "createList", DBG_NAME("listAry"), false);
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
Value *Compiler::genSentence(const DatumPtr &node, TypeMask expected)
{
    return generateVariadicConstruct(node, createSentence, "createSentence", DBG_NAME("sentenceAry"), false);
}
/***DOC FPUT
FPUT thing list

    outputs a list equal to its second input with one extra member,
    the first input, at the beginning.  If the second input is a word,
    then the first input must be a word, and FPUT is equivalent to WORD.

COD***/
// CMD FPUT 2 2 2 d
Value *Compiler::genFput(const DatumPtr &node, TypeMask expected)
{
    return generateFputlput(node, expected, false);
}

/***DOC LPUT
LPUT thing list

    outputs a list equal to its second input with one extra member,
    the first input, at the end. If the second input is a word,
    then the first input must be a word, and LPUT is equivalent to
    WORD with its inputs in the other order.

COD***/
// CMD LPUT 2 2 2 d
Value *Compiler::genLput(const DatumPtr &node, TypeMask expected)
{
    return generateFputlput(node, expected, true);
}

Value *Compiler::generateFputlput(const DatumPtr &node, TypeMask expected, bool isLput)
{
    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [thing, list] = cs.children();
    ASTNode *parent = node.astnodeValue();

    list = generateFromDatum(Datum::typeWordOrListMask, parent, list);

    BasicBlock *wordBB = scaff_->createBasicBlock(DBG_NAME("isWordBB"));
    BasicBlock *listBB = scaff_->createBasicBlock(DBG_NAME("isListBB"));
    BasicBlock *mergeBB = scaff_->createBasicBlock(DBG_NAME("mergeBB"));

    scaff_->builder_.CreateCondBr(generateIsWord(list), wordBB, listBB);

    scaff_->builder_.SetInsertPoint(wordBB);
    Value *wordThing = generateWordFromDatum(parent, thing);
    auto wordVector = isLput ? std::vector<Value *>{list, wordThing} : std::vector<Value *>{wordThing, list};
    AllocaInst *ary = generateAllocaAry(wordVector, DBG_NAME("wordAry"));
    Value *wordRetval =
        generateCallExtern(concatWord, PaAddr(scaff_->evaluator_), PaAddr(ary), PaInt32(ary->getArraySize()));
    BasicBlock *wordDoneBB = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.CreateBr(mergeBB);

    scaff_->builder_.SetInsertPoint(listBB);
    Value *listRetval = isLput ? generateCallExtern(lputList, PaAddr(scaff_->evaluator_), PaAddr(thing), PaAddr(list))
                               : generateCallExtern(fputList, PaAddr(scaff_->evaluator_), PaAddr(thing), PaAddr(list));
    scaff_->builder_.CreateBr(mergeBB);

    scaff_->builder_.SetInsertPoint(mergeBB);
    PHINode *phi = scaff_->builder_.CreatePHI(list->getType(), 2, DBG_NAME("putRetval"));
    phi->addIncoming(listRetval, listBB);
    phi->addIncoming(wordRetval, wordDoneBB);
    return cs.ret(phi);
}
/***DOC ARRAY
ARRAY size
(ARRAY size origin)

    outputs an array of "size" members (must be a non-negative integer),
    each of which initially is an empty list.  Array members can be
    selected with ITEM and changed with SETITEM.  The first member of
    the array is member number 1 unless an "origin" input (must be an
    integer) is given, in which case the first member of the array has
    that number as its index.  (Typically 0 is used as the origin if
    anything.)  Arrays are printed by PRINT and friends, and can be
    typed in, inside curly braces; indicate an origin with {a b c}@0.

COD***/
// CMD ARRAY 1 1 2 d
Value *Compiler::genArray(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    Value *size = cs.child(0);
    Value *origin = nullptr;
    if (cs.count() == 2)
    {
        origin = generateInt32FromDouble(node.astnodeValue(), cs.child(1), true);
    }
    else
    {
        origin = CoInt32(1);
    }

    size = generateNotNegativeInt32FromDouble(node.astnodeValue(), size);

    return cs.ret(generateCallExtern(createArray, PaAddr(scaff_->evaluator_), PaInt32(size), PaInt32(origin)));
}
/***DOC LISTTOARRAY
LISTTOARRAY list
(LISTTOARRAY list origin)

    outputs an array of the same size as the input list, whose members
    are the members of the input list.

COD***/
// CMD LISTTOARRAY 1 1 2 d
Value *Compiler::genListtoarray(const DatumPtr &node, TypeMask expected)
{
    std::vector<TypeMask> childTypes{TypeDatum};
    if (node.astnodeValue()->countOfChildren() == 2)
        childTypes.push_back(TypeReal);
    CallSite cs(this, node, childTypes);
    Value *list = cs.child(0);
    list = generateListFromDatum(node.astnodeValue(), list);
    Value *origin = nullptr;
    if (cs.count() == 2)
    {
        origin = generateInt32FromDouble(node.astnodeValue(), cs.child(1), true);
    }
    else
    {
        origin = CoInt32(1);
    }

    return cs.ret(generateCallExtern(listToArray, PaAddr(scaff_->evaluator_), PaAddr(list), PaInt32(origin)));
}
/***DOC ARRAYTOLIST
ARRAYTOLIST array

    outputs a list whose members are the members of the input array.
    The first member of the output is the first member of the array,
    regardless of the array's origin.

COD***/
// CMD ARRAYTOLIST 1 1 1 d
Value *Compiler::genArraytolist(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *array = cs.child(0);
    array = generateArrayFromDatum(node.astnodeValue(), array);
    return cs.ret(generateCallExtern(arrayToList, PaAddr(scaff_->evaluator_), PaAddr(array)));
}
/***DOC FIRST
FIRST thing

    if the input is a word, outputs the first character of the word.
    If the input is a list, outputs the first member of the list.
    If the input is an array, outputs the origin of the array (that
    is, the INDEX OF the first member of the array).

COD***/
// CMD FIRST 1 1 1 d
Value *Compiler::genFirst(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *thing = cs.child(0);

    auto validator = [this](Value *thing) {
        Value *isEmptyVal = generateCallExtern(isEmpty, PaAddr(thing));
        return scaff_->builder_.CreateICmpEQ(isEmptyVal, CoBool(false), DBG_NAME("isEmptyCond"));
    };
    thing = generateValidationDatum(node.astnodeValue(), thing, validator);

    return cs.ret(generateCallExtern(firstOfDatum, PaAddr(scaff_->evaluator_), PaAddr(thing)));
}
/***DOC LAST
LAST wordorlist

    if the input is a word, outputs the last character of the word.
    If the input is a list, outputs the last member of the list.

COD***/
// CMD LAST 1 1 1 d
Value *Compiler::genLast(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *wordorlist = cs.child(0);

    wordorlist = generateNotEmptyFromDatum(node.astnodeValue(), wordorlist, Datum::typeWordOrListMask);

    return cs.ret(generateCallExtern(lastOfDatum, PaAddr(scaff_->evaluator_), PaAddr(wordorlist)));
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
Value *Compiler::genButfirst(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *wordorlist = cs.child(0);

    wordorlist = generateNotEmptyFromDatum(node.astnodeValue(), wordorlist, Datum::typeWordOrListMask);

    return cs.ret(generateCallExtern(butFirstOfDatum, PaAddr(scaff_->evaluator_), PaAddr(wordorlist)));
}
/***DOC FIRSTS
FIRSTS list

    outputs a list of the first items of the members of its input, which
    must be a list of lists or a list of words.

COD***/
// CMD FIRSTS 1 1 1 d
Value *Compiler::genFirsts(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *list = cs.child(0);
    list = generateListFromDatum(node.astnodeValue(), list);
    return cs.ret(generateCallExtern(firstsOfList,
                              PaAddr(scaff_->evaluator_),
                              PaAddr(CoAddr(node.astnodeValue()->nodeName_.datumValue())),
                              PaAddr(list)));
}
/***DOC BUTFIRSTS BFS
BUTFIRSTS list
BFS list

    outputs a list of the butfirsts of the members of its input, which
    must be a list of lists or a list of words.

COD***/
// CMD BUTFIRSTS 1 1 1 d
// CMD BFS 1 1 1 d
Value *Compiler::genButfirsts(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *list = cs.child(0);
    list = generateListFromDatum(node.astnodeValue(), list);
    return cs.ret(generateCallExtern(butfirstsOfList,
                              PaAddr(scaff_->evaluator_),
                              PaAddr(CoAddr(node.astnodeValue()->nodeName_.datumValue())),
                              PaAddr(list)));
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
Value *Compiler::genButlast(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *wordorlist = cs.child(0);

    wordorlist = generateNotEmptyFromDatum(node.astnodeValue(), wordorlist, Datum::typeWordOrListMask);

    return cs.ret(generateCallExtern(butLastOfDatum, PaAddr(scaff_->evaluator_), PaAddr(wordorlist)));
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
Value *Compiler::genItem(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal, TypeDatum);
    auto [index, thing] = cs.children();

    // Instead of iterating a list twice, save the member when counting.
    // The slot lives in the compiled frame (not a genItem C++ local).
    AllocaInst *listItemSlot = scaff_->createEntryBlockAlloca(TyAddr, nullptr, DBG_NAME("listItem"));
    scaff_->builder_.CreateStore(CoAddr(0), listItemSlot);

    auto validator = [this, listItemSlot, thing](Value *index) {
        Value *isValid =
            generateCallExtern(isDatumIndexValid, PaAddr(thing), PaDouble(index), PaAddr(listItemSlot));
        return scaff_->builder_.CreateICmpEQ(isValid, CoBool(true), DBG_NAME("isDatumIndexValidCond"));
    };
    index = generateValidationDouble(node.astnodeValue(), index, validator);

    return cs.ret(generateCallExtern(
        itemOfDatum, PaAddr(scaff_->evaluator_), PaAddr(thing), PaDouble(index), PaAddr(listItemSlot)));
}
/***DOC SETITEM
SETITEM index array value

    command.  Replaces the "index"th member of "array" with the new
    "value".  Ensures that the resulting array is not circular, i.e.,
    "value" may not be a list or array that contains "array".

COD***/
// CMD SETITEM 3 3 3 n
Value *Compiler::genSetitem(const DatumPtr &node, TypeMask expected)
{
    return generateSetitem(node, expected, false);
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
Value *Compiler::genDotSetitem(const DatumPtr &node, TypeMask expected)
{
    return generateSetitem(node, expected, true);
}

Value *Compiler::generateSetitem(const DatumPtr &node, TypeMask expected, bool isDangerous)
{
    CallSite cs(this, node, TypeReal, TypeDatum, TypeDatum);
    auto [index, array, value] = cs.children();
    array = generateArrayFromDatum(node.astnodeValue(), array);

    auto indexValidator = [this, array](Value *index) {
        Value *isValid =
            generateCallExtern(isDatumIndexValid, PaAddr(array), PaDouble(index), PaAddr(CoAddr(0)));
        return scaff_->builder_.CreateICmpEQ(isValid, CoBool(true), DBG_NAME("isDatumIndexValidCond"));
    };
    index = generateValidationDouble(node.astnodeValue(), index, indexValidator);

    if (!isDangerous)
    {
        auto valueValidator = [this, array](Value *value) {
            Value *isValid = generateCallExtern(
                isDatumContainerOrInContainer, PaAddr(scaff_->evaluator_), PaAddr(array), PaAddr(value));
            return scaff_->builder_.CreateICmpEQ(isValid, CoBool(false), DBG_NAME("isDatumInContainerCond"));
        };
        value = generateValidationDatum(node.astnodeValue(), value, valueValidator);
    }

    generateCallExtern(setDatumAtIndexOfContainer, PaAddr(value), PaDouble(index), PaAddr(array));
    return cs.ret(generateVoidRetval(node));
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
Value *Compiler::genDotSetfirst(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [list, value] = cs.children();
    list = generateNotEmptyFromDatum(node.astnodeValue(), list, Datum::typeList);
    generateCallExtern(setFirstOfList, PaAddr(list), PaAddr(value));
    return cs.ret(generateVoidRetval(node));
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
Value *Compiler::genDotSetbf(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [list, value] = cs.children();
    list = generateNotEmptyFromDatum(node.astnodeValue(), list, Datum::typeList);
    value = generateListFromDatum(node.astnodeValue(), value);
    generateCallExtern(setButfirstOfList, PaAddr(list), PaAddr(value));
    return cs.ret(generateVoidRetval(node));
}
/***DOC WORDP WORD?
WORDP thing
WORD? thing

    outputs TRUE if the input is a word, FALSE otherwise.

COD***/
// CMD WORDP 1 1 1 b
// CMD WORD? 1 1 1 b
Value *Compiler::genWordp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *thing = cs.child(0);
    Value *thingType = generateGetDatumIsa(thing);
    Value *isType = scaff_->builder_.CreateICmpEQ(thingType, CoInt32(Datum::typeWord), DBG_NAME("isDatumTypeCond"));
    return cs.ret(isType);
}

/***DOC ARRAYP ARRAY?
ARRAYP thing
ARRAY? thing

    outputs TRUE if the input is an array, FALSE otherwise.

COD***/
// CMD ARRAYP 1 1 1 b
// CMD ARRAY? 1 1 1 b
Value *Compiler::genArrayp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *thing = cs.child(0);
    Value *thingType = generateGetDatumIsa(thing);
    Value *isType = scaff_->builder_.CreateICmpEQ(thingType, CoInt32(Datum::typeArray), DBG_NAME("isDatumTypeCond"));
    return cs.ret(isType);
}

/***DOC LISTP LIST?
LISTP thing
LIST? thing

    outputs TRUE if the input is a list, FALSE otherwise.

COD***/
// CMD LISTP 1 1 1 b
// CMD LIST? 1 1 1 b
Value *Compiler::genListp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *thing = cs.child(0);
    Value *thingType = generateGetDatumIsa(thing);
    Value *mask = scaff_->builder_.CreateAnd(thingType, CoInt32(Datum::typeList), DBG_NAME("dataTypeMask"));
    Value *cond = scaff_->builder_.CreateICmpNE(mask, CoInt32(0), DBG_NAME("typeTest"));
    return cs.ret(cond);
}

/***DOC EMPTYP EMPTY?
EMPTYP thing
EMPTY? thing

    outputs TRUE if the input is the empty word or the empty list,
    FALSE otherwise.

COD***/
// CMD EMPTYP 1 1 1 b
// CMD EMPTY? 1 1 1 b
Value *Compiler::genEmptyp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *thing = cs.child(0);
    return cs.ret(generateCallExtern(isEmpty, PaAddr(thing)));
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
Value *Compiler::genBeforep(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [word1, word2] = cs.children();
    word1 = generateWordFromDatum(node.astnodeValue(), word1);
    word2 = generateWordFromDatum(node.astnodeValue(), word2);
    return cs.ret(generateCallExtern(isBefore, PaAddr(scaff_->evaluator_), PaAddr(word1), PaAddr(word2)));
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
Value *Compiler::genDotEq(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [thing1, thing2] = cs.children();
    Value *isEqualCond = scaff_->builder_.CreateICmpEQ(thing1, thing2, DBG_NAME("isEqualCond"));
    return cs.ret(isEqualCond);
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
Value *Compiler::genMemberp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [thing1, thing2] = cs.children(); // thing, container
    return cs.ret(generateCallExtern(isMember, PaAddr(scaff_->evaluator_), PaAddr(thing1), PaAddr(thing2)));
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
Value *Compiler::genSubstringp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [thing1, thing2] = cs.children();
    return cs.ret(generateCallExtern(isSubstring, PaAddr(thing1), PaAddr(thing2)));
}
/***DOC NUMBERP NUMBER?
NUMBERP thing
NUMBER? thing

    outputs TRUE if the input is a number, FALSE otherwise.

COD***/
// CMD NUMBERP 1 1 1 b
// CMD NUMBER? 1 1 1 b
Value *Compiler::genNumberp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *thing = cs.child(0);
    return cs.ret(generateCallExtern(isNumber, PaAddr(thing)));
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
Value *Compiler::genVbarredp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *c = cs.child(0);
    c = generateSingleCharWordFromDatum(node.astnodeValue(), c);

    return cs.ret(generateCallExtern(isVbarred, PaAddr(c)));
}
/***DOC COUNT
COUNT thing

    outputs the number of characters in the input, if the input is a word;
    outputs the number of members in the input, if it is a list
    or an array.  (For an array, this may or may not be the index of the
    last member, depending on the array's origin.)

COD***/
// CMD COUNT 1 1 1 n
Value *Compiler::genCount(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *thing = cs.child(0);
    return cs.ret(generateCallExtern(datumCount, PaAddr(thing)));
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
Value *Compiler::genAscii(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *c = cs.child(0);
    c = generateSingleCharWordFromDatum(node.astnodeValue(), c);

    return cs.ret(generateCallExtern(ascii, PaAddr(c)));
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
Value *Compiler::genRawascii(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *c = cs.child(0);
    c = generateSingleCharWordFromDatum(node.astnodeValue(), c);

    return cs.ret(generateCallExtern(rawascii, PaAddr(c)));
}
/***DOC CHAR
CHAR int

    outputs the character represented in Unicode by the input,
    which must be an integer between 0 and 65535.

    See ASCII for discussion of Unicode characters.

COD***/
// CMD CHAR 1 1 1 d
Value *Compiler::genChar(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    Value *c = cs.child(0);

    Value *accepted = generateValidationDouble(node.astnodeValue(), c, [this](Value *candidate) {
        return generateIsExactIntegerInRange(candidate, 0.0, 65535.0);
    });
    Value *asInt = scaff_->builder_.CreateFPToUI(accepted, TyInt32, DBG_NAME("FpToInt"));
    return cs.ret(generateCallExtern(chr, PaAddr(scaff_->evaluator_), PaInt32(asInt)));
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
Value *Compiler::genMember(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [thing1, thing2] = cs.children();

    thing2 = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), thing2);
    return cs.ret(generateCallExtern(member, PaAddr(scaff_->evaluator_), PaAddr(thing1), PaAddr(thing2)));
}
/***DOC LOWERCASE
LOWERCASE word

    outputs a copy of the input word, but with all uppercase letters
    changed to the corresponding lowercase letter.

COD***/
// CMD LOWERCASE 1 1 1 d
Value *Compiler::genLowercase(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *word = cs.child(0);
    word = generateFromDatum(Datum::typeWord, node.astnodeValue(), word);
    return cs.ret(generateCallExtern(lowercase, PaAddr(scaff_->evaluator_), PaAddr(word)));
}
/***DOC UPPERCASE
UPPERCASE word

    outputs a copy of the input word, but with all lowercase letters
    changed to the corresponding uppercase letter.

COD***/
// CMD UPPERCASE 1 1 1 d
Value *Compiler::genUppercase(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *word = cs.child(0);
    word = generateFromDatum(Datum::typeWord, node.astnodeValue(), word);
    return cs.ret(generateCallExtern(uppercase, PaAddr(scaff_->evaluator_), PaAddr(word)));
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
Value *Compiler::genStandout(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *thing = cs.child(0);
    return cs.ret(generateCallExtern(standout, PaAddr(scaff_->evaluator_), PaAddr(thing)));
}
/***DOC PARSE
PARSE word

    outputs the list that would result if the input word were entered
    in response to a READLIST operation.  That is, PARSE READWORD has
    the same value as READLIST for the same characters read.

COD***/
// CMD PARSE 1 1 1 d
Value *Compiler::genParse(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *word = cs.child(0);
    word = generateFromDatum(Datum::typeWord, node.astnodeValue(), word);
    return cs.ret(generateCallExtern(parse, PaAddr(scaff_->evaluator_), PaAddr(word)));
}
/***DOC RUNPARSE
RUNPARSE wordorlist

    outputs the list that would result if the input word or list were
    entered as an instruction line; characters such as infix operators
    and parentheses are separate members of the output.  Note that
    sublists of a runparsed list are not themselves runparsed.


COD***/
// CMD RUNPARSE 1 1 1 d
Value *Compiler::genRunparse(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *wordorlist = cs.child(0);
    wordorlist = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), wordorlist);
    return cs.ret(generateCallExtern(runparseDatum, PaAddr(scaff_->evaluator_), PaAddr(wordorlist)));
}

/***DOC MDARRAY
MDARRAY sizelist					(library procedure)
(MDARRAY sizelist origin)

	outputs a multi-dimensional array.  The first input must be a list
	of one or more positive integers.  The second input, if present,
	must be a single integer that applies to every dimension of the array.
	Ex: (MDARRAY [3 5] 0) outputs a two-dimensional array whose members
	range from [0 0] to [2 4].

COD***/

/***DOC COMBINE
COMBINE thing1 thing2					(library procedure)

	if thing2 is a word, outputs WORD thing1 thing2.  If thing2 is a list,
	outputs FPUT thing1 thing2.

COD***/

/***DOC REVERSE
REVERSE list						(library procedure)

	outputs a list whose members are the members of the input list, in
	reverse order.

COD***/

/***DOC GENSYM
GENSYM							(library procedure)

	outputs a unique word each time it's invoked.  The words are of the
	form G1, G2, etc.


COD***/

/***DOC MDITEM
MDITEM indexlist array					(library procedure)

	outputs the member of the multidimensional "array" selected by
	the list of numbers "indexlist".

COD***/

/***DOC PICK
PICK list						(library procedure)

	outputs a randomly chosen member of the input list.

COD***/

/***DOC REMOVE
REMOVE thing list					(library procedure)

	outputs a copy of "list" with every member equal to "thing" removed.

COD***/

/***DOC REMDUP
REMDUP list						(library procedure)

	outputs a copy of "list" with duplicate members removed.  If two or
	more members of the input are equal, the rightmost of those members
	is the one that remains in the output.

COD***/

/***DOC QUOTED
QUOTED thing						(library procedure)

	outputs its input, if a list; outputs its input with a quotation
	mark prepended, if a word.


COD***/

/***DOC MDSETITEM
MDSETITEM indexlist array value				(library procedure)

	command.  Replaces the member of "array" chosen by "indexlist"
	with the new "value".

COD***/

/***DOC PUSH
PUSH stackname thing					(library procedure)

	command.  Adds the "thing" to the stack that is the value of the
	variable whose name is "stackname".  This variable must have a list
	as its value; the initial value should be the empty list.  New
	members are added at the front of the list.

COD***/

/***DOC POP
POP stackname						(library procedure)

	outputs the most recently PUSHed member of the stack that is the
	value of the variable whose name is "stackname" and removes that
	member from the stack.

COD***/

/***DOC QUEUE
QUEUE queuename thing					(library procedure)

	command.  Adds the "thing" to the queue that is the value of the
	variable whose name is "queuename".  This variable must have a list
	as its value; the initial value should be the empty list.  New
	members are added at the back of the list.

COD***/

/***DOC DEQUEUE
DEQUEUE queuename					(library procedure)

	outputs the least recently QUEUEd member of the queue that is the
	value of the variable whose name is "queuename" and removes that
	member from the queue.

COD***/
