
//===-- qlogo/kernel.cpp - Kernel class implementation -------*- C++ -*-===//
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
/// This file contains a part of the implementation of the Kernel class, which is the
/// executor proper of the QLogo language. Specifically, this file contains the
/// implementations of the arithmetic operations.
///
/// See README.md in this directory for information about the documentation
/// structure for each Kernel::exc* method.
///
//===----------------------------------------------------------------------===//

#include "astnode.h"
#include "kernel.h"

#include <error.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// NUMERIC OPERATIONS

/***DOC SUM
SUM num1 num2
(SUM num1 num2 num3 ...)
num1 + num2

    outputs the sum of its inputs.

COD***/
// CMD SUM 0 2 -1
DatumPtr Kernel::excSum(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double result = 0;

    for (int i = 0; i < h.countOfChildren(); ++i)
    {
        result += h.numberAtIndex(i);
    }

    return h.ret(result);
}

/***DOC DIFFERENCE
DIFFERENCE num1 num2
num1 - num2

    outputs the difference of its inputs.  Minus sign means infix
    difference in ambiguous contexts (when preceded by a complete
    expression), unless it is preceded by a space and followed
    by a nonspace.  (See also MINUS.)

COD***/
// CMD DIFFERENCE 2 2 2
DatumPtr Kernel::excDifference(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);

    double b = h.numberAtIndex(1);

    double c = a - b;

    return h.ret(c);
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
// CMD MINUS 1 1 1
// CMD - 1 1 1
DatumPtr Kernel::excMinus(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);

    return h.ret(-a);
}

/***DOC PRODUCT
PRODUCT num1 num2
(PRODUCT num1 num2 num3 ...)
num1 * num2

    outputs the product of its inputs.

COD***/
// CMD PRODUCT 0 2 -1
DatumPtr Kernel::excProduct(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double result = 1;

    for (int i = 0; i < h.countOfChildren(); ++i)
    {
        result *= h.numberAtIndex(i);
    }

    return h.ret(result);
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
// CMD QUOTIENT 1 2 2
DatumPtr Kernel::excQuotient(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a, c;

    if (h.countOfChildren() == 2)
    {
        a = h.numberAtIndex(0);
        double b = h.validatedNumberAtIndex(1, [](double candidate) { return candidate != 0; });

        c = a / b;
    }
    else
    {
        double a = h.validatedNumberAtIndex(0, [](double candidate) { return candidate != 0; });
        c = 1 / a;
    }

    return h.ret(c);
}

/***DOC REMAINDER
REMAINDER num1 num2

    outputs the remainder on dividing "num1" by "num2"; both must be
    integers and the result is an integer with the same sign as num1.

COD***/
// CMD REMAINDER 2 2 2
DatumPtr Kernel::excRemainder(DatumPtr node)
{
    ProcedureHelper h(this, node);
    int a = h.integerAtIndex(0);

    int b = h.validatedIntegerAtIndex(1, [](int candidate) { return candidate != 0; });

    double c = a % b;

    return h.ret(c);
}

/***DOC MODULO
MODULO num1 num2

    outputs the remainder on dividing "num1" by "num2"; both must be
    integers and the result is an integer with the same sign as num2.

COD***/
// CMD MODULO 2 2 2
DatumPtr Kernel::excModulo(DatumPtr node)
{
    ProcedureHelper h(this, node);
    int a = h.integerAtIndex(0);

    int b = h.validatedIntegerAtIndex(1, [](int candidate) { return candidate != 0; });

    int r = a % b;
    double c = (r * b < 0) ? r + b : r;

    return h.ret(c);
}

/***DOC INT
INT num

    outputs its input with fractional part removed, i.e., an integer
    with the same sign as the input, whose absolute value is the
    largest integer less than or equal to the absolute value of
    the input.

COD***/
// CMD INT 1 1 1
DatumPtr Kernel::excInt(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);

    double b = trunc(a);

    return h.ret(b);
}

/***DOC ROUND
ROUND num

    outputs the nearest integer to the input.

COD***/
// CMD ROUND 1 1 1
DatumPtr Kernel::excRound(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);

    double b = round(a);

    return h.ret(b);
}

/***DOC SQRT
SQRT num

    outputs the square root of the input, which must be nonnegative.

COD***/
// CMD SQRT 1 1 1
DatumPtr Kernel::excSqrt(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.validatedNumberAtIndex(0, [](double candidate) { return candidate >= 0; });

    double c = sqrt(a);

    return h.ret(c);
}

/***DOC POWER
POWER num1 num2

    outputs "num1" to the "num2" power.  If num1 is negative, then
    num2 must be an integer.

COD***/
// CMD POWER 2 2 2
DatumPtr Kernel::excPower(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);
    double b;
    if (a >= 0)
    {
        b = h.numberAtIndex(1);
    }
    else
    {
        b = h.validatedNumberAtIndex(1, [](double candidate) { return candidate == trunc(candidate); });
    }

    double c = pow(a, b);

    return h.ret(c);
}

/***DOC EXP
EXP num

    outputs e (2.718281828+) to the input power.

COD***/
// CMD EXP 1 1 1
DatumPtr Kernel::excExp(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);

    double c = exp(a);

    return h.ret(c);
}

/***DOC LOG10
LOG10 num

    outputs the common logarithm of the input.

COD***/
// CMD LOG10 1 1 1
DatumPtr Kernel::excLog10(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.validatedNumberAtIndex(0, [](double candidate) { return candidate >= 0; });

    double c = log10(a);

    return h.ret(c);
}

/***DOC LN
LN num

    outputs the natural logarithm of the input.

COD***/
// CMD LN 1 1 1
DatumPtr Kernel::excLn(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.validatedNumberAtIndex(0, [](double candidate) { return candidate >= 0; });

    double c = log(a);

    return h.ret(c);
}

/***DOC SIN
SIN degrees

    outputs the sine of its input, which is taken in degrees.

COD***/
// CMD SIN 1 1 1
DatumPtr Kernel::excSin(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);

    double c = sin(M_PI / 180 * a);

    return h.ret(c);
}

/***DOC RADSIN
RADSIN radians

    outputs the sine of its input, which is taken in radians.

COD***/
// CMD RADSIN 1 1 1
DatumPtr Kernel::excRadsin(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);

    double c = sin(a);

    return h.ret(c);
}

/***DOC COS
COS degrees

    outputs the cosine of its input, which is taken in degrees.

COD***/
// CMD COS 1 1 1
DatumPtr Kernel::excCos(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);

    double c = cos(M_PI / 180 * a);

    return h.ret(c);
}

/***DOC RADCOS
RADCOS radians

    outputs the cosine of its input, which is taken in radians.

COD***/
// CMD RADCOS 1 1 1
DatumPtr Kernel::excRadcos(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);

    double c = cos(a);

    return h.ret(c);
}

/***DOC ARCTAN
ARCTAN num
(ARCTAN x y)

    outputs the arctangent, in degrees, of its input.  With two
    inputs, outputs the arctangent of y/x, if x is nonzero, or
    90 or -90 depending on the sign of y, if x is zero.

COD***/
// CMD ARCTAN 1 1 2
DatumPtr Kernel::excArctan(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);
    if (node.astnodeValue()->countOfChildren() == 1)
    {
        double c = atan(a) * 180 / M_PI;

        return h.ret(c);
    }
    double b = h.numberAtIndex(1);

    double c = atan2(b, a) * 180 / M_PI;

    return h.ret(c);
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
// CMD RADARCTAN 1 1 2
DatumPtr Kernel::excRadarctan(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);
    if (node.astnodeValue()->countOfChildren() == 1)
    {
        double c = atan(a);

        return h.ret(c);
    }
    double b = h.numberAtIndex(1);

    double c = atan2(b, a);

    return h.ret(c);
}

// PREDICATES

/***DOC LESSP LESS?
LESSP num1 num2
LESS? num1 num2
num1 < num2

    outputs TRUE if its first input is strictly less than its second.

COD***/
// CMD LESSP 2 2 2
// CMD LESS? 2 2 2
DatumPtr Kernel::excLessp(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);
    double b = h.numberAtIndex(1);
    return h.ret(a < b);
}

/***DOC GREATERP GREATER?
GREATERP num1 num2
GREATER? num1 num2
num1 > num2

    outputs TRUE if its first input is strictly greater than its second.

COD***/
// CMD GREATERP 2 2 2
// CMD GREATER? 2 2 2
DatumPtr Kernel::excGreaterp(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);
    double b = h.numberAtIndex(1);
    return h.ret(a > b);
}

/***DOC LESSEQUALP LESSEQUAL?
LESSEQUALP num1 num2
LESSEQUAL? num1 num2
num1 <= num2

    outputs TRUE if its first input is less than or equal to its second.

COD***/
// CMD LESSEQUALP 2 2 2
// CMD LESSEQUAL? 2 2 2
DatumPtr Kernel::excLessequalp(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);
    double b = h.numberAtIndex(1);
    return h.ret(a <= b);
}

/***DOC GREATEREQUALP GREATEREQUAL?
GREATEREQUALP num1 num2
GREATEREQUAL? num1 num2
num1 >= num2

    outputs TRUE if its first input is greater than or equal to its second.

COD***/
// CMD GREATEREQUALP 2 2 2
// CMD GREATEREQUAL? 2 2 2
DatumPtr Kernel::excGreaterequalp(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double a = h.numberAtIndex(0);
    double b = h.numberAtIndex(1);
    return h.ret(a >= b);
}

// RANDOM NUMBERS

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
// CMD RANDOM 1 1 2
DatumPtr Kernel::excRandom(DatumPtr node)
{
    ProcedureHelper h(this, node);
    int start, end, result;

    if (node.astnodeValue()->countOfChildren() == 1)
    {
        // Generate a number between 0 (inclusive) and end (exclusive)
        end = h.validatedIntegerAtIndex(0, [](int candidate) { return (candidate > 0); });
        result = randomGenerator.bounded(end);
    }
    else
    {
        // Generate a number between start and end (both inclusive)
        start = h.validatedIntegerAtIndex(0, [](int candidate) { return (candidate < INT_MAX); });
        end = 1 +
              h.validatedIntegerAtIndex(1, [=](int candidate) { return (candidate < INT_MAX) && (candidate > start); });
        result = randomGenerator.bounded(start, end);
    }

    return h.ret(result);
}

/***DOC RERANDOM
RERANDOM
(RERANDOM seed)

    command.  Makes the results of RANDOM reproducible.  Ordinarily
    the sequence of random numbers is different each time Logo is
    used.  If you need the same sequence of pseudo-random numbers
    repeatedly, e.g. to debug a program, say RERANDOM before the
    first invocation of RANDOM.  If you need more than one repeatable
    sequence, you can give RERANDOM an integer input; each possible
    input selects a unique sequence of numbers.

COD***/
// CMD RERANDOM 0 0 1
DatumPtr Kernel::excRerandom(DatumPtr node)
{
    ProcedureHelper h(this, node);
    int seedVal;

    if (node.astnodeValue()->countOfChildren() == 1)
    {
        seedVal = h.integerAtIndex(0);
    }
    else
    {
        seedVal = (int)QRandomGenerator::global()->generate();
    }

    randomGenerator.seed(seedVal);
    return nothing;
}

// PRINT FORMATTING

/***DOC FORM
FORM num width precision

    outputs a word containing a printable representation of "num",
    possibly preceded by spaces (and therefore not a number for
    purposes of performing arithmetic operations), with at least
    "width" characters, including exactly "precision" digits after
    the decimal point.  (If "precision" is 0 then there will be no
    decimal point in the output.)

COD***/
// CMD FORM 3 3 3
DatumPtr Kernel::excForm(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double num = h.numberAtIndex(0);
    double width = h.integerAtIndex(1);
    int precision = h.validatedIntegerAtIndex(2, [](int candidate) { return candidate >= 0; });

    QString retval = QString("%1").arg(num, width, 'f', precision);

    return h.ret(retval);
}

// BITWISE OPERATORS

/***DOC BITAND
BITAND num1 num2
(BITAND num1 num2 num3 ...)

    outputs the bitwise AND of its inputs, which must be integers.

COD***/
// CMD BITAND 0 2 -1
DatumPtr Kernel::excBitand(DatumPtr node)
{
    ProcedureHelper h(this, node);
    int retval = -1;

    for (int i = 0; i < node.astnodeValue()->countOfChildren(); ++i)
    {
        int a = h.integerAtIndex(i);
        retval &= a;
    }

    return h.ret(retval);
}

/***DOC BITOR
BITOR num1 num2
(BITOR num1 num2 num3 ...)

    outputs the bitwise OR of its inputs, which must be integers.

COD***/
// CMD BITOR 0 2 -1
DatumPtr Kernel::excBitor(DatumPtr node)
{
    ProcedureHelper h(this, node);
    int retval = 0;

    for (int i = 0; i < node.astnodeValue()->countOfChildren(); ++i)
    {
        int a = h.integerAtIndex(i);
        retval |= a;
    }

    return h.ret(retval);
}

/***DOC BITXOR
BITXOR num1 num2
(BITXOR num1 num2 num3 ...)

    outputs the bitwise EXCLUSIVE OR of its inputs, which must be
    integers.

COD***/
// CMD BITXOR 0 2 -1
DatumPtr Kernel::excBitxor(DatumPtr node)
{
    ProcedureHelper h(this, node);
    int retval = 0;

    for (int i = 0; i < node.astnodeValue()->countOfChildren(); ++i)
    {
        int a = h.integerAtIndex(i);
        retval ^= a;
    }

    return h.ret(retval);
}

/***DOC BITNOT
BITNOT num

    outputs the bitwise NOT of its input, which must be an integer.

COD***/
// CMD BITNOT 1 1 1
DatumPtr Kernel::excBitnot(DatumPtr node)
{
    ProcedureHelper h(this, node);
    int a = h.integerAtIndex(0);
    int retval = ~a;
    return h.ret(retval);
}

/***DOC ASHIFT
ASHIFT num1 num2

    outputs "num1" arithmetic-shifted to the left by "num2" bits.
    If num2 is negative, the shift is to the right with sign
    extension.  The inputs must be integers.

COD***/
// CMD ASHIFT 2 2 2
DatumPtr Kernel::excAshift(DatumPtr node)
{
    ProcedureHelper h(this, node);
    int a = h.integerAtIndex(0);
    int e = h.integerAtIndex(1);
    int retval = (e < 0) ? a >> -e : a << e;
    return h.ret(retval);
}

/***DOC LSHIFT
LSHIFT num1 num2

    outputs "num1" logical-shifted to the left by "num2" bits.
    If num2 is negative, the shift is to the right with zero fill.
    The inputs must be integers.


COD***/
// CMD LSHIFT 2 2 2
DatumPtr Kernel::excLshift(DatumPtr node)
{
    ProcedureHelper h(this, node);
    unsigned int a = h.integerAtIndex(0);
    int e = h.integerAtIndex(1);
    unsigned int retval = (e < 0) ? a >> -e : a << e;
    return h.ret((int)retval);
}

// LOGICAL OPERATIONS
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
// CMD AND 0 2 -1
DatumPtr Kernel::excAnd(DatumPtr node)
{
    ProcedureHelper h(this, node);
    for (int i = 0; i < h.countOfChildren(); ++i)
    {
        bool a = h.boolAtIndex(i, true);
        if (!a)
            return h.ret(false);
    }

    return h.ret(true);
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
// CMD OR 0 2 -1
DatumPtr Kernel::excOr(DatumPtr node)
{
    ProcedureHelper h(this, node);
    for (int i = 0; i < h.countOfChildren(); ++i)
    {
        bool a = h.boolAtIndex(i, true);
        if (a)
            return h.ret(true);
    }

    return h.ret(false);
}

/***DOC NOT
NOT tf

    outputs TRUE if the input is FALSE, and vice versa.  The input can be
    a list, in which case it is taken as an expression to run; that
    expression must produce a TRUE or FALSE value.


COD***/
// CMD NOT 1 1 1
DatumPtr Kernel::excNot(DatumPtr node)
{
    ProcedureHelper h(this, node);
    bool a = h.boolAtIndex(0, true);

    return h.ret(!a);
}

// Add infix entries to table. This will cause an error if they are used as
// prefix operators.
// CMD + 1 1 1
// CMD * 1 1 1
// CMD / 1 1 1
// CMD < 1 1 1
// CMD > 1 1 1
// CMD = 1 1 1
// CMD <= 1 1 1
// CMD >= 1 1 1
// CMD <> 1 1 1
DatumPtr Kernel::excInfixError(DatumPtr node)
{
    ProcedureHelper h(this, node);
    Error::notEnough(node.astnodeValue()->nodeName);
    return nothing;
}
