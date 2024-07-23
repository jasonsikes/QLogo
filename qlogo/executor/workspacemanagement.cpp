
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
/// This file contains a part of the implementation of the Kernel class, which is the
/// executor proper of the QLogo language. Specifically, this file contains the
/// implementations for operations that manage the workspace, such as variables,
/// procedures, and property lists.
///
/// See README.md in this directory for information about the documentation
/// structure for each Kernel::exc* method.
///
//===----------------------------------------------------------------------===//

#include "astnode.h"
#include "controller/logocontroller.h"
#include "datum.h"
#include "error.h"
#include "kernel.h"
#include "parser.h"


/// @brief Extract the three components from a contents list.
/// @param contentslist The contents list to extract from.
/// @param proceduresList A pointer to a list to store the procedures.
/// @param variablesList A pointer to a list to store the variables.
/// @param propertiesList A pointer to a list to store the properties.
void extractFromContentslist(DatumPtr contentslist, List **proceduresList, List **variablesList, List **propertiesList)
{
    List *src = contentslist.listValue();
    *proceduresList = src->head.listValue();
    src = src->tail.listValue();
    *variablesList = src->head.listValue();
    src = src->tail.listValue();
    *propertiesList = src->head.listValue();
}

QString Kernel::executeText(const QString &text)
{
    QString inText = text;
    QString outText;

    QTextStream inQStream(&inText, QIODevice::ReadOnly);
    QTextStream outQStream(&outText, QIODevice::WriteOnly);

    TextStream inStream(&inQStream);
    TextStream outStream(&outQStream);

    Parser textParser;

    StreamRedirect sr(&inStream, &outStream, &textParser);

    bool shouldContinue = true;
    while (shouldContinue)
    {
        shouldContinue = getLineAndRunIt(false);
    }
    outStream.flush();
    return outText;
}

void Kernel::editAndRunWorkspaceText()
{
    const QString textRetval = Config::get().mainController()->editText(workspaceText);
    if (textRetval != workspaceText)
    {
        workspaceText = textRetval;
        QString output = executeText(textRetval);
        if (varLOADNOISILY())
        {
            sysPrint(output);
        }
    }
}

void Kernel::editAndRunFile()
{
    QString filepath = filepathForFilename(editFileName);
    QFile file(filepath);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        Error::cantOpen(editFileName);
    }
    QTextStream in(&file);
    QString fileText = in.readAll();

    const QString textRetval = Config::get().mainController()->editText(fileText);
    if (textRetval != "")
    {
        fileText = textRetval;
        file.seek(0);
        file.resize(0);
        QTextStream out(&file);
        out << fileText;
        QString output = executeText(fileText);
        if (varLOADNOISILY())
        {
            sysPrint(output);
        }
    }
}

DatumPtr Kernel::buildContentsList(showContents_t showWhat)
{
    List *retval = new List();
    retval->append(procedures->allProcedureNames(showWhat));
    retval->append(callStack.allVariables(showWhat));
    retval->append(plists.allPLists(showWhat));
    return retval;
}

DatumPtr Kernel::contentslistFromDatumPtr(DatumPtr sourceNode)
{
    List *sublists[3];
    DatumPtr locker[3];
    for (int i = 0; i < 3; ++i)
    {
        sublists[i] = new List();
        locker[i] = DatumPtr(sublists[i]);
    }

    if (sourceNode.isWord())
    {
        sublists[0]->append(sourceNode);
    }
    else if (sourceNode.isList())
    {
        unsigned parseLevel = 0;
        ListIterator i = sourceNode.listValue()->newIterator();
        while (i.elementExists())
        {
            if (parseLevel > 2)
                return nothing;
            DatumPtr d = i.element();
            if (d.isWord())
            {
                sublists[parseLevel]->append(d);
            }
            else if (d.isList())
            {
                ListIterator j = d.listValue()->newIterator();
                while (j.elementExists())
                {
                    DatumPtr e = j.element();
                    if (!e.isWord())
                        return nothing;
                    sublists[parseLevel]->append(e);
                }
                ++parseLevel;
            }
            else
            {
                return nothing;
            }
        }
    }
    else
    {
        return nothing;
    }

    List *retval = new List();
    for (int i = 0; i < 3; ++i)
    {
        retval->append(DatumPtr(sublists[i]));
    }
    return DatumPtr(retval);
}

void Kernel::processContentsListWithMethod(DatumPtr contentslist, void (Workspace::*method)(const QString &))
{
    List *proceduresList;
    List *variablesList;
    List *propertiesList;
    extractFromContentslist(contentslist, &proceduresList, &variablesList, &propertiesList);

    ListIterator i = proceduresList->newIterator();
    while (i.elementExists())
    {
        QString procname = i.element().wordValue()->keyValue();
        (procedures->*method)(procname);
    }

    i = variablesList->newIterator();
    while (i.elementExists())
    {
        QString varname = i.element().wordValue()->keyValue();
        (callStack.*method)(varname);
    }

    i = propertiesList->newIterator();
    while (i.elementExists())
    {
        DatumPtr listnameP = i.element();
        QString listname = listnameP.wordValue()->keyValue();
        (plists.*method)(listname);
    }
}

DatumPtr Kernel::queryContentsListWithMethod(DatumPtr contentslist, bool (Workspace::*method)(const QString &))
{
    List *proceduresList;
    List *variablesList;
    List *propertiesList;
    extractFromContentslist(contentslist, &proceduresList, &variablesList, &propertiesList);

    if (!proceduresList->isEmpty())
    {
        QString procname = proceduresList->head.wordValue()->keyValue();
        return DatumPtr((procedures->*method)(procname));
    }

    if (!variablesList->isEmpty())
    {
        QString varname = variablesList->head.wordValue()->keyValue();
        return DatumPtr((callStack.*method)(varname));
    }

    if (!propertiesList->isEmpty())
    {
        QString pname = propertiesList->head.wordValue()->keyValue();
        return DatumPtr((plists.*method)(pname));
    }
    return nothing;
}

QString Kernel::createPrintoutFromContentsList(DatumPtr contentslist, bool shouldValidate)
{
    QString retval("");

    List *proceduresList;
    List *variablesList;
    List *propertiesList;
    extractFromContentslist(contentslist, &proceduresList, &variablesList, &propertiesList);

    ListIterator i = proceduresList->newIterator();
    while (i.elementExists())
    {
        DatumPtr procedureText = procedures->procedureFulltext(i.element(), shouldValidate);
        ListIterator j = procedureText.listValue()->newIterator();
        while (j.elementExists())
        {
            QString line = j.element().wordValue()->printValue();
            line.append('\n');
            retval += line;
        }
    }

    i = variablesList->newIterator();
    while (i.elementExists())
    {
        DatumPtr varnameP = i.element();
        QString varname = varnameP.wordValue()->keyValue();
        DatumPtr value = callStack.datumForName(varname);
        if ((value == nothing) && shouldValidate)
        {
            Error::noValue(varnameP);
        }
        else
        {
            QString line = QObject::tr("Make \"%1 %2\n").arg(varname, procedures->printoutDatum(value));
            retval += line;
        }
    }

    i = propertiesList->newIterator();
    while (i.elementExists())
    {
        DatumPtr listnameP = i.element();
        QString listname = listnameP.wordValue()->keyValue();
        DatumPtr proplist = plists.getPropertyList(listname);
        ListIterator j = proplist.listValue()->newIterator();
        while (j.elementExists())
        {
            DatumPtr nameP = j.element();
            DatumPtr valueP = j.element();
            QString line = QObject::tr("Pprop %1 %2 %3\n")
                               .arg(procedures->printoutDatum(listnameP),
                                    procedures->printoutDatum(nameP),
                                    procedures->printoutDatum(valueP));
            retval += line;
        }
    }
    return retval;
}

// SPECIAL VARIABLES

/***DOC LOADNOISILY
LOADNOISILY						(variable)

    if TRUE, prints the names of procedures defined when loading
    from a file (including the temporary file made by EDIT).

COD***/

bool Kernel::varLOADNOISILY()
{
    DatumPtr retvalP = callStack.datumForName(QObject::tr("LOADNOISILY"));
    if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == QObject::tr("TRUE")))
        return true;
    return false;
}

/***DOC ALLOWGETSET
ALLOWGETSET						(variable)

    if TRUE, indicates that an attempt to use a procedure that doesn't
    exist should be taken as an implicit getter or setter procedure
    (setter if the first three letters of the name are SET) for a variable
    of the same name (without the SET if appropriate).

COD***/

bool Kernel::varALLOWGETSET()
{
    DatumPtr retvalP = callStack.datumForName(QObject::tr("ALLOWGETSET"));
    if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == QObject::tr("TRUE")))
        return true;
    return false;
}

/***DOC BUTTONACT
BUTTONACT						(variable)

    if nonempty, should be an instruction list that will be evaluated
    whenever a mouse button is pressed.  Note that the user may have
    released the button before the instructions are evaluated.  BUTTON
    will still output which button was most recently pressed.  CLICKPOS
    will output the position of the mouse cursor at the moment the
    button was pressed; this may be different from MOUSEPOS if the
    user moves the mouse after clicking.

    Note that it's possible for the user to press a button during the
    evaluation of the instruction list.  If this would confuse your
    program, prevent it by temporarily setting BUTTONACT to the empty
    list.  One easy way to do that is the following:

        make "buttonact [button.action]

        to button.action [:buttonact []]
        ... ; whatever you want the button to do
        end

COD***/

DatumPtr Kernel::varBUTTONACT()
{
    return callStack.datumForName(QObject::tr("BUTTONACT"));
}

/***DOC KEYACT
KEYACT							(variable)

    if nonempty, should be an instruction list that will be evaluated
    whenever a key is pressed on the keyboard.  The instruction list
    can use READCHAR to find out what key was pressed.  Note that only
    keys that produce characters qualify; pressing SHIFT or CONTROL
    alone will not cause KEYACT to be evaluated.

    Note that it's possible for the user to press a key during the
    evaluation of the instruction list.  If this would confuse your
    program, prevent it by temporarily setting KEYACT to the empty
    list.  One easy way to do that is the following:

        make "keyact [key.action]

        to key.action [:keyact []]
        ... ; whatever you want the key to do
        end

COD***/

DatumPtr Kernel::varKEYACT()
{
    return callStack.datumForName(QObject::tr("KEYACT"));
}

/***DOC FULLPRINTP
FULLPRINTP						(variable)

    if TRUE, then words that were created using backslash or vertical bar
    (to include characters that would otherwise not be treated as part of
    a word) are printed with the backslashes or vertical bars shown, so
    that the printed result could be re-read by Logo to produce the same
    value.  If FULLPRINTP is TRUE then the empty word (however it was
    created) prints as ||.  (Otherwise it prints as nothing at all.)

COD***/

bool Kernel::varFULLPRINTP()
{
    DatumPtr retvalP = callStack.datumForName(QObject::tr("FULLPRINTP"));
    if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == QObject::tr("TRUE")))
        return true;
    return false;
}

/***DOC PRINTDEPTHLIMIT
PRINTDEPTHLIMIT						(variable)

    if a nonnegative integer, indicates the maximum depth of sublist
    structure that will be printed by PRINT, etc.

COD***/

int Kernel::varPRINTDEPTHLIMIT()
{
    DatumPtr retvalP = callStack.datumForName(QObject::tr("PRINTDEPTHLIMIT"));
    if (retvalP.isWord())
    {
        double retval = retvalP.wordValue()->numberValue();
        if (!std::isnan(retval))
        {
            return (int)retval;
        }
    }
    return -1;
}

/***DOC PRINTWIDTHLIMIT
PRINTWIDTHLIMIT						(variable)

    if a nonnegative integer, indicates the maximum number of members
    in any one list that will be printed by PRINT, etc.

COD***/

int Kernel::varPRINTWIDTHLIMIT()
{
    DatumPtr retvalP = callStack.datumForName(QObject::tr("PRINTWIDTHLIMIT"));
    if (retvalP.isWord())
    {
        double retval = retvalP.wordValue()->numberValue();
        if (!std::isnan(retval))
        {
            return (int)retval;
        }
    }
    return -1;
}

/***DOC STARTUP
STARTUP							(variable)

    if assigned a list value in a file loaded by LOAD, that value is
    run as an instructionlist after the loading.

COD***/

DatumPtr Kernel::varSTARTUP()
{
    return callStack.datumForName(QObject::tr("STARTUP"));
}

/***DOC UNBURYONEDIT
UNBURYONEDIT						(variable)

    if TRUE, causes any procedure defined during EDIT or LOAD to be
    unburied, so that it will be saved by a later SAVE.  Files that
    want to define and bury procedures must do it in that order.

COD***/

bool Kernel::varUNBURYONEDIT()
{
    DatumPtr retvalP = callStack.datumForName(QObject::tr("UNBURYONEDIT"));
    if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == QObject::tr("TRUE")))
        return true;
    return false;
}

/***DOC CASEIGNOREDP
CASEIGNOREDP						(variable)

    if TRUE, indicates that lower case and upper case letters should be
    considered equal by EQUALP, BEFOREP, MEMBERP, etc.  Logo initially
    makes this variable TRUE, and buries it.

COD***/

bool Kernel::varCASEIGNOREDP()
{
    DatumPtr retvalP = callStack.datumForName(QObject::tr("CASEIGNOREDP"));
    if (retvalP.isWord() && (retvalP.wordValue()->keyValue() == QObject::tr("TRUE")))
        return true;
    return false;
}

// PROCEDURE DEFINITION

/***DOC TO
TO procname :input1 :input2 ...				(special form)

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
// CMD TO -1 -1 -1
// CMD .MACRO -1 -1 -1
DatumPtr Kernel::excTo(DatumPtr node)
{
    // None of the children of node are ASTNode. They have to be literal so there
    // is no procedurehelper here.
    if (!callStack.localFrame()->sourceNode.isNothing())
    {
        Error::toInProc(node.astnodeValue()->nodeName);
    }
    parser->inputProcedure(node, systemReadStream);
    return nothing;
}

/***DOC DEFINE
DEFINE procname text

    command.  Defines a procedure with name "procname" and text "text".
    If there is already a procedure with the same name, the new
    definition replaces the old one.  The text input must be a list
    whose members are lists.  The first member is a list of inputs;
    it looks like a TO line but without the word TO, without the
    procedure name, and without the colons before input names.  In
    other words, the members of this first sublist are words for
    the names of required inputs and lists for the names of optional
    or rest inputs.  The remaining sublists of the text input make
    up the body of the procedure, with one sublist for each instruction
    line of the body.  (There is no END line in the text input.)
    It is an error to redefine a primitive procedure.

COD***/
// CMD DEFINE 2 2 2
// CMD .DEFMACRO 2 2 2
DatumPtr Kernel::excDefine(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr text = h.validatedListAtIndex(1, [](DatumPtr candidate) {
        ListIterator iter = candidate.listValue()->newIterator();
        while (iter.elementExists())
        {
            DatumPtr line = iter.element();
            if (!line.isList())
                return false;
        }
        return true;
    });
    DatumPtr cmd = node.astnodeValue()->nodeName;
    DatumPtr procnameP = h.wordAtIndex(0);

    procedures->defineProcedure(cmd, procnameP, text, nothing);

    return nothing;
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
// CMD TEXT 1 1 1
DatumPtr Kernel::excText(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr procnameP = h.wordAtIndex(0);
    return h.ret(procedures->procedureText(procnameP));
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
// CMD FULLTEXT 1 1 1
DatumPtr Kernel::excFulltext(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr procnameP = h.wordAtIndex(0);
    return h.ret(procedures->procedureFulltext(procnameP));
}

/***DOC COPYDEF
COPYDEF newname oldname

    command.  Makes "newname" a procedure identical to "oldname".
    Neither may be a primitive.  If "newname" was already defined,
    its previous definition is lost.

    Note: dialects of Logo differ as to the order of inputs to COPYDEF.
    This dialect uses "MAKE order," not "NAME order."

COD***/
// CMD COPYDEF 2 2 2
DatumPtr Kernel::excCopydef(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr newname = h.wordAtIndex(0);
    DatumPtr oldname = h.wordAtIndex(1);

    procedures->copyProcedure(newname, oldname);

    return nothing;
}

// VARIABLE DEFINITION

/***DOC MAKE
MAKE varname value

    command.  Assigns the value "value" to the variable named "varname",
    which must be a word.  Variable names are case-insensitive.  If a
    variable with the same name already exists, the value of that
    variable is changed.  If not, a new global variable is created.

COD***/
// CMD MAKE 2 2 2
DatumPtr Kernel::excMake(DatumPtr node)
{
    ProcedureHelper h(this, node);

    QString lvalue = h.wordAtIndex(0).wordValue()->keyValue();
    DatumPtr rvalue = h.datumAtIndex(1);

    callStack.setDatumForName(rvalue, lvalue);

    if (callStack.isTraced(lvalue))
    {
        QString line = QObject::tr("Make \"%1 %2\n")
                           .arg(h.wordAtIndex(0).wordValue()->printValue(), procedures->unreadDatum(rvalue));
        sysPrint(line);
    }

    return nothing;
}

DatumPtr Kernel::excSetfoo(DatumPtr node)
{
    ProcedureHelper h(this, node);

    DatumPtr nodeName = node.astnodeValue()->nodeName;
    QString foo = nodeName.wordValue()->keyValue();

    QString lvalue = foo.right(foo.size() - 3);
    DatumPtr rvalue = h.datumAtIndex(0);

    if (!callStack.doesExist(lvalue))
    {
        Error::noHow(nodeName);
    }

    callStack.setDatumForName(rvalue, lvalue);

    if (callStack.isTraced(lvalue.toUpper()))
    {
        QString line = QString("%1 %2\n").arg(node.astnodeValue()->nodeName.wordValue()->printValue(),
                                              procedures->unreadDatum(rvalue));
        sysPrint(line);
    }

    return nothing;
}

DatumPtr Kernel::excFoo(DatumPtr node)
{
    DatumPtr fooP = node.astnodeValue()->nodeName;
    QString foo = fooP.wordValue()->keyValue();

    DatumPtr retval = callStack.datumForName(foo);
    if (retval == nothing)
        return Error::noHowRecoverable(fooP);
    return retval;
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
// TODO: [varname:varnamelist:etc] procedure is duplicated in excGlobal().
// CMD LOCAL 1 1 -1
DatumPtr Kernel::excLocal(DatumPtr node)
{
    ProcedureHelper h(this, node);
    for (int i = 0; i < h.countOfChildren(); ++i)
    {
        DatumPtr var = h.validatedDatumAtIndex(i, [](DatumPtr candidate) {
            if (candidate.isWord())
                return true;
            if (candidate.isList())
            {
                ListIterator j = candidate.listValue()->newIterator();
                while (j.elementExists())
                    if (!j.element().isWord())
                        return false;
                return true;
            }
            return false;
        });
        if (var.isWord())
        {
            makeVarLocal(var.wordValue()->keyValue());
        }
        else
        {
            ListIterator j = var.listValue()->newIterator();
            while (j.elementExists())
            {
                DatumPtr v = j.element();
                makeVarLocal(v.wordValue()->keyValue());
            }
        }
    }
    return nothing;
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
// CMD THING 1 1 1
DatumPtr Kernel::excThing(DatumPtr node)
{
    ProcedureHelper h(this, node);
    QString varName = h.wordAtIndex(0).wordValue()->keyValue();
    DatumPtr retval = h.ret(callStack.datumForName(varName));
    if (retval == nothing)
        return h.ret(Error::noValueRecoverable(h.datumAtIndex(0)));
    return retval;
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
// CMD GLOBAL 1 1 -1
DatumPtr Kernel::excGlobal(DatumPtr node)
{
    ProcedureHelper h(this, node);
    for (int i = 0; i < h.countOfChildren(); ++i)
    {
        DatumPtr var = h.validatedDatumAtIndex(i, [](DatumPtr candidate) {
            if (candidate.isWord())
                return true;
            if (candidate.isList())
            {
                ListIterator j = candidate.listValue()->newIterator();
                while (j.elementExists())
                    if (!j.element().isWord())
                        return false;
                return true;
            }
            return false;
        });
        if (var.isWord())
        {
            callStack.setVarAsGlobal(var.wordValue()->keyValue());
        }
        else
        {
            ListIterator j = var.listValue()->newIterator();
            while (j.elementExists())
            {
                DatumPtr v = j.element();
                callStack.setVarAsGlobal(v.wordValue()->keyValue());
            }
        }
    }
    return nothing;
}

// PROPERTY LISTS

/***DOC PPROP
PPROP plistname propname value

    command.  Adds a property to the "plistname" property list
    with name "propname" and value "value".

COD***/
// CMD PPROP 3 3 3
DatumPtr Kernel::excPprop(DatumPtr node)
{
    ProcedureHelper h(this, node);
    QString plistname = h.wordAtIndex(0).wordValue()->keyValue();
    QString propname = h.wordAtIndex(1).wordValue()->keyValue();
    DatumPtr value = h.datumAtIndex(2);
    plists.addProperty(plistname, propname, value);
    if (plists.isTraced(plistname))
    {
        QString line = QObject::tr("Pprop %1 %2 %3\n")
                           .arg(procedures->unreadDatum(h.datumAtIndex(0)),
                                procedures->unreadDatum(h.datumAtIndex(1)),
                                procedures->unreadDatum(value));
        sysPrint(line);
    }
    return nothing;
}

/***DOC GPROP
GPROP plistname propname

    outputs the value of the "propname" property in the "plistname"
    property list, or the empty list if there is no such property.

COD***/
// CMD GPROP 2 2 2
DatumPtr Kernel::excGprop(DatumPtr node)
{
    ProcedureHelper h(this, node);
    QString plistname = h.wordAtIndex(0).wordValue()->keyValue();
    QString propname = h.wordAtIndex(1).wordValue()->keyValue();
    return h.ret(plists.getProperty(plistname, propname));
}

/***DOC REMPROP
REMPROP plistname propname

    command.  Removes the property named "propname" from the
    property list named "plistname".

COD***/
// CMD REMPROP 2 2 2
DatumPtr Kernel::excRemprop(DatumPtr node)
{
    ProcedureHelper h(this, node);
    QString plistname = h.wordAtIndex(0).wordValue()->keyValue();
    QString propname = h.wordAtIndex(1).wordValue()->keyValue();
    plists.removeProperty(plistname, propname);

    return nothing;
}

/***DOC PLIST
PLIST plistname

    outputs a list whose odd-numbered members are the names, and
    whose even-numbered members are the values, of the properties
    in the property list named "plistname".  The output is a copy
    of the actual property list; changing properties later will not
    magically change a list output earlier by PLIST.


COD***/
// CMD PLIST 1 1 1
DatumPtr Kernel::excPlist(DatumPtr node)
{
    ProcedureHelper h(this, node);
    QString plistname = h.wordAtIndex(0).wordValue()->keyValue();
    return h.ret(plists.getPropertyList(plistname));
}

// PREDICATES

/***DOC PROCEDUREP PROCEDURE?
PROCEDUREP name
PROCEDURE? name

    outputs TRUE if the input is the name of a procedure.

COD***/
// CMD PROCEDUREP 1 1 1
// CMD PROCEDURE? 1 1 1
DatumPtr Kernel::excProcedurep(DatumPtr node)
{
    ProcedureHelper h(this, node);
    bool retval = procedures->isProcedure(h.wordAtIndex(0).wordValue()->keyValue());
    return h.ret(retval);
}

/***DOC PRIMITIVEP PRIMITIVE?
PRIMITIVEP name
PRIMITIVE? name

    outputs TRUE if the input is the name of a primitive procedure
    (one built into Logo).  Note that some of the procedures
    described in this document are library procedures, not primitives.

COD***/
// CMD PRIMITIVEP 1 1 1
// CMD PRIMITIVE? 1 1 1
DatumPtr Kernel::excPrimitivep(DatumPtr node)
{
    ProcedureHelper h(this, node);
    bool retval = procedures->isPrimitive(h.wordAtIndex(0).wordValue()->keyValue());
    return h.ret(retval);
}

/***DOC DEFINEDP DEFINED?
DEFINEDP name
DEFINED? name

    outputs TRUE if the input is the name of a user-defined procedure,
    including a library procedure.

COD***/
// CMD DEFINEDP 1 1 1
// CMD DEFINED? 1 1 1
DatumPtr Kernel::excDefinedp(DatumPtr node)
{
    ProcedureHelper h(this, node);
    bool retval = procedures->isDefined(h.wordAtIndex(0).wordValue()->keyValue());
    return h.ret(retval);
}

/***DOC NAMEP NAME?
NAMEP name
NAME? name

    outputs TRUE if the input is the name of a variable.

COD***/
// CMD NAMEP 1 1 1
// CMD NAME? 1 1 1
DatumPtr Kernel::excNamep(DatumPtr node)
{
    ProcedureHelper h(this, node);
    QString varname = h.wordAtIndex(0).wordValue()->keyValue();
    bool retval = (callStack.doesExist(varname));
    return h.ret(retval);
}

/***DOC PLISTP PLIST?
PLISTP name
PLIST? name

    outputs TRUE if the input is the name of a *nonempty* property list.
    (In principle every word is the name of a property list; if you haven't
    put any properties in it, PLIST of that name outputs an empty list,
    rather than giving an error message.)


COD***/
// CMD PLISTP 1 1 1
// CMD PLIST? 1 1 1
DatumPtr Kernel::excPlistp(DatumPtr node)
{
    ProcedureHelper h(this, node);
    QString listName = h.wordAtIndex(0).wordValue()->keyValue();
    bool retval = plists.isPropertyList(listName);
    return h.ret(retval);
}

// QUERIES

/***DOC CONTENTS
CONTENTS

    outputs a "contents list," i.e., a list of three lists containing
    names of defined procedures, variables, and property lists
    respectively.  This list includes all unburied named items in
    the workspace.

COD***/
// CMD CONTENTS 0 0 0
DatumPtr Kernel::excContents(DatumPtr node)
{
    ProcedureHelper h(this, node);
    return h.ret(buildContentsList(showUnburied));
}

/***DOC BURIED
BURIED

    outputs a contents list including all buried named items in
    the workspace.

COD***/
// CMD BURIED 0 0 0
DatumPtr Kernel::excBuried(DatumPtr node)
{
    ProcedureHelper h(this, node);
    return h.ret(buildContentsList(showBuried));
}

/***DOC TRACED
TRACED

    outputs a contents list including all traced named items in
    the workspace.

COD***/
// CMD TRACED 0 0 0
DatumPtr Kernel::excTraced(DatumPtr node)
{
    ProcedureHelper h(this, node);
    return h.ret(buildContentsList(showTraced));
}

/***DOC STEPPED
STEPPED

    outputs a contents list including all stepped named items in
    the workspace.

COD***/
// CMD STEPPED 0 0 0
DatumPtr Kernel::excStepped(DatumPtr node)
{
    ProcedureHelper h(this, node);
    return h.ret(buildContentsList(showStepped));
}

/***DOC PROCEDURES
PROCEDURES

    outputs a list of the names of all unburied user-defined procedures
    in the workspace.  Note that this is a list of names, not a
    contents list.  (However, procedures that require a contents list
    as input will accept this list.)

COD***/
// CMD PROCEDURES 0 0 0
DatumPtr Kernel::excProcedures(DatumPtr node)
{
    ProcedureHelper h(this, node);
    return h.ret(procedures->allProcedureNames(showUnburied));
}

/***DOC PRIMITIVES
PRIMITIVES

    outputs a list of the names of all primitive procedures
    in the workspace.  Note that this is a list of names, not a
    contents list.  (However, procedures that require a contents list
    as input will accept this list.)

COD***/
// CMD PRIMITIVES 0 0 0
DatumPtr Kernel::excPrimitives(DatumPtr node)
{
    ProcedureHelper h(this, node);
    return h.ret(procedures->allPrimitiveProcedureNames());
}

/***DOC NAMES
NAMES

    outputs a contents list consisting of an empty list (indicating
    no procedure names) followed by a list of all unburied variable
    names in the workspace.

COD***/
// CMD NAMES 0 0 0
DatumPtr Kernel::excNames(DatumPtr node)
{
    ProcedureHelper h(this, node);
    List *retval = new List();
    retval->append(DatumPtr(new List()));
    retval->append(callStack.allVariables(showUnburied));
    return h.ret(retval);
}

/***DOC PLISTS
PLISTS

    outputs a contents list consisting of two empty lists (indicating
    no procedures or variables) followed by a list of all unburied
    nonempty property lists in the workspace.

COD***/
// CMD PLISTS 0 0 0
DatumPtr Kernel::excPlists(DatumPtr node)
{
    ProcedureHelper h(this, node);
    List *retval = new List();
    retval->append(DatumPtr(new List()));
    retval->append(DatumPtr(new List()));
    retval->append(plists.allPLists(showUnburied));
    return h.ret(retval);
}

/***DOC ARITY
ARITY procedurename

    outputs a list of three numbers: the minimum, default, and maximum
    number of inputs for the procedure whose name is the input.  It is an
    error if there is no such procedure.  A maximum of -1 means that the
    number of inputs is unlimited.

COD***/
// CMD ARITY 1 1 1
DatumPtr Kernel::excArity(DatumPtr node)
{
    ProcedureHelper h(this, node);
    return h.ret(procedures->arity(h.wordAtIndex(0)));
}

/***DOC NODES
NODES

    outputs a list of two numbers.  The first represents the number of
    nodes of memory currently in use.  The second shows the maximum
    number of nodes that have been in use at any time since the last
    invocation of NODES.  (A node is a small block of computer memory
    as used by Logo.  Each word (string or number) uses one node. Each list
    or array uses one node plus more nodes for the elements.


COD***/
// CMD NODES 0 0 0
DatumPtr Kernel::excNodes(DatumPtr node)
{
    ProcedureHelper h(this, node);
    return h.ret(nodes());
}

// INSPECTION

/***DOC PRINTOUT PO
PRINTOUT contentslist
PO contentslist

    command.  Prints to the write stream the definitions of all
    procedures, variables, and property lists named in the input
    contents list.

COD***/
// CMD PRINTOUT 1 1 1
// CMD PO 1 1 1
DatumPtr Kernel::excPrintout(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr contentslist;
    h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
        contentslist = contentslistFromDatumPtr(candidate);
        return contentslist != nothing;
    });

    QString output = createPrintoutFromContentsList(contentslist);
    stdPrint(output);

    return nothing;
}

/***DOC POT
POT contentslist

    command.  Prints the title lines of the named procedures and
    the definitions of the named variables and property lists.
    For property lists, the entire list is shown on one line
    instead of as a series of PPROP instructions as in PO.

COD***/
// CMD POT 1 1 1
DatumPtr Kernel::excPot(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr contentslist;
    h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
        contentslist = contentslistFromDatumPtr(candidate);
        return contentslist != nothing;
    });

    List *proceduresList;
    List *variablesList;
    List *propertiesList;
    extractFromContentslist(contentslist, &proceduresList, &variablesList, &propertiesList);

    ListIterator i = proceduresList->newIterator();
    while (i.elementExists())
    {
        QString procedureTitle = procedures->procedureTitle(i.element());
        stdPrint(procedureTitle);
        stdPrint("\n");
    }

    i = variablesList->newIterator();
    while (i.elementExists())
    {
        DatumPtr varnameP = i.element();
        QString varname = varnameP.wordValue()->keyValue();
        DatumPtr value = callStack.datumForName(varname);
        if (value == nothing)
            Error::noValue(varnameP);
        QString line = QObject::tr("Make \"%1 %2\n").arg(varname, procedures->unreadDatum(value));
        stdPrint(line);
    }

    i = propertiesList->newIterator();
    while (i.elementExists())
    {
        DatumPtr listnameP = i.element();
        QString listname = listnameP.wordValue()->keyValue();
        DatumPtr proplist = plists.getPropertyList(listname);
        if (!proplist.listValue()->isEmpty())
        {
            QString line = QObject::tr("Plist %1 = %2\n")
                               .arg(procedures->unreadDatum(listnameP), procedures->unreadDatum(proplist, true));
            stdPrint(line);
        }
    }

    return nothing;
}

// WORKSPACE CONTROL

/***DOC ERASE ER
ERASE contentslist
ER contentslist

    command.  Erases from the workspace the procedures, variables,
    and property lists named in the input.  Primitive procedures may
    not be erased.

COD***/
// CMD ERASE 1 1 1
// CMD ER 1 1 1
DatumPtr Kernel::excErase(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr contentslist;
    h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
        contentslist = contentslistFromDatumPtr(candidate);
        return contentslist != nothing;
    });

    List *proceduresList;
    List *variablesList;
    List *propertiesList;
    extractFromContentslist(contentslist, &proceduresList, &variablesList, &propertiesList);

    ListIterator i = proceduresList->newIterator();
    while (i.elementExists())
    {
        DatumPtr nameP = i.element();
        procedures->eraseProcedure(nameP);
    }

    i = variablesList->newIterator();
    while (i.elementExists())
    {
        QString varname = i.element().wordValue()->keyValue();
        callStack.eraseVar(varname);
    }

    i = propertiesList->newIterator();
    while (i.elementExists())
    {
        DatumPtr listnameP = i.element();
        QString listname = listnameP.wordValue()->keyValue();
        plists.erasePropertyList(listname);
    }

    return nothing;
}

/***DOC ERALL
ERALL

    command.  Erases all unburied procedures, variables, and property
    lists from the workspace.  Abbreviates ERASE CONTENTS.

COD***/
// CMD ERALL 0 0 0
DatumPtr Kernel::excErall(DatumPtr node)
{
    ProcedureHelper h(this, node);
    procedures->eraseAllProcedures();
    callStack.eraseAll();
    plists.eraseAll();

    return nothing;
}

/***DOC ERPS
ERPS

    command.  Erases all unburied procedures from the workspace.
    Abbreviates ERASE PROCEDURES.

COD***/
// CMD ERPS 0 0 0
DatumPtr Kernel::excErps(DatumPtr node)
{
    ProcedureHelper h(this, node);
    procedures->eraseAllProcedures();

    return nothing;
}

/***DOC ERNS
ERNS

    command.  Erases all unburied variables from the workspace.
    Abbreviates ERASE NAMES.

COD***/
// CMD ERNS 0 0 0
DatumPtr Kernel::excErns(DatumPtr node)
{
    ProcedureHelper h(this, node);
    callStack.eraseAll();

    return nothing;
}

/***DOC ERPLS
ERPLS

    command.  Erases all unburied property lists from the workspace.
    Abbreviates ERASE PLISTS.

COD***/
// CMD ERPLS 0 0 0
DatumPtr Kernel::excErpls(DatumPtr node)
{
    ProcedureHelper h(this, node);
    plists.eraseAll();

    return nothing;
}

/***DOC BURY
BURY contentslist

    command.  Buries the procedures, variables, and property lists
    named in the input.  A buried item is not included in the lists
    output by CONTENTS, PROCEDURES, VARIABLES, and PLISTS, but is
    included in the list output by BURIED.  By implication, buried
    things are not printed by POALL or saved by SAVE.

COD***/
// CMD BURY 1 1 1
DatumPtr Kernel::excBury(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr contentslist;
    h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
        contentslist = contentslistFromDatumPtr(candidate);
        return contentslist != nothing;
    });

    processContentsListWithMethod(contentslist, &Workspace::bury);

    return nothing;
}

/***DOC UNBURY
UNBURY contentslist

    command.  Unburies the procedures, variables, and property lists
    named in the input.  That is, the named items will be returned to
    view in CONTENTS, etc.

COD***/
// CMD UNBURY 1 1 1
DatumPtr Kernel::excUnbury(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr contentslist;
    h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
        contentslist = contentslistFromDatumPtr(candidate);
        return contentslist != nothing;
    });

    processContentsListWithMethod(contentslist, &Workspace::unbury);

    return nothing;
}

/***DOC BURIEDP BURIED?
BURIEDP contentslist
BURIED? contentslist

    outputs TRUE if the first procedure, variable, or property list named
    in the contents list is buried, FALSE if not.  Only the first thing in
    the list is tested; the most common use will be with a word as input,
    naming a procedure, but a contents list is allowed so that you can
    BURIEDP [[] [VARIABLE]] or BURIEDP [[] [] [PROPLIST]].

COD***/
// CMD BURIEDP 1 1 1
// CMD BURIED? 1 1 1
DatumPtr Kernel::excBuriedp(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr retval;
    h.validatedDatumAtIndex(0, [&retval, this](DatumPtr candidate) {
        DatumPtr contentslist = contentslistFromDatumPtr(candidate);
        if (contentslist == nothing)
            return false;
        retval = queryContentsListWithMethod(contentslist, &Workspace::isBuried);

        return retval != nothing;
    });

    return h.ret(retval);
}

/***DOC TRACE
TRACE contentslist

    command.  Marks the named items for tracing.  A message is printed
    whenever a traced procedure is invoked, giving the actual input
    values, and whenever a traced procedure STOPs or OUTPUTs.  A
    message is printed whenever a new value is assigned to a traced
    variable using MAKE.  A message is printed whenever a new property
    is given to a traced property list using PPROP.

COD***/
// CMD TRACE 1 1 1
DatumPtr Kernel::excTrace(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr contentslist;
    h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
        contentslist = contentslistFromDatumPtr(candidate);
        return contentslist != nothing;
    });

    processContentsListWithMethod(contentslist, &Workspace::trace);

    return nothing;
}

/***DOC UNTRACE
UNTRACE contentslist

    command.  Turns off tracing for the named items.

COD***/
// CMD UNTRACE 1 1 1
DatumPtr Kernel::excUntrace(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr contentslist;
    h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
        contentslist = contentslistFromDatumPtr(candidate);
        return contentslist != nothing;
    });

    processContentsListWithMethod(contentslist, &Workspace::untrace);

    return nothing;
}

/***DOC TRACEDP TRACED?
TRACEDP contentslist
TRACED? contentslist

    outputs TRUE if the first procedure, variable, or property list named
    in the contents list is traced, FALSE if not.  Only the first thing in
    the list is tested; the most common use will be with a word as input,
    naming a procedure, but a contents list is allowed so that you can
    TRACEDP [[] [VARIABLE]] or TRACEDP [[] [] [PROPLIST]].

COD***/
// CMD TRACEDP 1 1 1
// CMD TRACED? 1 1 1
DatumPtr Kernel::excTracedp(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr retval;
    h.validatedDatumAtIndex(0, [&retval, this](DatumPtr candidate) {
        DatumPtr contentslist = contentslistFromDatumPtr(candidate);
        if (contentslist == nothing)
            return false;
        retval = queryContentsListWithMethod(contentslist, &Workspace::isTraced);

        return retval != nothing;
    });

    return h.ret(retval);
}

/***DOC STEP
STEP contentslist

    command.  Marks the named items for stepping.  Whenever a stepped
    procedure is invoked, each instruction line in the procedure body
    is printed before being executed, and Logo waits for the user to
    type a newline at the terminal.  A message is printed whenever a
    stepped variable name is "shadowed" because a local variable of
    the same name is created either as a procedure input or by the
    LOCAL command.

COD***/
// CMD STEP 1 1 1
DatumPtr Kernel::excStep(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr contentslist;
    h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
        contentslist = contentslistFromDatumPtr(candidate);
        return contentslist != nothing;
    });

    processContentsListWithMethod(contentslist, &Workspace::step);

    return nothing;
}

/***DOC UNSTEP
UNSTEP contentslist

    command.  Turns off stepping for the named items.

COD***/
// CMD UNSTEP 1 1 1
DatumPtr Kernel::excUnstep(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr contentslist;
    h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
        contentslist = contentslistFromDatumPtr(candidate);
        return contentslist != nothing;
    });

    processContentsListWithMethod(contentslist, &Workspace::unstep);

    return nothing;
}

/***DOC STEPPEDP STEPPED?
STEPPEDP contentslist
STEPPED? contentslist

    outputs TRUE if the first procedure, variable, or property list named
    in the contents list is stepped, FALSE if not.  Only the first thing
    in the list is tested; the most common use will be with a word as
    input, naming a procedure, but a contents list is allowed so that you
    can STEPPEDP [[] [VARIABLE]] or STEPPEDP [[] [] [PROPLIST]].

COD***/
// CMD STEPPEDP 1 1 1
// CMD STEPPED? 1 1 1
DatumPtr Kernel::excSteppedp(DatumPtr node)
{
    ProcedureHelper h(this, node);
    DatumPtr retval;
    h.validatedDatumAtIndex(0, [&retval, this](DatumPtr candidate) {
        DatumPtr contentslist = contentslistFromDatumPtr(candidate);
        if (contentslist == nothing)
            return false;
        retval = queryContentsListWithMethod(contentslist, &Workspace::isStepped);

        return retval != nothing;
    });

    return h.ret(retval);
}

/***DOC EDIT ED
EDIT contentslist
ED contentslist
(EDIT)
(ED)

    command.  If invoked with an input, EDIT writes the definitions
    of the named items into a temporary document and edits that document,
    using QLogo's build-in editor.  If invoked without an input, EDIT edits
    the same file left over from a previous EDIT or EDITFILE instruction.
    When you leave the editor, Logo reads the revised definitions and
    modifies the workspace accordingly.  It is not an error if the
    input includes names for which there is no previous definition.

    If there is a variable LOADNOISILY whose value is TRUE, then, after
    leaving the editor, TO commands in the temporary file print "PROCNAME
    defined" (where PROCNAME is the name of the procedure being defined);
    if LOADNOISILY is FALSE or undefined, TO commands in the file are
    carried out silently.

    Exceptionally, the EDIT command can be used without its default
    input and without parentheses provided that nothing follows it on
    the instruction line.

COD***/
// CMD EDIT 0 1 1
// CMD ED 0 1 1
DatumPtr Kernel::excEdit(DatumPtr node)
{
    ProcedureHelper h(this, node);
    if (h.countOfChildren() > 0)
    {
        DatumPtr contentslist;
        h.validatedDatumAtIndex(0, [&contentslist, this](DatumPtr candidate) {
            contentslist = contentslistFromDatumPtr(candidate);
            return contentslist != nothing;
        });

        workspaceText = createPrintoutFromContentsList(contentslist, false);

        editAndRunWorkspaceText();
    }
    else if (editFileName.isWord() && editFileName.wordValue()->printValue() != "")
    {
        editAndRunFile();
    }
    else
    {
        workspaceText = "";
        editAndRunWorkspaceText();
    }

    return nothing;
}

/***DOC EDITFILE
EDITFILE filename

    command.  Starts the Logo editor, like EDIT, but instead of editing
    a temporary document it edits the file specified by the input.  When you
    leave the editor, Logo reads the revised file, as for EDIT.
    EDITFILE also remembers the filename, so that a subsequent EDIT
    command with no input will re-edit the same file.

    EDITFILE is intended as an alternative to LOAD and SAVE.  You can
    maintain a workspace file yourself, controlling the order in which
    definitions appear, maintaining comments in the file, and so on.

COD***/
// CMD EDITFILE 1 1 1
DatumPtr Kernel::excEditfile(DatumPtr node)
{
    ProcedureHelper h(this, node);
    editFileName = h.wordAtIndex(0);
    editAndRunFile();
    return nothing;
}

/***DOC SAVE
SAVE filename

    command.  Saves the definitions of all unburied procedures,
    variables, and nonempty property lists in the named file.
    Equivalent to

            to save :filename
            local "oldwriter
            make "oldwriter writer
            openwrite :filename
            setwrite :filename
            poall
            setwrite :oldwriter
            close :filename
            end

    Exceptionally, SAVE can be used with no input and without parentheses
    if it is the last thing on the command line.  In this case, the
    filename from the most recent LOAD or SAVE command will be used.  (It
    is an error if there has been no previous LOAD or SAVE.)

COD***/
// CMD SAVE 1 1 1
DatumPtr Kernel::excSave(DatumPtr node)
{
    ProcedureHelper h(this, node);
    if (h.countOfChildren() > 0)
    {
        editFileName = h.wordAtIndex(0);
    }
    else
    {
        if (editFileName == nothing)
            Error::notEnough(node.astnodeValue()->nodeName);
    }

    QString filepath = filepathForFilename(editFileName.wordValue());
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        Error::cantOpen(editFileName);
    }

    DatumPtr contentList = buildContentsList(showUnburied);
    QString fileText = createPrintoutFromContentsList(contentList);

    file.seek(0);
    file.resize(0);
    QTextStream out(&file);
    out << fileText;

    return nothing;
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
// CMD LOAD 1 1 1
DatumPtr Kernel::excLoad(DatumPtr node)
{
    ProcedureHelper h(this, node);
    editFileName = h.wordAtIndex(0);
    DatumPtr oldStartup = varSTARTUP();
    DatumPtr retval;

    QString filepath = filepathForFilename(editFileName.wordValue());
    QFile file(filepath);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        Error::cantOpen(editFileName);
    }
    QTextStream in(&file);
    QString fileText = in.readAll();
    QString output = executeText(fileText);
    if (varLOADNOISILY())
    {
        sysPrint(output);
    }
    DatumPtr startup = varSTARTUP();
    if (oldStartup != startup)
    {
        if (startup.isWord() || startup.isList())
            retval = runList(startup);
    }

    return h.ret(retval);
}

/***DOC HELP
HELP name
(HELP)

    command.  Prints information from the reference manual about
    the primitive procedure named by the input.  With no input,
    lists all the primitives about which help is available.

    Exceptionally, the HELP command can be used without its default
    input and without parentheses provided that nothing follows it on
    the instruction line.

COD***/

/* TODO:
    If HELP is called with the name of a defined procedure for which there
    is no help file, it will print the title line of the procedure
    followed by lines from the procedure body that start with semicolon,
    stopping when a non-semicolon line is seen.
*/
// CMD HELP 0 -1 -1
DatumPtr Kernel::excHelp(DatumPtr node)
{
    ProcedureHelper h(this, node);

    if (h.countOfChildren() == 0)
    {
        QStringList cmds = help.allCommands();
        stdPrint(cmds.join(" ") + "\n");
    }
    else
    {
        DatumPtr cmdP = h.wordAtIndex(0);
        QString text = help.helpText(cmdP.wordValue()->keyValue());
        if (text.size() < 1)
            Error::noHow(cmdP);
        stdPrint(text);
    }
    return nothing;
}
