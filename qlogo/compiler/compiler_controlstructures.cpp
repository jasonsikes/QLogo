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
#include "sharedconstants.h"
#include "workspace/exports.inc"
#include "workspace/kernel.h"
#include "workspace/procedures.h"
using namespace llvm;
using namespace llvm::orc;

namespace
{

bool isTracedProcedureName(const DatumPtr &name)
{
    return Kernel::get().containsName(
        Kernel::NameMark::Traced, Kernel::NameKind::Procedures, name.toString(Datum::ToStringFlags_Key));
}

/// TRACE begin/end are compiled at the call site. A continuation reuses the
/// caller's frame and never runs that site, so a traced caller or callee
/// must take the real nested call instead.
bool tracedProcedureDisablesTco(ASTNode *callee)
{
    if (callee != nullptr && isTracedProcedureName(callee->nodeName_))
        return true;
    if (Kernel::get().callFrameStack_.empty())
        return false;
    CallFrame *cf = Kernel::get().currentCallFrame();
    if (!cf->sourceNode_.isASTNode())
        return false;
    return isTracedProcedureName(cf->sourceNode_.astnodeValue()->nodeName_);
}

} // namespace

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
    // IF (2 inputs): run the list only when true. The false arm is IF
    // node (nothing).
    //
    // Do not run [] — print if 1=2 ["hello] must be
    // "if didn't output to print", not "[] didn't output to print".
    //
    // IFELSE (3 inputs): select the list pointer, then run it once.

    auto generateListFn =
        node.astnodeValue()->inSequenceTail_ ? &Compiler::generateCallListContinuation : &Compiler::generateCallList;
    const bool hasElse = node.astnodeValue()->countOfChildren() == 3;

    std::vector<TypeMask> expectedAry = {TypeDB, TypeDatum};
    if (hasElse)
        expectedAry.push_back(TypeDatum);

    CallSite cs(this, node, expectedAry);
    const std::vector<Value *> &children = cs.children();

    Value *cond = children[0];
    Value *ift = children[1];
    Value *iff = hasElse ? children[2] : generateVoidRetval(node);

    if (cond->getType()->isPointerTy())
    {
        cond = generateListExecIfList(node.astnodeValue(), cond);
        cond = generateBoolFromDatum(node.astnodeValue(), cond);
    }

    Value *retval;
    if (hasElse)
    {
        Value *chosen = scaff_->builder_.CreateSelect(cond, ift, iff, DBG_NAME("chosen"));
        retval = (this->*generateListFn)(chosen);
    }
    else
    {
        BasicBlock *thenBB = scaff_->createBasicBlock(DBG_NAME("then"));
        BasicBlock *elseBB = scaff_->createBasicBlock(DBG_NAME("else"));
        BasicBlock *mergeBB = scaff_->createBasicBlock(DBG_NAME("merge"));

        scaff_->builder_.CreateCondBr(cond, thenBB, elseBB);

        scaff_->builder_.SetInsertPoint(thenBB);
        ift = (this->*generateListFn)(ift);
        thenBB = scaff_->builder_.GetInsertBlock();
        scaff_->builder_.CreateBr(mergeBB);

        scaff_->builder_.SetInsertPoint(elseBB);
        scaff_->builder_.CreateBr(mergeBB);

        scaff_->builder_.SetInsertPoint(mergeBB);
        PHINode *phiNode = scaff_->builder_.CreatePHI(TyAddr, 2, DBG_NAME("iftmp"));
        phiNode->addIncoming(ift, thenBB);
        phiNode->addIncoming(iff, elseBB);
        retval = phiNode;
    }
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC RUN
RUN instructionlist

command or operation.  Runs the Logo instructions in the input
list; outputs if the list contains an expression that outputs.

COD***/
// CMD RUN 1 1 1 dn
Value *Compiler::genRun(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *list = cs.child(0);
    list = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), list);
    if (node.astnodeValue()->inSequenceTail_)
    {
        return cs.ret(generateNotFlowControl(generateCallListContinuation(list)));
    }
    return cs.ret(generateNotFlowControl(generateCallList(list)));
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
    CallSite cs(this, node, TypeDatum);
    Value *list = cs.child(0);
    list = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), list);
    generateCallExtern(stdRuntimeStart);
    Value *result = generateCallList(list);
    Value *printed = generateCallExtern(stdRuntimeEnd, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())));
    generateNotFlowControl(printed);
    return cs.ret(generateNotFlowControl(result));
}

/***DOC REPEAT
REPEAT num instructionlist

command.  Runs the "instructionlist" repeatedly, "num" times.

COD***/
// CMD REPEAT 2 2 2 dn
Value *Compiler::genRepeat(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeReal, TypeDatum);
    auto [count, list] = cs.children();

    count = generateValidationDouble(node.astnodeValue(), count, [this](Value *candidate) {
        return generateIsExactIntegerInRange(candidate, 0.0, 2147483647.0);
    });
    list = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), list);
    return cs.ret(generateNotFlowControl(generateRepeatLike(node, list, count)));
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
    CallSite cs(this, node, TypeNothing);
    Value *repcountAddress = generateCallExtern(repcountAddr);
    Value *repcount = scaff_->builder_.CreateLoad(TyDouble, repcountAddress, DBG_NAME("repcount"));
    return cs.ret(repcount);
}

/***DOC BYE
BYE

 command.  Exits from Logo.

COD***/
// CMD BYE 0 0 0 n
Value *Compiler::genBye(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeNothing);
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

Value *Compiler::generateProcedureExit(const DatumPtr &node, TypeMask expected, TypeMask childExpected)
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
            CallSite cs(this, node, childExpected);
            Value *retval = cs.child(0);

            retval = generateCallExtern(
                getCtrlReturn, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())), PaAddr(retval));
            return cs.ret(retval);
        }
        if (proc.datumValue()->isa_ == Datum::typeProcedure && proc.procedureValue()->isMacro_)
        {
            // A macro must not be tail-called. Its output list is evaluated in this
            // procedure, and that result is what OUTPUT returns.
            CallSite cs(this, node, childExpected);
            Value *retval = cs.child(0);
            retval = generateNotFlowControl(retval);
            return cs.ret(generateCallExtern(
                getCtrlReturn, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())), PaAddr(retval)));
        }
        if (tracedProcedureDisablesTco(child.astnodeValue()))
        {
            // TRACE begin/end live at the call site. A continuation would skip
            // them, so evaluate the nested call and return its value instead.
            CallSite cs(this, node, childExpected);
            Value *retval = cs.child(0);
            retval = generateCallExtern(
                getCtrlReturn, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())), PaAddr(retval));
            return cs.ret(retval);
        }
        // Else it's a procedure. Generate a tail call to it.
        // Nested AST: CallSite would evaluate the child instead of tail-calling.
        // Arguments are always boxed Datum pointers (the alloca is TyAddr).
        // STOP (no TypeDatum in childExpected) still TCO's so `walk :n-1 stop`
        // does not grow the stack, but demandNothing forbids the callee's
        // output from becoming this procedure's output (.MAYBEOUTPUT does not).
        // allowNothing is STOP and .MAYBEOUTPUT: a missing value is not an error.
        // When this continuation is the result of a nested list (CATCH, REPEAT,
        // RUN, ...), the evaluator runs it as its own frame so the waiting
        // command still sees a throw or OUTPUT.
        Value *childAddr = CoAddr(child.astnodeValue());
        Value *sourceAddr = CoAddr(node.astnodeValue());

        AllocaInst *ary = generateChildrenAlloca(child.astnodeValue(), TypeDatum, DBG_NAME("childAry"));
        Value *demandNothing = CoBool((childExpected & TypeDatum) == 0);
        Value *allowNothing = CoBool((childExpected & TypeNothing) != 0);
        Value *retObj = generateCallExtern(getCtrlContinuation,
                                           PaAddr(scaff_->evaluator_),
                                           PaAddr(childAddr),
                                           PaAddr(ary),
                                           PaInt32(allocaN(ary)),
                                           PaBool(demandNothing),
                                           PaAddr(sourceAddr),
                                           PaBool(allowNothing));
        endAllocaLifetime(ary);
        return retObj;
    }
    // There is no child. Return nothing.
    CallSite cs(this, node, TypeNothing);
    Value *retval = generateVoidRetval(node);
    return cs.ret(generateCallExtern(
        getCtrlReturn, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())), PaAddr(retval)));
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
    CallSite cs(this, node, TypeDatum);
    Value *tag = cs.child(0);
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
    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [tag, list] = cs.children();
    tag = generateWordFromDatum(node.astnodeValue(), tag);
    list = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), list);
    Value *errActStash = generateCallExtern(beginCatch);

    Value *result = generateCallList(list);

    Value *retval = generateCallExtern(endCatch,
                                       PaAddr(scaff_->evaluator_),
                                       PaAddr(CoAddr(node.astnodeValue())),
                                       PaAddr(errActStash),
                                       PaAddr(result),
                                       PaAddr(tag));
    return cs.ret(generateNotFlowControl(retval));
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
    CallSite cs(this, node, TypeDatum);
    const std::vector<Value *> &children = cs.children();
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
    CallSite cs(this, node, TypeNothing);
    return cs.ret(generateCallExtern(getCurrentError, PaAddr(scaff_->evaluator_)));
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
    Q_UNUSED(expected);
    CallSite cs(this, node, TypeNothing);
    Value *retval = generateCallExtern(callPause, PaAddr(scaff_->evaluator_));
    return cs.ret(generateNotFlowControl(retval));
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
    CallSite cs(this, node, TypeDatum);
    Value *output = CoAddr(node.astnodeValue());
    if (cs.count() == 1)
    {
        output = cs.child(0);
    }
    return cs.ret(generateCallExtern(generateContinue, PaAddr(scaff_->evaluator_), PaAddr(output)));
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
    CallSite cs(this, node, TypeDatum);
    Value *instructionlist = cs.child(0);
    Value *result = generateCallList(instructionlist);
    result = generateCallExtern(processRunresult, PaAddr(scaff_->evaluator_), PaAddr(result));
    return cs.ret(generateNotFlowControl(result));
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
    CallSite cs(this, node, TypeDatum);
    Value *thing = cs.child(0);
    return cs.ret(generateCallExtern(stdMark, PaAddr(thing)));
}

/***DOC FOREVER
FOREVER instructionlist

    command.  Runs the "instructionlist" repeatedly, until something
    inside the instructionlist (such as STOP or THROW) makes it stop.

COD***/
// CMD FOREVER 1 1 1 n
Value *Compiler::genForever(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *list = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), cs.child(0));
    return generateRepeatLike(node, list, nullptr);
}

Value *Compiler::generateRepeatLike(const DatumPtr &node, Value *list, Value *count)
{
    const bool hasBound = count != nullptr;

    Value *repcountAddress = generateCallExtern(repcountAddr);
    Value *shadowedRepcount = scaff_->builder_.CreateLoad(TyDouble, repcountAddress, DBG_NAME("shadowedRepcount"));
    scaff_->builder_.CreateStore(CoDouble(1.0), repcountAddress);

    BasicBlock *headerBB = hasBound ? scaff_->createBasicBlock(DBG_NAME("header")) : nullptr;
    BasicBlock *bodyBB = scaff_->createBasicBlock(DBG_NAME("body"));
    BasicBlock *datumCheckBB = scaff_->createBasicBlock(DBG_NAME("datumCheck"));
    BasicBlock *loopNextBB = scaff_->createBasicBlock(DBG_NAME("loopNext"));
    BasicBlock *lastIterBB = hasBound ? scaff_->createBasicBlock(DBG_NAME("lastIter")) : nullptr;
    BasicBlock *noSayErrorBB = scaff_->createBasicBlock(DBG_NAME("noSayError"));
    BasicBlock *bailoutBB = scaff_->createBasicBlock(DBG_NAME("bailout"));
    BasicBlock *exitBB = hasBound ? scaff_->createBasicBlock(DBG_NAME("exit")) : nullptr;

    scaff_->addColdPathBlocks(noSayErrorBB, bailoutBB);
    scaff_->builder_.CreateBr(hasBound ? headerBB : bodyBB);

    Value *repcount = nullptr;
    if (hasBound)
    {
        scaff_->builder_.SetInsertPoint(headerBB);
        repcount = scaff_->builder_.CreateLoad(TyDouble, repcountAddress, DBG_NAME("repcount"));
        Value *shouldContinue = scaff_->builder_.CreateFCmpOLE(repcount, count, DBG_NAME("shouldContinue"));
        scaff_->builder_.CreateCondBr(shouldContinue, bodyBB, exitBB);
    }

    scaff_->builder_.SetInsertPoint(bodyBB);
    Value *result = generateCallList(list);
    BasicBlock *bodyDestBB = scaff_->builder_.GetInsertBlock();
    Value *resultType = generateGetDatumIsa(result);
    Value *flowMask =
        scaff_->builder_.CreateAnd(resultType, CoInt32(Datum::typeFlowControlMask), DBG_NAME("flowControlMask"));
    Value *notFlow = scaff_->builder_.CreateICmpEQ(flowMask, CoInt32(0), DBG_NAME("notFlow"));
    scaff_->builder_.CreateCondBr(notFlow, datumCheckBB, bailoutBB);

    scaff_->builder_.SetInsertPoint(datumCheckBB);
    Value *isDatum = scaff_->builder_.CreateAnd(resultType, CoInt32(Datum::typeDataMask), DBG_NAME("isDatumMask"));
    Value *noDatum = scaff_->builder_.CreateICmpEQ(isDatum, CoInt32(0), DBG_NAME("noDatum"));
    scaff_->builder_.CreateCondBr(noDatum, loopNextBB, hasBound ? lastIterBB : noSayErrorBB);

    scaff_->builder_.SetInsertPoint(loopNextBB);
    if (!hasBound)
        repcount = scaff_->builder_.CreateLoad(TyDouble, repcountAddress, DBG_NAME("repcount"));
    Value *incrRepcount = scaff_->builder_.CreateFAdd(repcount, CoDouble(1.0), DBG_NAME("incrRepcount"));
    scaff_->builder_.CreateStore(incrRepcount, repcountAddress);
    scaff_->builder_.CreateBr(hasBound ? headerBB : bodyBB);

    if (hasBound)
    {
        scaff_->builder_.SetInsertPoint(lastIterBB);
        Value *isLastIter = scaff_->builder_.CreateFCmpOEQ(repcount, count, DBG_NAME("isLastIter"));
        scaff_->builder_.CreateCondBr(isLastIter, exitBB, noSayErrorBB);
    }

    scaff_->builder_.SetInsertPoint(noSayErrorBB);
    Value *errNoSay = generateErrorNoSay(result);
    scaff_->builder_.CreateBr(bailoutBB);

    scaff_->builder_.SetInsertPoint(bailoutBB);
    PHINode *phiError = scaff_->builder_.CreatePHI(TyAddr, 2, DBG_NAME("errVal"));
    phiError->addIncoming(errNoSay, noSayErrorBB);
    phiError->addIncoming(result, bodyDestBB);
    scaff_->builder_.CreateStore(shadowedRepcount, repcountAddress);

    if (!hasBound)
        return generateImmediateReturn(phiError);

    generateReturn(phiError);

    scaff_->builder_.SetInsertPoint(exitBB);
    PHINode *phiNode = scaff_->builder_.CreatePHI(TyAddr, 2, DBG_NAME("retval"));
    phiNode->addIncoming(generateVoidRetval(node), headerBB);
    phiNode->addIncoming(result, lastIterBB);
    scaff_->builder_.CreateStore(shadowedRepcount, repcountAddress);
    return phiNode;
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
    CallSite cs(this, node, TypeBool);
    Value *tf = cs.child(0);
    generateCallExtern(saveTestResult, PaBool(tf));
    return cs.ret(generateVoidRetval(node));
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

    auto generateListFn =
        node.astnodeValue()->inSequenceTail_ ? &Compiler::generateCallListContinuation : &Compiler::generateCallList;

    // 0 = not tested, 3 = tested true, 2 = tested false.
    int8_t runCondition = testForTrue ? 3 : 2;
    int8_t noRunCondition = testForTrue ? 2 : 3;

    CallSite cs(this, node, TypeDatum);
    Value *instructionlist = cs.child(0);
    Value *testResult = generateCallExtern(getTestResult);
    SwitchInst *sw = scaff_->builder_.CreateSwitch(testResult, notTestedBB, 2);
    sw->addCase(CoInt8(runCondition), runListBB);
    sw->addCase(CoInt8(noRunCondition), noRunListBB);

    scaff_->builder_.SetInsertPoint(notTestedBB);
    Value *errVal = generateErrorNoTest(CoAddr(node.astnodeValue()->nodeName_.datumValue()));
    generateReturn(errVal);

    scaff_->builder_.SetInsertPoint(runListBB);
    Value *listRetval = (this->*generateListFn)(instructionlist);
    BasicBlock *listRetvalBB = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.CreateBr(mergeBB);

    scaff_->builder_.SetInsertPoint(noRunListBB);
    Value *noRetval = generateVoidRetval(node);
    scaff_->builder_.CreateBr(mergeBB);

    scaff_->builder_.SetInsertPoint(mergeBB);
    PHINode *retval = scaff_->builder_.CreatePHI(TyAddr, 2, DBG_NAME("retval"));
    retval->addIncoming(listRetval, listRetvalBB);
    retval->addIncoming(noRetval, noRunListBB);
    return cs.ret(generateNotFlowControl(retval));
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
    BasicBlock *namedProcedureBB = scaff_->createBasicBlock(DBG_NAME("namedProcedure"));
    BasicBlock *explicitSlotBB = scaff_->createBasicBlock(DBG_NAME("explicitSlot"));
    BasicBlock *lambdaBB = scaff_->createBasicBlock(DBG_NAME("lambda"));
    BasicBlock *procedureTextBB = scaff_->createBasicBlock(DBG_NAME("procedureText"));
    BasicBlock *mergeBB = scaff_->createBasicBlock(DBG_NAME("merge"));
    BasicBlock *invalidTemplateBB = scaff_->createBasicBlock(DBG_NAME("invalidTemplate"));

    scaff_->addColdPathBlocks(invalidTemplateBB);

    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [aTemplate, inputlist] = cs.children();

    Value *templateForm = nullptr;
    aTemplate = generateValidationDatum(node.astnodeValue(), aTemplate, [this, &templateForm](Value *candidate) {
        templateForm = generateCallExtern(getTemplateForm, PaAddr(scaff_->evaluator_), PaAddr(candidate));
        return scaff_->builder_.CreateICmpNE(templateForm, CoInt8(TemplateFormInvalid), DBG_NAME("templateCond"));
    });

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
    Value *namedProcedureResult =
        generateApplyBuiltCall(node, expected, aTemplate, inputList, buildApplyNamedCall, "buildApplyNamedCall");
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
    Value *procedureTextResult = generateApplyBuiltCall(
        node, expected, aTemplate, inputList, buildApplyProcedureText, "buildApplyProcedureText");
    BasicBlock *procedureTextDestBB = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.CreateBr(mergeBB);

    scaff_->builder_.SetInsertPoint(mergeBB);
    PHINode *retval = scaff_->builder_.CreatePHI(TyAddr, 4, DBG_NAME("retval"));
    retval->addIncoming(namedProcedureResult, namedProcedureDestBB);
    retval->addIncoming(slotResult, explicitSlotDestBB);
    retval->addIncoming(lambdaResult, lambdaDestBB);
    retval->addIncoming(procedureTextResult, procedureTextDestBB);
    return cs.ret(generateNotFlowControl(retval));
}

Value *Compiler::generateApplyBuiltCall(const DatumPtr &node,
                                        TypeMask expected,
                                        Value *aTemplate,
                                        Value *inputList,
                                        addr_t (*builder)(addr_t, addr_t, addr_t, addr_t) noexcept,
                                        const char *builderName)
{
    Value *callAST = generateCallExternTyped(builder,
                                             builderName,
                                             {PaAddr(scaff_->evaluator_),
                                              PaAddr(CoAddr(node.astnodeValue()->nodeName_.datumValue())),
                                              PaAddr(aTemplate),
                                              PaAddr(inputList)});
    callAST = generateNotFlowControl(callAST);
    auto generateListFn =
        node.astnodeValue()->inSequenceTail_ ? &Compiler::generateCallListContinuation : &Compiler::generateCallList;
    return (this->*generateListFn)(callAST);
}

Value *Compiler::generateApplyExplicitSlot(const DatumPtr &node, TypeMask expected, Value *aTemplate, Value *inputList)
{
    generateCallExtern(pushExplicitSlotList, PaAddr(inputList));
    Value *result = generateCallList(aTemplate);
    generateCallExtern(popExplicitSlotList);
    return result;
}

Value *Compiler::generateApplyLambda(const DatumPtr &node, TypeMask expected, Value *aTemplate, Value *inputList)
{
    Value *bodyOrErr = generateCallExtern(beginApplyLambda,
                                          PaAddr(scaff_->evaluator_),
                                          PaAddr(CoAddr(node.astnodeValue()->nodeName_.datumValue())),
                                          PaAddr(aTemplate),
                                          PaAddr(inputList));
    bodyOrErr = generateNotFlowControl(bodyOrErr);
    Value *result = generateCallList(bodyOrErr);
    generateCallExtern(endApplyLambda);
    return result;
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
    CallSite cs(this, node, TypeDatum);
    Value *index = CoAddr(Datum::notADatum());
    if (cs.count() == 1)
        index = cs.child(0);

    Value *result = generateCallExtern(
        getExplicitSlot, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())), PaAddr(index));
    return cs.ret(generateNotFlowControl(result));
}

/***DOC IGNORE
IGNORE value						(library procedure)

    command.  Does nothing.  Used when an expression is evaluated for
    a side effect and its actual value is unimportant.

COD***/

/***LIB IGNORE
;;; -*- logo -*-

to ignore :stuff
end

bury "ignore

BIL***/

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

/***LIB `
;;; -*- logo -*-

to ` :backq.list [:backq.depth 0]
if emptyp :backq.list [op []]
if equalp first :backq.list "` ~
   [op fput "`
            fput  (` first bf :backq.list :backq.depth+1)
                  (` bf bf :backq.list :backq.depth)]
if and wordp first :backq.list equalp first first :backq.list ", ~
   [op backq.unquote (bf first :backq.list) (bf :backq.list) :backq.depth]
if and wordp first :backq.list memberp first first :backq.list [" :] ~
   [op backq.word (first first :backq.list) (bf first :backq.list)
                  (bf :backq.list) :backq.depth]
if wordp first :backq.list ~
   [op fput first :backq.list (` bf :backq.list :backq.depth)]
op fput (` first :backq.list :backq.depth) (` bf :backq.list :backq.depth)
end

to backq.word :backq.symbol :backq.word :backq.rest :backq.depth
if emptyp :backq.word ~
   [output fput :backq.symbol (` :backq.rest :backq.depth)]
if not equalp first :backq.word ", ~
   [output fput (word :backq.symbol :backq.word)
                (` :backq.rest :backq.depth)]
localmake "result backq.unquote (bf :backq.word) :backq.rest :backq.depth
if wordp :result [output word :backq.symbol :result]
output fput (word :backq.symbol first :result) bf :result
end

to backq.unquote :unquote.symbol :unquote.rest :unquote.depth
localmake "unquote.splicing "false
if not emptyp :unquote.symbol [
   if equalp first :unquote.symbol "@ [
      make "unquote.splicing "true
      make "unquote.symbol butfirst :unquote.symbol
]]
if :unquote.depth=0 [
  if emptyp :unquote.symbol
     [output backq.combine run first :unquote.rest
                           (` bf :unquote.rest :unquote.depth)]
  output backq.combine run :unquote.symbol (` :unquote.rest :unquote.depth)
]
if emptyp :unquote.symbol ~
   [output fput (ifelse :unquote.splicing [",@] [",])
                fput (` first :unquote.rest :unquote.depth-1)
                     (` bf :unquote.rest :unquote.depth)]
if backq.all.commas :unquote.symbol ~
   [output fput (ifelse :unquote.splicing [",@] [",])
                fput (` (list :unquote.symbol first :unquote.rest)
                        :unquote.depth-1)
                     (` bf :unquote.rest :unquote.depth)]
output fput (ifelse :unquote.splicing [",@] [",]) ~
            fput (` (list :unquote.symbol) :unquote.depth-1) ~
                 (` :unquote.rest :unquote.depth)
end

to backq.combine :this :those
output ifelse :unquote.splicing [se :this :those] [fput :this :those]
end

to backq.all.commas :word
if emptyp :word [output "true]
if equalp first :word ", ~
   [if emptyp butfirst :word [output "true]
    if equalp first butfirst :word "@ [output backq.all.commas bf bf :word]
    output backq.all.commas butfirst :word]
output "false
end

bury [` backq.word backq.unquote backq.combine backq.all.commas]

BIL***/

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

/***LIB FOR
;;; -*- logo -*-

.macro for :for.values :for.instr ~
   [:for.var first :for.values] ~
   [:for.initial run first bf :for.values] ~
   [:for.final run first bf bf :for.values] ~
   [:for.step forstep] ~
   [:for.tester (ifelse :for.step < 0 ~
                        [[:for.initial < :for.final]] ~
                        [[:for.initial > :for.final]])]
local :for.var
catch "for.catchtag [op for.done runresult [forloop :for.initial]]
op []
end

to forloop :for.initial
make :for.var :for.initial
if run :for.tester [throw "for.catchtag]
run :for.instr
.maybeoutput forloop (:for.initial + :for.step)
end

to for.done :for.result
if emptyp :for.result [op [stop]]
op (list "output "first (list first :for.result))
end

to forstep
if equalp count :for.values 4 [op run last :for.values]
op ifelse :for.initial > :for.final [-1] [1]
end

bury [for forstep forloop for.done]

BIL***/

/***DOC DO.WHILE
DO.WHILE instructionlist tfexpression			(library procedure)

    command.  Repeatedly evaluates the "instructionlist" as long as the
    evaluated "tfexpression" remains TRUE.  Evaluates the first input
    first, so the "instructionlist" is always run at least once.  The
    "tfexpression" must be an expressionlist whose value when evaluated
    is TRUE or FALSE.

COD***/

/***LIB DO.WHILE
;;; -*- logo -*-

.macro do.while :while.instr :while.cond
op se :while.instr (list "while :while.cond :while.instr)
end

bury "do.while

BIL***/

/***DOC WHILE
WHILE tfexpression instructionlist			(library procedure)

    command.  Repeatedly evaluates the "instructionlist" as long as the
    evaluated "tfexpression" remains TRUE.  Evaluates the first input
    first, so the "instructionlist" may never be run at all.  The
    "tfexpression" must be an expressionlist whose value when evaluated
    is TRUE or FALSE.

COD***/

/***LIB WHILE
;;; -*- logo -*-

.macro while :while.cond :while.instr
if not run :while.cond [op []]
op se :while.instr (list "while :while.cond :while.instr)
end

bury "while

BIL***/

/***DOC DO.UNTIL
DO.UNTIL instructionlist tfexpression			(library procedure)

    command.  Repeatedly evaluates the "instructionlist" as long as the
    evaluated "tfexpression" remains FALSE.  Evaluates the first input
    first, so the "instructionlist" is always run at least once.  The
    "tfexpression" must be an expressionlist whose value when evaluated
    is TRUE or FALSE.

COD***/

/***LIB DO.UNTIL
;;; -*- logo -*-

.macro do.until :until.instr :until.cond
op se :until.instr (list "until :until.cond :until.instr)
end

bury "do.until

BIL***/

/***DOC UNTIL
UNTIL tfexpression instructionlist			(library procedure)

    command.  Repeatedly evaluates the "instructionlist" as long as the
    evaluated "tfexpression" remains FALSE.  Evaluates the first input
    first, so the "instructionlist" may never be run at all.  The
    "tfexpression" must be an expressionlist whose value when evaluated
    is TRUE or FALSE.

COD***/

/***LIB UNTIL
;;; -*- logo -*-

.macro until :until.cond :until.instr
if run :until.cond [op []]
op se :until.instr (list "until :until.cond :until.instr)
end

bury "until

BIL***/

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

/***LIB CASE
;;; -*- logo -*-

.macro case :case.value :case.clauses [:caseignoredp "true]
catch "case.error [output case.helper :case.value :case.clauses]
(throw "error [Empty CASE clause])
end

to case.helper :case.value :case.clauses
if emptyp :case.clauses [output []]
if emptyp first :case.clauses [throw "case.error]
if or equalp first first :case.clauses "else ~
      memberp :case.value first first :case.clauses ~
   [output butfirst first :case.clauses]
output case.helper :case.value butfirst :case.clauses
end

bury [case case.helper]

BIL***/

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

/***LIB COND
;;; -*- logo -*-

.macro cond :cond.clauses
localmake "cond.result cond.helper :cond.clauses
if equalp first :cond.result "error [(throw "error last :cond.result)]
output last :cond.result
end

to cond.helper :cond.clauses
if emptyp :cond.clauses [output [[] []]]
if emptyp first :cond.clauses [output [error [Empty COND clause]]]
if equalp first first :cond.clauses "else ~
   [output list [] butfirst first :cond.clauses]
ignore error
catch "error [localmake "cond.result run first first :cond.clauses]
localmake "cond.error error
if not emptyp :cond.error [output list "error item 2 :cond.error]
if not memberp :cond.result [true false] ~
   [output list "error fput :cond.result [not TRUE or FALSE]]
if :cond.result [output list [] butfirst first :cond.clauses]
output cond.helper butfirst :cond.clauses
end

bury [cond cond.helper]

BIL***/

/***DOC INVOKE
INVOKE template input					(library procedure)
(INVOKE template input1 input2 ...)

    command or operation.  Exactly like APPLY except that the inputs
    are provided as separate expressions rather than in a list.

COD***/

/***LIB INVOKE
;;; -*- logo -*-

to invoke :invoked.function [:invoke.inputs] 2
.maybeoutput apply :invoked.function :invoke.inputs
end

bury "invoke

BIL***/

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

/***LIB FOREACH
;;; -*- logo -*-

.macro foreach [:foreach.inputs] 2
catch "foreach.catchtag ~
      [op foreach.done runresult ~
        [foreach1 bl :foreach.inputs last :foreach.inputs 1]]
op []
end

to foreach1 :template.lists :foreach.template :template.number
if emptyp first :template.lists [throw "foreach.catchtag]
apply :foreach.template firsts :template.lists
.maybeoutput foreach1 bfs :template.lists :foreach.template :template.number+1
end

to foreach.done :foreach.result
if emptyp :foreach.result [op [stop]]
op (list "output "first (list first :foreach.result))
end

to ?rest [:which 1]
output bf item :which :template.lists
end

bury [foreach foreach1 foreach.done ?rest]

BIL***/

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

/***LIB MAP
;;; -*- logo -*-

to map :map.template [:template.lists] 2
op map1 :template.lists 1
end

to map1 :template.lists :template.number
if emptyp first :template.lists [output first :template.lists]
output combine (apply :map.template firsts :template.lists) ~
               (map1 bfs :template.lists :template.number+1)
end

to ?rest [:which 1]
output bf item :which :template.lists
end

bury [map map1 ?rest]

BIL***/

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

/***LIB MAP.SE
;;; -*- logo -*-

to map.se :map.se.template [:template.lists] 2
op map.se1 :template.lists 1
end

to map.se1 :template.lists :template.number
if emptyp first :template.lists [output []]
output sentence (apply :map.se.template firsts :template.lists) ~
                (map.se1 bfs :template.lists :template.number+1)
end

to ?rest [:which 1]
output bf item :which :template.lists
end

bury [map.se map.se1 ?rest]

BIL***/

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

/***LIB FILTER
;;; -*- logo -*-

to filter :filter.template :template.list [:template.number 1] ~
          [:template.lists (list :template.list)]
if emptyp :template.list [op :template.list]
if apply :filter.template (list first :template.list) ~
   [op combine (first :template.list) ~
               (filter :filter.template bf :template.list :template.number+1)]
op (filter :filter.template bf :template.list :template.number+1)
end

to ?rest [:which 1]
output bf item :which :template.lists
end

bury [filter ?rest]

BIL***/

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

/***LIB FIND
;;; -*- logo -*-

to find :find.template :template.list [:template.number 1] ~
        [:template.lists (list :template.list)]
if emptyp :template.list [op []]
if apply :find.template (list first :template.list) [op first :template.list]
op (find :find.template bf :template.list :template.number+1)
end

to ?rest [:which 1]
output bf item :which :template.lists
end

bury [find ?rest]

BIL***/

/***LIB REDUCE
;;; -*- logo -*-

to reduce :reduce.function :reduce.list
if emptyp bf :reduce.list [op first :reduce.list]
op apply :reduce.function (list (first :reduce.list) ~
                                (reduce :reduce.function bf :reduce.list))
end

bury "reduce

BIL***/

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

/***LIB CROSSMAP
;;; -*- logo -*-

to crossmap :cm.template [:cm.lists] 2
if emptyp bf :cm.lists [op cm1 first :cm.lists 1 []]
op cm1 :cm.lists 1 []
end

to cm1 :cm.lists :cm.level :template.vars
if emptyp :cm.lists [op (list apply :cm.template :template.vars)]
op cm2 first :cm.lists
end

to cm2 :cm.thislist
if emptyp :cm.thislist [op []]
local :cm.level
make :cm.level first :cm.thislist
op se (cm1 bf :cm.lists :cm.level+1 lput first :cm.thislist :template.vars) ~
      (cm2 bf :cm.thislist)
end

bury [crossmap cm1 cm2]

BIL***/

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

/***LIB CASCADE
;;; -*- logo -*-

to cascade :cascade.limit [:cascade.inputs] 3
if numberp :cascade.limit ~
   [if lessp :cascade.limit 0 ~
       [(throw "error (se [cascade doesn't like] :cascade.limit [as input]))] ~
    make "cascade.limit `[greaterp :template.number ,[int :cascade.limit]]]
local [cascade.templates template.vars cascade.final]
make "cascade.templates []
make "template.vars []
make "cascade.final [?1]
cascade.setup :cascade.inputs
op cascade1 1 :template.vars
end

to cascade.setup :inputs
if emptyp :inputs [stop]
if emptyp bf :inputs [make "cascade.final first :inputs stop]
make "cascade.templates lput first :inputs :cascade.templates
make "template.vars lput first bf :inputs :template.vars
cascade.setup bf bf :inputs
end

to cascade1 :template.number :template.vars
if apply :cascade.limit :template.vars [op apply :cascade.final :template.vars]
op cascade1 (:template.number+1) (cascade.eval :cascade.templates)
end

to cascade.eval :cascade.templates
if emptyp :cascade.templates [op []]
op fput (apply first :cascade.templates :template.vars) ~
        (cascade.eval bf :cascade.templates)
end

bury [cascade cascade.setup cascade1 cascade.eval]

BIL***/

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

/***LIB TRANSFER
;;; -*- logo -*-

to transfer :transfer.limit :transfer.template :transfer.init
output cascade.2 (ifelse emptyp :transfer.limit ~
                         [[emptyp ?2]] ~
                         [list "transfer.end.test :transfer.limit]) ~
                 :transfer.template [] [butfirst ?2] :transfer.init
end

to transfer.end.test :the.condition.expression
if emptyp ?2 [output "true]
output run :the.condition.expression
end

to ?in
output first ?2
end

to ?out
output ?1
end

bury [transfer transfer.end.test ?in ?out]

BIL***/

/***LIB #
;;; -*- logo -*-

to #
if not namep "template.number [op repcount]
op :template.number
end

bury "#

BIL***/

/***LIB CASCADE.2
;;; -*- logo -*-

to cascade.2 [:cascade2.inputs] 5
op apply "cascade :cascade2.inputs
end

bury "cascade.2

BIL***/
