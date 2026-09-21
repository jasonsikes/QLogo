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
#include "interface/streammanager.h"
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

/***DOC SHELL
SHELL command
(SHELL command wordflag)

    Under Unix and macOS, outputs the result of running "command" as a
    shell command.  (The command is sent to /bin/sh, not csh or other
    alternatives.)  If the command is a literal list in the instruction
    line, and if you want a backslash character sent to the shell, you
    must use \\ to get the backslash through Logo's reader intact.  The
    output is a list containing one member for each line generated by
    the shell command.  Ordinarily each such line is represented by a
    list in the output, as though the line were read using READLIST.  If
    a second input is given, regardless of the value of the input, each
    line is represented by a word in the output as though it were read
    with READWORD.  Example:

            to dayofweek
            output first first shell [date]
            end

    This is "first first" to extract the first word of the first (and
    only) line of the shell output.

    Under Windows, Logo behaves as under Unix (except that DOS-style
    commands are understood; use "dir" rather than "ls").

COD***/
// CMD SHELL 1 1 2 d
Value *Compiler::genShell(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeDatum);

    Value *command = generateChild(node.astnodeValue(), 0, TypeDatum);
    auto validator = [this](Value *candidate) {
        Value *isValid = generateCallExtern(isValidShellCommand, PaAddr(candidate));
        return scaff_->builder_.CreateICmpEQ(isValid, CoBool(true), DBG_NAME("isValidShellCommandCond"));
    };
    command = generateValidationDatum(node.astnodeValue(), command, validator);

    const bool hasWordflag = node.astnodeValue()->countOfChildren() == 2;
    if (hasWordflag)
        generateChild(node.astnodeValue(), 1, TypeDatum);

    Value *retval = generateCallExtern(
        stdShell, PaAddr(scaff_->evaluator_), PaAddr(command), PaBool(CoBool(hasWordflag)));
    return generateNotFlowControl(retval);
}

/***DOC SETPREFIX
SETPREFIX string

    command.  Sets a prefix that will be used as the implicit beginning
    of filenames in OPENREAD, OPENWRITE, OPENAPPEND, OPENUPDATE, LOAD,
    and SAVE commands.  Logo will put the appropriate separator
    character (slash for Unix, backslash for Windows) between the prefix
    and the filename entered by the user.  On Windows, either a forward
    slash or backslash can be used as a separator character.  The input
    to SETPREFIX must be a word, unless it is the empty list, to indicate
    that there should be no prefix.

COD***/
// CMD SETPREFIX 1 1 1 n
Value *Compiler::genSetprefix(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeNothing);

    Value *prefix = generateChild(node.astnodeValue(), 0, TypeDatum);
    auto validator = [this](Value *candidate) {
        Value *isa = generateGetDatumIsa(candidate);
        Value *wordMask = scaff_->builder_.CreateAnd(isa, CoInt32(Datum::typeWord), DBG_NAME("wordMask"));
        Value *isWord = scaff_->builder_.CreateICmpNE(wordMask, CoInt32(0), DBG_NAME("isWord"));
        Value *listMask = scaff_->builder_.CreateAnd(isa, CoInt32(Datum::typeList), DBG_NAME("listMask"));
        Value *isList = scaff_->builder_.CreateICmpNE(listMask, CoInt32(0), DBG_NAME("isList"));
        Value *empty = generateCallExtern(isEmpty, PaAddr(candidate));
        Value *isEmptyVal = scaff_->builder_.CreateICmpEQ(empty, CoBool(true), DBG_NAME("isEmpty"));
        Value *isEmptyList = scaff_->builder_.CreateAnd(isList, isEmptyVal, DBG_NAME("isEmptyList"));
        return scaff_->builder_.CreateOr(isWord, isEmptyList, DBG_NAME("isValidPrefix"));
    };
    prefix = generateValidationDatum(node.astnodeValue(), prefix, validator);

    generateCallExtern(stdSetPrefix, PaAddr(prefix));
    return generateVoidRetval(node);
}

/***DOC PREFIX
PREFIX

    outputs the current file prefix, or [] if there is no prefix.
    See SETPREFIX.

COD***/
// CMD PREFIX 0 0 0 d
Value *Compiler::genPrefix(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeDatum);
    return generateCallExtern(stdPrefix, PaAddr(scaff_->evaluator_));
}

/***DOC OPENREAD
OPENREAD filename

    command.  Opens the named file for reading.  The read position is
    initially at the beginning of the file.

    The input may also be a list whose first element is a variable
    name; any remaining members are ignored.  The named variable's
    word value is opened as a character buffer.  It is an error if
    the variable has no value.

COD***/
// CMD OPENREAD 1 1 1 n
Value *Compiler::genOpenread(const DatumPtr &node, TypeMask expected)
{
    return generateOpen(node, expected, static_cast<int32_t>(OpenMode::Read));
}

/***DOC OPENWRITE
OPENWRITE filename

    command.  Opens the named file for writing.  If the file already
    existed, the old version is deleted and a new, empty file created.

    The input may also be a list whose first element is a variable
    name; any remaining members are ignored.  (The other OPEN variants
    accept this list form as well.)  A character buffer is created and
    identified by that variable name (not by list identity).  After
    SETWRITE with the same name, printed characters are stored in the
    buffer.  After CLOSE with the same name, the buffer contents
    (treated as one long word, even if spaces and newlines are
    included) become the value of the specified variable.

COD***/
// CMD OPENWRITE 1 1 1 n
Value *Compiler::genOpenwrite(const DatumPtr &node, TypeMask expected)
{
    return generateOpen(node, expected, static_cast<int32_t>(OpenMode::Write));
}

/***DOC OPENAPPEND
OPENAPPEND filename

    command.  Opens the named file for writing.  If the file already
    exists, the write position is initially set to the end of the old
    file, so that newly written data will be appended to it.  If the
    file does not exist, it is created.

    The input may also be a list whose first element is a variable
    name; any remaining members are ignored.  If the named variable
    has no value, an empty buffer is created.

COD***/
// CMD OPENAPPEND 1 1 1 n
Value *Compiler::genOpenappend(const DatumPtr &node, TypeMask expected)
{
    return generateOpen(node, expected, static_cast<int32_t>(OpenMode::Append));
}

/***DOC OPENUPDATE
OPENUPDATE filename

    command.  Opens the named file for reading and writing.  The read and
    write position is initially set to the end of the old file, if any.
    If the file does not exist, it is created.  Note: each open file has
    only one position, for both reading and writing.  If a file opened
    for update is both READER and WRITER at the same time, then
    SETREADPOS will also affect WRITEPOS and vice versa.  Also, if you
    alternate reading and writing the same file, you must SETREADPOS
    between a write and a read, and SETWRITEPOS between a read and a
    write.

    The input may also be a list whose first element is a variable
    name; any remaining members are ignored.  If the named variable
    has no value, an empty buffer is created.

COD***/
// CMD OPENUPDATE 1 1 1 n
Value *Compiler::genOpenupdate(const DatumPtr &node, TypeMask expected)
{
    return generateOpen(node, expected, static_cast<int32_t>(OpenMode::Update));
}

Value *Compiler::generateOpen(const DatumPtr &node, TypeMask expected, int32_t mode)
{
    Q_ASSERT(expected && TypeNothing);

    Value *spec = generateChild(node.astnodeValue(), 0, TypeDatum);
    auto validator = [this](Value *candidate) {
        Value *isValid = generateCallExtern(isValidOpenSpec, PaAddr(candidate));
        return scaff_->builder_.CreateICmpEQ(isValid, CoBool(true), DBG_NAME("isValidOpenSpecCond"));
    };
    spec = generateValidationDatum(node.astnodeValue(), spec, validator);

    Value *retval = generateCallExtern(
        stdOpen,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(spec),
        PaInt32(CoInt32(mode)));
    return generateNotFlowControl(retval);
}

/***DOC CLOSE
CLOSE filename

    command.  Closes the named file.  If the file was currently the
    reader or writer, then the reader or writer is changed to the
    keyboard or screen, as if SETREAD [] or SETWRITE [] had been done.

COD***/
// CMD CLOSE 1 1 1 n
Value *Compiler::genClose(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeNothing);

    Value *spec = generateChild(node.astnodeValue(), 0, TypeDatum);
    auto validator = [this](Value *candidate) {
        Value *isValid = generateCallExtern(isValidOpenSpec, PaAddr(candidate));
        return scaff_->builder_.CreateICmpEQ(isValid, CoBool(true), DBG_NAME("isValidCloseSpecCond"));
    };
    spec = generateValidationDatum(node.astnodeValue(), spec, validator);

    Value *retval = generateCallExtern(
        stdClose,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(spec));
    return generateNotFlowControl(retval);
}

/***DOC ALLOPEN
ALLOPEN

    outputs a list whose members are the names of all files currently open.
    This list does not include the dribble file, if any.

COD***/
// CMD ALLOPEN 0 0 0 d
Value *Compiler::genAllopen(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeDatum);
    return generateCallExtern(stdAllOpen, PaAddr(scaff_->evaluator_));
}

/***DOC ERASEFILE ERF
ERASEFILE filename
ERF filename

    command.  Erases (deletes, removes) the named file, which should not
    currently be open.

COD***/
// CMD ERASEFILE 1 1 1 n
// CMD ERF 1 1 1 n
Value *Compiler::genErasefile(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeNothing);

    Value *filename = generateChild(node.astnodeValue(), 0, TypeDatum);
    filename = generateFromDatum(Datum::typeWord, node.astnodeValue(), filename);

    Value *retval = generateCallExtern(
        stdEraseFile,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(filename));
    return generateNotFlowControl(retval);
}

/***DOC DRIBBLE
DRIBBLE filename

    command.  Creates a new file whose name is the input, like OPENWRITE,
    and begins recording in that file everything that is read from the
    keyboard or written to the terminal.  That is, this writing is in
    addition to the writing to WRITER.  The intent is to create a
    transcript of a Logo session, including things like prompt
    characters and interactions.

COD***/
// CMD DRIBBLE 1 1 1 n
Value *Compiler::genDribble(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeNothing);

    Value *filename = generateChild(node.astnodeValue(), 0, TypeDatum);
    filename = generateFromDatum(Datum::typeWord, node.astnodeValue(), filename);

    Value *retval = generateCallExtern(
        stdDribble,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(filename));
    return generateNotFlowControl(retval);
}

/***DOC NODRIBBLE
NODRIBBLE

    command.  Stops copying information into the dribble file, and
    closes the file.

COD***/
// CMD NODRIBBLE 0 0 0 n
Value *Compiler::genNodribble(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeNothing);
    generateCallExtern(stdNoDribble);
    return generateVoidRetval(node);
}

/***DOC SETREAD
SETREAD filename

    command.  Makes the named file the read stream, used for READLIST,
    etc.  The file must already be open with OPENREAD or OPENUPDATE.  If
    the input is the empty list, then the read stream becomes the
    keyboard, as usual.  Changing the read stream does not close the
    file that was previously the read stream, so it is possible to
    alternate between files.

    In QLogo the input may also be a list whose first element is a
    variable name, matching OPENREAD's character-buffer form.  The
    buffer must already be open.

COD***/
// CMD SETREAD 1 1 1 n
Value *Compiler::genSetread(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeNothing);

    Value *spec = generateChild(node.astnodeValue(), 0, TypeDatum);
    auto validator = [this](Value *candidate) {
        Value *isOpenSpec = generateCallExtern(isValidOpenSpec, PaAddr(candidate));
        Value *openOk = scaff_->builder_.CreateICmpEQ(isOpenSpec, CoBool(true), DBG_NAME("isValidOpenSpecCond"));
        Value *isa = generateGetDatumIsa(candidate);
        Value *listMask = scaff_->builder_.CreateAnd(isa, CoInt32(Datum::typeList), DBG_NAME("listMask"));
        Value *isList = scaff_->builder_.CreateICmpNE(listMask, CoInt32(0), DBG_NAME("isList"));
        Value *empty = generateCallExtern(isEmpty, PaAddr(candidate));
        Value *isEmptyVal = scaff_->builder_.CreateICmpEQ(empty, CoBool(true), DBG_NAME("isEmpty"));
        Value *isEmptyList = scaff_->builder_.CreateAnd(isList, isEmptyVal, DBG_NAME("isEmptyList"));
        return scaff_->builder_.CreateOr(openOk, isEmptyList, DBG_NAME("isValidSetRead"));
    };
    spec = generateValidationDatum(node.astnodeValue(), spec, validator);

    Value *retval = generateCallExtern(
        stdSetRead,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(spec));
    return generateNotFlowControl(retval);
}

/***DOC READER
READER

    outputs the name of the current read stream file, or the empty list
    if the read stream is the terminal.

COD***/
// CMD READER 0 0 0 d
Value *Compiler::genReader(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeDatum);
    return generateCallExtern(stdReader, PaAddr(scaff_->evaluator_));
}

/***DOC SETWRITE
SETWRITE filename

    command.  Makes the named file the write stream, used for PRINT,
    etc.  The file must already be open with OPENWRITE, OPENAPPEND, or
    OPENUPDATE.  If the input is the empty list, then the write stream
    becomes the screen, as usual.  Changing the write stream does
    not close the file that was previously the write stream, so it is
    possible to alternate between files.

    In QLogo the input may also be a list whose first element is a
    variable name, matching OPENWRITE's character-buffer form.  The
    buffer must already be open.

COD***/
// CMD SETWRITE 1 1 1 n
Value *Compiler::genSetwrite(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeNothing);

    Value *spec = generateChild(node.astnodeValue(), 0, TypeDatum);
    auto validator = [this](Value *candidate) {
        Value *isOpenSpec = generateCallExtern(isValidOpenSpec, PaAddr(candidate));
        Value *openOk = scaff_->builder_.CreateICmpEQ(isOpenSpec, CoBool(true), DBG_NAME("isValidOpenSpecCond"));
        Value *isa = generateGetDatumIsa(candidate);
        Value *listMask = scaff_->builder_.CreateAnd(isa, CoInt32(Datum::typeList), DBG_NAME("listMask"));
        Value *isList = scaff_->builder_.CreateICmpNE(listMask, CoInt32(0), DBG_NAME("isList"));
        Value *empty = generateCallExtern(isEmpty, PaAddr(candidate));
        Value *isEmptyVal = scaff_->builder_.CreateICmpEQ(empty, CoBool(true), DBG_NAME("isEmpty"));
        Value *isEmptyList = scaff_->builder_.CreateAnd(isList, isEmptyVal, DBG_NAME("isEmptyList"));
        return scaff_->builder_.CreateOr(openOk, isEmptyList, DBG_NAME("isValidSetWrite"));
    };
    spec = generateValidationDatum(node.astnodeValue(), spec, validator);

    Value *retval = generateCallExtern(
        stdSetWrite,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(spec));
    return generateNotFlowControl(retval);
}

/***DOC WRITER
WRITER

    outputs the name of the current write stream file, or the empty list
    if the write stream is the screen.

COD***/
// CMD WRITER 0 0 0 d
Value *Compiler::genWriter(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeDatum);
    return generateCallExtern(stdWriter, PaAddr(scaff_->evaluator_));
}

/***DOC SETREADPOS
SETREADPOS charpos

    command.  Sets the file pointer of the read stream file so that the
    next READLIST, etc., will begin reading at the "charpos"th character
    in the file, counting from 0.  (That is, SETREADPOS 0 will start
    reading from the beginning of the file.)  Meaningless if the read
    stream is the keyboard.

COD***/
// CMD SETREADPOS 1 1 1 n
Value *Compiler::genSetreadpos(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeNothing);
    Value *num = generateChild(node.astnodeValue(), 0, TypeReal);
    Value *pos = generateNotNegativeInt32FromDouble(node.astnodeValue(), num);
    generateCallExtern(stdSetReadPos, PaInt32(pos));
    return generateVoidRetval(node);
}

/***DOC READPOS
READPOS

    outputs the file position of the current read stream file.

COD***/
// CMD READPOS 0 0 0 r
Value *Compiler::genReadpos(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeReal);
    return generateCallExtern(stdReadPos);
}

/***DOC SETWRITEPOS
SETWRITEPOS charpos

    command.  Sets the file pointer of the write stream file so that the
    next PRINT, etc., will begin writing at the "charpos"th character
    in the file, counting from 0.  (That is, SETWRITEPOS 0 will start
    writing from the beginning of the file.)  Meaningless if the write
    stream is the screen.

COD***/
// CMD SETWRITEPOS 1 1 1 n
Value *Compiler::genSetwritepos(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeNothing);
    Value *num = generateChild(node.astnodeValue(), 0, TypeReal);
    Value *pos = generateNotNegativeInt32FromDouble(node.astnodeValue(), num);
    generateCallExtern(stdSetWritePos, PaInt32(pos));
    return generateVoidRetval(node);
}

/***DOC WRITEPOS
WRITEPOS

    outputs the file position of the current write stream file.

COD***/
// CMD WRITEPOS 0 0 0 r
Value *Compiler::genWritepos(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeReal);
    return generateCallExtern(stdWritePos);
}
