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
    Q_ASSERT(expected && TypeNothing);
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
    Q_ASSERT(expected && TypeNothing);

    Value *procname = generateChild(node.astnodeValue(), 0, TypeDatum);
    Value *text = generateChild(node.astnodeValue(), 1, TypeDatum);
    procname = generateFromDatum(Datum::typeWord, node.astnodeValue(), procname);
    text = generateFromDatum(Datum::typeList, node.astnodeValue(), text);

    Value *retval = generateCallExtern(
        stdDefine,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(procname),
        PaAddr(text));
    return generateNotFlowControl(retval);
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
    Q_ASSERT(expected && TypeDatum);

    Value *procname = generateChild(node.astnodeValue(), 0, TypeDatum);
    procname = generateFromDatum(Datum::typeWord, node.astnodeValue(), procname);

    Value *retval = generateCallExtern(stdText, PaAddr(scaff_->evaluator_), PaAddr(procname));
    return generateNotFlowControl(retval);
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
    Q_ASSERT(expected && TypeDatum);

    Value *procname = generateChild(node.astnodeValue(), 0, TypeDatum);
    procname = generateFromDatum(Datum::typeWord, node.astnodeValue(), procname);

    Value *retval = generateCallExtern(stdFulltext, PaAddr(scaff_->evaluator_), PaAddr(procname));
    return generateNotFlowControl(retval);
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
    Q_ASSERT(expected && TypeNothing);

    Value *newname = generateChild(node.astnodeValue(), 0, TypeDatum);
    Value *oldname = generateChild(node.astnodeValue(), 1, TypeDatum);
    newname = generateFromDatum(Datum::typeWord, node.astnodeValue(), newname);
    oldname = generateFromDatum(Datum::typeWord, node.astnodeValue(), oldname);

    Value *retval = generateCallExtern(
        stdCopydef,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(newname),
        PaAddr(oldname));
    return generateNotFlowControl(retval);
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
    Q_ASSERT(expected && TypeDatum);

    Value *varname = generateChild(node.astnodeValue(), 0, TypeDatum);
    varname = generateFromDatum(Datum::typeWord, node.astnodeValue(), varname);

    Value *retval = generateCallExtern(stdThing, PaAddr(scaff_->evaluator_), PaAddr(varname));
    return generateNotFlowControl(retval);
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
    Q_ASSERT(expected && TypeNothing);

    Value *varname = generateChild(node.astnodeValue(), 0, TypeDatum);
    Value *value = generateChild(node.astnodeValue(), 1, TypeDatum);
    varname = generateFromDatum(Datum::typeWord, node.astnodeValue(), varname);

    generateCallExtern(setDatumForWord, PaAddr(value), PaAddr(varname));
    return generateVoidRetval(node);
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
    Q_ASSERT(expected && TypeNothing);

    AllocaInst *ary = generateChildrenAlloca(node.astnodeValue(), TypeDatum, DBG_NAME("localAry"));
    Value *retval = generateCallExtern(
        stdLocal,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(ary),
        PaInt32(ary->getArraySize()));
    return generateNotFlowControl(retval);
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
    Q_ASSERT(expected && TypeNothing);

    AllocaInst *ary = generateChildrenAlloca(node.astnodeValue(), TypeDatum, DBG_NAME("globalAry"));
    Value *retval = generateCallExtern(
        stdGlobal,
        PaAddr(scaff_->evaluator_),
        PaAddr(CoAddr(node.astnodeValue())),
        PaAddr(ary),
        PaInt32(ary->getArraySize()));
    return generateNotFlowControl(retval);
}
