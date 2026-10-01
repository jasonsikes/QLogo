//===-- qlogo/workspacemanagement.cpp - Workspace Management implementations -------*- C++ -*-===//
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
/// This file contains the implementation of the workspacemanagements methods
/// of the Compiler class.
///
//===----------------------------------------------------------------------===//

#include "astnode.h"
#include "compiler.h"
#include "compiler_internal.h"
#include "datum_types.h"
#include "workspace/exports.h"
#include "flowcontrol.h"
#include "workspace/kernel.h"
#include "sharedconstants.h"
#include "workspace/callframe.h"
using namespace llvm;
using namespace llvm::orc;

// PROCEDURE DEFINITION

/***DOC TO
TO procname :input1 :input2 ...                         (special form)

 command.  Prepares Logo to accept a procedure definition.  The
 procedure will be named "procname" and there must not already
 be a procedure by that name.  The inputs will be called "input1"
 etc.  Any number of inputs are allowed, including none.  Names
 of procedures and inputs are case-insensitive.

 Unlike every other Logo procedure, TO takes as its inputs the
 actual words typed in the instruction line, as if they were
 all quoted, rather than the results of evaluating expressions
 to provide the inputs.  (That's what "special form" means.)

 This version of Logo allows variable numbers of inputs to a
 procedure.  After the procedure name come four kinds of
 things, *in this order*:

     1.   0 or more REQUIRED inputs    :FOO :FROBOZZ
     2.   0 or more OPTIONAL inputs    [:BAZ 87] [:THINGO 5+9]
     3.   0 or 1 REST input            [:GARPLY]
     4.   0 or 1 DEFAULT number        5

 Every procedure has a MINIMUM, DEFAULT, and MAXIMUM
 number of inputs.  (The latter can be infinite.)

 The MINIMUM number of inputs is the number of required inputs,
 which must come first.  A required input is indicated by the

         :inputname

 notation.

 After all the required inputs can be zero or more optional inputs,
 each of which is represented by the following notation:

         [:inputname default.value.expression]

 When the procedure is invoked, if actual inputs are not supplied
 for these optional inputs, the default value expressions are
 evaluated to set values for the corresponding input names.  The
 inputs are processed from left to right, so a default value
 expression can be based on earlier inputs.  Example:

         to proc :inlist [:startvalue first :inlist]

 If the procedure is invoked by saying

         proc [a b c]

 then the variable INLIST will have the value [A B C] and the
 variable STARTVALUE will have the value A.  If the procedure
 is invoked by saying

         (proc [a b c] "x)

 then INLIST will have the value [A B C] and STARTVALUE will
 have the value X.

 After all the required and optional input can come a single "rest"
 input, represented by the following notation:

         [:inputname]

 This is a rest input rather than an optional input because there
 is no default value expression.  There can be at most one rest
 input.  When the procedure is invoked, the value of this inputname
 will be a list containing all of the actual inputs provided that
 were not used for required or optional inputs.  Example:

         to proc :in1 [:in2 "foo] [:in3 "baz] [:in4]

 If this procedure is invoked by saying

         proc "x

 then IN1 has the value X, IN2 has the value FOO, IN3 has the value
 BAZ, and IN4 has the value [] (the empty list).  If it's invoked
 by saying

         (proc "a "b "c "d "e)

 then IN1 has the value A, IN2 has the value B, IN3 has the value C,
 and IN4 has the value [D E].

 The MAXIMUM number of inputs for a procedure is infinite if a
 rest input is given; otherwise, it is the number of required
 inputs plus the number of optional inputs.

 The DEFAULT number of inputs for a procedure, which is the number
 of inputs that it will accept if its invocation is not enclosed
 in parentheses, is ordinarily equal to the minimum number.  If
 you want a different default number you can indicate that by
 putting the desired default number as the last thing on the
 TO line.  example:

         to proc :in1 [:in2 "foo] [:in3] 3

 This procedure has a minimum of one input, a default of three
 inputs, and an infinite maximum.

 Logo responds to the TO command by entering procedure definition
 mode.  The prompt character changes from "?" to ">" and whatever
 instructions you type become part of the definition until you
 type a line containing only the word END.

COD***/
// CMD TO -1 -1 -1 n
// CMD .MACRO -1 -1 -1 n
Value *Compiler::genInputProcedure(const DatumPtr &node, TypeMask expected)
{
    // We take the parameters as literals; we don't generate children.
    // Pass the node directly to the input function.
    return generateCallExtern(inputProcedure, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.datumValue())));
}

/***DOC DEFINE
DEFINE procname text

command.  Defines a procedure with name "procname" and text "text".
If there is already a procedure with the same name, the new
definition replaces the old one.  The text input must be a list
whose members are lists.  The first member is a list of inputs;
it looks like a TO line but without the word TO, without the
procedure name, and without the colons before input names.  In
other words, the members of this first sublist are words for the
names of required inputs and lists for the names of optional or
rest inputs.  The remaining sublists of the text input make up
the body of the procedure, with one sublist for each instruction
line of the body.  (There is no END line in the text input.)
It is an error to redefine a primitive procedure.

COD***/
/***DOC .DEFMACRO
.DEFMACRO procname text

command.  Defines a macro with name "procname" and text "text".
.DEFMACRO is exactly like DEFINE except that the new procedure
becomes a macro.  A macro's output is evaluated as Logo
instructions in the context of the macro's caller.  If there is
already a procedure with the same name, the new definition
replaces the old one.  It is an error to redefine a primitive
procedure.

COD***/
// CMD DEFINE 2 2 2 n
// CMD .DEFMACRO 2 2 2 n
Value *Compiler::genDefine(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [procname, text] = cs.children();
    procname = generateFromDatum(Datum::typeWord, node.astnodeValue(), procname);
    text = generateFromDatum(Datum::typeList, node.astnodeValue(), text);

    Value *retval = generateCallExtern(
        stdDefine,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(procname),
        PaAddr(text));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC TEXT
TEXT procname

    outputs the text of the procedure named "procname" in the form
    expected by DEFINE: a list of lists, the first of which describes
    the inputs to the procedure and the rest of which are the lines of
    its body.  The text does not reflect formatting information used
    when the procedure was defined, such as continuation lines and
    extra spaces.

COD***/
// CMD TEXT 1 1 1 d
Value *Compiler::genText(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *procname = cs.child(0);
    procname = generateFromDatum(Datum::typeWord, node.astnodeValue(), procname);

    Value *retval = generateCallExtern(stdText, PaAddr(scaff_->evaluator_), PaAddr(procname));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC FULLTEXT
FULLTEXT procname

    outputs a representation of the procedure "procname" in which
    formatting information is preserved.  If the procedure was defined
    with TO, EDIT, or LOAD, then the output is a list of words.  Each
    word represents one entire line of the definition in the form
    output by READWORD, including extra spaces and continuation lines.
    The last member of the output represents the END line.  If the
    procedure was defined with DEFINE, then the output is a list of
    lists.  If these lists are printed, one per line, the result will
    look like a definition using TO.  Note: the output from FULLTEXT
    is not suitable for use as input to DEFINE!

COD***/
// CMD FULLTEXT 1 1 1 d
Value *Compiler::genFulltext(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *procname = cs.child(0);
    procname = generateFromDatum(Datum::typeWord, node.astnodeValue(), procname);

    Value *retval = generateCallExtern(stdFulltext, PaAddr(scaff_->evaluator_), PaAddr(procname));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC COPYDEF
COPYDEF newname oldname

    command.  Makes "newname" a procedure identical to "oldname".
    The latter may be a primitive or a defined procedure.  If
    "newname" was already defined, the new definition replaces
    the old one.  It is an error to redefine a primitive
    procedure.  Note: dialects of Logo differ as to the order
    of inputs to COPYDEF.  This dialect uses "MAKE order," not
    "NAME order."

COD***/
// CMD COPYDEF 2 2 2 n
Value *Compiler::genCopydef(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [newname, oldname] = cs.children();
    newname = generateFromDatum(Datum::typeWord, node.astnodeValue(), newname);
    oldname = generateFromDatum(Datum::typeWord, node.astnodeValue(), oldname);

    Value *retval = generateCallExtern(
        stdCopydef,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(newname),
        PaAddr(oldname));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC THING
THING varname
:quoted.varname

    outputs the value of the variable whose name is the input.
    If there is more than one such variable, the innermost local
    variable of that name is chosen.  The colon notation is an
    abbreviation not for THING but for the combination

                thing "

    so that :FOO means THING "FOO.

COD***/
// CMD THING 1 1 1 d
Value *Compiler::genThing(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *varname = cs.child(0);
    varname = generateFromDatum(Datum::typeWord, node.astnodeValue(), varname);

    Value *lookedUp = generateCallExtern(getDatumForVarname, PaAddr(varname));
    BasicBlock *lookupBB = scaff_->builder_.GetInsertBlock();
    BasicBlock *noValueBB = scaff_->createBasicBlock(DBG_NAME("NoValue"));
    BasicBlock *hasValueBB = scaff_->createBasicBlock(DBG_NAME("hasValue"));

    scaff_->addColdPathBlocks(noValueBB);

    Value *dType = generateGetDatumIsa(lookedUp);
    Value *mask = scaff_->builder_.CreateAnd(dType, CoInt32(Datum::typeDataMask), DBG_NAME("dataMask"));
    Value *cond = scaff_->builder_.CreateICmpEQ(mask, CoInt32(0), DBG_NAME("dataMaskTest"));
    scaff_->builder_.CreateCondBr(cond, noValueBB, hasValueBB);

    // Unbound. Run ERRACT if it has a value; a datum from it stands in for THING.
    scaff_->builder_.SetInsertPoint(noValueBB);
    Value *handlerResult = generateCallExtern(handleNoValue, PaAddr(scaff_->evaluator_), PaAddr(varname));
    handlerResult = generateNotFlowControl(handlerResult);
    BasicBlock *recoveredBB = scaff_->builder_.GetInsertBlock();
    scaff_->builder_.CreateBr(hasValueBB);

    scaff_->builder_.SetInsertPoint(hasValueBB);
    PHINode *retval = scaff_->builder_.CreatePHI(TyAddr, 2, DBG_NAME("thing"));
    retval->addIncoming(lookedUp, lookupBB);
    retval->addIncoming(handlerResult, recoveredBB);
    return cs.ret(retval);
}

/***DOC MAKE
MAKE varname value

command.  Assigns the value "value" to the variable named "varname",
which must be a word.  Variable names are case-insensitive.  If a
variable with the same name already exists, the value of that
variable is changed.  If not, a new global variable is created.

COD***/
// CMD MAKE 2 2 2 n
Value *Compiler::genMake(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [varname, value] = cs.children();
    varname = generateFromDatum(Datum::typeWord, node.astnodeValue(), varname);

    DatumPtr valueNode = node.astnodeValue()->childAtIndex(1);
    if (valueNode.isASTNode() && valueNode.astnodeValue()->genExpression_ == &Compiler::genLiteral
        && valueNode.astnodeValue()->childAtIndex(0).isArray())
    {
        value = generateCallExtern(copyArray, PaAddr(scaff_->evaluator_), PaAddr(value));
    }

    generateCallExtern(setDatumForWord, PaAddr(value), PaAddr(varname));
    return cs.ret(generateVoidRetval(node));
}

Value *Compiler::genFoo(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeNothing);
    Value *retval = generateCallExtern(
        stdFoo, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())));
    return cs.ret(generateNotFlowControl(retval));
}

Value *Compiler::genSetfoo(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *value = cs.child(0);
    Value *retval = generateCallExtern(
        stdSetfoo,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(value));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC LOCAL
LOCAL varname
LOCAL varnamelist
(LOCAL varname1 varname2 ...)

    command.  Accepts as inputs one or more words, or a list of
    words.  A variable is created for each of these words, with
    that word as its name.  The variables are local to the
    currently running procedure.  Logo variables follow dynamic
    scope rules; a variable that is local to a procedure is
    available to any subprocedure invoked by that procedure.
    The variables created by LOCAL have no initial value; they
    must be assigned a value (e.g., with MAKE) before the procedure
    attempts to read their value.

COD***/
// CMD LOCAL 1 1 -1 n
Value *Compiler::genLocal(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    AllocaInst *ary = cs.allocaAry(DBG_NAME("localAry"));
    Value *retval = generateCallExtern(
        stdLocal,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(ary),
        PaInt32(ary->getArraySize()));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC GLOBAL
GLOBAL varname
GLOBAL varnamelist
(GLOBAL varname1 varname2 ...)

    command.  Accepts as inputs one or more words, or a list of
    words.  A global variable is created for each of these words, with
    that word as its name.  The only reason this is necessary is that
    you might want to use the "setter" notation SETXYZ for a variable
    XYZ that does not already have a value; GLOBAL "XYZ makes that legal.
    Note: If there is currently a local variable of the same name, this
    command does *not* make Logo use the global value instead of the
    local one.

COD***/
// CMD GLOBAL 1 1 -1 n
Value *Compiler::genGlobal(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    AllocaInst *ary = cs.allocaAry(DBG_NAME("globalAry"));
    Value *retval = generateCallExtern(
        stdGlobal,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(ary),
        PaInt32(ary->getArraySize()));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC PPROP
PPROP plistname propname value

    command.  Adds a property to the "plistname" property list
    with name "propname" and value "value".

COD***/
// CMD PPROP 3 3 3 n
Value *Compiler::genPprop(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum, TypeDatum, TypeDatum);
    auto [plistname, propname, value] = cs.children();
    plistname = generateFromDatum(Datum::typeWord, node.astnodeValue(), plistname);
    propname = generateFromDatum(Datum::typeWord, node.astnodeValue(), propname);

    generateCallExtern(stdPprop, PaAddr(plistname), PaAddr(propname), PaAddr(value));
    return cs.ret(generateVoidRetval(node));
}

/***DOC GPROP
GPROP plistname propname

    outputs the value of the "propname" property in the "plistname"
    property list, or the empty list if there is no such property.

COD***/
// CMD GPROP 2 2 2 d
Value *Compiler::genGprop(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [plistname, propname] = cs.children();
    plistname = generateFromDatum(Datum::typeWord, node.astnodeValue(), plistname);
    propname = generateFromDatum(Datum::typeWord, node.astnodeValue(), propname);

    Value *retval = generateCallExtern(stdGprop, PaAddr(scaff_->evaluator_), PaAddr(plistname), PaAddr(propname));
    return cs.ret(retval);
}

/***DOC REMPROP
REMPROP plistname propname

    command.  Removes the property named "propname" from the
    property list named "plistname".

COD***/
// CMD REMPROP 2 2 2 n
Value *Compiler::genRemprop(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum, TypeDatum);
    auto [plistname, propname] = cs.children();
    plistname = generateFromDatum(Datum::typeWord, node.astnodeValue(), plistname);
    propname = generateFromDatum(Datum::typeWord, node.astnodeValue(), propname);

    generateCallExtern(stdRemprop, PaAddr(plistname), PaAddr(propname));
    return cs.ret(generateVoidRetval(node));
}

/***DOC PLIST
PLIST plistname

    outputs a list whose odd-numbered members are the names, and
    whose even-numbered members are the values, of the properties
    in the property list named "plistname".  The output is a copy
    of the actual property list; changing properties later will not
    magically change a list output earlier by PLIST.

COD***/
// CMD PLIST 1 1 1 d
Value *Compiler::genPlist(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *plistname = cs.child(0);
    plistname = generateFromDatum(Datum::typeWord, node.astnodeValue(), plistname);

    Value *retval = generateCallExtern(stdPlist, PaAddr(scaff_->evaluator_), PaAddr(plistname));
    return cs.ret(retval);
}

// PREDICATES

/***DOC PROCEDUREP PROCEDURE?
PROCEDUREP name
PROCEDURE? name

    outputs TRUE if the input is the name of a procedure.

COD***/
// CMD PROCEDUREP 1 1 1 b
// CMD PROCEDURE? 1 1 1 b
Value *Compiler::genProcedurep(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *name = cs.child(0);
    name = generateFromDatum(Datum::typeWord, node.astnodeValue(), name);
    return cs.ret(generateCallExtern(stdProcedurep, PaAddr(name)));
}

/***DOC MACROP MACRO?
MACROP name
MACRO? name

    outputs TRUE if its input is the name of a macro.

COD***/
// CMD MACROP 1 1 1 b
// CMD MACRO? 1 1 1 b
Value *Compiler::genMacrop(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *name = cs.child(0);
    name = generateFromDatum(Datum::typeWord, node.astnodeValue(), name);
    return cs.ret(generateCallExtern(stdMacrop, PaAddr(name)));
}

/***DOC PRIMITIVEP PRIMITIVE?
PRIMITIVEP name
PRIMITIVE? name

    outputs TRUE if the input is the name of a primitive procedure
    (one built into Logo).  Note that some of the procedures
    described in this document are library procedures, not primitives.

COD***/
// CMD PRIMITIVEP 1 1 1 b
// CMD PRIMITIVE? 1 1 1 b
Value *Compiler::genPrimitivep(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *name = cs.child(0);
    name = generateFromDatum(Datum::typeWord, node.astnodeValue(), name);
    return cs.ret(generateCallExtern(stdPrimitivep, PaAddr(name)));
}

/***DOC DEFINEDP DEFINED?
DEFINEDP name
DEFINED? name

    outputs TRUE if the input is the name of a user-defined procedure,
    including a library procedure.

COD***/
// CMD DEFINEDP 1 1 1 b
// CMD DEFINED? 1 1 1 b
Value *Compiler::genDefinedp(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *name = cs.child(0);
    name = generateFromDatum(Datum::typeWord, node.astnodeValue(), name);
    return cs.ret(generateCallExtern(stdDefinedp, PaAddr(name)));
}

/***DOC NAMEP NAME?
NAMEP name
NAME? name

    outputs TRUE if the input is the name of a variable.

COD***/
// CMD NAMEP 1 1 1 b
// CMD NAME? 1 1 1 b
Value *Compiler::genNamep(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *name = cs.child(0);
    name = generateFromDatum(Datum::typeWord, node.astnodeValue(), name);
    return cs.ret(generateCallExtern(stdNamep, PaAddr(name)));
}

/***DOC PLISTP PLIST?
PLISTP name
PLIST? name

    outputs TRUE if the input is the name of a nonempty property list.
    (In principle every word is the name of a property list; if you haven't
    put any properties in it, PLIST of that name outputs an empty list.)

COD***/
// CMD PLISTP 1 1 1 b
// CMD PLIST? 1 1 1 b
Value *Compiler::genPlistp(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *name = cs.child(0);
    name = generateFromDatum(Datum::typeWord, node.astnodeValue(), name);
    return cs.ret(generateCallExtern(stdPlistp, PaAddr(name)));
}

/***DOC ARITY
ARITY procedurename

    outputs a list of three numbers: the minimum, default, and
    maximum number of inputs for the procedure whose name is the
    input.  It is an error if the input is not the name of a
    procedure.  The maximum is -1 if the procedure takes a
    variable number of inputs.

COD***/
// CMD ARITY 1 1 1 d
Value *Compiler::genArity(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *name = cs.child(0);
    name = generateFromDatum(Datum::typeWord, node.astnodeValue(), name);

    Value *retval = generateCallExtern(stdArity, PaAddr(scaff_->evaluator_), PaAddr(name));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC BURY
BURY contentslist

    command.  Buries the procedures, variables, and property
    lists named in the input.  A buried object is not included
    in the lists output by CONTENTS, PROCEDURES, NAMES, and
    PLISTS, but is included in the list output by BURIED.  By
    implication, buried things are not printed by POALL or
    saved by SAVE.

COD***/
// CMD BURY 1 1 1 n
Value *Compiler::genBury(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *contents = cs.child(0);
    Value *retval = generateCallExtern(
        stdBury,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(contents));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC TRACE
TRACE contentslist

    command.  Marks the named items for tracing.  A message is
    printed whenever a traced procedure is invoked, giving the
    actual input values, and whenever a traced procedure STOPs
    or OUTPUTs.  A message is printed whenever a new value is
    assigned to a traced variable using MAKE.  A message is
    printed whenever a new property is given to a traced
    property list using PPROP.

COD***/
// CMD TRACE 1 1 1 n
Value *Compiler::genTrace(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *contents = cs.child(0);
    Value *retval = generateCallExtern(
        stdTrace,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(contents));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC STEP
STEP contentslist

    command.  Marks the named items for stepping.  Whenever a
    stepped procedure is invoked, each instruction line in the
    procedure body is printed before being executed, and Logo
    waits for the user to type a newline at the terminal.  A
    message is printed whenever a stepped variable name is
    "shadowed" because a local variable of the same name is
    created either as a procedure input or by the LOCAL command.

COD***/
// CMD STEP 1 1 1 n
Value *Compiler::genStep(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *contents = cs.child(0);
    Value *retval = generateCallExtern(
        stdStep,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(contents));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC UNBURY
UNBURY contentslist

    command.  Unburies the procedures, variables, and property
    lists named in the input.  That is, the named items will be
    returned to view in CONTENTS, etc.

COD***/
// CMD UNBURY 1 1 1 n
Value *Compiler::genUnbury(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *contents = cs.child(0);
    Value *retval = generateCallExtern(
        stdUnbury,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(contents));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC UNTRACE
UNTRACE contentslist

    command.  Turns off tracing for the named items.

COD***/
// CMD UNTRACE 1 1 1 n
Value *Compiler::genUntrace(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *contents = cs.child(0);
    Value *retval = generateCallExtern(
        stdUntrace,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(contents));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC UNSTEP
UNSTEP contentslist

    command.  Turns off stepping for the named items.

COD***/
// CMD UNSTEP 1 1 1 n
Value *Compiler::genUnstep(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *contents = cs.child(0);
    Value *retval = generateCallExtern(
        stdUnstep,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(contents));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC BURIED
BURIED

    outputs a contents list including all buried named items
    in the workspace.

COD***/
// CMD BURIED 0 0 0 d
Value *Compiler::genBuried(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeNothing);
    return cs.ret(generateCallExtern(stdBuried, PaAddr(scaff_->evaluator_)));
}

/***DOC TRACED
TRACED

    outputs a contents list including all traced named items
    in the workspace.

COD***/
// CMD TRACED 0 0 0 d
Value *Compiler::genTraced(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeNothing);
    return cs.ret(generateCallExtern(stdTraced, PaAddr(scaff_->evaluator_)));
}

/***DOC STEPPED
STEPPED

    outputs a contents list including all stepped named items
    in the workspace.

COD***/
// CMD STEPPED 0 0 0 d
Value *Compiler::genStepped(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeNothing);
    return cs.ret(generateCallExtern(stdStepped, PaAddr(scaff_->evaluator_)));
}

/***DOC PROCEDURES
PROCEDURES

    outputs a list of the names of all unburied user-defined procedures
    in the workspace.  Note that this is a list of names, not a
    contents list.  (However, procedures that require a contents list
    as input will accept this list.)

COD***/
// CMD PROCEDURES 0 0 0 d
Value *Compiler::genProcedures(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeNothing);
    return cs.ret(generateCallExtern(stdProcedures, PaAddr(scaff_->evaluator_)));
}

/***DOC PRIMITIVES
PRIMITIVES

    outputs a list of the names of all primitive procedures
    in the workspace.  Note that this is a list of names, not a
    contents list.  (However, procedures that require a contents list
    as input will accept this list.)

COD***/
// CMD PRIMITIVES 0 0 0 d
Value *Compiler::genPrimitives(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeNothing);
    return cs.ret(generateCallExtern(stdPrimitives, PaAddr(scaff_->evaluator_)));
}

/***DOC NAMES
NAMES

    outputs a contents list consisting of an empty list (indicating
    no procedure names) followed by a list of all unburied variable
    names in the workspace.

COD***/
// CMD NAMES 0 0 0 d
Value *Compiler::genNames(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeNothing);
    return cs.ret(generateCallExtern(stdNames, PaAddr(scaff_->evaluator_)));
}

/***DOC PLISTS
PLISTS

    outputs a contents list consisting of two empty lists (indicating
    no procedures or variables) followed by a list of all unburied
    nonempty property lists in the workspace.

COD***/
// CMD PLISTS 0 0 0 d
Value *Compiler::genPlists(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeNothing);
    return cs.ret(generateCallExtern(stdPlists, PaAddr(scaff_->evaluator_)));
}

/***DOC CONTENTS
CONTENTS

    outputs a "contents list," i.e., a list of three lists containing
    names of defined procedures, variables, and property lists
    respectively.  This list includes all unburied named items in
    the workspace.

COD***/
// CMD CONTENTS 0 0 0 d
Value *Compiler::genContents(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeNothing);
    return cs.ret(generateCallExtern(stdContents, PaAddr(scaff_->evaluator_)));
}

/***DOC NODES
NODES

    outputs a list of two numbers.  The first is the number of nodes of
    memory currently in use.  The second is the maximum number of nodes
    used at any time since the last invocation of NODES, or since this
    session of Logo began if NODES has not been used.

COD***/
// CMD NODES 0 0 0 d
Value *Compiler::genNodes(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeNothing);
    return cs.ret(generateCallExtern(stdNodes, PaAddr(scaff_->evaluator_)));
}

/***DOC PRINTOUT PO
PRINTOUT contentslist
PO contentslist

    command.  Prints to the write stream the definitions of all
    procedures, variables, and property lists named in the input
    contents list.

COD***/
// CMD PRINTOUT 1 1 1 n
// CMD PO 1 1 1 n
Value *Compiler::genPrintout(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *contents = cs.child(0);
    Value *retval = generateCallExtern(
        stdPrintout,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(contents));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC POT
POT contentslist

    command.  Prints the title lines of the named procedures and
    the definitions of the named variables and property lists.
    For property lists, the entire list is shown on one line
    instead of as a series of PPROP instructions as in PO.

COD***/
// CMD POT 1 1 1 n
Value *Compiler::genPot(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *contents = cs.child(0);
    Value *retval = generateCallExtern(
        stdPot,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(contents));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC ERASE ER
ERASE contentslist
ER contentslist

    command.  Erases from the workspace the procedures, variables,
    and property lists named in the input.  Primitive procedures may
    not be erased unless the variable REDEFP has the value TRUE.

COD***/
// CMD ERASE 1 1 1 n
// CMD ER 1 1 1 n
Value *Compiler::genErase(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *contents = cs.child(0);
    Value *retval = generateCallExtern(
        stdErase,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(contents));
    return cs.ret(generateNotFlowControl(retval));
}

Value *Compiler::generateEraseUnburied(const DatumPtr &node, TypeMask expected,
                                       bool procedures, bool variables, bool plists)
{
    CallSite cs(this, node, TypeNothing);
    generateCallExtern(
        stdEraseUnburied,
        PaBool(CoBool(procedures)),
        PaBool(CoBool(variables)),
        PaBool(CoBool(plists)));
    return cs.ret(generateVoidRetval(node));
}

/***DOC ERALL
ERALL

    command.  Erases all unburied procedures, variables, and property
    lists from the workspace.  Abbreviates ERASE CONTENTS.

COD***/
// CMD ERALL 0 0 0 n
Value *Compiler::genErall(const DatumPtr &node, TypeMask expected)
{
    return generateEraseUnburied(node, expected, true, true, true);
}

/***DOC ERPS
ERPS

    command.  Erases all unburied procedures from the workspace.
    Abbreviates ERASE PROCEDURES.

COD***/
// CMD ERPS 0 0 0 n
Value *Compiler::genErps(const DatumPtr &node, TypeMask expected)
{
    return generateEraseUnburied(node, expected, true, false, false);
}

/***DOC ERNS
ERNS

    command.  Erases all unburied variables from the workspace.
    Abbreviates ERASE NAMES.

COD***/
// CMD ERNS 0 0 0 n
Value *Compiler::genErns(const DatumPtr &node, TypeMask expected)
{
    return generateEraseUnburied(node, expected, false, true, false);
}

/***DOC ERPLS
ERPLS

    command.  Erases all unburied property lists from the workspace.
    Abbreviates ERASE PLISTS.

COD***/
// CMD ERPLS 0 0 0 n
Value *Compiler::genErpls(const DatumPtr &node, TypeMask expected)
{
    return generateEraseUnburied(node, expected, false, false, true);
}

/***DOC BURIEDP BURIED?
BURIEDP contentslist
BURIED? contentslist

    outputs TRUE if the first procedure, variable, or property list
    named in the contents list is buried, FALSE if not.  Only the
    first thing in the list is tested; the most common use will be
    with a word as input, naming a procedure, but a contents list
    is allowed so that you can BURIEDP [[] [VARIABLE]] or
    BURIEDP [[] [] [PROPLIST]].

COD***/
// CMD BURIEDP 1 1 1 d
// CMD BURIED? 1 1 1 d
Value *Compiler::genBuriedp(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *contents = cs.child(0);
    Value *retval = generateCallExtern(
        stdBuriedp,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(contents));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC TRACEDP TRACED?
TRACEDP contentslist
TRACED? contentslist

    outputs TRUE if the first procedure, variable, or property list
    named in the contents list is traced, FALSE if not.  Only the
    first thing in the list is tested; the most common use will be
    with a word as input, naming a procedure, but a contents list
    is allowed so that you can TRACEDP [[] [VARIABLE]] or
    TRACEDP [[] [] [PROPLIST]].

COD***/
// CMD TRACEDP 1 1 1 d
// CMD TRACED? 1 1 1 d
Value *Compiler::genTracedp(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *contents = cs.child(0);
    Value *retval = generateCallExtern(
        stdTracedp,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(contents));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC STEPPEDP STEPPED?
STEPPEDP contentslist
STEPPED? contentslist

    outputs TRUE if the first procedure, variable, or property list
    named in the contents list is stepped, FALSE if not.  Only the
    first thing in the list is tested; the most common use will be
    with a word as input, naming a procedure, but a contents list
    is allowed so that you can STEPPEDP [[] [VARIABLE]] or
    STEPPEDP [[] [] [PROPLIST]].

COD***/
// CMD STEPPEDP 1 1 1 d
// CMD STEPPED? 1 1 1 d
Value *Compiler::genSteppedp(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *contents = cs.child(0);
    Value *retval = generateCallExtern(
        stdSteppedp,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(contents));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC SAVE
SAVE filename

    command.  Saves the definitions of all unburied procedures, variables,
    and nonempty property lists in the named file.  Equivalent to

        to save :filename
        local "oldwriter
        make "oldwriter writer
        openwrite :filename
        setwrite :filename
        poall
        setwrite :oldwriter
        close :filename
        end

    except that SAVE does not change WRITER.  Buried names are omitted.

    Exceptionally, SAVE can be used with no input and without parentheses
    if it is the last thing on the command line.  In this case, the
    filename from the most recent LOAD or SAVE command will be used.
    (It is an error if there has been no previous LOAD or SAVE.)

COD***/
// CMD SAVE 0 -1 1 n
Value *Compiler::genSave(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *filenameAddr = CoAddr(0);
    if (cs.count() > 0)
    {
        filenameAddr = generateFromDatum(Datum::typeWord, node.astnodeValue(), cs.child(0));
    }
    Value *retval = generateCallExtern(
        stdSave,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(filenameAddr));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC LOAD
LOAD filename

    command.  Reads instructions from the named file and executes
    them.  The file can include procedure definitions with TO, and
    these are accepted even if a procedure by the same name already
    exists.  If the file assigns a list value to a variable named
    STARTUP, then that list is run as an instructionlist after the
    file is loaded.  If there is a variable LOADNOISILY whose value
    is TRUE, then TO commands in the file print "PROCNAME defined"
    (where PROCNAME is the name of the procedure being defined); if
    LOADNOISILY is FALSE or undefined, TO commands in the file are
    carried out silently.

COD***/
// CMD LOAD 1 1 1 n
Value *Compiler::genLoad(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *filename = cs.child(0);
    filename = generateFromDatum(Datum::typeWord, node.astnodeValue(), filename);

    Value *retval = generateCallExtern(
        stdLoad,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(filename));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC HELP
HELP name
(HELP)

    command.  Prints information from the reference manual about
    the primitive procedure named by the input.  With no input,
    lists all the primitives about which help is available.

    If HELP is called with the name of a defined procedure for which there
    is no help file, it will print the title line of the procedure
    followed by lines from the procedure body that start with semicolon,
    stopping when a non-semicolon line is seen.

    Exceptionally, the HELP command can be used without its default
    input and without parentheses provided that nothing follows it on
    the instruction line.

COD***/
// CMD HELP 0 -1 1 n
Value *Compiler::genHelp(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *nameAddr = CoAddr(0);
    if (cs.count() > 0)
        nameAddr = cs.child(0);
    Value *retval = generateCallExtern(
        stdHelp, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())), PaAddr(nameAddr));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC EDIT ED
EDIT contentslist
ED contentslist
(EDIT)
(ED)

    command.  Edits the definitions of the named item(s), using the
    Logo editor.  The contents list is in the same format as for
    PRINTOUT.  If there is no input, EDIT edits all unburied
    procedures.  Exceptionally, EDIT can be used without its default
    input and without parentheses provided that nothing follows it
    on the instruction line.

    EDIT writes the definitions to a temporary file in the directory
    named by SETTEMPLOC, then invokes the editor named by SETEDITOR
    (or Psychi's built-in editor when QLogo is talking to Psychi).
    When the editor finishes, Logo LOADs the file and deletes it.
    THROW "SYSTEM leaves the temporary file on disk.

COD***/
// CMD EDIT 0 -1 1 n
// CMD ED 0 -1 1 n
Value *Compiler::genEdit(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    Value *contentsAddr = CoAddr(0);
    if (cs.count() > 0)
        contentsAddr = cs.child(0);
    Value *retval = generateCallExtern(
        stdEdit, PaAddr(scaff_->evaluator_), PaAddr(CoAddr(node.astnodeValue())), PaAddr(contentsAddr));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC EDITFILE
EDITFILE filename

    command.  Starts the Logo editor, like EDIT, but instead of
    editing a contents list it edits the named file.  When the
    editor is finished, Logo LOADs the file.  A missing file is
    created empty.  The file is not deleted.

COD***/
// CMD EDITFILE 1 1 1 n
Value *Compiler::genEditfile(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *filename = cs.child(0);
    filename = generateFromDatum(Datum::typeWord, node.astnodeValue(), filename);

    Value *retval = generateCallExtern(
        stdEditfile,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(filename));
    return cs.ret(generateNotFlowControl(retval));
}

/***DOC SETEDITOR
SETEDITOR command

    command.  Sets the editor invoked by EDIT and EDITFILE when QLogo
    is not using Psychi's built-in editor.  The input is a word, or a
    non-empty list of words.  The file path is appended as a final
    argument.  The default is the EDITOR environment variable, or nano.

COD***/
// CMD SETEDITOR 1 1 1 n
Value *Compiler::genSeteditor(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *command = cs.child(0);
    auto validator = [this](Value *candidate) {
        Value *isValid = generateCallExtern(isValidShellCommand, PaAddr(candidate));
        return scaff_->builder_.CreateICmpEQ(isValid, CoBool(true), DBG_NAME("isValidEditorCond"));
    };
    command = generateValidationDatum(node.astnodeValue(), command, validator);

    generateCallExtern(stdSeteditor, PaAddr(command));
    return cs.ret(generateVoidRetval(node));
}

/***DOC EDITOR
EDITOR

    outputs the current SETEDITOR command.  See SETEDITOR.

COD***/
// CMD EDITOR 0 0 0 d
Value *Compiler::genEditor(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeNothing);
    return cs.ret(generateCallExtern(stdEditor, PaAddr(scaff_->evaluator_)));
}

/***DOC SETTEMPLOC
SETTEMPLOC path

    command.  Sets the directory where EDIT writes its temporary file.
    The input must be a word, unless it is the empty list, to use the
    system temporary directory.

COD***/
// CMD SETTEMPLOC 1 1 1 n
Value *Compiler::genSettemploc(const DatumPtr &node, TypeMask expected)
{

    CallSite cs(this, node, TypeDatum);
    Value *path = generateWordOrEmptyListFromDatum(node.astnodeValue(), cs.child(0));

    generateCallExtern(stdSettemploc, PaAddr(path));
    return cs.ret(generateVoidRetval(node));
}

/***DOC TEMPLOC
TEMPLOC

    outputs the directory EDIT uses for its temporary file.  See
    SETTEMPLOC.

COD***/
// CMD TEMPLOC 0 0 0 d
Value *Compiler::genTemploc(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeNothing);
    return cs.ret(generateCallExtern(stdTemploc, PaAddr(scaff_->evaluator_)));
}

/***DOC GC
GC
(GC anything)

    command.  In UCBLogo this runs the garbage collector.  QLogo frees
    memory as soon as a word, list, or array is no longer needed, so
    this does nothing.  An optional input is accepted and ignored.

COD***/
// CMD GC 0 0 1 n
Value *Compiler::genGc(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    return cs.ret(generateVoidRetval(node));
}

/***DOC .SETSEGMENTSIZE
.SETSEGMENTSIZE num

    command.  In UCBLogo this sets how many nodes are allocated from
    the operating system at once.  QLogo does not use segment
    allocation, so this does nothing.

COD***/
// CMD .SETSEGMENTSIZE 1 1 1 n
Value *Compiler::genDotSetsegmentsize(const DatumPtr &node, TypeMask expected)
{
    CallSite cs(this, node, TypeDatum);
    return cs.ret(generateVoidRetval(node));
}


/***DOC NAME
NAME value varname					(library procedure)

	command.  Same as MAKE but with the inputs in reverse order.

COD***/

/***DOC LOCALMAKE
LOCALMAKE varname value				(library procedure)

	command.  Makes the named variable local, like LOCAL, and
	assigns it the given value, like MAKE.

COD***/

/***DOC NAMELIST
NAMELIST varname					(library procedure)
NAMELIST varnamelist

	outputs a contents list consisting of an empty list followed by
	a list of the name or names given as input.  This is useful in
	conjunction with workspace control procedures that require a contents
	list as input.

COD***/

/***DOC PLLIST
PLLIST plname						(library procedure)
PLLIST plnamelist

	outputs a contents list consisting of two empty lists followed by
	a list of the name or names given as input.  This is useful in
	conjunction with workspace control procedures that require a contents
	list as input.

COD***/

/***DOC POALL
POALL							(library procedure)

	command.  Prints all unburied definitions in the workspace.
	Abbreviates PO CONTENTS.

COD***/

/***DOC POPS
POPS							(library procedure)

	command.  Prints the definitions of all unburied procedures in
	the workspace.  Abbreviates PO PROCEDURES.

COD***/

/***DOC PONS
PONS							(library procedure)

	command.  Prints the definitions of all unburied variables in
	the workspace.  Abbreviates PO NAMES.

COD***/

/***DOC POPLS
POPLS							(library procedure)

	command.  Prints the contents of all unburied nonempty property
	lists in the workspace.  Abbreviates PO PLISTS.

COD***/

/***DOC PON
PON varname						(library procedure)
PON varnamelist

	command.  Prints the definitions of the named variable(s).
	Abbreviates PO NAMELIST varname(list).

COD***/

/***DOC POPL
POPL plname						(library procedure)
POPL plnamelist

	command.  Prints the definitions of the named property list(s).
	Abbreviates PO PLLIST plname(list).

COD***/

/***DOC POTS
POTS							(library procedure)

	command.  Prints the title lines of all unburied procedures
	in the workspace.  Abbreviates POT PROCEDURES.


COD***/

/***DOC ERN
ERN varname						(library procedure)
ERN varnamelist

	command.  Erases from the workspace the variable(s) named in the
	input.  Abbreviates ERASE NAMELIST varname(list).

COD***/

/***DOC ERPL
ERPL plname						(library procedure)
ERPL plnamelist

	command.  Erases from the workspace the property list(s) named in the
	input.  Abbreviates ERASE PLLIST plname(list).

COD***/

/***DOC BURYALL
BURYALL							(library procedure)

	command.  Abbreviates BURY CONTENTS.

COD***/

/***DOC BURYNAME
BURYNAME varname					(library procedure)
BURYNAME varnamelist

	command.  Abbreviates BURY NAMELIST varname(list).

COD***/

/***DOC UNBURYALL
UNBURYALL						(library procedure)

	command.  Abbreviates UNBURY BURIED.

COD***/

/***DOC UNBURYNAME
UNBURYNAME varname					(library procedure)
UNBURYNAME varnamelist

	command.  Abbreviates UNBURY NAMELIST varname(list).

COD***/

/***DOC EDALL
EDALL							(library procedure)

	command.  Abbreviates EDIT CONTENTS.

COD***/

/***DOC EDPS
EDPS							(library procedure)

	command.  Abbreviates EDIT PROCEDURES.

COD***/

/***DOC EDNS
EDNS							(library procedure)

	command.  Abbreviates EDIT NAMES.

COD***/

/***DOC EDPLS
EDPLS							(library procedure)

	command.  Abbreviates EDIT PLISTS.

COD***/

/***DOC EDN
EDN varname						(library procedure)
EDN varnamelist

	command.  Abbreviates EDIT NAMELIST varname(list).

COD***/

/***DOC EDPL
EDPL plname						(library procedure)
EDPL plnamelist

	command.  Abbreviates EDIT PLLIST plname(list).

COD***/

/***DOC SAVEL
SAVEL contentslist filename				(library procedure)

	command.  Saves the definitions of the procedures, variables, and
	property lists specified by "contentslist" to the file named
	"filename".

COD***/
