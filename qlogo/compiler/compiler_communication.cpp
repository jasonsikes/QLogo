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

#include "astnode.h"
#include "compiler.h"
#include "compiler_internal.h"
#include "sharedconstants.h"
#include "workspace/exports.h"

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
Value *Compiler::genPrint(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeNothing);

    AllocaInst *ary = generateChildrenAlloca(node.astnodeValue(), TypeDatum, DBG_NAME("printAry"));
    Value *useShow = CoBool(false);
    Value *addNewline = CoBool(true);

    return generateCallExtern(
        stdWriteDatumAry, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())), PaAddr(ary), PaInt32(ary->getArraySize()), PaBool(useShow), PaBool(addNewline));
}

/***DOC SHOW
SHOW thing
(SHOW thing1 thing2 ...)

 command.  Prints the input or inputs like PRINT, except that
 if an input is a list it is printed inside square brackets.


COD***/
// CMD SHOW 0 1 -1 n
Value *Compiler::genShow(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeNothing);

    AllocaInst *ary = generateChildrenAlloca(node.astnodeValue(), TypeDatum, DBG_NAME("showAry"));
    Value *useShow = CoBool(true);
    Value *addNewline = CoBool(true);

    return generateCallExtern(
        stdWriteDatumAry, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())), PaAddr(ary), PaInt32(ary->getArraySize()), PaBool(useShow), PaBool(addNewline));
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
Value *Compiler::genType(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeNothing);

    AllocaInst *ary = generateChildrenAlloca(node.astnodeValue(), TypeDatum, DBG_NAME("typeAry"));
    Value *useShow = CoBool(false);
    Value *addNewline = CoBool(false);

    return generateCallExtern(
        stdWriteDatumAry, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())), PaAddr(ary), PaInt32(ary->getArraySize()), PaBool(useShow), PaBool(addNewline));
}

/***DOC READLIST RL
READLIST
RL

    reads a line from the read stream (initially the keyboard) and
    outputs that line as a list.  The line is separated into members as
    though it were typed in square brackets in an instruction.  If the
    read stream is a file, and the end of file is reached, READLIST
    outputs the empty word (not the empty list).  READLIST processes
    backslash, vertical bar, and tilde characters in the read stream;
    the output list will not contain these characters but they will have
    had their usual effect.  READLIST does not, however, treat semicolon
    as a comment character.

COD***/
// CMD READLIST 0 0 0 d
// CMD RL 0 0 0 d
Value *Compiler::genReadlist(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeDatum);
    Value *retval = generateCallExtern(stdReadList, PaAddr(scaff_->evaluator_));
    return generateNotFlowControl(retval);
}

/***DOC READWORD RW
READWORD
RW

    reads a line from the read stream and outputs that line as a word.
    The output is a single word even if the line contains spaces,
    brackets, etc.  If the read stream is a file, and the end of file is
    reached, READWORD outputs the empty list (not the empty word).
    READWORD processes backslash, vertical bar, and tilde characters in
    the read stream.  In the case of a tilde used for line continuation,
    the output word DOES include the tilde and the newline characters, so
    that the user program can tell exactly what the user entered.
    Vertical bars in the line are also preserved in the output.
    Backslash characters are not preserved in the output.

COD***/
// CMD READWORD 0 0 0 d
// CMD RW 0 0 0 d
Value *Compiler::genReadword(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeDatum);
    Value *retval = generateCallExtern(stdReadWord, PaAddr(scaff_->evaluator_));
    return generateNotFlowControl(retval);
}

/***DOC READRAWLINE
READRAWLINE

    reads a line from the read stream and outputs that line as a word.
    The output is a single word even if the line contains spaces,
    brackets, etc.  If the read stream is a file, and the end of file is
    reached, READRAWLINE outputs the empty list (not the empty word).
    READRAWLINE outputs the exact string of characters as they appear
    in the line, with no special meaning for backslash, vertical bar,
    tilde, or any other formatting characters.

COD***/
// CMD READRAWLINE 0 0 0 d
Value *Compiler::genReadrawline(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeDatum);
    Value *retval = generateCallExtern(stdReadRawline, PaAddr(scaff_->evaluator_));
    return generateNotFlowControl(retval);
}

/***DOC READCHAR RC
READCHAR
RC

    reads a single character from the read stream and outputs that
    character as a word.  If the read stream is a file, and the end of
    file is reached, READCHAR outputs the empty list (not the empty
    word).  If the read stream is the keyboard, echoing is turned off
    when READCHAR is invoked, and remains off until READLIST or READWORD
    is invoked or a Logo prompt is printed.  Backslash, vertical bar,
    and tilde characters have no special meaning in this context.

COD***/

/***DOC READCHARS RCS
READCHARS num
RCS num

    reads "num" characters from the read stream and outputs those
    characters as a word.  If the read stream is a file, and the end of
    file is reached, READCHARS outputs the empty list (not the empty
    word).  If the read stream is a terminal, echoing is turned off
    when READCHARS is invoked, and remains off until READLIST or READWORD
    is invoked or a Logo prompt is printed.  Backslash, vertical bar,
    and tilde characters have no special meaning in this context.

COD***/
// CMD READCHAR 0 0 0 d
// CMD RC 0 0 0 d
// CMD READCHARS 1 1 1 d
// CMD RCS 1 1 1 d
Value *Compiler::genReadchar(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeDatum);

    Value *count = CoInt32(1);
    if (node.astnodeValue()->countOfChildren() == 1)
    {
        Value *num = generateChild(node.astnodeValue(), 0, TypeReal);
        count = generateNotNegativeInt32FromDouble(node.astnodeValue(), num);
    }

    Value *retval = generateCallExtern(stdReadChars, PaAddr(scaff_->evaluator_), PaInt32(count));
    return generateNotFlowControl(retval);
}

/***DOC FILEDIALOG
FILEDIALOG

    GUI only. Presents a modal file dialog to the user. The user is then
    given an opportunity to select a file. Outputs the file path of the file
    selected by the user or an empty list if user pressed 'Cancel'.

COD***/
// CMD FILEDIALOG 0 0 0 d
Value *Compiler::genFiledialog(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeDatum);
    CHECK_GRAPHICS_MODE
    Value *retval = generateCallExtern(stdFileDialog, PaAddr(scaff_->evaluator_));
    return generateNotFlowControl(retval);
}

/***DOC COPYRIGHT
COPYRIGHT

    command.  Prints a copyright message to the current write stream.

COD***/
// CMD COPYRIGHT 0 0 0 n
Value *Compiler::genCopyright(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeNothing);
    Value *retval = generateCallExtern(
        stdCopyright, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())));
    return generateNotFlowControl(retval);
}
