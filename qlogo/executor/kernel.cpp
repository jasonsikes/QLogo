
//===-- qlogo/kernel.cpp - Kernel class implementation -------*- C++ -*-===//
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
/// This file contains a part of the implementation of the Kernel class, which is the
/// executor proper of the QLogo language. Specifically, this file contains the
/// kernel methods that support and maintain the state of QLogo execution.
///
/// See README.md in this directory for information about the documentation
/// structure for each Kernel::exc* method.
///
//===----------------------------------------------------------------------===//

#include "kernel.h"
#include "astnode.h"
#include "datum.h"
#include "parser.h"
#include <QApplication> // quit()
#include <QColor>
#include <QFont>
#include <QImage>
#include <stdlib.h> // arc4random_uniform()

#include "error.h"
#include "runparser.h"
#include "turtle.h"

#include "controller/logocontroller.h"

// The maximum depth of procedure iterations before error is thrown.
const int maxIterationDepth = 1000;

StreamRedirect::StreamRedirect(TextStream *newReadStream, TextStream *newWriteStream, Parser *newParser)
{
    originalWriteStream = Config::get().mainKernel()->writeStream;
    originalSystemWriteStream = Config::get().mainKernel()->systemWriteStream;
    originalReadStream = Config::get().mainKernel()->readStream;
    originalSystemReadStream = Config::get().mainKernel()->systemReadStream;
    originalParser = Config::get().mainKernel()->parser;

    Config::get().mainKernel()->writeStream = newWriteStream;
    Config::get().mainKernel()->systemWriteStream = newWriteStream;
    Config::get().mainKernel()->readStream = newReadStream;
    Config::get().mainKernel()->systemReadStream = newReadStream;
    Config::get().mainKernel()->parser = newParser;
}

StreamRedirect::~StreamRedirect()
{
    Config::get().mainKernel()->parser = originalParser;
    Config::get().mainKernel()->writeStream = originalWriteStream;
    Config::get().mainKernel()->readStream = originalReadStream;
    Config::get().mainKernel()->systemWriteStream = originalSystemWriteStream;
    Config::get().mainKernel()->systemReadStream = originalSystemReadStream;
}

// This doesn't do anything or get called. It's just a token that gets passed
// when GOTO is used
DatumPtr Kernel::excGotoToken(DatumPtr)
{
    return nothing;
}

bool Kernel::isInputRedirected()
{
    return readStream != stdioStream;
}

bool Kernel::numbersFromList(QVector<double> &retval, DatumPtr l)
{
    if (!l.isList())
        return false;
    ListIterator iter = l.listValue()->newIterator();

    retval.clear();
    while (iter.elementExists())
    {
        DatumPtr n = iter.element();
        if (!n.isWord())
            return false;
        double v = n.wordValue()->numberValue();
        if (std::isnan(v))
            return false;
        retval.push_back(v);
    }
    return true;
}

bool Kernel::colorFromDatumPtr(QColor &retval, DatumPtr colorP)
{
    if (colorP.isWord())
    {
        double colorNum = colorP.wordValue()->numberValue();
        if (!std::isnan(colorNum))
        {
            if ((colorNum != round(colorNum)) || (colorNum < 0) || (colorNum >= palette.size()))
                return false;
            retval = palette[colorNum];
            if (!retval.isValid())
                retval = palette[0];
            return true;
        }
        retval = QColor(colorP.wordValue()->printValue().toLower());
        return retval.isValid();
    }
    else if (colorP.isList())
    {
        QVector<double> v;
        if (!numbersFromList(v, colorP.listValue()))
            return false;
        if ((v.size() != 3) && (v.size() != 4))
            return false;
        for (int i = 0; i < v.size(); ++i)
        {
            if ((v[i] < 0) || (v[i] > 100))
                return false;
            v[i] *= 255.0 / 100;
        }
        double alpha = (v.size() == 4) ? v[3] : 255;
        retval = QColor(v[0], v[1], v[2], alpha);
        return true;
    }
    return false;
}

bool Kernel::getLineAndRunIt(bool shouldHandleError)
{
    QString prompt;
    Q_ASSERT(callStack.size() > 0);
    if (callStack.localFrame()->sourceNode.isASTNode())
    {
        prompt = callStack.localFrame()->sourceNode.astnodeValue()->nodeName.printValue();
    }
    prompt += "? ";

    try
    {
        DatumPtr line = systemReadStream->readlistWithPrompt(prompt, true);
        if (line == nothing)
            return false; // EOF
        if (line.listValue()->isEmpty())
            return true;

        DatumPtr result = runList(line);
        if (result != nothing)
            Error::dontSay(result);
    }
    catch (Error *e)
    {
        if (shouldHandleError)
        {
            if (e->tag.isWord())
            {
                if (e->tag.wordValue()->keyValue() == QObject::tr("TOPLEVEL"))
                {
                    sysPrint("\n");
                    return true;
                }
                if (e->tag.wordValue()->keyValue() == QObject::tr("SYSTEM"))
                {
                    sysPrint("\n");
                    Config::get().mainController()->systemStop();
                    return false;
                }
            }
            sysPrint(e->errorText.printValue());
            if (e->procedure != nothing)
                sysPrint(QString(" in ") + e->procedure.astnodeValue()->nodeName.printValue());
            sysPrint("\n");
            if (e->instructionLine != nothing)
            {
                sysPrint(procedures->unreadDatum(e->instructionLine, true));
                sysPrint("\n");
            }
            registerError(nothing);
        }
        else
        {
            throw e;
        }
    }
    return true;
}

/***DOC ERRACT
ERRACT							(variable)

    When set to a value that is not "False"false" nor an empty list,
    the command interpreter will execute PAUSE to enable the user to
    inspect the state of the program.


COD***/

DatumPtr Kernel::registerError(DatumPtr anError, bool allowErract, bool allowRecovery)
{
    currentError = anError;
    ProcedureHelper::setIsErroring(anError != nothing);
    if (anError != nothing)
    {
        Error *e = currentError.errorValue();

        // An error with a message shifts the blame to the calling method.
        if ((e->code == ERR_CUSTOM_THROW) && (callStack.size() > 1))
        {
            CallFrame *frame = callStack.parentFrame();
            e->procedure = frame->sourceNode;
            if (!e->procedure.isNothing())
                e->instructionLine = frame->localEvaluator()->list;
            else
                e->instructionLine = nothing;
        }
        else
        {
            Q_ASSERT(callStack.size() > 0);
            CallFrame *frame = callStack.localFrame();
            e->procedure = frame->sourceNode;
            if (!e->procedure.isNothing())
                e->instructionLine = currentLine;
            else
                e->instructionLine = nothing;
        }

        // TODO: need varERRACT()
        DatumPtr erractP = callStack.datumForName(QObject::tr("ERRACT"));
        bool shouldPause = (!callStack.globalFrame()->sourceNode.isNothing()) &&
                           ((erractP.isList() && (!erractP.listValue()->isEmpty())) ||
                            (erractP.isWord() && (erractP.wordValue()->rawValue().size() > 0)));

        if (allowErract && shouldPause)
        {
            sysPrint(e->errorText.printValue());
            sysPrint("\n");
            ProcedureHelper::setIsErroring(false);
            currentError = nothing;

            DatumPtr retval = pause();

            if (retval == nothing)
                Error::throwError(DatumPtr(QObject::tr("TOPLEVEL")), nothing);
            if (allowRecovery)
            {
                return retval;
            }
            sysPrint(QObject::tr("You don't say what to do with %1").arg(retval.printValue()));
            return nothing;
        }
        else
        {
            throw anError.errorValue();
        }
    }
    return nothing;
}

void Kernel::initPalette()
{
    const int paletteSize = 101;
    palette.clear();
    palette.reserve(paletteSize);
    palette.push_back(QColor(QStringLiteral("black")));       // 0
    palette.push_back(QColor(QStringLiteral("blue")));        // 1
    palette.push_back(QColor(QStringLiteral("green")));       // 2
    palette.push_back(QColor(QStringLiteral("cyan")));        // 3
    palette.push_back(QColor(QStringLiteral("red")));         // 4
    palette.push_back(QColor(QStringLiteral("magenta")));     // 5
    palette.push_back(QColor(QStringLiteral("yellow")));      // 6
    palette.push_back(QColor(QStringLiteral("white")));       // 7
    palette.push_back(QColor(QStringLiteral("brown")));       // 8
    palette.push_back(QColor(QStringLiteral("tan")));         // 9
    palette.push_back(QColor(QStringLiteral("forestgreen"))); // 10
    palette.push_back(QColor(QStringLiteral("aqua")));        // 11
    palette.push_back(QColor(QStringLiteral("salmon")));      // 12
    palette.push_back(QColor(QStringLiteral("purple")));      // 13
    palette.push_back(QColor(QStringLiteral("orange")));      // 14
    palette.push_back(QColor(QStringLiteral("grey")));        // 15
    palette.resize(paletteSize);
}

// Some Logo vars are set here and not used anywhere else.
// Documentation is here because it doesn't fit anywhere else.

/***DOC LOGOPLATFORM
LOGOPLATFORM						(variable)

    one of the following words: OSX, WINDOWS, or UNIX.


COD***/

/***DOC LOGOVERSION
LOGOVERSION						(variable)

    a real number indicating the Logo version number, e.g., 5.5

COD***/

/***DOC COMMANDLINE
COMMANDLINE						(variable)

    contains all text on the command line used to start Logo.

COD***/

void Kernel::initVariables(void)
{
    DatumPtr platform(LOGOPLATFORM);
    DatumPtr version(LOGOVERSION);
    DatumPtr trueDatumPtr(QObject::tr("true"));

    callStack.setDatumForName(platform, QObject::tr("LOGOPLATFORM"));
    callStack.setDatumForName(version, QObject::tr("LOGOVERSION"));
    callStack.setDatumForName(trueDatumPtr, QObject::tr("ALLOWGETSET"));
    callStack.bury(QObject::tr("LOGOPLATFORM"));
    callStack.bury(QObject::tr("LOGOVERSION"));
    callStack.bury(QObject::tr("ALLOWGETSET"));

    List *argv = new List;
    for (auto &arg : Config::get().ARGV)
    {
        argv->append(DatumPtr(arg));
    }
    DatumPtr commandLine(argv);
    callStack.setDatumForName(commandLine, QObject::tr("COMMANDLINE"));
    callStack.bury(QObject::tr("COMMANDLINE"));
}

Kernel::Kernel()
{
    Config::get().setMainKernel(this);
    stdioStream = new TextStream(NULL);
    readStream = stdioStream;
    systemReadStream = stdioStream;
    writeStream = stdioStream;
    systemWriteStream = stdioStream;

    turtle = new Turtle;
    procedures = new Procedures;
    parser = new Parser;

    // callStack holds a pointer to the new frame so it will be deleted when this
    // Kernel is deleted.
    new CallFrame(&callStack);

    initVariables();
    initPalette();

    filePrefix = new List();
}

Kernel::~Kernel()
{
    closeAll();
    delete parser;
    delete procedures;
    delete turtle;

    Q_ASSERT(callStack.size() == 1);
    callStack.stack.removeLast();
    Config::get().setMainKernel(NULL);
}

void Kernel::makeVarLocal(const QString &varname)
{
    if (callStack.size() <= 1)
        return;
    if (callStack.isStepped(varname))
    {
        QString line = varname + QObject::tr(" shadowed by local in procedure call");
        if ((callStack.size() > 1) && (!callStack.parentFrame()->sourceNode.isNothing()))
        {
            line += " in " + callStack.parentFrame()->sourceNode.astnodeValue()->nodeName.wordValue()->printValue();
        }
        sysPrint(line + "\n");
    }
    callStack.setVarAsLocal(varname);
}

DatumPtr Kernel::executeProcedureCore(DatumPtr node)
{
    ProcedureHelper h(this, node);
    // The first child is the body of the procedure
    DatumPtr proc = h.datumAtIndex(0);

    // The remaining children are the parameters
    int childIndex = 1;

    // first assign the REQUIRED params
    QList<QString> &requiredInputs = proc.procedureValue()->requiredInputs;
    for (auto &name : requiredInputs)
    {
        DatumPtr value = h.datumAtIndex(childIndex);
        ++childIndex;
        makeVarLocal(name);
        callStack.setDatumForName(value, name);
    }

    // then assign the OPTIONAL params
    QList<QString> &optionalInputs = proc.procedureValue()->optionalInputs;
    QList<DatumPtr> &optionalDefaults = proc.procedureValue()->optionalDefaults;

    auto defaultIter = optionalDefaults.begin();
    for (auto &name : optionalInputs)
    {
        DatumPtr value;
        if (childIndex < h.countOfChildren())
        {
            value = h.datumAtIndex(childIndex);
            ++childIndex;
        }
        else
        {
            // The first element is the name of the default, ignore it unless there
            // is an error. The tail is the default expression.
            DatumPtr defaultList = *defaultIter;
            DatumPtr defaultValue = defaultList.listValue()->tail;
            QList<DatumPtr> *parsedList = parser->astFromList(defaultValue.listValue());
            if (parsedList->size() != 1)
                Error::badDefaultExpression(defaultList);

            value = runList(defaultValue);
        }
        makeVarLocal(name);
        callStack.setDatumForName(value, name);
        ++defaultIter;
    }

    // Finally, take in the remainder (if any) as a list.
    if (proc.procedureValue()->restInput != "")
    {
        const QString &name = proc.procedureValue()->restInput;
        DatumPtr remainderList = DatumPtr(new List());
        while (childIndex < h.countOfChildren())
        {
            DatumPtr value = h.datumAtIndex(childIndex);
            remainderList.listValue()->append(value);
            ++childIndex;
        }
        makeVarLocal(name);
        callStack.setDatumForName(remainderList, name);
    }

    // Execute the commands in the procedure.

    DatumPtr retval;
    {
        ListIterator iter = proc.procedureValue()->instructionList.listValue()->newIterator();
        bool isStepped = procedures->isStepped(node.astnodeValue()->nodeName.wordValue()->keyValue());
        while (iter.elementExists() && (retval == nothing))
        {
            currentLine = iter.element();
            if (isStepped)
            {
                QString line = h.indent() + procedures->unreadDatum(currentLine, true);
                sysPrint(line);
                systemReadStream->readrawlineWithPrompt(" >>>");
            }
            retval = runList(currentLine);
            if (retval.isASTNode())
            {
                ASTNode *a = retval.astnodeValue();
                if (a->kernel == &Kernel::excGotoToken)
                {
                    QString tag = a->childAtIndex(0).wordValue()->keyValue();
                    DatumPtr startingLine = proc.procedureValue()->tagToLine[tag];
                    iter = proc.procedureValue()->instructionList.listValue()->newIterator();
                    while (iter.elementExists() && (currentLine != startingLine))
                    {
                        currentLine = iter.element();
                    }
                    retval = runList(currentLine, tag);
                }
            }
        }
    }

    if ((retval != nothing) && !retval.isASTNode())
        Error::dontSay(retval);

    if (h.isTraced && retval.isASTNode())
    {
        KernelMethod method = retval.astnodeValue()->kernel;
        if (method == &Kernel::excStop)
        {
            if (retval.astnodeValue()->countOfChildren() > 0)
            {
                retval = retval.astnodeValue()->childAtIndex(0);
                if (retval != nothing)
                {
                    Error::dontSay(retval);
                }
            }
            else
            {
                retval = nothing;
            }
        }
        else if (method == &Kernel::excOutput)
        {
            DatumPtr p = retval.astnodeValue()->childAtIndex(0);
            KernelMethod temp_method = p.astnodeValue()->kernel;
            DatumPtr temp_retval = (this->*temp_method)(p);
            if (temp_retval == nothing)
                Error::didntOutput(p.astnodeValue()->nodeName, retval.astnodeValue()->nodeName);
            retval = temp_retval;
        }
        else if (method == &Kernel::excDotMaybeoutput)
        {
            retval = retval.astnodeValue()->childAtIndex(0);
        }
        else
        {
            retval = (this->*method)(retval);
        } // /if method == ...
    }
    return h.ret(retval);
}

DatumPtr Kernel::executeProcedure(DatumPtr node)
{
    if (callStack.size() > maxIterationDepth)
    {
        Error::stackOverflow();
    }

    CallFrame cf(&callStack, node.astnodeValue());

    DatumPtr retval = executeProcedureCore(node);
    ASTNode *lastOutputCmd = NULL;

    while (retval.isASTNode())
    {
        KernelMethod method = retval.astnodeValue()->kernel;
        if ((method == &Kernel::excOutput) || (method == &Kernel::excDotMaybeoutput) ||
            ((method == &Kernel::excStop) && (retval.astnodeValue()->countOfChildren() > 0)))
        {
            if (method == &Kernel::excOutput)
            {
                lastOutputCmd = retval.astnodeValue();
            }
            node = retval.astnodeValue()->childAtIndex(0);
            method = node.astnodeValue()->kernel;

            // if the output is a procedure, then trampoline
            if (method == &Kernel::executeProcedure)
            {
                retval = executeProcedureCore(node);
            }
            else
            {
                retval = (this->*method)(node);
            }
            if ((retval == nothing) && (lastOutputCmd != NULL))
            {
                Error::didntOutput(node.astnodeValue()->nodeName, lastOutputCmd->nodeName);
            }
        }
        else if (method == &Kernel::excStop)
        {
            if (lastOutputCmd == NULL)
            {
                return nothing;
            }
            else
            {
                Error::didntOutput(node.astnodeValue()->nodeName, lastOutputCmd->nodeName);
            }
        }
        else
        {
            retval = (this->*method)(retval);
        }
    } // /while isASTNode

    return retval;
}

DatumPtr Kernel::executeMacro(DatumPtr node)
{
    DatumPtr retval;
tailCall:
    node = executeProcedure(node);
    if (!node.isList())
    {
        return Error::macroReturned(node);
    }

    // The result is a list, which means we will execute it, similar to
    // runList(), but without tags and watching for tail recursion.
    QList<DatumPtr> *parsedList = parser->astFromList(node.listValue());

    Evaluator e(node, callStack.localFrame()->evalStack);

    for (int i = 0; i < parsedList->size(); ++i)
    {
        if (!retval.isNothing())
        {
            if (retval.isASTNode())
            {
                // TODO: need a new Error
                Error::insideRunresult(retval.astnodeValue()->nodeName);
            }
            Error::dontSay(retval);
        }
        DatumPtr statement = (*parsedList)[i];
        KernelMethod method = statement.astnodeValue()->kernel;
        if (method == &Kernel::executeMacro)
        {
            // This is a tail macro call.
            node = statement;
            goto tailCall;
        }
        retval = (this->*method)(statement);
    }
    return retval;
}

ASTNode *Kernel::astnodeValue(DatumPtr caller, DatumPtr value)
{
    if (!value.isASTNode())
        Error::doesntLike(caller.astnodeValue()->nodeName, value);
    return value.astnodeValue();
}

DatumPtr Kernel::executeLiteral(DatumPtr node)
{
    return node.astnodeValue()->childAtIndex(0);
}

DatumPtr Kernel::executeValueOf(DatumPtr node)
{
    DatumPtr varnameP = node.astnodeValue()->childAtIndex(0);
    QString varName = varnameP.wordValue()->keyValue();
    DatumPtr retval = callStack.datumForName(varName);
    if (retval == nothing)
        return (Error::noValueRecoverable(varnameP));
    return retval;
}

SignalsEnum_t Kernel::interruptCheck()
{
    SignalsEnum_t latestSignal = Config::get().mainController()->latestSignal();
    if (latestSignal == toplevelSignal)
    {
        if (callStack.globalFrame()->sourceNode != NULL)
            Error::throwError(DatumPtr(QObject::tr("TOPLEVEL")), nothing);
    }
    else if (latestSignal == pauseSignal)
    {
        if (callStack.globalFrame()->sourceNode != NULL)
            pause();
    }
    else if (latestSignal == systemSignal)
    {
        Error::throwError(DatumPtr(QObject::tr("SYSTEM")), nothing);
    }
    return latestSignal;
}

DatumPtr Kernel::runList(DatumPtr listP, QString startTag)
{
    bool tagHasBeenFound = (startTag.isNull());
    DatumPtr retval;

    interruptCheck();

    if (listP.isWord())
        listP = runparse(listP);

    if (!listP.isList())
    {
        Error::noHow(listP);
    }

    Q_ASSERT(callStack.size() > 0);
    Evaluator e(listP, callStack.localFrame()->evalStack);

    QList<DatumPtr> *parsedList = parser->astFromList(listP.listValue());
    for (int i = 0; i < parsedList->size(); ++i)
    {
        if (!retval.isNothing())
        {
            if (retval.isASTNode())
            {
                return retval;
            }
            Error::dontSay(retval);
        }
        DatumPtr statement = (*parsedList)[i];
        KernelMethod method = statement.astnodeValue()->kernel;
        if (tagHasBeenFound)
        {
            retval = (this->*method)(statement);
        }
        else
        {
            if (method == &Kernel::excTag)
            {
                ASTNode *child = statement.astnodeValue()->childAtIndex(0).astnodeValue();
                if (child->kernel == &Kernel::executeLiteral)
                {
                    DatumPtr v = child->childAtIndex(0);
                    if (v.isWord())
                    {
                        QString tag = v.wordValue()->keyValue();
                        tagHasBeenFound = (startTag == tag);
                    }
                }
            }
        }
    }

    return retval;
}

/***DOC WAIT
WAIT time

    command.  Delays further execution for "time" 60ths of a second.
    Also causes any buffered characters destined for the terminal to
    be printed immediately.  WAIT 0 can be used to achieve this
    buffer flushing without actually waiting.

COD***/
// CMD WAIT 1 1 1
DatumPtr Kernel::excWait(DatumPtr node)
{
    ProcedureHelper h(this, node);
    double value = h.validatedNumberAtIndex(0, [](double candidate) { return candidate >= 0; });
    Config::get().mainController()->mwait((1000.0 / 60) * value);
    return nothing;
}

DatumPtr Kernel::excNoop(DatumPtr node)
{
    ProcedureHelper h(this, node);
    return h.ret();
}

DatumPtr Kernel::excErrorNoGui(DatumPtr node)
{
    ProcedureHelper h(this, node);
    Error::noGraphics();
    return h.ret();
}

DatumPtr Kernel::pause()
{
    if (isPausing)
    {
        sysPrint(QObject::tr("Already Pausing"));
        return nothing;
    }

    // TODO: PAUSE and getLineAndRunIt should not be procedure but evalStack entry.
    isPausing = true;
    StreamRedirect streamScope(stdioStream, stdioStream, parser);

    sysPrint(QObject::tr("Pausing...\n"));

    forever
    {
        try
        {
            bool shouldContinue = true;
            while (shouldContinue)
            {
                shouldContinue = getLineAndRunIt(false);
            }
        }
        catch (Error *e)
        {
            if ((e->code == 14) && (e->tag.wordValue()->keyValue() == QObject::tr("PAUSE")))
            {
                DatumPtr retval = e->output;
                registerError(nothing);
                isPausing = false;
                return retval;
            }
            if ((e->code == 14) && ((e->tag.wordValue()->keyValue() == QObject::tr("TOPLEVEL")) ||
                                    (e->tag.wordValue()->keyValue() == QObject::tr("SYSTEM"))))
            {
                isPausing = false;
                throw e;
            }
            sysPrint(e->errorText.printValue());
            sysPrint("\n");
            registerError(nothing);
        }
    }
    isPausing = false;
    return nothing;
}
