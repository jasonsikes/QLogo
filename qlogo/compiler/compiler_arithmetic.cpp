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

#include "astnode.h"
#include "compiler.h"
#include "compiler_internal.h"
#include "datum_types.h"
#include "sharedconstants.h"
#include "workspace/callframe.h"
#include "workspace/exports.h"
#include "workspace/kernel.h"

using namespace llvm;
using namespace llvm::orc;

namespace
{
// Exact in IEEE-754 double (powers of two, or integers with |x| <= 2^53).
constexpr double kInt32MinAsDouble = -2147483648.0;
constexpr double kInt32MaxAsDouble = 2147483647.0;
constexpr double kUInt32MaxAsDouble = 4294967295.0;
} // namespace

Value *Compiler::generateIsExactIntegerInRange(Value *src, double lo, double hi)
{
    Value *geLo = scaff_->builder_.CreateFCmpOGE(src, CoDouble(lo), DBG_NAME("geLo"));
    Value *leHi = scaff_->builder_.CreateFCmpOLE(src, CoDouble(hi), DBG_NAME("leHi"));
    Value *inRange = scaff_->builder_.CreateAnd(geLo, leHi, DBG_NAME("inRange"));
    // llvm.trunc stays in double, so this never poisons (unlike fptosi of NaN/Inf/out-of-range).
    Value *truncated =
        scaff_->builder_.CreateUnaryIntrinsic(Intrinsic::trunc, src, {}, DBG_NAME("trunc"));
    Value *isInt = scaff_->builder_.CreateFCmpOEQ(src, truncated, DBG_NAME("isExactInt"));
    return scaff_->builder_.CreateAnd(inRange, isInt, DBG_NAME("isExactIntInRange"));
}

Value *Compiler::generateIsExactInt32(Value *src, bool isSigned)
{
    if (isSigned)
        return generateIsExactIntegerInRange(src, kInt32MinAsDouble, kInt32MaxAsDouble);
    return generateIsExactIntegerInRange(src, 0.0, kUInt32MaxAsDouble);
}

Value *Compiler::generateInt32FromDouble(ASTNode *parent, Value *src, bool isSigned)
{
    Value *accepted = generateValidationDouble(parent, src, [this, isSigned](Value *candidate) {
        return generateIsExactInt32(candidate, isSigned);
    });
    return isSigned ? scaff_->builder_.CreateFPToSI(accepted, TyInt32, DBG_NAME("FpToInt"))
                    : scaff_->builder_.CreateFPToUI(accepted, TyInt32, DBG_NAME("FpToInt"));
}

Value *Compiler::generateNotNegativeInt32FromDouble(ASTNode *parent, Value *src)
{
    Value *accepted = generateValidationDouble(parent, src, [this](Value *candidate) {
        return generateIsExactIntegerInRange(candidate, 0.0, kInt32MaxAsDouble);
    });
    return scaff_->builder_.CreateFPToSI(accepted, TyInt32, DBG_NAME("FpToInt"));
}

Value *Compiler::generateNotZeroInt32FromDouble(ASTNode *parent, Value *src)
{
    Value *accepted = generateValidationDouble(parent, src, [this](Value *candidate) {
        Value *isInt = generateIsExactInt32(candidate, true);
        Value *notZero = scaff_->builder_.CreateFCmpONE(candidate, CoDouble(0.0), DBG_NAME("notZero"));
        return scaff_->builder_.CreateAnd(isInt, notZero, DBG_NAME("isNonzeroInt32"));
    });
    return scaff_->builder_.CreateFPToSI(accepted, TyInt32, DBG_NAME("FpToInt"));
}

Value *Compiler::generateI32Shift(Value *value, Value *count, bool arithmetic)
{
    Value *isLeft = scaff_->builder_.CreateICmpSGE(count, CoInt32(0), DBG_NAME("isLeft"));
    // Wrapping negate: INT_MIN stays INT_MIN, then tooBig catches it as unsigned 2^31.
    Value *negCount = scaff_->builder_.CreateSub(CoInt32(0), count, DBG_NAME("negCount"));
    Value *absCount = scaff_->builder_.CreateSelect(isLeft, count, negCount, DBG_NAME("absCount"));
    Value *tooBig = scaff_->builder_.CreateICmpUGE(absCount, CoInt32(32), DBG_NAME("tooBig"));
    // Shift amount is always 0..31, so shl/ashr/lshr are defined.
    Value *safeAmt = scaff_->builder_.CreateSelect(tooBig, CoInt32(31), absCount, DBG_NAME("safeAmt"));

    Value *left = scaff_->builder_.CreateShl(value, safeAmt, DBG_NAME("leftShift"));
    left = scaff_->builder_.CreateSelect(tooBig, CoInt32(0), left, DBG_NAME("leftSat"));

    Value *right;
    if (arithmetic)
    {
        // ashr by 31 already sign-fills; no extra select.
        right = scaff_->builder_.CreateAShr(value, safeAmt, DBG_NAME("rightShift"));
    }
    else
    {
        right = scaff_->builder_.CreateLShr(value, safeAmt, DBG_NAME("rightShift"));
        right = scaff_->builder_.CreateSelect(tooBig, CoInt32(0), right, DBG_NAME("rightSat"));
    }
    return scaff_->builder_.CreateSelect(isLeft, left, right, DBG_NAME("shiftResult"));
}

Value *Compiler::generateI32SRem(Value *num, Value *denom)
{
    Value *isMin = scaff_->builder_.CreateICmpEQ(num, CoInt32(0x80000000), DBG_NAME("isIntMin"));
    Value *isNegOne = scaff_->builder_.CreateICmpEQ(denom, CoInt32(-1), DBG_NAME("isNegOne"));
    Value *overflow = scaff_->builder_.CreateAnd(isMin, isNegOne, DBG_NAME("sremOverflow"));
    // srem INT_MIN, 1 == 0, which is the mathematical remainder of INT_MIN / -1.
    Value *safeDenom = scaff_->builder_.CreateSelect(overflow, CoInt32(1), denom, DBG_NAME("safeDenom"));
    return scaff_->builder_.CreateSRem(num, safeDenom, DBG_NAME("remainder"));
}

Value *Compiler::generateNotZeroFromDouble(ASTNode *parent, Value *src)
{
    return generateValidationDouble(parent, src, [this](Value *val) {
        return scaff_->builder_.CreateFCmpONE(val, CoDouble(0.0), DBG_NAME("isZeroTest"));
    });
}

Value *Compiler::generateNotNegativeFromDouble(ASTNode *parent, Value *src)
{
    return generateValidationDouble(parent, src, [this](Value *val) {
        return scaff_->builder_.CreateFCmpOGE(val, CoDouble(0.0), DBG_NAME("isZeroTest"));
    });
}

Value *Compiler::generateGTZeroFromDouble(ASTNode *parent, Value *src)
{
    return generateValidationDouble(parent, src, [this](Value *val) {
        return scaff_->builder_.CreateFCmpOGT(val, CoDouble(0.0), DBG_NAME("isZeroTest"));
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
Value *Compiler::genArctan(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    const std::vector<Value *> &children = cs.children();

    Value *radToDeg = CoDouble(180 / PI);
    Value *retval;

    // Calculate atan() or atan2() depending on number of children.
    if (children.size() == 1)
    {
        retval = scaff_->builder_.CreateUnaryIntrinsic(Intrinsic::atan, children[0], {}, DBG_NAME("atan"));
    }
    else
    {
        retval = scaff_->builder_.CreateBinaryIntrinsic(
            Intrinsic::atan2, children[1], children[0], {}, DBG_NAME("atan2"));
    }
    retval = scaff_->builder_.CreateFMul(retval, radToDeg, DBG_NAME("theta"));
    return cs.ret(retval);
}

/***DOC ASHIFT
ASHIFT num1 num2

 outputs "num1" arithmetic-shifted to the left by "num2" bits.
 If num2 is negative, the shift is to the right with sign
 extension.  The inputs must be integers.

COD***/
// CMD ASHIFT 2 2 2 r
Value *Compiler::genAshift(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal, TypeReal);
    auto [num1, num2] = cs.children();

    num1 = generateInt32FromDouble(node.astnodeValue(), num1, true);
    num2 = generateInt32FromDouble(node.astnodeValue(), num2, true);
    Value *retval = generateI32Shift(num1, num2, true);
    return cs.ret(scaff_->builder_.CreateSIToFP(retval, TyDouble, DBG_NAME("IntToFP")));
}

/***DOC LSHIFT
LSHIFT num1 num2

 outputs "num1" logical-shifted to the left by "num2" bits.
 If num2 is negative, the shift is to the right with zero fill.
 The inputs must be integers.

COD***/
// CMD LSHIFT 2 2 2 r
Value *Compiler::genLshift(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal, TypeReal);
    auto [num1, num2] = cs.children();

    num1 = generateInt32FromDouble(node.astnodeValue(), num1, true);
    num2 = generateInt32FromDouble(node.astnodeValue(), num2, true);
    Value *retval = generateI32Shift(num1, num2, false);
    return cs.ret(scaff_->builder_.CreateSIToFP(retval, TyDouble, DBG_NAME("IntToFP")));
}

/***DOC BITAND
BITAND num1 num2
(BITAND num1 num2 num3 ...)

 outputs the bitwise AND of its inputs, which must be unsigned integers.

COD***/
// CMD BITAND 0 2 -1 r
Value *Compiler::genBitand(const DatumPtr &node, TypeMask expected)
{
    return generateBitOp(node, expected, Instruction::And);
}

/***DOC BITOR
BITOR num1 num2
(BITOR num1 num2 num3 ...)

 outputs the bitwise OR of its inputs, which must be integers.

COD***/
// CMD BITOR 0 2 -1 r
Value *Compiler::genBitor(const DatumPtr &node, TypeMask expected)
{
    return generateBitOp(node, expected, Instruction::Or);
}

/***DOC BITXOR
BITXOR num1 num2
(BITXOR num1 num2 num3 ...)

 outputs the bitwise EXCLUSIVE OR of its inputs, which must be
 integers.

COD***/
// CMD BITXOR 0 2 -1 r
Value *Compiler::genBitxor(const DatumPtr &node, TypeMask expected)
{
    return generateBitOp(node, expected, Instruction::Xor);
}

Value *Compiler::generateBitOp(const DatumPtr &node, TypeMask expected, Instruction::BinaryOps op)
{
    Q_ASSERT(op == Instruction::And || op == Instruction::Or || op == Instruction::Xor);

    CallSite cs(this, node, TypeReal);
    std::vector<Value *> children = cs.children();

    for (auto &child : children)
        child = generateInt32FromDouble(node.astnodeValue(), child, true);

    if (children.size() == 0)
        return cs.ret(CoDouble(op == Instruction::And ? -1.0 : 0.0));

    const char *opName = op == Instruction::And ? "BitAND" : op == Instruction::Or ? "BitOR" : "BitXOR";
    Value *retval = children[0];
    for (int i = 1; i < children.size(); ++i)
        retval = scaff_->builder_.CreateBinOp(op, retval, children[i], DBG_NAME(opName));
    return cs.ret(scaff_->builder_.CreateSIToFP(retval, TyDouble, DBG_NAME("IntToFP")));
}

/***DOC BITNOT
BITNOT num

 outputs the bitwise NOT of its input, which must be an integer.

COD***/
// CMD BITNOT 1 1 1 r
Value *Compiler::genBitnot(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    Value *num = cs.child(0);

    num = generateInt32FromDouble(node.astnodeValue(), num, true);
    num = scaff_->builder_.CreateXor(num, CoInt32(-1), DBG_NAME("bitNOT"));

    return cs.ret(scaff_->builder_.CreateSIToFP(num, TyDouble, DBG_NAME("IntToFP")));
}

/***DOC MODULO
MODULO num1 num2

 outputs the remainder on dividing "num1" by "num2"; both must be
 integers and the result is an integer with the same sign as num2.

COD***/
// CMD MODULO 2 2 2 r
Value *Compiler::genModulo(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal, TypeReal);
    auto [num, denom] = cs.children();

    num = generateInt32FromDouble(node.astnodeValue(), num, true);
    denom = generateNotZeroInt32FromDouble(node.astnodeValue(), denom);

    Value *r = generateI32SRem(num, denom);
    // UCBLogo (math.c, fcn 'm'): if ((r < 0) != (d < 0)) r += d;
    // Zero is treated as non-negative, so MODULO 6 -3 is -3, not 0.
    Value *rNeg = scaff_->builder_.CreateICmpSLT(r, CoInt32(0), DBG_NAME("rNeg"));
    Value *dNeg = scaff_->builder_.CreateICmpSLT(denom, CoInt32(0), DBG_NAME("dNeg"));
    Value *adjust = scaff_->builder_.CreateXor(rNeg, dNeg, DBG_NAME("signsDiffer"));
    Value *adjusted = scaff_->builder_.CreateAdd(r, denom, DBG_NAME("rPlusDenom"));
    r = scaff_->builder_.CreateSelect(adjust, adjusted, r, DBG_NAME("modulo"));
    return cs.ret(scaff_->builder_.CreateSIToFP(r, TyDouble, DBG_NAME("IntToFP")));
}

/***DOC SQRT
SQRT num

 outputs the square root of the input, which must be nonnegative.

COD***/
// CMD SQRT 1 1 1 r
Value *Compiler::genSqrt(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    Value *num = cs.child(0);

    num = generateNotNegativeFromDouble(node.astnodeValue(), num);
    return cs.ret(scaff_->builder_.CreateUnaryIntrinsic(Intrinsic::sqrt, num, {}, DBG_NAME("sqrt")));
}

/***DOC POWER
POWER num1 num2

 outputs "num1" to the "num2" power.  If num1 is negative, then
 num2 must be an integer.

COD***/
// CMD POWER 2 2 2 r
Value *Compiler::genPower(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal, TypeReal);
    auto [num1, num2] = cs.children();

    BasicBlock *startBB = scaff_->builder_.GetInsertBlock();
    Function *theFunction = startBB->getParent();

    BasicBlock *isNegativeBB = scaff_->createBasicBlock(DBG_NAME("isNegative"));
    BasicBlock *notNegativeBB = BasicBlock::Create(*scaff_->theContext_, DBG_NAME("notNegative"));

    Value *cond = scaff_->builder_.CreateFCmpOGE(num1, CoDouble(0.0), DBG_NAME("isNegativeTest"));
    scaff_->builder_.CreateCondBr(cond, notNegativeBB, isNegativeBB);

    scaff_->builder_.SetInsertPoint(isNegativeBB);
    Value *num2Int = generateValidationDouble(node.astnodeValue(), num2, [this](Value *candidate) {
        return generateIsExactInt32(candidate, true);
    });
    BasicBlock *postNegativeBB = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.CreateBr(notNegativeBB);

    theFunction->insert(theFunction->end(), notNegativeBB);
    scaff_->builder_.SetInsertPoint(notNegativeBB);
    PHINode *num2Phi = scaff_->builder_.CreatePHI(TyDouble, 2, DBG_NAME("num2Phi"));
    num2Phi->addIncoming(num2, startBB);
    num2Phi->addIncoming(num2Int, postNegativeBB);
    return cs.ret(scaff_->builder_.CreateBinaryIntrinsic(Intrinsic::pow, num1, num2Phi, {}, DBG_NAME("pow")));
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
Value *Compiler::genMinus(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    Value *num = cs.child(0);

    return cs.ret(scaff_->builder_.CreateFNeg(num, DBG_NAME("negtmp")));
}

/***DOC PRODUCT
PRODUCT num1 num2
(PRODUCT num1 num2 num3 ...)
num1 * num2

outputs the product of its inputs.

COD***/
// CMD PRODUCT 0 2 -1 r
Value *Compiler::genProduct(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    const std::vector<Value *> &children = cs.children();

    // No children? Return identity.
    if (children.size() == 0)
        return cs.ret(CoDouble(1.0));

    // Loop through the children accumulating from the left.
    Value *accum = children[0];
    for (int i = 1; i < children.size(); ++i)
    {
        accum = scaff_->builder_.CreateFMul(accum, children[i], DBG_NAME("multmp"));
    }
    return cs.ret(accum);
}

/***DOC SUM
SUM num1 num2
(SUM num1 num2 num3 ...)
num1 + num2

outputs the sum of its inputs.

COD***/
// CMD SUM 0 2 -1 r
Value *Compiler::genSum(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    const std::vector<Value *> &children = cs.children();

    // No children? Return identity.
    if (children.size() == 0)
        return cs.ret(CoDouble(0.0));

    // Loop through the children accumulating from the left.
    Value *accum = children[0];
    for (int i = 1; i < children.size(); ++i)
    {
        accum = scaff_->builder_.CreateFAdd(accum, children[i], DBG_NAME("addtmp"));
    }
    return cs.ret(accum);
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
Value *Compiler::genDifference(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal, TypeReal);
    auto [num1, num2] = cs.children();

    return cs.ret(scaff_->builder_.CreateFSub(num1, num2, DBG_NAME("subtmp")));
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
Value *Compiler::genQuotient(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    const std::vector<Value *> &children = cs.children();

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
    return cs.ret(scaff_->builder_.CreateFDiv(num, denom, DBG_NAME("quotmp")));
}

/***DOC REMAINDER
REMAINDER num1 num2

 outputs the remainder on dividing "num1" by "num2"; both must be
 integers and the result is an integer with the same sign as num1.

COD***/
// CMD REMAINDER 2 2 2 r
Value *Compiler::genRemainder(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal, TypeReal);
    auto [num, denom] = cs.children();

    num = generateInt32FromDouble(node.astnodeValue(), num, true);
    denom = generateNotZeroInt32FromDouble(node.astnodeValue(), denom);
    Value *retval = generateI32SRem(num, denom);
    return cs.ret(scaff_->builder_.CreateSIToFP(retval, TyDouble, DBG_NAME("IntToFP")));
}

/***DOC SIN
SIN degrees

outputs the sine of its input, which is taken in degrees.

COD***/
// CMD SIN 1 1 1 r
Value *Compiler::genSin(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    Value *num = cs.child(0);

    Value *degToRad = CoDouble(PI / 180);
    Value *theta = scaff_->builder_.CreateFMul(num, degToRad, DBG_NAME("theta"));
    return cs.ret(scaff_->builder_.CreateUnaryIntrinsic(Intrinsic::sin, theta, {}, DBG_NAME("sin")));
}

/***DOC INT
INT num

outputs its input with fractional part removed, i.e., an integer
with the same sign as the input, whose absolute value is the
largest integer less than or equal to the absolute value of
the input.

COD***/
// CMD INT 1 1 1 r
Value *Compiler::genInt(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    Value *num = cs.child(0);

    return cs.ret(scaff_->builder_.CreateUnaryIntrinsic(Intrinsic::trunc, num, {}, DBG_NAME("trunc")));
}

/***DOC ROUND
ROUND num

outputs the nearest integer to the input.

COD***/
// CMD ROUND 1 1 1 r
Value *Compiler::genRound(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    Value *num = cs.child(0);

    return cs.ret(scaff_->builder_.CreateUnaryIntrinsic(Intrinsic::round, num, {}, DBG_NAME("round")));
}

/***DOC EXP
EXP num

outputs e (2.718281828+) to the input power.

COD***/
// CMD EXP 1 1 1 r
Value *Compiler::genExp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    Value *num = cs.child(0);

    return cs.ret(scaff_->builder_.CreateUnaryIntrinsic(Intrinsic::exp, num, {}, DBG_NAME("exp")));
}

/***DOC LOG10
LOG10 num

 outputs the common logarithm of the input. Input must be greater
 than zero.

COD***/
// CMD LOG10 1 1 1 r
Value *Compiler::genLog10(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    Value *num = cs.child(0);

    num = generateGTZeroFromDouble(node.astnodeValue(), num);
    return cs.ret(scaff_->builder_.CreateUnaryIntrinsic(Intrinsic::log10, num, {}, DBG_NAME("log10")));
}

/***DOC LN
LN num

 outputs the natural logarithm of the input. Input must be greater
 than zero.

COD***/
// CMD LN 1 1 1 r
Value *Compiler::genLn(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    Value *num = cs.child(0);

    num = generateGTZeroFromDouble(node.astnodeValue(), num);
    return cs.ret(scaff_->builder_.CreateUnaryIntrinsic(Intrinsic::log, num, {}, DBG_NAME("ln")));
}

/***DOC RADSIN
RADSIN radians

outputs the sine of its input, which is taken in radians.

COD***/
// CMD RADSIN 1 1 1 r
Value *Compiler::genRadsin(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    Value *num = cs.child(0);

    return cs.ret(scaff_->builder_.CreateUnaryIntrinsic(Intrinsic::sin, num, {}, DBG_NAME("sin")));
}

/***DOC RADCOS
RADCOS radians

outputs the cosine of its input, which is taken in radians.

COD***/
// CMD RADCOS 1 1 1 r
Value *Compiler::genRadcos(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    Value *num = cs.child(0);

    return cs.ret(scaff_->builder_.CreateUnaryIntrinsic(Intrinsic::cos, num, {}, DBG_NAME("cos")));
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
Value *Compiler::genRadarctan(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    const std::vector<Value *> &children = cs.children();

    Value *retval;

    // Calculate atan() or atan2() depending on number of children.
    if (children.size() == 1)
    {
        retval = scaff_->builder_.CreateUnaryIntrinsic(Intrinsic::atan, children[0], {}, DBG_NAME("atan"));
    }
    else
    {
        retval = scaff_->builder_.CreateBinaryIntrinsic(
            Intrinsic::atan2, children[1], children[0], {}, DBG_NAME("atan2"));
    }
    return cs.ret(retval);
}

/***DOC COS
COS degrees

outputs the cosine of its input, which is taken in degrees.

COD***/
// CMD COS 1 1 1 r
Value *Compiler::genCos(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    Value *num = cs.child(0);

    Value *degToRad = CoDouble(PI / 180);
    Value *theta = scaff_->builder_.CreateFMul(num, degToRad, DBG_NAME("theta"));
    return cs.ret(scaff_->builder_.CreateUnaryIntrinsic(Intrinsic::cos, theta, {}, DBG_NAME("cos")));
}

/***DOC LESSP LESS?
LESSP num1 num2
LESS? num1 num2
num1 < num2

outputs TRUE if its first input is strictly less than its second.

COD***/
// CMD LESSP 2 2 2 b
// CMD LESS? 2 2 2 b
Value *Compiler::genLessp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal, TypeReal);
    auto [num1, num2] = cs.children();

    return cs.ret(scaff_->builder_.CreateFCmpOLT(num1, num2, DBG_NAME("lessp")));
}

/***DOC GREATERP GREATER?
GREATERP num1 num2
GREATER? num1 num2
num1 > num2

outputs TRUE if its first input is strictly greater than its second.

COD***/
// CMD GREATERP 2 2 2 b
// CMD GREATER? 2 2 2 b
Value *Compiler::genGreaterp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal, TypeReal);
    auto [num1, num2] = cs.children();

    return cs.ret(scaff_->builder_.CreateFCmpOGT(num1, num2, DBG_NAME("greaterp")));
}

/***DOC LESSEQUALP LESSEQUAL?
LESSEQUALP num1 num2
LESSEQUAL? num1 num2
num1 <= num2

outputs TRUE if its first input is less than or equal to its second.

COD***/
// CMD LESSEQUALP 2 2 2 b
// CMD LESSEQUAL? 2 2 2 b
Value *Compiler::genLessequalp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal, TypeReal);
    auto [num1, num2] = cs.children();

    return cs.ret(scaff_->builder_.CreateFCmpOLE(num1, num2, DBG_NAME("lessp")));
}

/***DOC GREATEREQUALP GREATEREQUAL?
GREATEREQUALP num1 num2
GREATEREQUAL? num1 num2
num1 >= num2

outputs TRUE if its first input is greater than or equal to its second.

COD***/
// CMD GREATEREQUALP 2 2 2 b
// CMD GREATEREQUAL? 2 2 2 b
Value *Compiler::genGreaterequalp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal, TypeReal);
    auto [num1, num2] = cs.children();

    return cs.ret(scaff_->builder_.CreateFCmpOGE(num1, num2, DBG_NAME("greaterp")));
}

/***DOC NOT
NOT tf

outputs TRUE if the input is FALSE, and vice versa.  The input can be
a list, in which case it is taken as an expression to run; that
expression must produce a TRUE or FALSE value.


COD***/
// CMD NOT 1 1 1 b
Value *Compiler::genNot(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeBool);
    Value *tf = cs.child(0);

    return cs.ret(scaff_->builder_.CreateSub(CoBool(1), tf, DBG_NAME("not")));
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
Value *Compiler::genRandom(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    std::vector<Value *> children = cs.children();

    if (children.size() == 1)
    {
        Value *accepted = generateValidationDouble(node.astnodeValue(), children[0], [this](Value *candidate) {
            return generateIsExactIntegerInRange(candidate, 1.0, kInt32MaxAsDouble);
        });
        Value *num = scaff_->builder_.CreateFPToSI(accepted, TyInt32, DBG_NAME("FpToInt"));
        return cs.ret(generateCallExtern(random1, PaInt32(num)));
    }

    Value *start = generateInt32FromDouble(node.astnodeValue(), children[0], true);
    Value *acceptedEnd = generateValidationDouble(node.astnodeValue(), children[1], [this, start](Value *candidate) {
        Value *isInt = generateIsExactInt32(candidate, true);
        Value *startAsDouble = scaff_->builder_.CreateSIToFP(start, TyDouble, DBG_NAME("startAsDouble"));
        Value *endGtStart = scaff_->builder_.CreateFCmpOGT(candidate, startAsDouble, DBG_NAME("endGtStart"));
        return scaff_->builder_.CreateAnd(isInt, endGtStart, DBG_NAME("isValidTest"));
    });
    Value *endInt = scaff_->builder_.CreateFPToSI(acceptedEnd, TyInt32, DBG_NAME("FpToInt"));

    return cs.ret(generateCallExtern(random2, PaInt32(start), PaInt32(endInt)));
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
Value *Compiler::genRerandom(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal);
    const std::vector<Value *> &children = cs.children();

    if (children.size() == 1)
    {
        Value *seed = generateNotNegativeFromDouble(node.astnodeValue(), children[0]);
        seed = generateInt32FromDouble(node.astnodeValue(), seed, true);
        generateCallExtern(setRandomWithSeed, PaInt32(seed));
    }
    else
    {
        generateCallExtern(setRandom);
    }
    return cs.ret(generateVoidRetval(node));
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
Value *Compiler::genForm(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal, TypeReal, TypeReal);
    auto [num, width, precision] = cs.children();

    width = generateInt32FromDouble(node.astnodeValue(), width, true);

    // TODO: Combine into one validation function.
    precision = generateNotNegativeFromDouble(node.astnodeValue(), precision);
    precision = generateInt32FromDouble(node.astnodeValue(), precision, true);
    return cs.ret(generateCallExtern(
        getFormForNumber, PaAddr(scaff_->evaluator_), PaDouble(num), PaInt32(width), PaInt32(precision)));
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
Value *Compiler::genInfixError(const DatumPtr &node, TypeMask expected)
{
    generateChild(node.astnodeValue(), 0, TypeDatum);

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
Value *Compiler::genAnd(const DatumPtr &node, TypeMask expected)
{
    return generateAndOr(node, expected, true);
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
Value *Compiler::genOr(const DatumPtr &node, TypeMask expected)
{
    return generateAndOr(node, expected, false);
}

Value *Compiler::generateAndOr(const DatumPtr &node, TypeMask expected, bool isAnd)
{
    Function *theFunction = scaff_->builder_.GetInsertBlock()->getParent();

    CallSite cs(this, node, TypeDB);
    const std::vector<Value *> &children = cs.children();

    // If there are no children, simply return isAnd
    if (children.size() == 0)
        return cs.ret(CoBool(isAnd));

    BasicBlock *continueBB; // Where to go if a test results in mayContinue
    BasicBlock *exitNoContBB = BasicBlock::Create(*scaff_->theContext_, DBG_NAME("exitNoCont"));
    BasicBlock *exitMayContBB = BasicBlock::Create(*scaff_->theContext_, DBG_NAME("exitMayCont"));
    BasicBlock *exitBB = BasicBlock::Create(*scaff_->theContext_, DBG_NAME("exit"));
    for (auto &child : children)
    {
        Value *c = child;

        // If input is a Datum type (can be word or list)
        if (c->getType()->isPointerTy())
        {
            c = generateListExecIfList(node.astnodeValue(), c);
            c = generateBoolFromDatum(node.astnodeValue(), c);
            // bool continues.
        }
        if (c->getType()->isIntegerTy(1))
        {
            continueBB = BasicBlock::Create(*scaff_->theContext_, DBG_NAME("isPossCont"));
            Value *cond = scaff_->builder_.CreateICmpEQ(c, CoBool(isAnd), DBG_NAME("isPossTest"));
            scaff_->builder_.CreateCondBr(cond, continueBB, exitNoContBB);

            theFunction->insert(theFunction->end(), continueBB);
            scaff_->builder_.SetInsertPoint(continueBB);
        }
        else
        {
            return generateImmediateReturn(generateErrorNoLike(node.astnodeValue(), c));
        }
    }
    // If we made it this far then all tests have resulted in isAnd
    scaff_->builder_.CreateBr(exitMayContBB);

    theFunction->insert(theFunction->end(), exitMayContBB);
    scaff_->builder_.SetInsertPoint(exitMayContBB);
    scaff_->builder_.CreateBr(exitBB);

    // If we get here then a test resulted in !isAnd
    theFunction->insert(theFunction->end(), exitNoContBB);
    scaff_->builder_.SetInsertPoint(exitNoContBB);
    scaff_->builder_.CreateBr(exitBB);

    // Return the T/F
    theFunction->insert(theFunction->end(), exitBB);
    scaff_->builder_.SetInsertPoint(exitBB);
    PHINode *phiNode = scaff_->builder_.CreatePHI(TyBool, 2, DBG_NAME("retval"));
    phiNode->addIncoming(CoBool(!isAnd), exitNoContBB);
    phiNode->addIncoming(CoBool(isAnd), exitMayContBB);
    return cs.ret(phiNode);
}

Value *Compiler::generateListExecIfList(ASTNode *parent, Value *c)
{
    // Test that this is a List object
    BasicBlock *isListBB = scaff_->createBasicBlock(DBG_NAME("isList"));
    BasicBlock *isNothingBB = scaff_->createBasicBlock(DBG_NAME("isNothing"));
    BasicBlock *notListBB = scaff_->createBasicBlock(DBG_NAME("notList"));

    scaff_->addColdPathBlocks(isNothingBB);

    BasicBlock *listTestBB = scaff_->builder_.GetInsertBlock();
    Value *dType = generateGetDatumIsa(c);
    Value *mask = scaff_->builder_.CreateAnd(dType, CoInt32(Datum::typeList), DBG_NAME("dataTypeMask"));
    Value *cond = scaff_->builder_.CreateICmpNE(mask, CoInt32(0), DBG_NAME("dataTypeMaskTest"));
    scaff_->builder_.CreateCondBr(cond, isListBB, notListBB);

    scaff_->builder_.SetInsertPoint(isListBB);
    // The list gets executed (push/suspend/resume/pop); on return we are in the resume block.
    Value *listRunResult = generateCallList(c, TypeDatum);
    BasicBlock *listRunResultBB = scaff_->builder_.GetInsertBlock();
    Value *listRunResultType = generateGetDatumIsa(listRunResult);
    Value *listRunResultCond = scaff_->builder_.CreateICmpEQ(
        listRunResultType, CoInt32(Datum::typeASTNode), DBG_NAME("listRunResultTypeTest"));
    scaff_->builder_.CreateCondBr(listRunResultCond, isNothingBB, notListBB);

    // List execution resulted in nothing. Run ERRACT; a datum from it stands in for the list.
    scaff_->builder_.SetInsertPoint(isNothingBB);
    Value *recovered = generateRecoverDidntOutput(c, parent);
    BasicBlock *recoveredBB = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.CreateBr(notListBB);

    scaff_->builder_.SetInsertPoint(notListBB);
    PHINode *retval = scaff_->builder_.CreatePHI(TyAddr, 3, DBG_NAME("isWordPhi"));
    retval->addIncoming(listRunResult, listRunResultBB);
    retval->addIncoming(c, listTestBB);
    retval->addIncoming(recovered, recoveredBB);

    return retval;
}

/***DOC ISEQ
ISEQ from to						(library procedure)

    outputs a list of the integers from FROM to TO, inclusive.

        ? show iseq 3 7
        [3 4 5 6 7]
        ? show iseq 7 3
        [7 6 5 4 3]

COD***/

/***DOC RSEQ
RSEQ from to count					(library procedure)

    outputs a list of COUNT equally spaced rational numbers
    between FROM and TO, inclusive.

        ? show rseq 3 5 9
        [3 3.25 3.5 3.75 4 4.25 4.5 4.75 5]
        ? show rseq 3 5 5
        [3 3.5 4 4.5 5]


COD***/
