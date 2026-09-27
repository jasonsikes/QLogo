//===-- qlogo/controlstructures.cpp - Control Structure implementations -------*- C++ -*-===//
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
/// This file contains the implementation of the control structure methods of the
/// Compiler class.
///
//===----------------------------------------------------------------------===//

#include "astnode.h"
#include "compiler.h"
#include "compiler_internal.h"
#include "workspace/exports.h"
#include "workspace/procedures.h"
#include "sharedconstants.h"
using namespace llvm;
using namespace llvm::orc;

// This is not a user command.
Value *Compiler::genNoop(const DatumPtr &node, TypeMask expected)
{
    // Do nothing.
    return generateVoidRetval(node);
}

/***DOC IF
IF tf instructionlist
(IF tf instructionlist1 instructionlist2)

command.  If the first input has the value TRUE, then IF runs
the second input.  If the first input has the value FALSE, then
IF does nothing.  (If given a third input, IF acts like IFELSE,
as described below.)  It is an error if the first input is not
either TRUE or FALSE.

COD***/

/***DOC IFELSE
IFELSE tf instructionlist1 instructionlist2

command or operation.  If the first input has the value TRUE, then
IFELSE runs the second input.  If the first input has the value FALSE,
then IFELSE runs the third input.  IFELSE outputs a value if the
instructionlist contains an expression that outputs a value.

COD***/
// CMD IF 2 2 3 dn
// CMD IFELSE 3 3 3 dn
Value *Compiler::genIfelse(const DatumPtr &node, TypeMask expected)
{
    // This is so hacky. Here's how it plays out:

    // Case "IF" (2 parameters):
    //     ift = execute instructionlist1
    //     iff = UNBOUND
    //     retval = either ift or iff

    // Case "IFELSE" (3 parameters):
    //     ift = instructionlist1
    //     iff = instructionlist2
    //     retval = execute either ift or iff

    auto generateListFn = node.astnodeValue()->inSequenceTail_ ? &Compiler::generateCallListContinuation : &Compiler::generateCallList;

    std::vector<TypeMask> expectedAry = {TypeDB, expected};
    if (node.astnodeValue()->countOfChildren() == 3)
    {
        expectedAry.push_back(expected);
    }
    std::vector<Value *> children = generateChildren(node.astnodeValue(), expectedAry);

    BasicBlock *thenBB = scaff_->createBasicBlock(DBG_NAME("then"));
    BasicBlock *elseBB = scaff_->createBasicBlock(DBG_NAME("else"));
    BasicBlock *mergeBB = scaff_->createBasicBlock(DBG_NAME("merge"));

    Value *cond = children[0];
    Value *ift = children[1];
    Value *iff = (children.size() == 3) ? children[2] : CoAddr(node.astnodeValue());

    // If input is a Datum type (can be word or list)
    if (cond->getType()->isPointerTy())
    {
        cond = generateListExecIfList(node.astnodeValue(), cond);
        cond = generateBoolFromDatum(node.astnodeValue(), cond);
        // bool continues.
    }

    cond = scaff_->builder_.CreateICmpEQ(cond, CoBool(1), DBG_NAME("ifcond"));
    scaff_->builder_.CreateCondBr(cond, thenBB, elseBB);

    // If true
    scaff_->builder_.SetInsertPoint(thenBB);
    if (children.size() == 2)
    {
        ift = (this->*generateListFn)(ift, expected);
        thenBB = scaff_->builder_.GetInsertBlock();
    }
    scaff_->builder_.CreateBr(mergeBB);

    // If false
    scaff_->builder_.SetInsertPoint(elseBB);
    scaff_->builder_.CreateBr(mergeBB);

    // Merge the two branches.
    scaff_->builder_.SetInsertPoint(mergeBB);
    PHINode *phiNode = scaff_->builder_.CreatePHI(TyAddr, 2, DBG_NAME("iftmp"));
    phiNode->addIncoming(ift, thenBB);
    phiNode->addIncoming(iff, elseBB);

    Value *retval = phiNode;

    if (children.size() == 3)
    {
        retval = (this->*generateListFn)(retval, expected);
    }
    return retval;
}

/***DOC RUN
RUN instructionlist

command or operation.  Runs the Logo instructions in the input
list; outputs if the list contains an expression that outputs.

COD***/
// CMD RUN 1 1 1 dn
Value *Compiler::genRun(const DatumPtr &node, TypeMask expected)
{
    Value *list = generateChild(node.astnodeValue(), 0, TypeDatum);
    if (node.astnodeValue()->inSequenceTail_) {
        return generateCallListContinuation(list, expected);
    }
    return generateCallList(list, expected);
}

/***DOC RUNTIME
RUNTIME instructionlist

    command or operation.  Runs the Logo instructions in the input
    list while measuring elapsed time.  After the list finishes,
    prints the elapsed time in seconds.  Outputs if the list
    contains an expression that outputs.

COD***/
// CMD RUNTIME 1 1 1 dn
Value *Compiler::genRuntime(const DatumPtr &node, TypeMask expected)
{
    Value *list = generateChild(node.astnodeValue(), 0, TypeDatum);
    list = generateFromDatum(Datum::typeList, node.astnodeValue(), list);
    generateCallExtern(stdRuntimeStart);
    Value *result = generateCallList(list, expected);
    generateCallExtern(stdRuntimeEnd);
    return result;
}

/***DOC REPEAT
REPEAT num instructionlist

command.  Runs the "instructionlist" repeatedly, "num" times.

COD***/
// CMD REPEAT 2 2 2 dn
Value *Compiler::genRepeat(const DatumPtr &node, TypeMask expected)
{
    Value *count = generateChild(node.astnodeValue(), 0, TypeReal);
    Value *list = generateChild(node.astnodeValue(), 1, TypeDatum);

    auto countValidator = [this](Value *candidate) {
        BasicBlock *intCheckBB = scaff_->builder_.GetInsertBlock();

        BasicBlock *negativeCheckBB = scaff_->createBasicBlock(DBG_NAME("negativeCheck"));
        BasicBlock *mergeBB = scaff_->createBasicBlock(DBG_NAME("merge"));

        Value *candidateInt = scaff_->builder_.CreateFPToSI(candidate, TyInt32, DBG_NAME("FpToInt"));
        Value *candidateCheck = scaff_->builder_.CreateSIToFP(candidateInt, TyDouble, DBG_NAME("FpToIntCheck"));
        Value *intCheckCond = scaff_->builder_.CreateFCmpOEQ(candidate, candidateCheck, DBG_NAME("isIntTest"));
        scaff_->builder_.CreateCondBr(intCheckCond, negativeCheckBB, mergeBB);

        scaff_->builder_.SetInsertPoint(negativeCheckBB);
        Value *isNegativeCond = scaff_->builder_.CreateFCmpOGE(candidate, CoDouble(0.0), DBG_NAME("isNotNegative"));
        scaff_->builder_.CreateBr(mergeBB);

        scaff_->builder_.SetInsertPoint(mergeBB);
        PHINode *phiNode = scaff_->builder_.CreatePHI(isNegativeCond->getType(), 2, DBG_NAME("isNotNegativeIntResult"));
        phiNode->addIncoming(intCheckCond, intCheckBB);
        phiNode->addIncoming(isNegativeCond, negativeCheckBB);
        return phiNode;
    };
    count = generateValidationDouble(node.astnodeValue(), count, countValidator);
    list = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), list);

    // Get the current value of repcount since we are shadowing it.
    Value *repcountAddress = generateCallExtern(repcountAddr);
    Value *shadowedRepcount = scaff_->builder_.CreateLoad(TyDouble, repcountAddress, DBG_NAME("shadowedRepcount"));

    scaff_->builder_.CreateStore(CoDouble(1.0), repcountAddress);

    BasicBlock *loopBB = scaff_->createBasicBlock(DBG_NAME("loop"));
    BasicBlock *whileBB = scaff_->createBasicBlock(DBG_NAME("while"));
    BasicBlock *datumCheckBB = scaff_->createBasicBlock(DBG_NAME("datumCheck"));
    BasicBlock *loopNextBB = scaff_->createBasicBlock(DBG_NAME("loopNext"));
    BasicBlock *datumIsLastBB = scaff_->createBasicBlock(DBG_NAME("datumIsLast"));
    BasicBlock *noSayErrorBB = scaff_->createBasicBlock(DBG_NAME("noSayError"));
    BasicBlock *bailoutBB = scaff_->createBasicBlock(DBG_NAME("bailout"));
    BasicBlock *exitBB = scaff_->createBasicBlock(DBG_NAME("exit"));

    scaff_->addColdPathBlocks(noSayErrorBB, bailoutBB);
    scaff_->builder_.CreateBr(loopBB);

    scaff_->builder_.SetInsertPoint(loopBB);
    Value *repcount = scaff_->builder_.CreateLoad(TyDouble, repcountAddress, DBG_NAME("repcount"));
    Value *isLast = scaff_->builder_.CreateFCmpULE(repcount, count, DBG_NAME("isLast"));
    scaff_->builder_.CreateCondBr(isLast, whileBB, exitBB);

    scaff_->builder_.SetInsertPoint(whileBB);
    Value *result = generateCallList(list, TypeDatum);
    whileBB = scaff_->builder_.GetInsertBlock(); // Update the whileBB for the PHI since the call list changes the block.
    Value *resultType = generateGetDatumIsa(result);
    Value *mask = scaff_->builder_.CreateAnd(resultType, CoInt32(Datum::typeFlowControlMask), DBG_NAME("flowControlMask"));
    Value *cond = scaff_->builder_.CreateICmpEQ(mask, CoInt32(0), DBG_NAME("flowControlCond"));
    scaff_->builder_.CreateCondBr(cond, datumCheckBB, bailoutBB);

    scaff_->builder_.SetInsertPoint(datumCheckBB);
    Value *isDatum = scaff_->builder_.CreateAnd(resultType, CoInt32(Datum::typeDataMask), DBG_NAME("isDatumMask"));
    Value *isDatumCond = scaff_->builder_.CreateICmpEQ(isDatum, CoInt32(0), DBG_NAME("isDatumCond"));
    scaff_->builder_.CreateCondBr(isDatumCond, loopNextBB, datumIsLastBB);

    scaff_->builder_.SetInsertPoint(loopNextBB);
    Value *incrRepcount = scaff_->builder_.CreateFAdd(repcount, CoDouble(1.0), DBG_NAME("incrRepcount"));
    scaff_->builder_.CreateStore(incrRepcount, repcountAddress);
    scaff_->builder_.CreateBr(loopBB);

    scaff_->builder_.SetInsertPoint(datumIsLastBB);
    Value *isLastCount = scaff_->builder_.CreateFCmpUEQ(repcount, count, DBG_NAME("isLastCount"));
    scaff_->builder_.CreateCondBr(isLastCount, exitBB, noSayErrorBB);

    scaff_->builder_.SetInsertPoint(noSayErrorBB);
    Value *errNoSay = generateErrorNoSay(result);
    scaff_->builder_.CreateBr(bailoutBB);

    scaff_->builder_.SetInsertPoint(bailoutBB);
    PHINode *phiError = scaff_->builder_.CreatePHI(TyAddr, 2, DBG_NAME("errVal"));
    phiError->addIncoming(errNoSay, noSayErrorBB);
    phiError->addIncoming(result, whileBB);
    scaff_->builder_.CreateStore(shadowedRepcount, repcountAddress);
    generateReturn(phiError);

    scaff_->builder_.SetInsertPoint(exitBB);
    PHINode *phiNode = scaff_->builder_.CreatePHI(TyAddr, 2, DBG_NAME("retval"));
    phiNode->addIncoming(generateVoidRetval(node), loopBB);
    phiNode->addIncoming(result, datumIsLastBB);
    scaff_->builder_.CreateStore(shadowedRepcount, repcountAddress);
    return phiNode;
}

/***DOC REPCOUNT #
REPCOUNT
#

outputs the repetition count of the innermost current REPEAT or
FOREVER, starting from 1.  If no REPEAT or FOREVER is active,
outputs -1.

The abbreviation # can be used for REPCOUNT unless the REPEAT is
inside the template input to a higher order procedure such as
FOREACH, in which case # has a different meaning.

COD***/
// CMD REPCOUNT 0 0 0 r
Value *Compiler::genRepcount(const DatumPtr &node, TypeMask expected)
{
    Value *repcountAddress = generateCallExtern(repcountAddr);
    Value *repcount = scaff_->builder_.CreateLoad(TyDouble, repcountAddress, DBG_NAME("repcount"));
    return repcount;
}

/***DOC BYE
BYE

 command.  Exits from Logo.

COD***/
// CMD BYE 0 0 0 n
Value *Compiler::genBye(const DatumPtr &node, TypeMask expected)
{
    return generateImmediateReturn(generateErrorSystem());
}

/***DOC OUTPUT OP
OUTPUT value
OP value

    command.  Ends the running of the procedure in which it appears.
    That procedure outputs the value "value" to the context in which
    it was invoked.  Don't be confused: OUTPUT itself is a command,
    but the procedure that invokes OUTPUT is an operation.

COD***/
// CMD OUTPUT 1 1 1 n
// CMD OP 1 1 1 n
Value *Compiler::genOutput(const DatumPtr &node, TypeMask expected)
{
    return generateProcedureExit(node, expected, TypeDatum);
}

/***DOC STOP
STOP

    command.  Ends the running of the procedure in which it appears.
    Control is returned to the context in which that procedure was
    invoked.  The stopped procedure does not output a value.

COD***/
// CMD STOP 0 0 1 n
Value *Compiler::genStop(const DatumPtr &node, TypeMask expected)
{
    return generateProcedureExit(node, expected, TypeNothing);
}

/***DOC .MAYBEOUTPUT
.MAYBEOUTPUT value                  (special form)

    works like OUTPUT except that the expression that provides the
    input value might not, in fact, output a value, in which case
    the effect is like STOP.  This is intended for use in control
    structure definitions, for cases in which you don't know whether
    or not some expression produces a value.  Example:

        to invoke :function [:inputs] 2
        .maybeoutput apply :function :inputs
        end

        ? (invoke "print "a "b "c)
        a b c
        ? print (invoke "word "a "b "c)
        abc

    This is an alternative to RUNRESULT.  It's fast and easy to use,
    at the cost of being an exception to Logo's evaluation rules.
    (Ordinarily, it should be an error if the expression that's
    supposed to provide an input to something doesn't have a value.)

COD***/
// CMD .MAYBEOUTPUT 1 1 1 n
Value *Compiler::genMaybeoutput(const DatumPtr &node, TypeMask expected)
{
    return generateProcedureExit(node, expected, TypeDN);
}

Value *Compiler::generateProcedureExit(const DatumPtr &node,
                                       TypeMask expected,
                                       TypeMask childExpected)
{
    // If the treeifyer attached an output value to the node, then we need to process that.
    if (node.astnodeValue()->countOfChildren() > 0)
    {
        DatumPtr child = node.astnodeValue()->childAtIndex(0);
        DatumPtr proc = child.astnodeValue()->procedure_;
        if (proc.isNothing())
        {
            // It's a primitive, not a procedure. Generate a call to it.
            // Then generate a return of the value.
            Value *retval = generateChild(node.astnodeValue(), child, childExpected);

            retval = generateCallExtern(
                getCtrlReturn, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())), PaAddr(retval));
            return retval;
        }
        if (proc.datumValue()->isa_ == Datum::typeProcedure && proc.procedureValue()->isMacro_)
        {
            // A macro must not be tail-called. Its output list is evaluated in this
            // procedure, and that result is what OUTPUT returns.
            Value *retval = generateChild(node.astnodeValue(), child, childExpected);
            retval = generateNotFlowControl(retval);
            return generateCallExtern(
                getCtrlReturn, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())), PaAddr(retval));
        }
        // Else it's a procedure. Generate a tail call to it.
        Value *childAddr = CoAddr(child.astnodeValue());

        // TODO: Instead of TypeDatum, should we use childExpected?
        AllocaInst *ary = generateChildrenAlloca(child.astnodeValue(), TypeDatum, DBG_NAME("childAry"));
        Value *retObj = generateCallExtern(getCtrlContinuation,
                                           PaAddr(scaff_->evaluator_),
                                           PaAddr(childAddr),
                                           PaAddr(ary),
                                           PaInt32(ary->getArraySize()));
        return retObj;
    }
    // There is no child. Return nothing.
    Value *retval = generateVoidRetval(node);
    return generateCallExtern(
        getCtrlReturn, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())), PaAddr(retval));
}

/***DOC TAG
TAG quoted.word

    command.  Does nothing.  The input must be a literal word following
    a quotation mark ("), not the result of a computation.  Tags are
    used by the GOTO command.

COD***/
// CMD TAG 1 1 1 n
Value *Compiler::genTag(const DatumPtr &node, TypeMask expected)
{
    // Do nothing. We need to generate something in case this is the only
    // ASTNode in the block.
    // Note that if the input is not a literal word following a quotation mark,
    // no error is generated.

    // We need distinct function addresses for genTag and genNoop.
    // Some compilers *helpfully* optimize by merging identical function bodies
    // which causes us to be unable to distinguish between TAG and NOOP nodes.
    // So keep this line intact!
    (void)node.astnodeValue()->countOfChildren();

    return generateVoidRetval(node);
}

/***DOC GOTO
GOTO word

    command.  Looks for a TAG command with the same input in the same
    procedure, and continues running the procedure from the location of
    that TAG.  It is meaningless to use GOTO outside of a procedure.

COD***/
// CMD GOTO 1 1 1 n
Value *Compiler::genGoto(const DatumPtr &node, TypeMask expected)
{
    Value *nodeAddr = CoAddr(node.astnodeValue());
    Value *tag = generateChild(node.astnodeValue(), 0, TypeDatum);
    tag = generateWordFromDatum(node.astnodeValue(), tag);
    Value *retObj = generateCallExtern(getCtrlGoto, PaAddr(scaff_->evaluator_), PaAddr(nodeAddr), PaAddr(tag));
    generateImmediateReturn(retObj);
    return generateVoidRetval(node);
}

/***DOC CATCH
CATCH tag instructionlist

    command or operation.  Runs its second input.  Outputs if that
    instructionlist outputs.  If, while running the instructionlist,
    a THROW instruction is executed with a tag equal to the first
    input (case-insensitive comparison), then the running of the
    instructionlist is terminated immediately.  In this case the CATCH
    outputs if a value input is given to THROW.  The tag must be a word.

    If the tag is the word ERROR, then any error condition that arises
    during the running of the instructionlist has the effect of THROW
    "ERROR instead of printing an error message and returning to
    toplevel.  The CATCH does not output if an error is caught.  Also,
    during the running of the instructionlist, the variable ERRACT is
    temporarily unbound.  (If there is an error while ERRACT has a
    value, that value is taken as an instructionlist to be run after
    printing the error message.  Typically the value of ERRACT, if any,
    is the list [PAUSE].)

COD***/
// CMD CATCH 2 2 2 dn
Value *Compiler::genCatch(const DatumPtr &node, TypeMask expected)
{
    Value *tag = generateChild(node.astnodeValue(), 0, TypeDatum);
    Value *instructionlist = generateChild(node.astnodeValue(), 1, TypeDatum);
    generateWordFromDatum(node.astnodeValue(), tag);
    Value *errActStash = generateCallExtern(beginCatch);

    Value *result = generateCallList(instructionlist, expected);

    Value *retval = generateCallExtern(endCatch,
                                       PaAddr(scaff_->evaluator_),
                                       PaAddr(CoAddr(node.astnodeValue())),
                                       PaAddr(errActStash),
                                       PaAddr(result),
                                       PaAddr(tag));
    return retval;
}

/***DOC THROW
THROW tag
(THROW tag value)

    command.  Must be used within the scope of a CATCH with an equal
    tag.  Ends the running of the instructionlist of the CATCH.  If
    THROW is used with only one input, the corresponding CATCH does
    not output a value.  If THROW is used with two inputs, the second
    provides an output for the CATCH.

    THROW "TOPLEVEL can be used to terminate all running procedures and
    interactive pauses, and return to the toplevel instruction prompt.
    Typing the system interrupt character (alt-S for wxWidgets; otherwise
    normally control-C for Unix, control-Q for DOS, or command-period for
    Mac) has the same effect.

    THROW "ERROR can be used to generate an error condition.  If the
    error is not caught, it prints a message (THROW "ERROR) with the
    usual indication of where the error (in this case the THROW)
    occurred.  If a second input is used along with a tag of ERROR,
    that second input is used as the text of the error message
    instead of the standard message.  Also, in this case, the location
    indicated for the error will be, not the location of the THROW,
    but the location where the procedure containing the THROW was
    invoked.  This allows user-defined procedures to generate error
    messages as if they were primitives.  Note: in this case the
    corresponding CATCH "ERROR, if any, does not output, since the second
    input to THROW is not considered a return value.

    THROW "SYSTEM immediately leaves Logo, returning to the operating
    system, without printing the usual parting message and without
    deleting any editor temporary file written by EDIT.

COD***/
// CMD THROW 1 1 2 n
Value *Compiler::genThrow(const DatumPtr &node, TypeMask expected)
{
    std::vector<Value *> children = generateChildren(node.astnodeValue(), TypeDatum);
    Value *tag = generateWordFromDatum(node.astnodeValue(), children[0]);
    Value *output = (children.size() == 1) ? CoAddr(Datum::notADatum()) : children[1];
    Value *errObj = generateCallExtern(getErrorCustom, PaAddr(scaff_->evaluator_), PaAddr(tag), PaAddr(output));
    return generateImmediateReturn(errObj);
}

/***DOC ERROR
ERROR

    outputs a list describing the error just caught, if any.  If there was
    not an error caught since the last use of ERROR, the empty list will
    be output.  The error list contains four members: an integer code
    corresponding to the type of error, the text of the error message (as
    a single word including spaces), the name of the procedure in which
    the error occurred, and the instruction line on which the error
    occurred.

COD***/
// CMD ERROR 0 0 0 d
Value *Compiler::genError(const DatumPtr &node, TypeMask expected)
{
    return generateCallExtern(getCurrentError, PaAddr(scaff_->evaluator_));
}
/***DOC PAUSE
PAUSE

    command or operation.  Enters an interactive pause.  The user is
    prompted for instructions, as at toplevel, but with a prompt that
    includes the name of the procedure in which PAUSE was invoked.
    Local variables of that procedure are available during the pause.
    PAUSE outputs if the pause is ended by a CONTINUE with an input.

    If the variable ERRACT exists, and an error condition occurs,
    an interactive pause will be entered.  This allows the user to check
    values of local variables at the time of the error.

COD***/
// CMD PAUSE 0 0 0 dn
Value *Compiler::genPause(const DatumPtr &node, TypeMask expected)
{
    Q_UNUSED(node);
    Q_UNUSED(expected);
    Value *retval = generateCallExtern(callPause, PaAddr(scaff_->evaluator_));
    return generateNotFlowControl(retval);
}

/***DOC CONTINUE CO
CONTINUE value
CO value
(CONTINUE)
(CO)

    command.  Ends the current interactive pause, returning to the
    context of the PAUSE invocation that began it.  If CONTINUE is
    given an input, that value is used as the output from the PAUSE.
    If not, the PAUSE does not output.

    Exceptionally, the CONTINUE command can be used without its default
    input and without parentheses provided that nothing follows it on
    the instruction line.

COD***/
// CMD CONTINUE 0 -1 1 dn
// CMD CO 0 -1 1 dn
Value *Compiler::genContinue(const DatumPtr &node, TypeMask expected)
{
    Value *output = CoAddr(node.astnodeValue());
    if (node.astnodeValue()->countOfChildren() == 1)
    {
        output = generateChild(node.astnodeValue(), 0, TypeDatum);
    }
    return generateCallExtern(generateContinue, PaAddr(scaff_->evaluator_), PaAddr(output));
}
/***DOC RUNRESULT
RUNRESULT instructionlist

    runs the instructions in the input; outputs an empty list if
    those instructions produce no output, or a list whose only
    member is the output from running the input instructionlist.
    Useful for inventing command-or-operation control structures:

        local "result
        make "result runresult [something]
        if emptyp :result [stop]
        output first :result

COD***/
// CMD RUNRESULT 1 1 1 d
Value *Compiler::genRunresult(const DatumPtr &node, TypeMask expected)
{
    Value *instructionlist = generateChild(node.astnodeValue(), 0, TypeDatum);
    Value *result = generateCallList(instructionlist, TypeDN);
    return generateCallExtern(processRunresult, PaAddr(scaff_->evaluator_), PaAddr(result));
}

/***DOC MARK
MARK value

    Takes the input, places a mark on it, and outputs whatever was input. The
    address of the marked value will be printed out. Later if or when the value
    is no longer being used by QLogo, a message will again be printed out when
    the internal representation of the value is destroyed, including the address.

    Example: MAKE "A MARK 2+2

    or:

    PRINT MARK "Hello

COD***/
// CMD MARK 1 1 1 d
Value *Compiler::genMark(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeDatum);
    Value *thing = generateChild(node.astnodeValue(), 0, TypeDatum);
    return generateCallExtern(stdMark, PaAddr(thing));
}

/***DOC FOREVER
FOREVER instructionlist

    command.  Runs the "instructionlist" repeatedly, until something
    inside the instructionlist (such as STOP or THROW) makes it stop.

COD***/
// CMD FOREVER 1 1 1 n
Value *Compiler::genForever(const DatumPtr &node, TypeMask expected)
{
    Value *list = generateChild(node.astnodeValue(), 0, TypeDatum);
    list = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), list);

    // Get the current value of repcount since we are shadowing it.
    Value *repcountAddress = generateCallExtern(repcountAddr);
    Value *shadowedRepcount = scaff_->builder_.CreateLoad(TyDouble, repcountAddress, DBG_NAME("shadowedRepcount"));

    scaff_->builder_.CreateStore(CoDouble(1.0), repcountAddress);

    BasicBlock *whileBB = scaff_->createBasicBlock(DBG_NAME("while"));
    BasicBlock *datumCheckBB = scaff_->createBasicBlock(DBG_NAME("datumCheck"));
    BasicBlock *loopNextBB = scaff_->createBasicBlock(DBG_NAME("loopNext"));
    BasicBlock *noSayErrorBB = scaff_->createBasicBlock(DBG_NAME("noSayError"));
    BasicBlock *bailoutBB = scaff_->createBasicBlock(DBG_NAME("bailout"));

    scaff_->addColdPathBlocks(noSayErrorBB, bailoutBB);

    scaff_->builder_.CreateBr(whileBB);

    scaff_->builder_.SetInsertPoint(whileBB);
    Value *result = generateCallList(list, TypeDatum);
    BasicBlock *whileBBDestination = scaff_->builder_.GetInsertBlock();
    Value *resultType = generateGetDatumIsa(result);
    Value *mask = scaff_->builder_.CreateAnd(resultType, CoInt32(Datum::typeFlowControlMask), DBG_NAME("flowControlMask"));
    Value *cond = scaff_->builder_.CreateICmpEQ(mask, CoInt32(0), DBG_NAME("flowControlCond"));
    scaff_->builder_.CreateCondBr(cond, datumCheckBB, bailoutBB);

    scaff_->builder_.SetInsertPoint(datumCheckBB);
    Value *isDatum = scaff_->builder_.CreateAnd(resultType, CoInt32(Datum::typeDataMask), DBG_NAME("isDatumMask"));
    Value *isDatumCond = scaff_->builder_.CreateICmpEQ(isDatum, CoInt32(0), DBG_NAME("isDatumCond"));
    scaff_->builder_.CreateCondBr(isDatumCond, loopNextBB, noSayErrorBB);

    scaff_->builder_.SetInsertPoint(loopNextBB);
    Value *repcount = scaff_->builder_.CreateLoad(TyDouble, repcountAddress, DBG_NAME("repcount"));
    Value *incrRepcount = scaff_->builder_.CreateFAdd(repcount, CoDouble(1.0), DBG_NAME("incrRepcount"));
    scaff_->builder_.CreateStore(incrRepcount, repcountAddress);
    scaff_->builder_.CreateBr(whileBB);

    scaff_->builder_.SetInsertPoint(noSayErrorBB);
    Value *errNoSay = generateErrorNoSay(result);
    scaff_->builder_.CreateBr(bailoutBB);

    scaff_->builder_.SetInsertPoint(bailoutBB);
    PHINode *phiError = scaff_->builder_.CreatePHI(TyAddr, 2, DBG_NAME("errVal"));
    phiError->addIncoming(errNoSay, noSayErrorBB);
    phiError->addIncoming(result, whileBBDestination);
    scaff_->builder_.CreateStore(shadowedRepcount, repcountAddress);
    return generateImmediateReturn(phiError);
}

/***DOC TEST
TEST tf

    command.  Remembers its input, which must be TRUE or FALSE, for use
    by later IFTRUE or IFFALSE instructions.  The effect of TEST is local
    to the procedure in which it is used; any corresponding IFTRUE or
    IFFALSE must be in the same procedure or a subprocedure.

COD***/
// CMD TEST 1 1 1 n
Value *Compiler::genTest(const DatumPtr &node, TypeMask expected)
{
    Value *tf = generateChild(node.astnodeValue(), 0, TypeBool);
    generateCallExtern(saveTestResult, PaBool(tf));
    return generateVoidRetval(node);
}
/***DOC IFTRUE IFT
IFTRUE instructionlist
IFT instructionlist

    command.  Runs its input if the most recent TEST instruction had
    a TRUE input.  The TEST must have been in the same procedure or a
    superprocedure.

COD***/
// CMD IFTRUE 1 1 1 dn
// CMD IFT 1 1 1 dn
Value *Compiler::generateIftrue(const DatumPtr &node, TypeMask expected)
{
    return generateIftruefalse(node, expected, true);
}

/***DOC IFFALSE IFF
IFFALSE instructionlist
IFF instructionlist

    command.  Runs its input if the most recent TEST instruction had
    a FALSE input.  The TEST must have been in the same procedure or a
    superprocedure.

COD***/
// CMD IFFALSE 1 1 1 dn
// CMD IFF 1 1 1 dn
Value *Compiler::generateIffalse(const DatumPtr &node, TypeMask expected)
{
    return generateIftruefalse(node, expected, false);
}

Value *Compiler::generateIftruefalse(const DatumPtr &node, TypeMask expected, bool testForTrue)
{
    BasicBlock *notTestedBB = scaff_->createBasicBlock(DBG_NAME("notTested"));
    BasicBlock *runListBB = scaff_->createBasicBlock(DBG_NAME("runList"));
    BasicBlock *noRunListBB = scaff_->createBasicBlock(DBG_NAME("noRunList"));
    BasicBlock *mergeBB = scaff_->createBasicBlock(DBG_NAME("merge"));

    scaff_->addColdPathBlocks(notTestedBB);

    auto generateListFn = node.astnodeValue()->inSequenceTail_ ? &Compiler::generateCallListContinuation : &Compiler::generateCallList;

    // 0 = not tested, 3 = tested true, 2 = tested false.
    int8_t runCondition = testForTrue ? 3 : 2;
    int8_t noRunCondition = testForTrue ? 2 : 3;

    Value *instructionlist = generateChild(node.astnodeValue(), 0, TypeDatum);
    Value *testResult = generateCallExtern(getTestResult);
    SwitchInst *sw = scaff_->builder_.CreateSwitch(testResult, notTestedBB, 3);
    sw->addCase(CoInt8(0), notTestedBB);
    sw->addCase(CoInt8(runCondition), runListBB);
    sw->addCase(CoInt8(noRunCondition), noRunListBB);

    scaff_->builder_.SetInsertPoint(notTestedBB);
    Value *errVal = generateErrorNoTest(CoAddr(node.astnodeValue()->nodeName_.datumValue()));
    generateReturn(errVal);

    scaff_->builder_.SetInsertPoint(runListBB);
    Value *listRetval = (this->*generateListFn)(instructionlist, expected);
    BasicBlock *listRetvalBB = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.CreateBr(mergeBB);

    scaff_->builder_.SetInsertPoint(noRunListBB);
    Value *noRetval = generateVoidRetval(node);
    scaff_->builder_.CreateBr(mergeBB);

    scaff_->builder_.SetInsertPoint(mergeBB);
    PHINode *retval = scaff_->builder_.CreatePHI(TyAddr, 2, DBG_NAME("retval"));
    retval->addIncoming(listRetval, listRetvalBB);
    retval->addIncoming(noRetval, noRunListBB);
    return retval;
}


/***DOC APPLY
APPLY template inputlist

    command or operation.  Runs the "template," filling its slots with
    the members of "inputlist."  The number of members in "inputlist"
    must be an acceptable number of slots for "template."  It is
    illegal to apply the primitive TO as a template, but anything else
    is okay.  APPLY outputs what "template" outputs, if anything.

COD***/
// CMD APPLY 2 2 2 dn
Value *Compiler::genApply(const DatumPtr &node, TypeMask expected)
{
    BasicBlock *validTemplateBB = scaff_->createBasicBlock(DBG_NAME("validTemplate"));
    BasicBlock *namedProcedureBB = scaff_->createBasicBlock(DBG_NAME("namedProcedure"));
    BasicBlock *explicitSlotBB = scaff_->createBasicBlock(DBG_NAME("explicitSlot"));
    BasicBlock *lambdaBB = scaff_->createBasicBlock(DBG_NAME("lambda"));
    BasicBlock *procedureTextBB = scaff_->createBasicBlock(DBG_NAME("procedureText"));
    BasicBlock *mergeBB = scaff_->createBasicBlock(DBG_NAME("merge"));
    BasicBlock *invalidTemplateBB = scaff_->createBasicBlock(DBG_NAME("invalidTemplate"));

    scaff_->addColdPathBlocks(invalidTemplateBB);

    Value *aTemplate = generateChild(node.astnodeValue(), 0, TypeDatum);
    Value *inputlist = generateChild(node.astnodeValue(), 1, TypeDatum);

    Value *templateForm = generateCallExtern(getTemplateForm, PaAddr(scaff_->evaluator_), PaAddr(aTemplate));

    Value *templateCond = scaff_->builder_.CreateICmpEQ(templateForm, CoInt8(TemplateFormInvalid), DBG_NAME("templateCond"));
    scaff_->builder_.CreateCondBr(templateCond, invalidTemplateBB, validTemplateBB);

    scaff_->builder_.SetInsertPoint(validTemplateBB);
    Value *inputList = generateListFromDatum(node.astnodeValue(), inputlist);
    SwitchInst *sw = scaff_->builder_.CreateSwitch(templateForm, invalidTemplateBB, 4);
    sw->addCase(CoInt8(TemplateFormNamedProcedure), namedProcedureBB);
    sw->addCase(CoInt8(TemplateFormExplicitSlot), explicitSlotBB);
    sw->addCase(CoInt8(TemplateFormLambda), lambdaBB);
    sw->addCase(CoInt8(TemplateFormProcedureText), procedureTextBB);

    scaff_->builder_.SetInsertPoint(invalidTemplateBB);
    Value *errVal = generateErrorNoLike(node.astnodeValue(), aTemplate);
    generateReturn(errVal);

    scaff_->builder_.SetInsertPoint(namedProcedureBB);
    Value *namedProcedureResult = generateApplyNamedProcedure(node, expected, aTemplate, inputList);
    BasicBlock *namedProcedureDestBB = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.CreateBr(mergeBB);

    scaff_->builder_.SetInsertPoint(explicitSlotBB);
    Value *slotResult = generateApplyExplicitSlot(node, expected, aTemplate, inputList);
    BasicBlock *explicitSlotDestBB = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.CreateBr(mergeBB);

    scaff_->builder_.SetInsertPoint(lambdaBB);
    Value *lambdaResult = generateApplyLambda(node, expected, aTemplate, inputList);
    BasicBlock *lambdaDestBB = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.CreateBr(mergeBB);

    scaff_->builder_.SetInsertPoint(procedureTextBB);
    Value *procedureTextResult = generateApplyProcedureText(node, expected, aTemplate, inputList);
    BasicBlock *procedureTextDestBB = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.CreateBr(mergeBB);

    scaff_->builder_.SetInsertPoint(mergeBB);
    PHINode *retval = scaff_->builder_.CreatePHI(TyAddr, 4, DBG_NAME("retval"));
    retval->addIncoming(namedProcedureResult, namedProcedureDestBB);
    retval->addIncoming(slotResult, explicitSlotDestBB);
    retval->addIncoming(lambdaResult, lambdaDestBB);
    retval->addIncoming(procedureTextResult, procedureTextDestBB);
    return retval;
}

Value *Compiler::generateApplyNamedProcedure(const DatumPtr &node, TypeMask expected,
                                             Value *aTemplate, Value *inputList)
{
    Value *callAST = generateCallExtern(buildApplyNamedCall,
                                        PaAddr(scaff_->evaluator_),
                                        PaAddr(CoAddr(node.astnodeValue()->nodeName_.datumValue())),
                                        PaAddr(aTemplate),
                                        PaAddr(inputList));
    callAST = generateNotFlowControl(callAST);
    auto generateListFn = node.astnodeValue()->inSequenceTail_ ? &Compiler::generateCallListContinuation
                                                              : &Compiler::generateCallList;
    return (this->*generateListFn)(callAST, expected);
}

Value *Compiler::generateApplyExplicitSlot(const DatumPtr &node, TypeMask expected,
                                           Value *aTemplate, Value *inputList)
{
    generateCallExtern(pushExplicitSlotList, PaAddr(inputList));
    Value *result = generateCallList(aTemplate, expected);
    generateCallExtern(popExplicitSlotList);
    return generateNotFlowControl(result);
}

Value *Compiler::generateApplyLambda(const DatumPtr &node, TypeMask expected,
                                     Value *aTemplate, Value *inputList)
{
    Value *bodyOrErr = generateCallExtern(beginApplyLambda,
                                          PaAddr(scaff_->evaluator_),
                                          PaAddr(CoAddr(node.astnodeValue()->nodeName_.datumValue())),
                                          PaAddr(aTemplate),
                                          PaAddr(inputList));
    bodyOrErr = generateNotFlowControl(bodyOrErr);
    Value *result = generateCallList(bodyOrErr, expected);
    generateCallExtern(endApplyLambda);
    return generateNotFlowControl(result);
}

Value *Compiler::generateApplyProcedureText(const DatumPtr &node, TypeMask expected,
                                            Value *aTemplate, Value *inputList)
{
    Value *callAST = generateCallExtern(buildApplyProcedureText,
                                        PaAddr(scaff_->evaluator_),
                                        PaAddr(CoAddr(node.astnodeValue()->nodeName_.datumValue())),
                                        PaAddr(aTemplate),
                                        PaAddr(inputList));
    callAST = generateNotFlowControl(callAST);
    auto generateListFn = node.astnodeValue()->inSequenceTail_ ? &Compiler::generateCallListContinuation
                                                              : &Compiler::generateCallList;
    return (this->*generateListFn)(callAST, expected);
}

/***DOC ?
?
(? number)

    outputs a member of the current explicit-slot template input list.
    ? is equivalent to (? 1).  (? n) outputs the nth member (1-based).
    The form ?n is equivalent to (? n).

COD***/
// CMD ? 0 0 1 d
Value *Compiler::genQ(const DatumPtr &node, TypeMask expected)
{
    Q_ASSERT(expected && TypeDatum);

    Value *index = CoAddr(Datum::notADatum());
    if (node.astnodeValue()->countOfChildren() == 1)
        index = generateChild(node.astnodeValue(), 0, TypeDatum);

    Value *result = generateCallExtern(getExplicitSlot,
                                       PaAddr(scaff_->evaluator_),
                                       PaAddr(CoAddr(node.astnodeValue())),
                                       PaAddr(index));
    return generateNotFlowControl(result);
}


/***DOC IGNORE
IGNORE value						(library procedure)

	command.  Does nothing.  Used when an expression is evaluated for
	a side effect and its actual value is unimportant.

COD***/

/***DOC `
` list							(library procedure)

	outputs a list equal to its input but with certain substitutions.
	If a member of the input list is the word "," (comma) then the
	following member should be an instructionlist that produces an
	output when run.  That output value replaces the comma and the
	instructionlist.  If a member of the input list is the word ",@"
	(comma atsign) then the following member should be an instructionlist
	that outputs a list when run.  The members of that list replace the
	,@ and the instructionlist.  Example:

		show `[foo baz ,[bf [a b c]] garply ,@[bf [a b c]]]

	will print

		[foo baz [b c] garply b c]

	A word starting with , or ,@ is treated as if the rest of the word
	were a one-word list, e.g., ,:FOO is equivalent to ,[:FOO].

	A word starting with ", (quote comma) or :, (colon comma) becomes a
	word starting with " or : but with the result of running the
	substitution (or its first word, if the result is a list) replacing
	what comes after the comma.

	Backquotes can be nested.  Substitution is done only for commas at
	the same depth as the backquote in which they are found:

		? show `[a `[b ,[1+2] ,[foo ,[1+3] d] e] f]
		[a ` [b , [1+2] , [foo 4 d] e] f]

		?make "name1 "x
		?make "name2 "y
		? show `[a `[b ,:,:name1 ,",:name2 d] e]
		[a ` [b , [:x] , ["y] d] e]

COD***/

/***DOC FOR
FOR forcontrol instructionlist				(library procedure)

	command.  The first input must be a list containing three or four
	members: (1) a word, which will be used as the name of a local
	variable; (2) a word or list that will be evaluated as by RUN to
	determine a number, the starting value of the variable; (3) a word
	or list that will be evaluated to determine a number, the limit value
	of the variable; (4) an optional word or list that will be evaluated
	to determine the step size.  If the fourth member is missing, the
	step size will be 1 or -1 depending on whether the limit value is
	greater than or less than the starting value, respectively.

	The second input is an instructionlist.  The effect of FOR is to run
	that instructionlist repeatedly, assigning a new value to the control
	variable (the one named by the first member of the forcontrol list)
	each time.  First the starting value is assigned to the control
	variable.  Then the value is compared to the limit value.  FOR is
	complete when the sign of (current - limit) is the same as the sign
	of the step size.  (If no explicit step size is provided, the
	instructionlist is always run at least once.  An explicit step size
	can lead to a zero-trip FOR, e.g., FOR [I 1 0 1] ...)  Otherwise, the
	instructionlist is run, then the step is added to the current value
	of the control variable and FOR returns to the comparison step.

		? for [i 2 7 1.5] [print :i]
		2
		3.5
		5
		6.5
		?

COD***/

/***DOC DO.WHILE
DO.WHILE instructionlist tfexpression			(library procedure)

	command.  Repeatedly evaluates the "instructionlist" as long as the
	evaluated "tfexpression" remains TRUE.  Evaluates the first input
	first, so the "instructionlist" is always run at least once.  The
	"tfexpression" must be an expressionlist whose value when evaluated
	is TRUE or FALSE.

COD***/

/***DOC WHILE
WHILE tfexpression instructionlist			(library procedure)

	command.  Repeatedly evaluates the "instructionlist" as long as the
	evaluated "tfexpression" remains TRUE.  Evaluates the first input
	first, so the "instructionlist" may never be run at all.  The
	"tfexpression" must be an expressionlist whose value when evaluated
	is TRUE or FALSE.

COD***/

/***DOC DO.UNTIL
DO.UNTIL instructionlist tfexpression			(library procedure)

	command.  Repeatedly evaluates the "instructionlist" as long as the
	evaluated "tfexpression" remains FALSE.  Evaluates the first input
	first, so the "instructionlist" is always run at least once.  The
	"tfexpression" must be an expressionlist whose value when evaluated
	is TRUE or FALSE.

COD***/

/***DOC UNTIL
UNTIL tfexpression instructionlist			(library procedure)

	command.  Repeatedly evaluates the "instructionlist" as long as the
	evaluated "tfexpression" remains FALSE.  Evaluates the first input
	first, so the "instructionlist" may never be run at all.  The
	"tfexpression" must be an expressionlist whose value when evaluated
	is TRUE or FALSE.

COD***/

/***DOC CASE
CASE value clauses					(library procedure)

	command or operation.  The second input is a list of lists (clauses);
	each clause is a list whose first element is either a list of values
	or the word ELSE and whose butfirst is a Logo expression or
	instruction.  CASE examines the clauses in order.  If a clause begins
	with the word ELSE (upper or lower case), then the butfirst of that
	clause is evaluated and CASE outputs its value, if any.  If the first
	input to CASE is a member of the first element of a clause, then the
	butfirst of that clause is evaluated and CASE outputs its value, if
	any.  If neither of these conditions is met, then CASE goes on to the
	next clause.  If no clause is satisfied, CASE does nothing.  Example:

		to vowelp :letter
		output case :letter [ [[a e i o u] "true] [else "false] ]
		end

COD***/

/***DOC COND
COND clauses						(library procedure)

	command or operation.  The input is a list of lists (clauses); each
	clause is a list whose first element is either an expression whose
	value is TRUE or FALSE, or the word ELSE, and whose butfirst is a Logo
	expression or instruction.  COND examines the clauses in order.  If a
	clause begins with the word ELSE (upper or lower case), then the
	butfirst of that clause is evaluated and CASE outputs its value, if
	any.  Otherwise, the first element of the clause is evaluated; the
	resulting value must be TRUE or FALSE.  If it's TRUE, then the
	butfirst of that clause is evaluated and COND outputs its value, if
	any.  If the value is FALSE, then COND goes on to the next clause.  If
	no clause is satisfied, COND does nothing.  Example:

		to evens :numbers	; select even numbers from a list
		op cond [ [[emptyp :numbers] []]
		          [[evenp first :numbers]  ; assuming EVENP is defined
		           fput first :numbers evens butfirst :numbers]
		          [else evens butfirst :numbers] ]
		end


COD***/

/***DOC INVOKE
INVOKE template input					(library procedure)
(INVOKE template input1 input2 ...)

	command or operation.  Exactly like APPLY except that the inputs
	are provided as separate expressions rather than in a list.

COD***/

/***DOC FOREACH
FOREACH data template					(library procedure)
(FOREACH data1 data2 ... template)

	command.  Evaluates the template list repeatedly, once for each
	member of the data list.  If more than one data list are given,
	each of them must be the same length.  (The data inputs can be
	words, in which case the template is evaluated once for each
	character.)

	In a template, the symbol ?REST represents the portion of the
	data input to the right of the member currently being used as
	the ? slot-filler.  That is, if the data input is [A B C D E]
	and the template is being evaluated with ? replaced by B, then
	?REST would be replaced by [C D E].  If multiple parallel slots
	are used, then (?REST 1) goes with ?1, etc.

	In a template, the symbol # represents the position in the data
	input of the member currently being used as the ? slot-filler.
	That is, if the data input is [A B C D E] and the template is
	being evaluated with ? replaced by B, then # would be replaced
	by 2.

COD***/

/***DOC MAP
MAP template data					(library procedure)
(MAP template data1 data2 ...)

	outputs a word or list, depending on the type of the data input,
	of the same length as that data input.  (If more than one data
	input are given, the output is of the same type as data1.)  Each
	member of the output is the result of evaluating the template
	list, filling the slots with the corresponding member(s) of the
	data input(s).  (All data inputs must be the same length.)  In the
	case of a word output, the results of the template evaluation must
	be words, and they are concatenated with WORD.

	In a template, the symbol ?REST represents the portion of the
	data input to the right of the member currently being used as
	the ? slot-filler.  That is, if the data input is [A B C D E]
	and the template is being evaluated with ? replaced by B, then
	?REST would be replaced by [C D E].  If multiple parallel slots
	are used, then (?REST 1) goes with ?1, etc.

	In a template, the symbol # represents the position in the data
	input of the member currently being used as the ? slot-filler.
	That is, if the data input is [A B C D E] and the template is
	being evaluated with ? replaced by B, then # would be replaced
	by 2.

COD***/

/***DOC MAP.SE
MAP.SE template data					(library procedure)
(MAP.SE template data1 data2 ...)

	outputs a list formed by evaluating the template list repeatedly
	and concatenating the results using SENTENCE.  That is, the
	members of the output are the members of the results of the
	evaluations.  The output list might, therefore, be of a different
	length from that of the data input(s).  (If the result of an
	evaluation is the empty list, it contributes nothing to the final
	output.)  The data inputs may be words or lists.

	In a template, the symbol ?REST represents the portion of the
	data input to the right of the member currently being used as
	the ? slot-filler.  That is, if the data input is [A B C D E]
	and the template is being evaluated with ? replaced by B, then
	?REST would be replaced by [C D E].  If multiple parallel slots
	are used, then (?REST 1) goes with ?1, etc.

	In a template, the symbol # represents the position in the data
	input of the member currently being used as the ? slot-filler.
	That is, if the data input is [A B C D E] and the template is
	being evaluated with ? replaced by B, then # would be replaced
	by 2.

COD***/

/***DOC FILTER
FILTER tftemplate data					(library procedure)

	outputs a word or list, depending on the type of the data input,
	containing a subset of the members (for a list) or characters (for
	a word) of the input.  The template is evaluated once for each
	member or character of the data, and it must produce a TRUE or
	FALSE value.  If the value is TRUE, then the corresponding input
	constituent is included in the output.

		? print filter "vowelp "elephant
		eea
		?

	In a template, the symbol ?REST represents the portion of the
	data input to the right of the member currently being used as
	the ? slot-filler.  That is, if the data input is [A B C D E]
	and the template is being evaluated with ? replaced by B, then
	?REST would be replaced by [C D E].

	In a template, the symbol # represents the position in the data
	input of the member currently being used as the ? slot-filler.
	That is, if the data input is [A B C D E] and the template is
	being evaluated with ? replaced by B, then # would be replaced
	by 2.

COD***/

/***DOC FIND
FIND tftemplate data					(library procedure)

	outputs the first constituent of the data input (the first member
	of a list, or the first character of a word) for which the value
	produced by evaluating the template with that consituent in its
	slot is TRUE.  If there is no such constituent, the empty list
	is output.

	In a template, the symbol ?REST represents the portion of the
	data input to the right of the member currently being used as
	the ? slot-filler.  That is, if the data input is [A B C D E]
	and the template is being evaluated with ? replaced by B, then
	?REST would be replaced by [C D E].

	In a template, the symbol # represents the position in the data
	input of the member currently being used as the ? slot-filler.
	That is, if the data input is [A B C D E] and the template is
	being evaluated with ? replaced by B, then # would be replaced
	by 2.

COD***/

/***DOC REDUCE
REDUCE template data					(library procedure)

	outputs the result of applying the template to accumulate the
	members of the data input.  The template must be a two-slot
	function.  Typically it is an associative function name like SUM.
	If the data input has only one constituent (member in a list or
	character in a word), the output is that consituent.  Otherwise,
	the template is first applied with ?1 filled with the next-to-last
	consitient and ?2 with the last constituent.  Then, if there are
	more constituents, the template is applied with ?1 filled with the
	next constituent to the left and ?2 with the result from the
	previous evaluation.  This process continues until all constituents
	have been used.  The data input may not be empty.

	Note: If the template is, like SUM, the name of a procedure that is
	capable of accepting arbitrarily many inputs, it is more efficient
	to use APPLY instead of REDUCE.  The latter is good for associative
	procedures that have been written to accept exactly two inputs:

		to max :a :b
		output ifelse :a > :b [:a] [:b]
		end

		print reduce "max [...]

	Alternatively, REDUCE can be used to write MAX as a procedure
	that accepts any number of inputs, as SUM does:

		to max [:inputs] 2
		if emptyp :inputs ~
		   [(throw "error [not enough inputs to max])]
		output reduce [ifelse ?1 > ?2 [?1] [?2]] :inputs
		end

COD***/

/***DOC CROSSMAP
CROSSMAP template listlist				(library procedure)
(CROSSMAP template data1 data2 ...)

	outputs a list containing the results of template evaluations.
	Each data list contributes to a slot in the template; the number
	of slots is equal to the number of data list inputs.  As a special
	case, if only one data list input is given, that list is taken as
	a list of data lists, and each of its members contributes values
	to a slot.  CROSSMAP differs from MAP in that instead of taking
	members from the data inputs in parallel, it takes all possible
	combinations of members of data inputs, which need not be the same
	length.

		? show (crossmap [word ?1 ?2] [a b c] [1 2 3 4])
		[a1 a2 a3 a4 b1 b2 b3 b4 c1 c2 c3 c4]
		?

	For compatibility with the version in the first edition of CSLS,
	CROSSMAP templates may use the notation :1 instead of ?1 to indicate
	slots.

COD***/

/***DOC CASCADE
CASCADE endtest template startvalue			(library procedure)
(CASCADE endtest tmp1 sv1 tmp2 sv2 ...)
(CASCADE endtest tmp1 sv1 tmp2 sv2 ... finaltemplate)

	outputs the result of applying a template (or several templates,
	as explained below) repeatedly, with a given value filling the
	slot the first time, and the result of each application filling
	the slot for the following application.

	In the simplest case, CASCADE has three inputs.  The second input
	is a one-slot expression template.  That template is evaluated
	some number of times (perhaps zero).  On the first evaluation,
	the slot is filled with the third input; on subsequent evaluations,
	the slot is filled with the result of the previous evaluation.
	The number of evaluations is determined by the first input.  This
	can be either a nonnegative integer, in which case the template is
	evaluated that many times, or a predicate expression template, in
	which case it is evaluated (with the same slot filler that will be
	used for the evaluation of the second input) repeatedly, and the
	CASCADE evaluation continues as long as the predicate value is
	FALSE.  (In other words, the predicate template indicates the
	condition for stopping.)

	If the template is evaluated zero times, the output from CASCADE
	is the third (startvalue) input.  Otherwise, the output is the
	value produced by the last template evaluation.

	CASCADE templates may include the symbol # to represent the number
	of times the template has been evaluated.  This slot is filled with
	1 for the first evaluation, 2 for the second, and so on.

		? show cascade 5 [lput # ?] []
		[1 2 3 4 5]
		? show cascade [vowelp first ?] [bf ?] "spring
		ing
		? show cascade 5 [# * ?] 1
		120
		?

	Several cascaded results can be computed in parallel by providing
	additional template-startvalue pairs as inputs to CASCADE.  In this
	case, all templates (including the endtest template, if used) are
	multi-slot, with the number of slots equal to the number of pairs of
	inputs.  In each round of evaluations, ?2, for example, represents the
	result of evaluating the second template in the previous round.  If
	the total number of inputs (including the first endtest input) is odd,
	then the output from CASCADE is the final value of the first template.
	If the total number of inputs is even, then the last input is a
	template that is evaluated once, after the end test is satisfied, to
	determine the output from CASCADE.

		to fibonacci :n
		output (cascade :n [?1 + ?2] 1 [?1] 0)
		end

		to piglatin :word
		output (cascade [vowelp first ?] ~
				[word bf ? first ?] ~
				:word ~
				[word ? "ay])
		end

COD***/

/***DOC TRANSFER
TRANSFER endtest template inbasket			(library procedure)

	outputs the result of repeated evaluation of the template.
	The template is evaluated once for each member of the list
	"inbasket."  TRANSFER maintains an "outbasket" that is
	initially the empty list.  After each evaluation of the
	template, the resulting value becomes the new outbasket.

	In the template, the symbol ?IN represents the current member
	from the inbasket; the symbol ?OUT represents the entire
	current outbasket.  Other slot symbols should not be used.

	If the first (endtest) input is an empty list, evaluation
	continues until all inbasket members have been used.  If not,
	the first input must be a predicate expression template, and
	evaluation continues until either that template's value is TRUE
	or the inbasket is used up.


COD***/

/***DOC MACROEXPAND
MACROEXPAND expr					(library procedure)

	takes as its input a Logo expression that invokes a macro (that is,
	one that begins with the name of a macro) and outputs the the Logo
	expression into which the macro would translate the input expression.


		.macro localmake :name :value
		op `[local ,[word "" :name] apply "make [,[:name] ,[:value]]]
		end

		? show macroexpand [localmake "pi 3.14159]
		[local "pi apply "make [pi 3.14159]]


COD***/
