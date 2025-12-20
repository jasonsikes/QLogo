//===-- qlogo/communication.cpp - Communication implementations -------*- C++ -*-===//
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
/// This file contains the implementation of the communication methods of the
/// Compiler class.
///
//===----------------------------------------------------------------------===//

#include "compiler_private.h"
#include "astnode.h"
#include "compiler.h"

using namespace llvm;
using namespace llvm::orc;

/***DOC PRINT PR
PRINT thing
PR thing
(PRINT thing1 thing2 ...)
(PR thing1 thing2 ...)

command.  Prints the input or inputs to the current write stream
(initially the screen).  All the inputs are printed on a single
line, separated by spaces, ending with a newline.  If an input is a
list, square brackets are not printed around it, but brackets are
printed around sublists.  Braces are always printed around arrays.

COD***/
// CMD PRINT 0 1 -1 n
// CMD PR 0 1 -1 n
Value *Compiler::genPrint(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnNothing);

    AllocaInst *ary = generateChildrenAlloca(node.astnodeValue(), RequestReturnDatum, "printAry");
    Value *useShow = CoBool(false);
    Value *addNewline = CoBool(true);

    generateCallExtern(TyAddr, "stdWriteDatumAry", {PaAddr(ary), PaInt32(ary->getArraySize()), PaBool(useShow), PaBool(addNewline)});
    return generateVoidRetval(node);
}

/***DOC SHOW
SHOW thing
(SHOW thing1 thing2 ...)

 command.  Prints the input or inputs like PRINT, except that
 if an input is a list it is printed inside square brackets.


COD***/
// CMD SHOW 0 1 -1 n
Value *Compiler::genShow(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnNothing);

    AllocaInst *ary = generateChildrenAlloca(node.astnodeValue(), RequestReturnDatum, "showAry");
    Value *useShow = CoBool(true);
    Value *addNewline = CoBool(true);

    generateCallExtern(TyAddr, "stdWriteDatumAry", {PaAddr(ary), PaInt32(ary->getArraySize()), PaBool(useShow), PaBool(addNewline)});
    return generateVoidRetval(node);
}

/***DOC TYPE
TYPE thing
(TYPE thing1 thing2 ...)

    command.  Prints the input or inputs like PRINT, except that no
    newline character is printed at the end and multiple inputs are not
    separated by spaces.  Note: printing to the terminal is ordinarily
    "line buffered"; that is, the characters you print using TYPE will
    not actually appear on the screen until either a newline character
    is printed (for example, by PRINT or SHOW) or Logo tries to read
    from the keyboard (either at the request of your program or after an
    instruction prompt).  This buffering makes the program much faster
    than it would be if each character appeared immediately, and in most
    cases the effect is not disconcerting.  To accommodate programs that
    do a lot of positioned text display using TYPE, Logo will force
    printing whenever CURSOR or SETCURSOR is invoked.  This solves most
    buffering problems.  Still, on occasion you may find it necessary to
    force the buffered characters to be printed explicitly; this can be
    done using the WAIT command.  WAIT 0 will force printing without
    actually waiting.

COD***/
// CMD TYPE 0 1 -1 n
Value *Compiler::genType(const DatumPtr &node, RequestReturnType returnType)
{
    Q_ASSERT(returnType && RequestReturnNothing);

    AllocaInst *ary = generateChildrenAlloca(node.astnodeValue(), RequestReturnDatum, "typeAry");
    Value *useShow = CoBool(false);
    Value *addNewline = CoBool(false);

    generateCallExtern(TyAddr, "stdWriteDatumAry", {PaAddr(ary), PaInt32(ary->getArraySize()), PaBool(useShow), PaBool(addNewline)});
    return generateVoidRetval(node);
}
