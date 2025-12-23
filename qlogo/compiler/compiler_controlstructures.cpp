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

#include "compiler_private.h"
#include "astnode.h"
#include "compiler.h"
#include "sharedconstants.h"
#include "kernel.h"
#include "flowcontrol.h"
#include "datum_types.h"

#include <QObject>

/// @brief return the address of the repcount variable.
EXPORTC addr_t repcountAddr(void)
{
    void *retval = &Config::get().mainKernel()->callStack.repcount;
    return (addr_t)retval;
}

using namespace llvm;
using namespace llvm::orc;

// This is not a user command.
Value *Compiler::genNoop(const DatumPtr &node, RequestReturnType returnType)
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
Value *Compiler::genIfelse(const DatumPtr &node, RequestReturnType returnType)
{
    std::vector<RequestReturnType> returnTypeAry = {RequestReturnDB, returnType};
    if (node.astnodeValue()->countOfChildren() == 3)
    {
        returnTypeAry.push_back(returnType);
    }
    std::vector<Value *> children = generateChildren(node.astnodeValue(), returnTypeAry);

    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    BasicBlock *thenBB = BasicBlock::Create(*scaff->theContext, "then", theFunction);
    BasicBlock *elseBB = BasicBlock::Create(*scaff->theContext, "else", theFunction);
    BasicBlock *mergeBB = BasicBlock::Create(*scaff->theContext, "ifcont", theFunction);


    Value *cond = children[0];
    Value *ift;
    Value *iff;

    // If input is a Datum type (can be word or list)
    if (cond->getType()->isPointerTy())
    {
        cond = generateListExecIfList(node.astnodeValue(), cond);
        cond = generateBoolFromDatum(node.astnodeValue(), cond);
        // bool continues.
    }


    cond = scaff->builder.CreateICmpEQ(cond, CoBool(1), "ifcond");
    scaff->builder.CreateCondBr(cond, thenBB, elseBB);

    // Emit then value.
    scaff->builder.SetInsertPoint(thenBB);
    ift = generateCallList(children[1], returnType);
    scaff->builder.CreateBr(mergeBB);
    // Codegen of 'Then' can change the current block, update thenBB for the PHI.
    thenBB = scaff->builder.GetInsertBlock();

    // Emit else block.
    scaff->builder.SetInsertPoint(elseBB);

    // What we do here depends on if this is an IF or IFELSE
    if (children.size() == 3)
    {
        iff = generateCallList(children[2], returnType);
    } else {
        iff = CoAddr(node.astnodeValue());
    }

    scaff->builder.CreateBr(mergeBB);
    // Codegen of 'Else' can change the current block, update elseBB for the PHI.
    elseBB = scaff->builder.GetInsertBlock();

    // Emit merge block.
    scaff->builder.SetInsertPoint(mergeBB);
    PHINode *phiNode = scaff->builder.CreatePHI(TyAddr, 2, "iftmp");

    phiNode->addIncoming(ift, thenBB);
    phiNode->addIncoming(iff, elseBB);
    return phiNode;
}

/***DOC RUN
RUN instructionlist

command or operation.  Runs the Logo instructions in the input
list; outputs if the list contains an expression that outputs.

COD***/
// CMD RUN 1 1 1 dn
Value *Compiler::genRun(const DatumPtr &node, RequestReturnType returnType)
{
    Value *list = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    return generateCallList(list, returnType);
}

/***DOC REPEAT
REPEAT num instructionlist

command.  Runs the "instructionlist" repeatedly, "num" times.

COD***/
// CMD REPEAT 2 2 2 dn
Value *Compiler::genRepeat(const DatumPtr &node, RequestReturnType returnType)
{
    Value *count = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *list = generateChild(node.astnodeValue(), 1, RequestReturnDatum);

    auto countValidator = [this](Value *candidate) {
        BasicBlock *intCheckBB = scaff->builder.GetInsertBlock();
        Function *theFunction = intCheckBB->getParent();

        BasicBlock *negativeCheckBB = BasicBlock::Create(*scaff->theContext, "negativeCheck", theFunction);
        BasicBlock *mergeBB = BasicBlock::Create(*scaff->theContext, "merge", theFunction);

        Value *candidateInt = scaff->builder.CreateFPToSI(candidate, TyInt32, "FpToInt");
        Value *candidateCheck = scaff->builder.CreateSIToFP(candidateInt, TyDouble, "FpToIntCheck");
        Value *intCheckCond = scaff->builder.CreateFCmpOEQ(candidate, candidateCheck, "isIntTest");
        scaff->builder.CreateCondBr(intCheckCond, negativeCheckBB, mergeBB);

        scaff->builder.SetInsertPoint(negativeCheckBB);
        Value *isNegativeCond = scaff->builder.CreateFCmpOGE(candidate, CoDouble(0.0), "isNotNegative");
        scaff->builder.CreateBr(mergeBB);

        scaff->builder.SetInsertPoint(mergeBB);
        PHINode *phiNode = scaff->builder.CreatePHI(isNegativeCond->getType(), 2, "isNotNegativeIntResult");
        phiNode->addIncoming(intCheckCond, intCheckBB);
        phiNode->addIncoming(isNegativeCond, negativeCheckBB);
        return phiNode;
    };
    count = generateValidationDouble(node.astnodeValue(), count, countValidator);
    list = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), list);

    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();

    // Get the current value of repcount since we are shadowing it.
    Value *repcountAddr = generateCallExtern(TyAddr, "repcountAddr", {});
    Value *shadowedRepcount = scaff->builder.CreateLoad(TyDouble, repcountAddr, "shadowedRepcount");

    scaff->builder.CreateStore(CoDouble(1.0), repcountAddr);

    BasicBlock *loopBB = BasicBlock::Create(*scaff->theContext, "loop", theFunction);
    BasicBlock *whileBB = BasicBlock::Create(*scaff->theContext, "while", theFunction);
    BasicBlock *datumCheckBB = BasicBlock::Create(*scaff->theContext, "datumCheck", theFunction);
    BasicBlock *loopNextBB = BasicBlock::Create(*scaff->theContext, "loopNext", theFunction);
    BasicBlock *datumIsLastBB = BasicBlock::Create(*scaff->theContext, "datumIsLast", theFunction);
    BasicBlock *noSayErrorBB = BasicBlock::Create(*scaff->theContext, "noSayError", theFunction);
    BasicBlock *bailoutBB = BasicBlock::Create(*scaff->theContext, "bailout", theFunction);
    BasicBlock *exitBB = BasicBlock::Create(*scaff->theContext, "exit", theFunction);
    scaff->builder.CreateBr(loopBB);

    scaff->builder.SetInsertPoint(loopBB);
    Value *repcount = scaff->builder.CreateLoad(TyDouble, repcountAddr, "repcount");
    Value *isLast = scaff->builder.CreateFCmpULE(repcount, count, "isLast");
    scaff->builder.CreateCondBr(isLast, whileBB, exitBB);

    scaff->builder.SetInsertPoint(whileBB);
    Value *result = generateCallList(list, RequestReturnDatum);
    Value *resultType = generateGetDatumIsa(result);
    Value *mask = scaff->builder.CreateAnd(resultType, CoInt32(Datum::typeFlowControlMask), "flowControlMask");
    Value *cond = scaff->builder.CreateICmpEQ(mask, CoInt32(0), "flowControlCond");
    scaff->builder.CreateCondBr(cond, datumCheckBB, bailoutBB);

    scaff->builder.SetInsertPoint(datumCheckBB);
    Value *isDatum = scaff->builder.CreateAnd(resultType, CoInt32(Datum::typeDataMask), "isDatumMask");
    Value *isDatumCond = scaff->builder.CreateICmpEQ(isDatum, CoInt32(0), "isDatumCond");
    scaff->builder.CreateCondBr(isDatumCond, loopNextBB, datumIsLastBB);

    scaff->builder.SetInsertPoint(loopNextBB);
    Value *incrRepcount = scaff->builder.CreateFAdd(repcount, CoDouble(1.0), "incrRepcount");
    scaff->builder.CreateStore(incrRepcount, repcountAddr);
    scaff->builder.CreateBr(loopBB);

    scaff->builder.SetInsertPoint(datumIsLastBB);
    Value *isLastCount = scaff->builder.CreateFCmpUEQ(repcount, count, "isLastCount");
    scaff->builder.CreateCondBr(isLastCount, exitBB, noSayErrorBB);

    scaff->builder.SetInsertPoint(noSayErrorBB);
    Value *errNoSay = generateErrorNoSay(result);
    scaff->builder.CreateBr(bailoutBB);

    scaff->builder.SetInsertPoint(bailoutBB);
    PHINode *phiError = scaff->builder.CreatePHI(TyAddr, 2, "errVal");
    phiError->addIncoming(errNoSay, noSayErrorBB);
    phiError->addIncoming(result, whileBB);
    scaff->builder.CreateStore(shadowedRepcount, repcountAddr);
    scaff->builder.CreateRet(phiError);

    scaff->builder.SetInsertPoint(exitBB);
    PHINode *phiNode = scaff->builder.CreatePHI(TyAddr, 2, "retval");
    phiNode->addIncoming(generateVoidRetval(node), loopBB);
    phiNode->addIncoming(result, datumIsLastBB);
    scaff->builder.CreateStore(shadowedRepcount, repcountAddr);
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
Value *Compiler::genRepcount(const DatumPtr &node, RequestReturnType returnType)
{
    Value *repcountAddr = generateCallExtern(TyAddr, "repcountAddr", {});
    Value *repcount = scaff->builder.CreateLoad(TyDouble, repcountAddr, "repcount");
    return repcount;
}

/***DOC BYE
BYE

 command.  Exits from Logo.

COD***/
// CMD BYE 0 0 0 n
Value *Compiler::genBye(const DatumPtr &node, RequestReturnType returnType)
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
Value *Compiler::genOutput(const DatumPtr &node, RequestReturnType returnType)
{
    return generateProcedureExit(node, returnType, RequestReturnDatum);
}

/***DOC STOP
STOP

    command.  Ends the running of the procedure in which it appears.
    Control is returned to the context in which that procedure was
    invoked.  The stopped procedure does not output a value.

COD***/
// CMD STOP 0 0 1 n
Value *Compiler::genStop(const DatumPtr &node, RequestReturnType returnType)
{
    return generateProcedureExit(node, returnType, RequestReturnNothing);
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
Value *Compiler::genMaybeoutput(const DatumPtr &node, RequestReturnType returnType)
{
    return generateProcedureExit(node, returnType, RequestReturnDN);
}

Value *Compiler::generateProcedureExit(const DatumPtr &node, RequestReturnType returnType, RequestReturnType paramRequestType)
{
    // If the parser attached an output value to the node, then we need to process that.
    if (node.astnodeValue()->countOfChildren() > 0) {
        DatumPtr child = node.astnodeValue()->childAtIndex(0);
        DatumPtr proc = child.astnodeValue()->procedure;
        if (proc.isNothing()) {
            // It's a primitive, not a procedure. Generate a call to it.
            // Then generate a return of the value.
            Value *retval = generateChild(node.astnodeValue(), child, paramRequestType);

            retval = generateCallExtern(TyAddr, "getCtrlReturn", {PaAddr(evaluator), PaAddr(CoAddr(node.astnodeValue())), PaAddr(retval)});
            return retval;
        }
        // Else it's a procedure. Generate a tail call to it.
        Value *childAddr = CoAddr(child.astnodeValue());

        // TODO: Instead of RequestReturnDatum, should we use paramRequestType?
        AllocaInst *ary = generateChildrenAlloca(child.astnodeValue(), RequestReturnDatum, "childAry");
        Value *retObj = generateCallExtern(TyAddr, "getCtrlContinuation", {PaAddr(evaluator), PaAddr(childAddr), PaAddr(ary), PaInt32(ary->getArraySize())});
        return retObj;
    }
    // There is no child. Return nothing.
    Value *retval = generateVoidRetval(node);
    return generateCallExtern(TyAddr, "getCtrlReturn", {PaAddr(evaluator), PaAddr(CoAddr(node.astnodeValue())), PaAddr(retval)});
}


/***DOC TAG
TAG quoted.word

    command.  Does nothing.  The input must be a literal word following
    a quotation mark ("), not the result of a computation.  Tags are
    used by the GOTO command.

COD***/
// CMD TAG 1 1 1 n
Value *Compiler::genTag(const DatumPtr &node, RequestReturnType returnType)
{
    // Do nothing. We need to generate something in case this is the only
    // ASTNode in the block.
    // Note that if the input is not a literal word following a quotation mark,
    // no error is generated.
    return generateVoidRetval(node);
}

/***DOC GOTO
GOTO word

    command.  Looks for a TAG command with the same input in the same
    procedure, and continues running the procedure from the location of
    that TAG.  It is meaningless to use GOTO outside of a procedure.

COD***/
// CMD GOTO 1 1 1 n
Value *Compiler::genGoto(const DatumPtr &node, RequestReturnType returnType)
{
    Value *nodeAddr = CoAddr(node.astnodeValue());
    Value *tag = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    tag = generateWordFromDatum(node.astnodeValue(), tag);
    Value *retObj = generateCallExtern(TyAddr, "getCtrlGoto", {PaAddr(evaluator), PaAddr(nodeAddr), PaAddr(tag)});
    return retObj;
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
Value *Compiler::genCatch(const DatumPtr &node, RequestReturnType returnType)
{
    Value *tag = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *instructionlist = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    generateWordFromDatum(node.astnodeValue(), tag);
    Value *errActStash = generateCallExtern(TyAddr, "beginCatch", {PaAddr(evaluator)});

    Value *result = generateCallList(instructionlist, returnType);

    Value *retval = generateCallExtern(TyAddr, "endCatch", {PaAddr(evaluator),
                                                                 PaAddr(CoAddr(node.astnodeValue())),
                                                                 PaAddr(errActStash),
                                                                 PaAddr(result),
                                                                 PaAddr(tag)});
    return retval;
}

EXPORTC addr_t beginCatch(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    Word* erractWord = reinterpret_cast<Word *>(Config::get().mainKernel()->specialVar(SpecialNames::ERRACT));
    Datum* erractValue = Config::get().mainKernel()->callStack.datumForName(erractWord->toString(Datum::ToStringFlags_Key)).datumValue();

    // Save the erract value.
    if (erractValue->isa != Datum::typeNothing) {
        erractValue->retainCount++;
        Config::get().mainKernel()->callStack.setDatumForName(nothing(), erractWord->toString(Datum::ToStringFlags_Key));
    }
    return reinterpret_cast<addr_t>(erractValue);
}

EXPORTC addr_t endCatch(addr_t eAddr, addr_t nodeAddr, addr_t errActAddr, addr_t resultAddr, addr_t tagAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    Word* erractWord = reinterpret_cast<Word *>(Config::get().mainKernel()->specialVar(SpecialNames::ERRACT));
    auto *erractValue = reinterpret_cast<Datum *>(errActAddr);
    auto *result = reinterpret_cast<Datum *>(resultAddr);
    Word *tag = reinterpret_cast<Word *>(tagAddr);

    // Restore the erract value.
    if (erractValue->isa != Datum::typeNothing) {
        DatumPtr erractValuePtr = DatumPtr(erractValue);
        Config::get().mainKernel()->callStack.setDatumForName(erractValuePtr, erractWord->toString(Datum::ToStringFlags_Key));
        erractValue->retainCount--;
    }

    if (result->isa == Datum::typeError) {
        auto *err = reinterpret_cast<FCError *>(result);
        QString tagStr = tag->toString(Datum::ToStringFlags_Key);

        if ((tagStr == QObject::tr("ERROR")) 
        && ((err->code == ErrCode::ERR_NO_CATCH)
        && (err->tag().toString(Datum::ToStringFlags_Key) == QObject::tr("ERROR"))
        || (err->code != ErrCode::ERR_NO_CATCH))) {
            e->watch(err);
            return nodeAddr;
        } else if ((err->code == ErrCode::ERR_NO_CATCH)
        && (err->tag().toString(Datum::ToStringFlags_Key) == tagStr)) {
            e->watch(err);
            auto retval = reinterpret_cast<addr_t>(err->output().datumValue());
            Config::get().mainKernel()->currentError = nothing();
            return retval;
        }
        return resultAddr;
    }
    
    return reinterpret_cast<addr_t>(result);
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
Value *Compiler::genThrow(const DatumPtr &node, RequestReturnType returnType)
{
    std::vector<Value *> children = generateChildren(node.astnodeValue(), RequestReturnDatum);
    Value *tag = generateWordFromDatum(node.astnodeValue(), children[0]);
    Value *output = (children.size() == 1) ? CoAddr(Datum::getInstance()) : children[1];
    Value *errObj = generateCallExtern(TyAddr, "getErrorCustom", {PaAddr(evaluator), PaAddr(tag), PaAddr(output)});
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
Value *Compiler::genError(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, "getCurrentError", {PaAddr(evaluator)});
}

EXPORTC addr_t getCurrentError(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr errPtr = Config::get().mainKernel()->currentError;

    ListBuilder retvalBuilder;
    if ( ! errPtr.isNothing()) {
        auto *err = reinterpret_cast<FCError *>(errPtr.datumValue());
        retvalBuilder.append(DatumPtr(err->code));
        retvalBuilder.append(err->message());
        retvalBuilder.append(err->procedure());
        retvalBuilder.append(err->line());
    }
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
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
Value *Compiler::genPause(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, "callPause", {PaAddr(evaluator)});
}

EXPORTC addr_t callPause(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *retval = Config::get().mainKernel()->pause().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
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
Value *Compiler::genContinue(const DatumPtr &node, RequestReturnType returnType)
{
    Value *output = CoAddr(node.astnodeValue());
    if (node.astnodeValue()->countOfChildren() == 1) {
        output = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    }
    return generateCallExtern(TyAddr, "generateContinue", {PaAddr(evaluator), PaAddr(output)});
}

EXPORTC addr_t generateContinue(addr_t eAddr, addr_t outputAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *output = reinterpret_cast<Datum *>(outputAddr);

    FCError *err = FCError::custom(DatumPtr(QObject::tr("PAUSE")), nothing(), DatumPtr(output));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
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
Value *Compiler::genRunresult(const DatumPtr &node, RequestReturnType returnType)
{
    Value *instructionlist = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *result = generateCallList(instructionlist, RequestReturnDN);
    return generateCallExtern(TyAddr, "processRunresult", {PaAddr(evaluator), PaAddr(result)});
}

EXPORTC addr_t processRunresult(addr_t eAddr, addr_t resultAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *result = reinterpret_cast<Datum *>(resultAddr);
    Datum *retval;

    if ((result->isa & Datum::typeDataMask) != 0) {
        retval = new List(result, EmptyList::instance());
    }
    else if ((result->isa & Datum::typeUnboundMask) != 0) {
        retval = EmptyList::instance();
    }
    else {
        // Pass through whatever we got because it's not good.
        return resultAddr;
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}


/***DOC FOREVER
FOREVER instructionlist

    command.  Runs the "instructionlist" repeatedly, until something
    inside the instructionlist (such as STOP or THROW) makes it stop.

COD***/
// CMD FOREVER 1 1 1 n
Value *Compiler::genForever(const DatumPtr &node, RequestReturnType returnType)
{
    Value *list = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    list = generateFromDatum(Datum::typeWordOrListMask, node.astnodeValue(), list);

    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();

    // Get the current value of repcount since we are shadowing it.
    Value *repcountAddr = generateCallExtern(TyAddr, "repcountAddr", {});
    Value *shadowedRepcount = scaff->builder.CreateLoad(TyDouble, repcountAddr, "shadowedRepcount");

    scaff->builder.CreateStore(CoDouble(1.0), repcountAddr);

    BasicBlock *whileBB = BasicBlock::Create(*scaff->theContext, "while", theFunction);
    BasicBlock *datumCheckBB = BasicBlock::Create(*scaff->theContext, "datumCheck", theFunction);
    BasicBlock *loopNextBB = BasicBlock::Create(*scaff->theContext, "loopNext", theFunction);
    BasicBlock *noSayErrorBB = BasicBlock::Create(*scaff->theContext, "noSayError", theFunction);
    BasicBlock *bailoutBB = BasicBlock::Create(*scaff->theContext, "bailout", theFunction);
    BasicBlock *throwawayBB = BasicBlock::Create(*scaff->theContext, "throwaway", theFunction);
    scaff->builder.CreateBr(whileBB);

    scaff->builder.SetInsertPoint(whileBB);
    Value *result = generateCallList(list, RequestReturnDatum);
    Value *resultType = generateGetDatumIsa(result);
    Value *mask = scaff->builder.CreateAnd(resultType, CoInt32(Datum::typeFlowControlMask), "flowControlMask");
    Value *cond = scaff->builder.CreateICmpEQ(mask, CoInt32(0), "flowControlCond");
    scaff->builder.CreateCondBr(cond, datumCheckBB, bailoutBB);

    scaff->builder.SetInsertPoint(datumCheckBB);
    Value *isDatum = scaff->builder.CreateAnd(resultType, CoInt32(Datum::typeDataMask), "isDatumMask");
    Value *isDatumCond = scaff->builder.CreateICmpEQ(isDatum, CoInt32(0), "isDatumCond");
    scaff->builder.CreateCondBr(isDatumCond, loopNextBB, noSayErrorBB);

    scaff->builder.SetInsertPoint(loopNextBB);
    Value *repcount = scaff->builder.CreateLoad(TyDouble, repcountAddr, "repcount");
    Value *incrRepcount = scaff->builder.CreateFAdd(repcount, CoDouble(1.0), "incrRepcount");
    scaff->builder.CreateStore(incrRepcount, repcountAddr);
    scaff->builder.CreateBr(whileBB);

    scaff->builder.SetInsertPoint(noSayErrorBB);
    Value *errNoSay = generateErrorNoSay(result);
    scaff->builder.CreateBr(bailoutBB);

    scaff->builder.SetInsertPoint(bailoutBB);
    PHINode *phiError = scaff->builder.CreatePHI(TyAddr, 2, "errVal");
    phiError->addIncoming(errNoSay, noSayErrorBB);
    phiError->addIncoming(result, whileBB);
    scaff->builder.CreateStore(shadowedRepcount, repcountAddr);
    scaff->builder.CreateRet(phiError);

    // We will never reach here, but the compiler requires a current block and a return value.
    scaff->builder.SetInsertPoint(throwawayBB);
    return generateVoidRetval(node);
}



/***DOC TEST
TEST tf

    command.  Remembers its input, which must be TRUE or FALSE, for use
    by later IFTRUE or IFFALSE instructions.  The effect of TEST is local
    to the procedure in which it is used; any corresponding IFTRUE or
    IFFALSE must be in the same procedure or a subprocedure.

COD***/
// CMD TEST 1 1 1 n
Value *Compiler::genTest(const DatumPtr &node, RequestReturnType returnType)
{
    Value *tf = generateChild(node.astnodeValue(), 0, RequestReturnBool);
    generateCallExtern(TyVoid, "saveTestResult", {PaAddr(evaluator), PaBool(tf)});
    return generateVoidRetval(node);
}


EXPORTC void saveTestResult(addr_t eAddr, bool tf)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainKernel()->callStack.setTest(tf);
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
Value *Compiler::generateIftrue(const DatumPtr &node, RequestReturnType returnType)
{
    return generateIftruefalse(node, returnType, true);
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
Value *Compiler::generateIffalse(const DatumPtr &node, RequestReturnType returnType)
{
    return generateIftruefalse(node, returnType, false);
}

Value *Compiler::generateIftruefalse(const DatumPtr &node, RequestReturnType returnType, bool testForTrue)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    BasicBlock *notTestedBB = BasicBlock::Create(*scaff->theContext, "notTested", theFunction);
    BasicBlock *isTestedBB = BasicBlock::Create(*scaff->theContext, "isTested", theFunction);
    BasicBlock *runListBB = BasicBlock::Create(*scaff->theContext, "runList", theFunction);
    BasicBlock *noRunListBB = BasicBlock::Create(*scaff->theContext, "noRunList", theFunction);
    BasicBlock *returnBB = BasicBlock::Create(*scaff->theContext, "return", theFunction);

    Value *instructionlist = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *testResult = generateCallExtern(TyBool, "getIsTested", {PaAddr(evaluator)});
    Value *cond = scaff->builder.CreateICmpEQ(testResult, CoBool(1), "isTested");
    scaff->builder.CreateCondBr(cond, isTestedBB, notTestedBB);

    scaff->builder.SetInsertPoint(notTestedBB);
    Value *errVal = generateErrorNoTest(CoAddr(node.astnodeValue()->nodeName.datumValue()));
    scaff->builder.CreateRet(errVal);

    scaff->builder.SetInsertPoint(isTestedBB);
    testResult = generateCallExtern(TyBool, "getTestResult", {PaAddr(evaluator)});
    cond = scaff->builder.CreateICmpEQ(testResult, CoBool(testForTrue), "testResult");
    scaff->builder.CreateCondBr(cond, runListBB, noRunListBB);

    scaff->builder.SetInsertPoint(runListBB);
    Value *listRetval = generateCallList(instructionlist, returnType);
    scaff->builder.CreateBr(returnBB);

    scaff->builder.SetInsertPoint(noRunListBB);
    Value *noRetval = generateVoidRetval(node);
    scaff->builder.CreateBr(returnBB);

    scaff->builder.SetInsertPoint(returnBB);
    PHINode *retval = scaff->builder.CreatePHI(TyAddr, 2, "retval");
    retval->addIncoming(listRetval, runListBB);
    retval->addIncoming(noRetval, noRunListBB);
    return retval;
}

EXPORTC bool getIsTested(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    return Config::get().mainKernel()->callStack.isTested();
}

EXPORTC bool getTestResult(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    return Config::get().mainKernel()->callStack.testedState();
}
