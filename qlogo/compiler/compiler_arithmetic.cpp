//===-- qlogo/arithmetic.cpp - Arithmetic implementations -------*- C++ -*-===//
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
/// This file contains the implementation of the math methods of the
/// Compiler class.
///
//===----------------------------------------------------------------------===//

#include "compiler_private.h"
#include "datum.h"
#include "astnode.h"
#include "compiler.h"
#include "workspace/callframe.h"
#include <QRandomGenerator>

static QRandomGenerator randomGenerator;

/// Generate a random nonnegative integer less than num given
/// @param num an upper bound (exclusive) to the random number.
/// @return the random number generated.
EXPORTC double random1(int32_t num)
{
    int result = randomGenerator.bounded(num);
    return (double)result;
}

/// Generate a random integer between start and end (both inclusive)
/// @param start a lower bound (inclusive) to the random number.
/// @param end an upper bound (inclusive) to the random number.
/// @return the random number generated.
EXPORTC double random2(int32_t start, int32_t end)
{
    int result = randomGenerator.bounded(start, end + 1);
    return (double)result;
}

/// Set the seed for the random number generator
/// @param seed the seed.
/// @return nothing.
EXPORTC addr_t setRandomWithSeed(int32_t seed)
{
    randomGenerator.seed(seed);
    return nullptr;
}

/// Set the seed for the random number generator using the system seed
/// @return nothing.
EXPORTC addr_t setRandom()
{
    QRandomGenerator *s = QRandomGenerator::system();
    quint32 seed = s->generate();
    randomGenerator.seed(seed);
    return nullptr;
}

/// Generate a Word(string) from a number that is formatted according to the
/// other parameters.
/// @param eAddr a pointer to the Evaluator object
/// @param num the number to apply formatting to.
/// @param width the minimum number of characters to use. Spaces may be added.
/// @param precision the number of digits to add after the decimal point.
/// @return A Word(string) with formatting applied.
EXPORTC addr_t getFormForNumber(addr_t eAddr, double num, uint32_t width, int32_t precision)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    QString retval = QString("%1").arg(num, width, 'f', precision);
    Word *w = new Word(retval);
    e->watch(w);

    return reinterpret_cast<addr_t>(w);
}
using namespace llvm;
using namespace llvm::orc;

Value *Compiler::generateInt32FromDouble(ASTNode *parent, Value *src, bool isSigned)
{
    Value *retval = nullptr;
    auto validator = [this, isSigned, &retval](Value *candidate) {
        retval = isSigned ? scaff->builder.CreateFPToSI(candidate, TyInt32, "FpToInt")
                          : scaff->builder.CreateFPToUI(candidate, TyInt32, "FpToInt");
        Value *retvalCheck = isSigned ? scaff->builder.CreateSIToFP(retval, TyDouble, "FpToIntCheck")
                                      : scaff->builder.CreateUIToFP(retval, TyDouble, "FpToIntCheck");
        return scaff->builder.CreateFCmpOEQ(candidate, retvalCheck, "isValidTest");
    };
    generateValidationDouble(parent, src, validator);
    return retval;
}

Value *Compiler::generateNotZeroFromDouble(ASTNode *parent, Value *src)
{
    return generateValidationDouble(parent, src, [this](Value *val) {
        return scaff->builder.CreateFCmpONE(val, CoDouble(0.0), "isZeroTest");
    });
}

Value *Compiler::generateNotNegativeFromDouble(ASTNode *parent, Value *src)
{
    return generateValidationDouble(parent, src, [this](Value *val) {
        return scaff->builder.CreateFCmpOGE(val, CoDouble(0.0), "isZeroTest");
    });
}

Value *Compiler::generateGTZeroFromDouble(ASTNode *parent, Value *src)
{
    return generateValidationDouble(parent, src, [this](Value *val) {
        return scaff->builder.CreateFCmpOGT(val, CoDouble(0.0), "isZeroTest");
    });
}

/***DOC ARCTAN
ARCTAN num
(ARCTAN x y)

 outputs the arctangent, in degrees, of its input.  With two
 inputs, outputs the arctangent of y/x, if x is nonzero, or
 90 or -90 depending on the sign of y, if x is zero.

COD***/
// CMD ARCTAN 1 1 2 r
Value *Compiler::genArctan(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    Value *radToDeg = CoDouble(180 / M_PI);
    Value *retval;

    // Calculate atan() or atan2() depending on number of children.
    if (children.size() == 1)
    {
        retval = generateCallExtern(TyDouble, "atan", {PaDouble(children[0])});
    }
    else
    {
        retval = generateCallExtern(TyDouble, "atan2", {PaDouble(children[1]), PaDouble(children[0])});
    }
    retval = scaff->builder.CreateFMul(retval, radToDeg, "theta");
    return retval;
}

/***DOC ASHIFT
ASHIFT num1 num2

 outputs "num1" arithmetic-shifted to the left by "num2" bits.
 If num2 is negative, the shift is to the right with sign
 extension.  The inputs must be integers.

COD***/
// CMD ASHIFT 2 2 2 r
Value *Compiler::genAshift(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    Value *num1 = generateInt32FromDouble(node.astnodeValue(), children[0], true);
    Value *num2 = generateInt32FromDouble(node.astnodeValue(), children[1], true);
    Value *retval = scaff->builder.CreateAlloca(TyInt32, CoInt32(1), "ashiftAlloca");

    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();

    BasicBlock *leftShiftBB = BasicBlock::Create(*scaff->theContext, "leftShiftBB", theFunction);
    BasicBlock *rightShiftBB = BasicBlock::Create(*scaff->theContext, "rightShiftBB", theFunction);
    BasicBlock *mergeBB = BasicBlock::Create(*scaff->theContext, "shiftCont", theFunction);

    Value *cond = scaff->builder.CreateICmpSGE(num2, CoInt32(0), "isGE0");
    scaff->builder.CreateCondBr(cond, leftShiftBB, rightShiftBB);

    scaff->builder.SetInsertPoint(leftShiftBB);
    Value *lsResult = scaff->builder.CreateShl(num1, num2, "leftShift");
    scaff->builder.CreateStore(lsResult, retval);
    scaff->builder.CreateBr(mergeBB);

    scaff->builder.SetInsertPoint(rightShiftBB);
    num2 = scaff->builder.CreateSub(CoInt32(0), num2, "negNum2");
    Value *rsResult = scaff->builder.CreateAShr(num1, num2, "rightShift");
    scaff->builder.CreateStore(rsResult, retval);
    scaff->builder.CreateBr(mergeBB);

    scaff->builder.SetInsertPoint(mergeBB);
    retval = scaff->builder.CreateLoad(TyInt32, retval, "loadResult");
    return scaff->builder.CreateSIToFP(retval, TyDouble, "IntToFP");
}

/***DOC LSHIFT
LSHIFT num1 num2

 outputs "num1" logical-shifted to the left by "num2" bits.
 If num2 is negative, the shift is to the right with zero fill.
 The inputs must be integers.

COD***/
// CMD LSHIFT 2 2 2 r
Value *Compiler::genLshift(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    Value *num1 = generateInt32FromDouble(node.astnodeValue(), children[0], true);
    Value *num2 = generateInt32FromDouble(node.astnodeValue(), children[1], true);
    Value *retval = scaff->builder.CreateAlloca(TyInt32, CoInt32(1), "lshiftAlloca");

    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();

    BasicBlock *leftShiftBB = BasicBlock::Create(*scaff->theContext, "leftShiftBB", theFunction);
    BasicBlock *rightShiftBB = BasicBlock::Create(*scaff->theContext, "rightShiftBB", theFunction);
    BasicBlock *mergeBB = BasicBlock::Create(*scaff->theContext, "shiftCont", theFunction);

    Value *cond = scaff->builder.CreateICmpSGE(num2, CoInt32(0), "isGE0");
    scaff->builder.CreateCondBr(cond, leftShiftBB, rightShiftBB);

    scaff->builder.SetInsertPoint(leftShiftBB);
    Value *lsResult = scaff->builder.CreateShl(num1, num2, "leftShift");
    scaff->builder.CreateStore(lsResult, retval);
    scaff->builder.CreateBr(mergeBB);

    scaff->builder.SetInsertPoint(rightShiftBB);
    num2 = scaff->builder.CreateSub(CoInt32(0), num2, "negNum2");
    Value *rsResult = scaff->builder.CreateLShr(num1, num2, "rightShift");
    scaff->builder.CreateStore(rsResult, retval);
    scaff->builder.CreateBr(mergeBB);

    scaff->builder.SetInsertPoint(mergeBB);
    retval = scaff->builder.CreateLoad(TyInt32, retval, "loadResult");
    return scaff->builder.CreateSIToFP(retval, TyDouble, "IntToFP");
}

/***DOC BITAND
BITAND num1 num2
(BITAND num1 num2 num3 ...)

 outputs the bitwise AND of its inputs, which must be unsigned integers.

COD***/
// CMD BITAND 0 2 -1 r
Value *Compiler::genBitand(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    for (int i = 0; i < children.size(); ++i)
    {
        children[i] = generateInt32FromDouble(node.astnodeValue(), children[i], true);
    }

    if (children.size() == 0)
    {
        return CoDouble(-1.0);
    }

    Value *retval = children[0];

    for (int i = 1; i < children.size(); ++i)
    {
        retval = scaff->builder.CreateAnd(retval, children[i], "BitAND");
    }
    return scaff->builder.CreateSIToFP(retval, TyDouble, "IntToFP");
}

/***DOC BITOR
BITOR num1 num2
(BITOR num1 num2 num3 ...)

 outputs the bitwise OR of its inputs, which must be integers.

COD***/
// CMD BITOR 0 2 -1 r
Value *Compiler::genBitor(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    for (int i = 0; i < children.size(); ++i)
    {
        children[i] = generateInt32FromDouble(node.astnodeValue(), children[i], true);
    }

    if (children.size() == 0)
    {
        return CoDouble(0.0);
    }

    Value *retval = children[0];

    for (int i = 1; i < children.size(); ++i)
    {
        retval = scaff->builder.CreateOr(retval, children[i], "BitOR");
    }
    return scaff->builder.CreateSIToFP(retval, TyDouble, "IntToFP");
}

/***DOC BITXOR
BITXOR num1 num2
(BITXOR num1 num2 num3 ...)

 outputs the bitwise EXCLUSIVE OR of its inputs, which must be
 integers.

COD***/
// CMD BITXOR 0 2 -1 r
Value *Compiler::genBitxor(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    for (int i = 0; i < children.size(); ++i)
    {
        children[i] = generateInt32FromDouble(node.astnodeValue(), children[i], true);
    }

    if (children.size() == 0)
    {
        return CoDouble(0.0);
    }

    Value *retval = children[0];

    for (int i = 1; i < children.size(); ++i)
    {
        retval = scaff->builder.CreateXor(retval, children[i], "BitXOR");
    }
    return scaff->builder.CreateSIToFP(retval, TyDouble, "IntToFP");
}

/***DOC BITNOT
BITNOT num

 outputs the bitwise NOT of its input, which must be an integer.

COD***/
// CMD BITNOT 1 1 1 r
Value *Compiler::genBitnot(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnBool);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    Value *num = generateInt32FromDouble(node.astnodeValue(), children[0], true);
    num = scaff->builder.CreateXor(num, CoInt32(-1), "bitNOT");

    return scaff->builder.CreateSIToFP(num, TyDouble, "IntToFP");
}

/***DOC MODULO
MODULO num1 num2

 outputs the remainder on dividing "num1" by "num2"; both must be
 integers and the result is an integer with the same sign as num2.

COD***/
// CMD MODULO 2 2 2 r
Value *Compiler::genModulo(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();

    generateNotZeroFromDouble(node.astnodeValue(), children[1]);
    for (int i = 0; i < children.size(); ++i)
    {
        children[i] = generateInt32FromDouble(node.astnodeValue(), children[i], true);
    }

    Value *retvalLoc = scaff->builder.CreateAlloca(TyInt32, CoInt32(1), "retvalLoc");

    BasicBlock *pos1BB = BasicBlock::Create(*scaff->theContext, "pos1BB", theFunction);
    BasicBlock *neg1BB = BasicBlock::Create(*scaff->theContext, "neg1BB", theFunction);
    BasicBlock *addbBB = BasicBlock::Create(*scaff->theContext, "addbBB", theFunction);
    BasicBlock *contBB = BasicBlock::Create(*scaff->theContext, "contBB", theFunction);

    Value *num = children[0];
    Value *denom = children[1];
    Value *r = scaff->builder.CreateSRem(num, denom, "remainder");
    scaff->builder.CreateStore(r, retvalLoc);

    Value *c1 = scaff->builder.CreateICmpSLT(r, CoInt32(0), "cond1");
    scaff->builder.CreateCondBr(c1, neg1BB, pos1BB);

    scaff->builder.SetInsertPoint(pos1BB);
    Value *c2 = scaff->builder.CreateICmpSLT(denom, CoInt32(0), "cond2");
    scaff->builder.CreateCondBr(c2, addbBB, contBB);

    scaff->builder.SetInsertPoint(neg1BB);
    Value *c3 = scaff->builder.CreateICmpSLT(denom, CoInt32(0), "cond3");
    scaff->builder.CreateCondBr(c3, contBB, addbBB);

    scaff->builder.SetInsertPoint(addbBB);
    Value *retvalRB = scaff->builder.CreateAdd(r, denom, "addB");
    scaff->builder.CreateStore(retvalRB, retvalLoc);
    scaff->builder.CreateBr(contBB);

    scaff->builder.SetInsertPoint(contBB);

    r = scaff->builder.CreateLoad(TyInt32, retvalLoc);
    return scaff->builder.CreateSIToFP(r, TyDouble, "IntToFP");
}

/***DOC SQRT
SQRT num

 outputs the square root of the input, which must be nonnegative.

COD***/
// CMD SQRT 1 1 1 r
Value *Compiler::genSqrt(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    Value *num = children[0];

    generateNotNegativeFromDouble(node.astnodeValue(), num);
    return generateCallExtern(TyDouble, "sqrt", {PaDouble(num)});
}

/***DOC POWER
POWER num1 num2

 outputs "num1" to the "num2" power.  If num1 is negative, then
 num2 must be an integer.

COD***/
// CMD POWER 2 2 2 r
Value *Compiler::genPower(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    Value *num1 = children[0];
    Value *num2 = children[1];

    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();

    BasicBlock *isNegativeBB = BasicBlock::Create(*scaff->theContext, "isNegative", theFunction);
    BasicBlock *notNegativeBB = BasicBlock::Create(*scaff->theContext, "notNegative", theFunction);

    Value *cond = scaff->builder.CreateFCmpOGE(num1, CoDouble(0.0), "isNegativeTest");
    scaff->builder.CreateCondBr(cond, notNegativeBB, isNegativeBB);

    scaff->builder.SetInsertPoint(isNegativeBB);
    generateInt32FromDouble(node.astnodeValue(), num2, true);
    scaff->builder.CreateBr(notNegativeBB);

    scaff->builder.SetInsertPoint(notNegativeBB);

    return generateCallExtern(TyDouble, "pow", {PaDouble(num1), PaDouble(num2)});
}

/***DOC MINUS
MINUS num
- num

outputs the negative of its input.  Minus sign means unary minus if
the previous token is an infix operator or open parenthesis, or it is
preceded by a space and followed by a nonspace.  There is a difference
in binding strength between the two forms:

MINUS 3 + 4	means	-(3+4)
- 3 + 4		means	(-3)+4

COD***/
// CMD MINUS 1 1 1 r
// CMD - 1 1 1 r
Value *Compiler::genMinus(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    return scaff->builder.CreateFNeg(children[0], "negtmp");
}

/***DOC PRODUCT
PRODUCT num1 num2
(PRODUCT num1 num2 num3 ...)
num1 * num2

outputs the product of its inputs.

COD***/
// CMD PRODUCT 0 2 -1 r
Value *Compiler::genProduct(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    // No children? Return identity.
    if (children.size() == 0)
        return CoDouble(1.0);

    // Loop through the children accumulating from the left.
    Value *accum = children[0];
    for (int i = 1; i < children.size(); ++i)
    {
        accum = scaff->builder.CreateFMul(accum, children[i], "multmp");
    }
    return accum;
}

/***DOC SUM
SUM num1 num2
(SUM num1 num2 num3 ...)
num1 + num2

outputs the sum of its inputs.

COD***/
// CMD SUM 0 2 -1 r
Value *Compiler::genSum(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    // No children? Return identity.
    if (children.size() == 0)
        return CoDouble(0.0);

    // Loop through the children accumulating from the left.
    Value *accum = children[0];
    for (int i = 1; i < children.size(); ++i)
    {
        accum = scaff->builder.CreateFAdd(accum, children[i], "addtmp");
    }
    return accum;
}

/***DOC DIFFERENCE
DIFFERENCE num1 num2
num1 - num2

outputs the difference of its inputs.  Minus sign means infix
difference in ambiguous contexts (when preceded by a complete
expression), unless it is preceded by a space and followed
by a nonspace.  (See also MINUS.)

COD***/
// CMD DIFFERENCE 2 2 2 r
Value *Compiler::genDifference(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    return scaff->builder.CreateFSub(children[0], children[1], "subtmp");
}

/***DOC QUOTIENT
QUOTIENT num1 num2
(QUOTIENT num)
num1 / num2

outputs the quotient of its inputs.  The quotient of two integers
is an integer if and only if the dividend is a multiple of the divisor.
(In other words, QUOTIENT 5 2 is 2.5, not 2, but QUOTIENT 4 2 is
2, not 2.0 -- it does the right thing.)  With a single input,
QUOTIENT outputs the reciprocal of the input.

COD***/
// CMD QUOTIENT 1 2 2 r
Value *Compiler::genQuotient(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    Value *num;
    Value *denom;

    // One child? Calculate reciprocal.
    if (children.size() == 1)
    {
        num = CoDouble(1.0);
        denom = children[0];
    }
    else
    {
        num = children[0];
        denom = children[1];
    }

    denom = generateNotZeroFromDouble(node.astnodeValue(), denom);
    return scaff->builder.CreateFDiv(num, denom, "quotmp");
}

/***DOC REMAINDER
REMAINDER num1 num2

 outputs the remainder on dividing "num1" by "num2"; both must be
 integers and the result is an integer with the same sign as num1.

COD***/
// CMD REMAINDER 2 2 2 r
Value *Compiler::genRemainder(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);
    Value *num = generateInt32FromDouble(node.astnodeValue(), children[0], true);
    Value *denom = generateNotZeroFromDouble(node.astnodeValue(), children[1]);
    denom = generateInt32FromDouble(node.astnodeValue(), denom, true);
    Value *retval = scaff->builder.CreateSRem(num, denom, "remainder");
    return scaff->builder.CreateSIToFP(retval, TyDouble, "IntToFP");
}

/***DOC SIN
SIN degrees

outputs the sine of its input, which is taken in degrees.

COD***/
// CMD SIN 1 1 1 r
Value *Compiler::genSin(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    Value *degToRad = CoDouble(M_PI / 180);
    Value *theta = scaff->builder.CreateFMul(children[0], degToRad, "theta");
    return generateCallExtern(TyDouble, "sin", {PaDouble(theta)});
}

/***DOC INT
INT num

outputs its input with fractional part removed, i.e., an integer
with the same sign as the input, whose absolute value is the
largest integer less than or equal to the absolute value of
the input.

COD***/
// CMD INT 1 1 1 r
Value *Compiler::genInt(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);
    return generateCallExtern(TyDouble, "trunc", {PaDouble(children[0])});
}

/***DOC ROUND
ROUND num

outputs the nearest integer to the input.

COD***/
// CMD ROUND 1 1 1 r
Value *Compiler::genRound(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);
    return generateCallExtern(TyDouble, "round", {PaDouble(children[0])});
}

/***DOC EXP
EXP num

outputs e (2.718281828+) to the input power.

COD***/
// CMD EXP 1 1 1 r
Value *Compiler::genExp(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);
    return generateCallExtern(TyDouble, "exp", {PaDouble(children[0])});
}

/***DOC LOG10
LOG10 num

 outputs the common logarithm of the input. Input must be greater
 than zero.

COD***/
// CMD LOG10 1 1 1 r
Value *Compiler::genLog10(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);
    generateGTZeroFromDouble(node.astnodeValue(), children[0]);
    return generateCallExtern(TyDouble, "log10", {PaDouble(children[0])});
}

/***DOC LN
LN num

 outputs the natural logarithm of the input. Input must be greater
 than zero.

COD***/
// CMD LN 1 1 1 r
Value *Compiler::genLn(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);
    generateGTZeroFromDouble(node.astnodeValue(), children[0]);
    return generateCallExtern(TyDouble, "log", {PaDouble(children[0])});
}

/***DOC RADSIN
RADSIN radians

outputs the sine of its input, which is taken in radians.

COD***/
// CMD RADSIN 1 1 1 r
Value *Compiler::genRadsin(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);
    return generateCallExtern(TyDouble, "sin", {PaDouble(children[0])});
}

/***DOC RADCOS
RADCOS radians

outputs the cosine of its input, which is taken in radians.

COD***/
// CMD RADCOS 1 1 1 r
Value *Compiler::genRadcos(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);
    return generateCallExtern(TyDouble, "cos", {PaDouble(children[0])});
}

/***DOC RADARCTAN
RADARCTAN num
(RADARCTAN x y)

 outputs the arctangent, in radians, of its input.  With two
 inputs, outputs the arctangent of y/x, if x is nonzero, or
 pi/2 or -pi/2 depending on the sign of y, if x is zero.

 The expression 2*(RADARCTAN 0 1) can be used to get the
 value of pi.

COD***/
// CMD RADARCTAN 1 1 2 r
Value *Compiler::genRadarctan(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    Value *retval;

    // Calculate atan() or atan2() depending on number of children.
    if (children.size() == 1)
    {
        retval = generateCallExtern(TyDouble, "atan", {PaDouble(children[0])});
    }
    else
    {
        retval = generateCallExtern(TyDouble, "atan2", {PaDouble(children[1]), PaDouble(children[0])});
    }
    return retval;
}

/***DOC COS
COS degrees

outputs the cosine of its input, which is taken in degrees.

COD***/
// CMD COS 1 1 1 r
Value *Compiler::genCos(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnReal);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    Value *degToRad = CoDouble(M_PI / 180);
    Value *theta = scaff->builder.CreateFMul(children[0], degToRad, "theta");
    return generateCallExtern(TyDouble, "cos", {PaDouble(theta)});
}

/***DOC LESSP LESS?
LESSP num1 num2
LESS? num1 num2
num1 < num2

outputs TRUE if its first input is strictly less than its second.

COD***/
// CMD LESSP 2 2 2 b
// CMD LESS? 2 2 2 b
Value *Compiler::genLessp(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnBool);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    return scaff->builder.CreateFCmpULT(children[0], children[1], "lessp");
}

/***DOC GREATERP GREATER?
GREATERP num1 num2
GREATER? num1 num2
num1 > num2

outputs TRUE if its first input is strictly greater than its second.

COD***/
// CMD GREATERP 2 2 2 b
// CMD GREATER? 2 2 2 b
Value *Compiler::genGreaterp(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnBool);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    return scaff->builder.CreateFCmpUGT(children[0], children[1], "greaterp");
}

/***DOC LESSEQUALP LESSEQUAL?
LESSEQUALP num1 num2
LESSEQUAL? num1 num2
num1 <= num2

outputs TRUE if its first input is less than or equal to its second.

COD***/
// CMD LESSEQUALP 2 2 2 b
// CMD LESSEQUAL? 2 2 2 b
Value *Compiler::genLessequalp(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnBool);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    return scaff->builder.CreateFCmpULE(children[0], children[1], "lessp");
}

/***DOC GREATEREQUALP GREATEREQUAL?
GREATEREQUALP num1 num2
GREATEREQUAL? num1 num2
num1 >= num2

outputs TRUE if its first input is greater than or equal to its second.

COD***/
// CMD GREATEREQUALP 2 2 2 b
// CMD GREATEREQUAL? 2 2 2 b
Value *Compiler::genGreaterequalp(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnBool);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    return scaff->builder.CreateFCmpUGE(children[0], children[1], "greaterp");
}

/***DOC NOT
NOT tf

outputs TRUE if the input is FALSE, and vice versa.  The input can be
a list, in which case it is taken as an expression to run; that
expression must produce a TRUE or FALSE value.


COD***/
// CMD NOT 1 1 1 b
Value *Compiler::genNot(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnBool);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnBool);

    return scaff->builder.CreateSub(CoBool(1), children[0], "not");
}

/***DOC RANDOM
RANDOM num
(RANDOM start end)

 with one input, outputs a random nonnegative integer less than its
 input, which must be a positive integer.

 With two inputs, RANDOM outputs a random integer greater than or
 equal to the first input, and less than or equal to the second
 input.  Both inputs must be integers, and the first must be less
 than the second.  (RANDOM 0 9) is equivalent to RANDOM 10;
 (RANDOM 3 8) is equivalent to (RANDOM 6)+3.

COD***/
// CMD RANDOM 1 1 2 r
Value *Compiler::genRandom(DatumPtr node, RequestReturnType returnType)
{
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);
    std::vector<Value *> iChildren;
    iChildren.reserve(children.size());

    if (children.size() == 1)
    {
        generateNotZeroFromDouble(node.astnodeValue(), children[0]);
    }

    for (int i = 0; i < children.size(); ++i)
    {
        iChildren.push_back(generateInt32FromDouble(node.astnodeValue(), children[i], true));
    }

    if (children.size() == 1)
    {
        return generateCallExtern(TyDouble, "random1", {PaInt32(iChildren[0])});
    }

    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();

    BasicBlock *notGTBB = BasicBlock::Create(*scaff->theContext, "notGT", theFunction);
    BasicBlock *isGTBB = BasicBlock::Create(*scaff->theContext, "isGT", theFunction);

    Value *start = iChildren[0];
    Value *end = iChildren[1];

    Value *cond = scaff->builder.CreateICmpSLT(start, end, "isValidTest");
    scaff->builder.CreateCondBr(cond, isGTBB, notGTBB);

    scaff->builder.SetInsertPoint(notGTBB);
    Value *errWhat = generateWordFromDouble(children[1]);
    Value *errObj = generateErrorNoLike(node.astnodeValue(), errWhat);
    scaff->builder.CreateRet(errObj);

    scaff->builder.SetInsertPoint(isGTBB);

    return generateCallExtern(TyDouble, "random2", {PaInt32(iChildren[0]), PaInt32(iChildren[1])});
}

/***DOC RERANDOM
RERANDOM
(RERANDOM seed)

 command.  Makes the results of RANDOM reproducible.  Ordinarily
 the sequence of random numbers is different each time Logo is
 used.  If you need the same sequence of pseudo-random numbers
 repeatedly, e.g. to debug a program, say RERANDOM before the
 first invocation of RANDOM.  If you need more than one repeatable
 sequence, you can give RERANDOM a nonnegative integer input; each
 possible input selects a unique sequence of numbers.

COD***/
// CMD RERANDOM 0 0 1 n
Value *Compiler::genRerandom(DatumPtr node, RequestReturnType returnType)
{
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);

    if (children.size() == 1)
    {
        Value *seed = generateNotNegativeFromDouble(node.astnodeValue(), children[0]);
        seed = generateInt32FromDouble(node.astnodeValue(), seed, true);
        generateCallExtern(TyVoid, "setRandomWithSeed", {PaInt32(seed)});
    }
    else
    {
        generateCallExtern(TyVoid, "setRandom", {});
    }
    return generateVoidRetval(node);
}

/***DOC FORM
FORM num width precision

 outputs a word containing a printable representation of "num",
 possibly preceded by spaces (and therefore not a number for
 purposes of performing arithmetic operations), with at least
 "width" characters, including exactly "precision" digits after
 the decimal point.  (If "precision" is 0 then there will be no
 decimal point in the output.)

COD***/
// CMD FORM 3 3 3 d
Value *Compiler::genForm(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnBool);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnReal);
    Value *num = children[0];
    Value *width = generateInt32FromDouble(node.astnodeValue(), children[1], true);
    Value *precision = generateNotNegativeFromDouble(node.astnodeValue(), children[2]);
    precision = generateInt32FromDouble(node.astnodeValue(), precision, true);
    return generateCallExtern(
        TyAddr, "getFormForNumber", {PaAddr(evaluator), PaDouble(num), PaInt32(width), PaInt32(precision)});
}

// Add infix entries to table. This will cause an error if they are used as
// prefix operators.
// CMD + 1 1 1 d
// CMD * 1 1 1 d
// CMD / 1 1 1 d
// CMD < 1 1 1 d
// CMD > 1 1 1 d
// CMD = 1 1 1 d
// CMD <= 1 1 1 d
// CMD >= 1 1 1 d
// CMD <> 1 1 1 d
Value *Compiler::genInfixError(DatumPtr node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnBool);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnDatum);

    Value *err = generateErrorNotEnoughInputs(node.astnodeValue());
    return generateImmediateReturn(err);
}

/***DOC AND
AND tf1 tf2
    (AND tf1 tf2 tf3 ...)

 outputs TRUE if all inputs are TRUE, otherwise FALSE.  All inputs
 must be TRUE or FALSE.  (Comparison is case-insensitive regardless
 of the value of CASEIGNOREDP.  That is, "true" or "True" or "TRUE"
 are all the same.)  An input can be a list, in which case it is
 taken as an expression to run; that expression must produce a TRUE
 or FALSE value.  List expressions are evaluated from left to right;
 as soon as a FALSE value is found, the remaining inputs are not
 examined.  Example:

 MAKE "RESULT AND [NOT (:X = 0)] [(1 / :X) > .5]

 to avoid the division by zero if the first part is false.

COD***/
// CMD AND 0 2 -1 b
Value *Compiler::genAnd(DatumPtr node, RequestReturnType returnType)
{
    return generateAndOr(node, returnType, true);
}

/***DOC OR
OR tf1 tf2
(OR tf1 tf2 tf3 ...)

 outputs TRUE if any input is TRUE, otherwise FALSE.  All inputs
 must be TRUE or FALSE.  (Comparison is case-insensitive regardless
 of the value of CASEIGNOREDP.  That is, "true" or "True" or "TRUE"
 are all the same.)  An input can be a list, in which case it is
 taken as an expression to run; that expression must produce a TRUE
 or FALSE value.  List expressions are evaluated from left to right;
 as soon as a TRUE value is found, the remaining inputs are not
 examined.  Example:
     IF OR :X=0 [some.long.computation] [...]
 to avoid the long computation if the first condition is met.

COD***/
// CMD OR 0 2 -1 b
Value *Compiler::genOr(DatumPtr node, RequestReturnType returnType)
{
    return generateAndOr(node, returnType, false);
}

Value *Compiler::generateAndOr(DatumPtr node, RequestReturnType returnType, bool isAnd)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();

    Q_ASSERT(returnType && RequestReturnBool);
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnDatum);

    // If there are no children, simply return isAnd
    if (children.size() == 0)
        return CoBool(isAnd);

    BasicBlock *continueBB; // Where to go if a test results in mayContinue
    BasicBlock *exitNoContBB = BasicBlock::Create(*scaff->theContext, "exitNoCont");
    BasicBlock *exitMayContBB = BasicBlock::Create(*scaff->theContext, "exitMayCont");
    BasicBlock *exitBB = BasicBlock::Create(*scaff->theContext, "exit");
    for (int i = 0; i < children.size(); ++i)
    {
        Value *c = children[i];

        // If input is a Datum type (can be word or list)
        if (c->getType()->isPointerTy())
        {
            // Test that this is a List object
            BasicBlock *isListBB = BasicBlock::Create(*scaff->theContext, "isList", theFunction);
            BasicBlock *isNothingBB = BasicBlock::Create(*scaff->theContext, "isNothing", theFunction);
            BasicBlock *notListBB = BasicBlock::Create(*scaff->theContext, "notList", theFunction);

            BasicBlock *listTestBB = scaff->builder.GetInsertBlock();
            Value *dType = generateGetDatumIsa(c);
            Value *cond = scaff->builder.CreateICmpEQ(dType, CoInt32(Datum::typeList), "typeTest");
            scaff->builder.CreateCondBr(cond, isListBB, notListBB);

            scaff->builder.SetInsertPoint(isListBB);
            // The list gets executed.
            Value *listRunResult = generateCallList(c, RequestReturnDatum);
            Value *listRunResultType = generateGetDatumIsa(listRunResult);
            Value *listRunResultCond = scaff->builder.CreateICmpEQ(
                listRunResultType, CoInt32(Datum::typeASTNode), "listRunResultTypeTest");
            scaff->builder.CreateCondBr(listRunResultCond, isNothingBB, notListBB);

            // List execution resulted in nothing.
            scaff->builder.SetInsertPoint(isNothingBB);
            Value *errNoOutput = generateErrorNoOutput(c, node.astnodeValue());
            scaff->builder.CreateRet(errNoOutput);

            // Here we need to be sure we have a word, and subsequently, a bool.
            scaff->builder.SetInsertPoint(notListBB);
            PHINode *isWordPhi = scaff->builder.CreatePHI(TyAddr, 2, "isWordPhi");
            isWordPhi->addIncoming(listRunResult, isListBB);
            isWordPhi->addIncoming(c, listTestBB);

            c = generateWordFromDatum(node.astnodeValue(), isWordPhi);
            c = generateBoolFromWord(node.astnodeValue(), c);
            // bool continues.
        }
        if (c->getType()->isIntegerTy(1))
        {
            continueBB = BasicBlock::Create(*scaff->theContext, "isPossCont");
            Value *cond = scaff->builder.CreateICmpEQ(c, CoBool(isAnd), "isPossTest");
            scaff->builder.CreateCondBr(cond, continueBB, exitNoContBB);

            theFunction->insert(theFunction->end(), continueBB);
            scaff->builder.SetInsertPoint(continueBB);
        }
        else
        {
            return generateImmediateReturn(generateErrorNoLike(node.astnodeValue(), c));
        }
    }
    // If we made it this far then all tests have resulted in isAnd
    scaff->builder.CreateBr(exitMayContBB);

    theFunction->insert(theFunction->end(), exitMayContBB);
    scaff->builder.SetInsertPoint(exitMayContBB);
    scaff->builder.CreateBr(exitBB);

    // If we get here then a test resulted in !isAnd
    theFunction->insert(theFunction->end(), exitNoContBB);
    scaff->builder.SetInsertPoint(exitNoContBB);
    scaff->builder.CreateBr(exitBB);

    // Return the T/F
    theFunction->insert(theFunction->end(), exitBB);
    scaff->builder.SetInsertPoint(exitBB);
    PHINode *phiNode = scaff->builder.CreatePHI(TyBool, 2, "retval");
    phiNode->addIncoming(CoBool(!isAnd), exitNoContBB);
    phiNode->addIncoming(CoBool(isAnd), exitMayContBB);
    return phiNode;
}
