
//===-- qlogo/kernel.cpp - Kernel class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Kernel class, which is the
/// executor proper of the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "kernel.h"
#include "datum_word.h"
#include "datum_astnode.h"


#include <functional>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// NUMERIC OPERATIONS

DatumP Kernel::excSum(DatumP node) {
  ProcedureHelper h(this, node);
  double result = 0;

  for (int i = 0; i < h.countOfChildren(); ++i) {
    result += h.numberAtIndex(i);
  }

  return h.ret(result);
}

DatumP Kernel::excDifference(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);

  double b = h.numberAtIndex(1);

  double c = a - b;

  return h.ret(c);
}

DatumP Kernel::excMinus(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);

  return h.ret(-a);
}

DatumP Kernel::excProduct(DatumP node) {
  ProcedureHelper h(this, node);
  double result = 1;

  for (int i = 0; i < h.countOfChildren(); ++i) {
    result *= h.numberAtIndex(i);
  }

  return h.ret(result);
}

DatumP Kernel::excQuotient(DatumP node) {
  ProcedureHelper h(this, node);
  double a, c;

  if (h.countOfChildren() == 2) {
    a = h.numberAtIndex(0);
    double b = h.validatedNumberAtIndex(
        1, [](double candidate) { return candidate != 0; });

    c = a / b;
  } else {
    double a = h.validatedNumberAtIndex(
        0, [](double candidate) { return candidate != 0; });
    c = 1 / a;
  }

  return h.ret(c);
}

DatumP Kernel::excRemainder(DatumP node) {
  ProcedureHelper h(this, node);
  int a = h.integerAtIndex(0);

  int b = h.validatedIntegerAtIndex(
      1, [](int candidate) { return candidate != 0; });

  double c = a % b;

  return h.ret(c);
}

DatumP Kernel::excModulo(DatumP node) {
  ProcedureHelper h(this, node);
  int a = h.integerAtIndex(0);

  int b = h.validatedIntegerAtIndex(
      1, [](int candidate) { return candidate != 0; });

  int r = a % b;
  double c = (r * b < 0) ? r + b : r;

  return h.ret(c);
}

DatumP Kernel::excInt(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);

  double b = trunc(a);

  return h.ret(b);
}

DatumP Kernel::excRound(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);

  double b = round(a);

  return h.ret(b);
}

DatumP Kernel::excSqrt(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.validatedNumberAtIndex(
      0, [](double candidate) { return candidate >= 0; });

  double c = sqrt(a);

  return h.ret(c);
}

DatumP Kernel::excPower(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);
  double b;
  if (a >= 0) {
    b = h.numberAtIndex(1);
  } else {
    b = h.validatedNumberAtIndex(
        1, [](double candidate) { return candidate == trunc(candidate); });
  }

  double c = pow(a, b);

  return h.ret(c);
}

DatumP Kernel::excExp(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);

  double c = exp(a);

  return h.ret(c);
}

DatumP Kernel::excLog10(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.validatedNumberAtIndex(
      0, [](double candidate) { return candidate >= 0; });

  double c = log10(a);

  return h.ret(c);
}

DatumP Kernel::excLn(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.validatedNumberAtIndex(
      0, [](double candidate) { return candidate >= 0; });

  double c = log(a);

  return h.ret(c);
}

DatumP Kernel::excSin(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);

  double c = sin(M_PI / 180 * a);

  return h.ret(c);
}

DatumP Kernel::excRadsin(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);

  double c = sin(a);

  return h.ret(c);
}

DatumP Kernel::excCos(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);

  double c = cos(M_PI / 180 * a);

  return h.ret(c);
}

DatumP Kernel::excRadcos(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);

  double c = cos(a);

  return h.ret(c);
}

DatumP Kernel::excArctan(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);
  if (node.astnodeValue()->countOfChildren() == 1) {
    double c = atan(a) * 180 / M_PI;

    return h.ret(c);
  }
  double b = h.numberAtIndex(1);

  double c = atan2(b, a) * 180 / M_PI;

  return h.ret(c);
}

DatumP Kernel::excRadarctan(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);
  if (node.astnodeValue()->countOfChildren() == 1) {
    double c = atan(a);

    return h.ret(c);
  }
  double b = h.numberAtIndex(1);

  double c = atan2(b, a);

  return h.ret(c);
}

// PREDICATES

DatumP Kernel::excLessp(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);
  double b = h.numberAtIndex(1);
  return h.ret(a < b);
}

DatumP Kernel::excGreaterp(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);
  double b = h.numberAtIndex(1);
  return h.ret(a > b);
}

DatumP Kernel::excLessequalp(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);
  double b = h.numberAtIndex(1);
  return h.ret(a <= b);
}

DatumP Kernel::excGreaterequalp(DatumP node) {
  ProcedureHelper h(this, node);
  double a = h.numberAtIndex(0);
  double b = h.numberAtIndex(1);
  return h.ret(a >= b);
}

// RANDOM NUMBERS

DatumP Kernel::excRandom(DatumP node) {
  ProcedureHelper h(this, node);
  int start, end;

  // If this assert fails then I need to rethink this part.
  Q_ASSERT(sizeof(int) > sizeof(uint32_t));
  const int qlogo_maxint = 0xffffffff; // Maximum value of uint32_t

  if (node.astnodeValue()->countOfChildren() == 1) {
    start = 0;
    end = h.validatedIntegerAtIndex(0, [](int candidate) {
      return (candidate >= 0) && (candidate <= qlogo_maxint);
    });
    if (end > 0)
      end = end - 1;
  } else {
    start = h.validatedIntegerAtIndex(0, [](int candidate) {
      return (candidate >= 0) && (candidate <= qlogo_maxint);
    });
    end = h.validatedIntegerAtIndex(1, [=](int candidate) {
      return (candidate <= qlogo_maxint) && (candidate >= start);
    });
  }

  double result = (double) randomFromRange( (uint32_t) start, (uint32_t) end);

  return h.ret(result);
}


// PRINT FORMATTING

DatumP Kernel::excForm(DatumP node) {
  ProcedureHelper h(this, node);
  double num = h.numberAtIndex(0);
  double width = h.integerAtIndex(1);
  int precision = h.validatedIntegerAtIndex(
      2, [](int candidate) { return candidate >= 0; });

  QString retval = QString("%1").arg(num, width, 'f', precision);

  return h.ret(retval);
}

// BITWISE OPERATORS

DatumP Kernel::excBitand(DatumP node) {
  ProcedureHelper h(this, node);
  int retval = -1;

  for (int i = 0; i < node.astnodeValue()->countOfChildren(); ++i) {
    int a = h.integerAtIndex(i);
    retval &= a;
  }

  return h.ret(retval);
}

DatumP Kernel::excBitor(DatumP node) {
  ProcedureHelper h(this, node);
  int retval = 0;

  for (int i = 0; i < node.astnodeValue()->countOfChildren(); ++i) {
    int a = h.integerAtIndex(i);
    retval |= a;
  }

  return h.ret(retval);
}

DatumP Kernel::excBitxor(DatumP node) {
  ProcedureHelper h(this, node);
  int retval = 0;

  for (int i = 0; i < node.astnodeValue()->countOfChildren(); ++i) {
    int a = h.integerAtIndex(i);
    retval ^= a;
  }

  return h.ret(retval);
}

DatumP Kernel::excBitnot(DatumP node) {
  ProcedureHelper h(this, node);
  int a = h.integerAtIndex(0);
  int retval = ~a;
  return h.ret(retval);
}

DatumP Kernel::excAshift(DatumP node) {
  ProcedureHelper h(this, node);
  int a = h.integerAtIndex(0);
  int e = h.integerAtIndex(1);
  int retval = (e < 0) ? a >> -e : a << e;
  return h.ret(retval);
}

DatumP Kernel::excLshift(DatumP node) {
  ProcedureHelper h(this, node);
  unsigned int a = h.integerAtIndex(0);
  int e = h.integerAtIndex(1);
  unsigned int retval = (e < 0) ? a >> -e : a << e;
  return h.ret((int)retval);
}

// LOGICAL OPERATIONS

DatumP Kernel::excAnd(DatumP node) {
  ProcedureHelper h(this, node);
  for (int i = 0; i < h.countOfChildren(); ++i) {
    bool a = h.boolAtIndex(i, true);
    if (!a)
      return h.ret(false);
  }

  return h.ret(true);
}

DatumP Kernel::excOr(DatumP node) {
  ProcedureHelper h(this, node);
  for (int i = 0; i < h.countOfChildren(); ++i) {
    bool a = h.boolAtIndex(i, true);
    if (a)
      return h.ret(true);
  }

  return h.ret(false);
}

DatumP Kernel::excNot(DatumP node) {
  ProcedureHelper h(this, node);
  bool a = h.boolAtIndex(0, true);

  return h.ret(!a);
}
