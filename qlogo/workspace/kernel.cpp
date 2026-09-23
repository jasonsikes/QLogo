
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

#include "workspace/kernel.h"
#include "astnode.h"
#include "compiler.h"
#include "interface/streammanager.h"
#include "interface/textstream.h"
#include "datum_types.h"
#include "flowcontrol.h"
#include "sharedconstants.h"
#include "treeifyer.h"
#include "workspace/procedures.h"
#include "workspace/specialvars.h"
#include "runparser.h"
#include "workspace/turtle.h"
#include "interface/logointerface.h"
#include <QCoreApplication> // quit()
#include <QColor>
#include <QDebug>
#include <QDir>
#include <QFont>
#include <QImage>
#include <iostream>
#include <cstdlib> // arc4random_uniform()


#ifdef QLOGO_DEBUG
void ece_trace(const QString &msg)
{
    static bool doTrace = Config::get().traceEvaluator_;
    if (doTrace)
    {
        qInfo() << "ece_trace: " << msg;
    }
}
#else
#define ece_trace(msg) ((void)0)
#endif

bool Kernel::numbersFromList(QVector<double> &retval, const DatumPtr &listP) const
{
    if (!listP.isList())
        return false;
    ListIterator iter = listP.listValue()->newIterator();

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

bool Kernel::colorFromDatumPtr(QColor &retval, const DatumPtr &colorP) const
{
    if (colorP.isWord())
    {
        double colorNum = colorP.wordValue()->numberValue();
        if (colorP.wordValue()->numberIsValid)
        {
            if ((colorNum != round(colorNum)) || (colorNum < 0) || (colorNum >= palette_.size()))
                return false;
            retval = palette_[colorNum];
            if (!retval.isValid())
                retval = palette_[0];
            return true;
        }
        retval = QColor(colorP.wordValue()->toString().toLower());
        return retval.isValid();
    }
    else if (colorP.isList())
    {
        QVector<double> v;
        if (!numbersFromList(v, colorP.listValue()))
            return false;
        if ((v.size() != 3) && (v.size() != 4))
            return false;
        for (double &i : v)
        {
            if ((i < 0) || (i > 100))
                return false;
            i *= 255.0 / 100;
        }
        double alpha = (v.size() == 4) ? v[3] : 255;
        retval = QColor(v[0], v[1], v[2], alpha);
        return true;
    }
    return false;
}

DatumPtr Kernel::readEvalPrintLoop(bool isPausing, const QString &prompt)
{
    ReplScope repl;
    callFrameStack_.emplace_back(nullptr, nullptr, 0);
    QString localPrompt = prompt + "? ";
    DatumPtr result;
    forever
    {
        try
        {
            DatumPtr line = StreamManager::get().readProgramList(localPrompt, true);

            Q_ASSERT(callFrameStack_.back().evaluationStackSize() == 0);
            Q_ASSERT(callFrameStack_.back().sourceNode_.isNothing());

            if (line.isNothing())
            {
                 // EOF
                result = nothing();
                goto bailout;
            }
            callFrameStack_.back().pushEvaluator(line);
            result = runECE();
        }
        catch (FCError *e)
        {
            // While FCError objects can be returned from an execution in the form of a DatumPtr,
            // sometimes exceptions are thrown by the parser.
            // When that happens, wrap it into a DatumPtr.
            result = DatumPtr(e);
        }

        if ((result.datumValue()->isa_ & Datum::typeDataMask) != 0)
        {
            result = DatumPtr(FCError::dontSay(result));
        }

        if (result.isFlowControl())
        {
            // Flow control instructions are not allowed in the REPL.
            DatumPtr blame;

            if (result.flowControlValue()->isa_ == Datum::typeReturn)
            {
                blame = result.flowControlValue()->sourceNode_.astnodeValue()->nodeName_;
                result = DatumPtr(FCError::notInsideProcedure(blame));
            }
            // TODO: other flow control types
        }

        if (result.isErr())
        {
            FCError *e = result.errValue();
            if (e->tag().isWord() && (e->code_ == ErrCode::ERR_NO_CATCH))
            {
                if (e->tag().toString(Datum::ToStringFlags_Key) == QObject::tr("TOPLEVEL"))
                {
                    StreamManager::get().printToTerminal("\n");
                    continue;
                }
                if (e->tag().toString(Datum::ToStringFlags_Key) == QObject::tr("SYSTEM"))
                {
                    StreamManager::get().printToTerminal("\n");
                    Config::get().mainInterface()->closeInterface();
                    QCoreApplication::quit();
                    result = nothing();
                    goto bailout;
                }
                if (e->tag().toString(Datum::ToStringFlags_Key) == QObject::tr("PAUSE") && isPausing)
                {
                    result = e->output();
                    goto bailout;
                }
            }
            StreamManager::get().printToTerminal(e->toString() + "\n");
            continue;
        }
    }

bailout:
    callFrameStack_.pop_back();
    return result;
}

DatumPtr Kernel::procnameFromNode(ASTNode *node)
{
    DatumPtr command = node->nodeName_;

    DatumPtr procnameP = node->childAtIndex(0);
    if (!procnameP.isWord())
        throw FCError::doesntLike(command, procnameP);

    procnameP.wordValue()->numberValue();
    if (procnameP.wordValue()->numberIsValid)
        throw FCError::doesntLike(command, procnameP);

    QString procname = procnameP.toString(Datum::ToStringFlags_Key);

    QChar firstChar = (procname)[0];
    if ((firstChar == '"') || (firstChar == ':') || (firstChar == '(') || (firstChar == ')'))
        throw FCError::doesntLike(command, procnameP);

    return procnameP;
}

DatumPtr Kernel::procParametersFromNode(ASTNode *node)
{
    ListBuilder paramListBuilder;
    for (int i = 1; i < node->countOfChildren(); ++i)
    {
        paramListBuilder.append(Procedures::cleanFormal(node->nodeName_, node->childAtIndex(i)));
    }
    return paramListBuilder.finishedList();
}

Datum *Kernel::inputProcedure(ASTNode *node)
{
    Datum *retval = node;
    try
    {
        // command is the first word in the input line, (".MACRO" or "TO").
        DatumPtr command = node->nodeName_;
        if (node->countOfChildren() == 0)
            throw FCError::notEnoughInputs(command);

        // procnameP is the name of the procedure, the second word in the input line,
        // following ".MACRO" or "TO".
        DatumPtr procnameP = procnameFromNode(node);
        QString procname = procnameP.toString(Datum::ToStringFlags_Key);

        // TODO: Enable conditional redefinition of procedures, such as after editing.
        if (Procedures::get().isProcedure(procname))
            throw FCError::procDefined(procnameP);

        // Assign the procedure's parameter names and default values.
        DatumPtr parameterList = procParametersFromNode(node);

        Procedures::get().validateParameters(command, parameterList);

        QList<DatumPtr> sourceText = StreamManager::get().programReaderLineHistory();
        ListBuilder textBuilder;
        textBuilder.append(parameterList);
        Procedures::readProcedureBody(*StreamManager::get().programReader(), textBuilder, sourceText, QStringLiteral("> "));
        DatumPtr textP = textBuilder.finishedList();

        Procedures::get().defineProcedure(command, procnameP, textP, sourceText);

        QString message = QObject::tr("%1 defined\n");
        message = message.arg(procnameP.toString());
        StreamManager::get().printToTerminal(message);
    }
    catch (FCError *err)
    {
        retval = err;
    }
    return retval;
}

/***DOC ERRACT
ERRACT							(variable)

    When set to a value that is not FALSE nor an empty string nor an empty list,
    the command interpreter will execute PAUSE to enable the user to
    inspect the state of the program.


COD***/

void Kernel::initPalette()
{
    // UCBLogo has 101 colors, from 0 to 100.
    const int paletteSize = 101;
    palette_.clear();
    palette_.reserve(paletteSize);
    palette_.push_back(QColor(QStringLiteral("black")));       // 0
    palette_.push_back(QColor(QStringLiteral("blue")));        // 1
    palette_.push_back(QColor(QStringLiteral("green")));       // 2
    palette_.push_back(QColor(QStringLiteral("cyan")));        // 3
    palette_.push_back(QColor(QStringLiteral("red")));         // 4
    palette_.push_back(QColor(QStringLiteral("magenta")));     // 5
    palette_.push_back(QColor(QStringLiteral("yellow")));      // 6
    palette_.push_back(QColor(QStringLiteral("white")));       // 7
    palette_.push_back(QColor(QStringLiteral("brown")));       // 8
    palette_.push_back(QColor(QStringLiteral("tan")));         // 9
    palette_.push_back(QColor(QStringLiteral("forestgreen"))); // 10
    palette_.push_back(QColor(QStringLiteral("aqua")));        // 11
    palette_.push_back(QColor(QStringLiteral("salmon")));      // 12
    palette_.push_back(QColor(QStringLiteral("purple")));      // 13
    palette_.push_back(QColor(QStringLiteral("orange")));      // 14
    palette_.push_back(QColor(QStringLiteral("grey")));        // 15
    palette_.resize(paletteSize);
}

void Kernel::initVariables()
{
    ListBuilder builder;
    for (auto &arg : Config::get().ARGV_)
    {
        builder.append(DatumPtr(arg));
    }

    DatumPtr platform(LOGOPLATFORM);
    DatumPtr version(LOGOVERSION);
    DatumPtr trueDatumPtr(QObject::tr("true"));
    DatumPtr commandLine = builder.finishedList();

    setDatumForName(commandLine, SpecialVars::name(SpecialVars::Id::CommandLine));
    setDatumForName(platform, SpecialVars::name(SpecialVars::Id::LogoPlatform));
    setDatumForName(version, SpecialVars::name(SpecialVars::Id::LogoVersion));
    setDatumForName(trueDatumPtr, SpecialVars::name(SpecialVars::Id::AllowGetSet));
    // TODO: Bury these variables:
    // "LOGOPLATFORM"
    // "LOGOVERSION"
    // "ALLOWGETSET"
    // "COMMANDLINE"
}

void Kernel::eraseVar(const QString &name)
{
    variables_.remove(name);
}

void Kernel::setDatumForName(const DatumPtr &aDatum, const QString &name)
{
    variables_.insert(name, aDatum);
}

DatumPtr Kernel::datumForName(const QString &name) const
{
    auto result = variables_.find(name);
    if (result != variables_.end())
    {
        return *result;
    }
    return nothing();
}

bool Kernel::doesExist(const QString &name) const
{
    return variables_.contains(name);
}

DatumPtr Kernel::allVariables() const
{
    ListBuilder builder;
    for (auto &varname : variables_.keys())
    {
        builder.append(DatumPtr(varname));
    }
    return builder.finishedList();
}

QSet<QString> &Kernel::namesFor(NameMark mark, NameKind kind)
{
    const auto index = static_cast<qsizetype>(mark) * static_cast<qsizetype>(NameKind::Count)
                       + static_cast<qsizetype>(kind);
    return nameSets_[index];
}

const QSet<QString> &Kernel::namesFor(NameMark mark, NameKind kind) const
{
    return const_cast<Kernel *>(this)->namesFor(mark, kind);
}

void Kernel::addName(NameMark mark, NameKind kind, const QString &name)
{
    namesFor(mark, kind).insert(name);
}

void Kernel::removeName(NameMark mark, NameKind kind, const QString &name)
{
    namesFor(mark, kind).remove(name);
}

bool Kernel::containsName(NameMark mark, NameKind kind, const QString &name) const
{
    return namesFor(mark, kind).contains(name);
}

const QSet<QString> &Kernel::nameList(NameMark mark, NameKind kind) const
{
    return namesFor(mark, kind);
}

void Kernel::setTest(bool isTrue)
{
    currentCallFrame()->testState_ = isTrue ? 3 : 2;
}

int8_t Kernel::testedState() const
{
    for (auto it = callFrameStack_.crbegin(); it != callFrameStack_.crend(); ++it)
    {
        auto *frame = &(*it);
        if (frame->testState_ != 0)
        {
            return frame->testState_;
        }
    }
    return 0;
}

DatumPtr Kernel::explicitSlotList() const
{
    if (explicitSlotListStack_.isEmpty())
        return nothing();
    return explicitSlotListStack_.top();
}


Kernel::Kernel()
{
    initVariables();
    initPalette();

    isPausing_ = false;
}

Kernel::~Kernel()
{
    Q_ASSERT(callFrameStack_.size() == 0);
}

/***************************************************
 * ECE operations
 ***************************************************/

void Kernel::beginProcedure(ASTNode *node, Datum **paramAry, uint32_t paramCount)
{
    ece_trace("beginProcedure: " + node->nodeName_.toString());
    callFrameStack_.emplace_back(node, paramAry, paramCount);
    nextOperation_ = &Kernel::ece_decideEmptyEvaluationStack;
}

CallFrame *Kernel::currentCallFrame()
{
    return &(callFrameStack_.back());
}

Evaluator *Kernel::currentEvaluator()
{
    return currentCallFrame()->topEvaluator();
}

 DatumPtr Kernel::runECE()
 {
     nextOperation_ = &Kernel::ece_evaluateStack;
     currentCallFrame()->retvalToParent_ = nothing();
 
     while (nextOperation_ != nullptr)
     {
         (this->*nextOperation_)();
     }
 
     return currentCallFrame()->retvalToParent_;
 }

 void Kernel::ece_evaluateStack()
{
    ece_trace("ece_evaluateStack: executing " + currentEvaluator()->list_.toString());
    // GOTO stores the destination block on the CallFrame; consume it here so the
    // newly pushed line starts at the tagged block.
    int32_t jumpLocation = currentCallFrame()->jumpLocation_;
    currentCallFrame()->jumpLocation_ = 0;
    if (currentEvaluator()->exec(jumpLocation))
    {
        if (currentEvaluator()->listToRunAfterCurrentList_.isList())
        {
            nextOperation_ = &Kernel::ece_chainList;
        }
        else
        {
            nextOperation_ = &Kernel::ece_popEvaluator;
        }
    }
}

void Kernel::ece_chainList()
{
    ece_trace("ece_chainList");
    Evaluator *e = currentEvaluator();
    Q_ASSERT(e->handle_ == nullptr);
    e->list_ = e->listToRunAfterCurrentList_;
    e->listToRunAfterCurrentList_ = DatumPtr();
    nextOperation_ = &Kernel::ece_evaluateStack;
}

void Kernel::ece_decideEmptyEvaluationStack()
{
    ece_trace("ece_decideEmptyEvaluationStack");
    if (currentCallFrame()->sourceNode_.isNothing())
    {
        // Empty source node means this frame is REPL. Return to the caller.
        nextOperation_ = nullptr;
        return;
    }

    // We are running a procedure.

    if (currentCallFrame()->isReadingArgs_)
    {
        // We are processing the arguments.
        if ( ! currentCallFrame()->currentParameterName_.isEmpty())
        {
            // We have finished processing a default value for an optional parameter.
            DatumPtr defaultValue = currentCallFrame()->retvalToParent_;

            // Any result that is not a data type gets converted to a bad default error.
            if ((defaultValue.isa() & Datum::typeDataMask) == 0)
            {
                currentCallFrame()->retvalToParent_ = DatumPtr(FCError::badDefault(currentCallFrame()->currentArgument_));
                nextOperation_ = &Kernel::ece_exitProcedure;
            }

            currentCallFrame()->setVarAsLocal(currentCallFrame()->currentParameterName_);
            ece_trace("ece_decideEmptyEvaluationStack: setting variable " + currentCallFrame()->currentParameterName_ + " to " + defaultValue.toString());
            setDatumForName(defaultValue, currentCallFrame()->currentParameterName_);
            currentCallFrame()->currentParameterName_.clear();
            nextOperation_ = &Kernel::ece_processParameters;
            return;
        }
        else
        {
            nextOperation_ = &Kernel::ece_processParameters;
            return;
        }
    }

    // If there is a flow control instruction, or we have reached the end of the procedure, we need to handle it.
    if (currentCallFrame()->retvalToParent_.isFlowControl() || currentCallFrame()->runningSourceList_.listValue()->isEmpty())
    {
        nextOperation_ = &Kernel::ece_exitProcedure;
        return;
    }

    // Get the next line of the procedure and push it onto the evaluation stack.
    DatumPtr line = currentCallFrame()->runningSourceList_.listValue()->head;
    ece_trace("ece_decideEmptyEvaluationStack: pushing line " + line.toString() + " onto the evaluation stack");
    currentCallFrame()->pushEvaluator(line);
    nextOperation_ = &Kernel::ece_evaluateStack;
}

void Kernel::ece_popEvaluator()
{
    ece_trace("ece_popEvaluator");
    CallFrame *cf = currentCallFrame();
    Evaluator *e = currentEvaluator();
    DatumPtr completedList = e->list_;
    DatumPtr startingList = e->startingList_;
    cf->retvalToParent_ = DatumPtr(e->retvalToParent_);
    retvalSourceList_ = completedList;
    ece_trace("ece_popEvaluator: returning value " + cf->retvalToParent_.toString() + " from source list " + retvalSourceList_.toString());

    cf->popEvaluator();

    if (cf->evaluationStackSize() > 0)
    {
        cf->topEvaluator()->retvalFromChild_ = cf->retvalToParent_;
        nextOperation_ = &Kernel::ece_evaluateStack;
    }
    else
    {
        // Finished the frame's top-level list. If this evaluator was created for the
        // current procedure body line, advance past it (use startingList_: list_ may
        // have been replaced by ece_chainList for a tail RUN).
        if (cf->sourceNode_.isASTNode() && cf->runningSourceList_.isList() &&
            !cf->runningSourceList_.listValue()->isEmpty() &&
            cf->runningSourceList_.listValue()->head.datumValue() == startingList.datumValue())
        {
            cf->runningSourceList_ = cf->runningSourceList_.listValue()->tail;
        }
        nextOperation_ = &Kernel::ece_decideEmptyEvaluationStack;
    }
}

void Kernel::ece_exitProcedure()
{
    ece_trace("ece_exitProcedure");
    
    // There shouldn't be any remaining evaluators on the evaluation stack.
    Q_ASSERT(currentCallFrame()->evaluationStackSize() == 0);

    // retval_ contains the result of the procedure.
    switch (currentCallFrame()->retvalToParent_.isa())
    {
    case Datum::typeError:
    {
        // The error is passed through to the caller.
        DatumPtr retval = currentCallFrame()->retvalToParent_;
        ece_trace("ece_exitProcedure with error: popping call frame");
        callFrameStack_.pop_back();
        currentCallFrame()->topEvaluator()->retvalFromChild_ = retval;
        nextOperation_ = &Kernel::ece_evaluateStack;
        break;
    }
    case Datum::typeContinuation:
    {
        // Tail call inside this frame. Macro calls are not continuations; if this frame
        // is a macro, isMacroCall_ stays set so the eventual output is still expanded.
        auto *continuation = static_cast<FCContinuation*>(currentCallFrame()->retvalToParent_.flowControlValue());
        auto *procedure = continuation->procedure().astnodeValue();
        auto arguments = continuation->params().first();
        currentCallFrame()->applyContinuation(procedure, arguments);
        nextOperation_ = &Kernel::ece_processParameters;
        break;
    }
    case Datum::typeReturn:
    {
        DatumPtr retval = currentCallFrame()->retvalToParent_.flowControlValue()->data_;
        if (currentCallFrame()->isMacroCall_)
        {
            ece_trace("ece_exitProcedure with macro return");
            ece_expandMacroReturn(retval);
            break;
        }
        // The return value is passed through to the caller.
        ece_trace("ece_exitProcedure with return: popping call frame");
        callFrameStack_.pop_back();
        ece_resumeCaller(retval);
        break;
    }
    case Datum::typeNothing:
    case Datum::typeNothingPersistent:
    case Datum::typeASTNode:
    {
        if (currentCallFrame()->isMacroCall_)
        {
            ece_trace("ece_exitProcedure macro did not output");
            ece_expandMacroReturn(nothing());
            break;
        }
        DatumPtr retval = currentCallFrame()->sourceNode_;
        ece_trace("ece_exitProcedure with no return value: popping call frame");
        callFrameStack_.pop_back();
        ece_resumeCaller(retval);
        break;
    }
    case Datum::typeGoto:
    {
        auto *gotoControl = static_cast<FCGoto*>(currentCallFrame()->retvalToParent_.flowControlValue());
        auto [location, blockId] = gotoControl->location();
        currentCallFrame()->runningSourceList_ = location;
        currentCallFrame()->jumpLocation_ = blockId;
        currentCallFrame()->retvalToParent_ = nothing();
        nextOperation_ = &Kernel::ece_decideEmptyEvaluationStack;
        break;
    }
    default:
        Q_ASSERT(false);
        break;
    }
}

void Kernel::ece_resumeCaller(const DatumPtr &result)
{
    Q_ASSERT(currentCallFrame()->evaluationStackSize() > 0);
    currentCallFrame()->topEvaluator()->retvalFromChild_ = result;
    nextOperation_ = &Kernel::ece_evaluateStack;
}

void Kernel::ece_expandMacroReturn(const DatumPtr &value)
{
    // Hold the output across the frame pop. The frame owns the FCReturn that points at it.
    DatumPtr expansion = value;
    bool expectsSingle = currentCallFrame()->macroExpectsSingleExpression_;
    bool tailSplice = currentCallFrame()->macroTailSplice_;

    ece_trace("ece_expandMacroReturn: popping macro frame");
    callFrameStack_.pop_back();
    Q_ASSERT(currentCallFrame()->evaluationStackSize() > 0);

    if (!expansion.isList())
    {
        ece_resumeCaller(DatumPtr(FCError::macroReturnedNotList(expansion)));
        return;
    }

    if (expectsSingle)
    {
        QList<DatumPtr> expressions;
        try
        {
            expressions = Treeifier::astFromList(expansion.listValue());
        }
        catch (FCError *err)
        {
            ece_resumeCaller(DatumPtr(err));
            return;
        }
        if (expressions.size() != 1)
        {
            FCError *err = expressions.isEmpty() ? FCError::macroReturnedNotList(expansion)
                                                 : FCError::multipleExpressions(expansion);
            ece_resumeCaller(DatumPtr(err));
            return;
        }
    }

    if (tailSplice)
    {
        // The caller is suspended on this macro and has nothing left to run after it.
        // Replace that suspended line so a recursive expansion reuses the evaluator.
        ece_trace("ece_expandMacroReturn: splicing into caller line");
        currentCallFrame()->topEvaluator()->replaceRunningList(expansion);
        nextOperation_ = &Kernel::ece_evaluateStack;
        return;
    }

    ece_trace("ece_expandMacroReturn: running expansion before resuming caller");
    currentCallFrame()->pushEvaluator(expansion);
    nextOperation_ = &Kernel::ece_evaluateStack;
}

void Kernel::ece_processParameters()
{
    ece_trace("ece_processParameters");
    while ( ! currentCallFrame()->parameters_.listValue()->isEmpty())
    {
        currentCallFrame()->currentArgument_ = currentCallFrame()->parameters_.listValue()->head;
        DatumPtr parameter = currentCallFrame()->currentArgument_;
        currentCallFrame()->parameters_ = currentCallFrame()->parameters_.listValue()->tail;

        if (parameter.isWord())
        {
            // If the word is a number then break out of the loop.
            // (it represents the default number of parameters, ignore.)
            parameter.wordValue()->numberValue();
            if (parameter.wordValue()->numberIsValid)
            {
                break;
            }
            // A word parameter gets assigned the current argument.
            currentCallFrame()->currentParameterName_ = parameter.toString(Datum::ToStringFlags_Key);
            DatumPtr argument = currentCallFrame()->arguments_.listValue()->head;
            DatumPtr tail = currentCallFrame()->arguments_.listValue()->tail;
            currentCallFrame()->arguments_ = tail;
            currentCallFrame()->setVarAsLocal(currentCallFrame()->currentParameterName_);
            ece_trace("ece_processParameters: setting variable " + currentCallFrame()->currentParameterName_ + " to " + argument.toString());
            setDatumForName(argument, currentCallFrame()->currentParameterName_);
        }
        else
        {
            // This is either a parameter with a default value, or a rest parameter.
            // Either way, the first element is the name of the parameter.
            currentCallFrame()->currentParameterName_ = parameter.listValue()->head.toString(Datum::ToStringFlags_Key);

            // The remainder of the list is the default value, if it exists.
            DatumPtr defaultValue = parameter.listValue()->tail;
            if (defaultValue.listValue()->isEmpty())
            {
                // No default value, so the remainder of the arguments become the value.
                currentCallFrame()->setVarAsLocal(currentCallFrame()->currentParameterName_);
                setDatumForName(currentCallFrame()->arguments_, currentCallFrame()->currentParameterName_);
                break;
            }
            else
            {
                // We have a default value. Do we have an argument to assign to it?
                if (currentCallFrame()->arguments_.listValue()->isEmpty())
                {
                    // we need to evaluate the default value so we can assign it to the parameter name.
                    currentCallFrame()->pushEvaluator(defaultValue);
                    nextOperation_ = &Kernel::ece_evaluateStack;
                    return;
                }
                else
                {
                    // else, we assign the provided argument
                    DatumPtr argument = currentCallFrame()->arguments_.listValue()->head;
                    DatumPtr tail = currentCallFrame()->arguments_.listValue()->tail;
                    currentCallFrame()->arguments_ = tail;
                    currentCallFrame()->setVarAsLocal(currentCallFrame()->currentParameterName_);
                    ece_trace("ece_processParameters: setting variable " + currentCallFrame()->currentParameterName_ +
                              " to " + argument.toString());
                    setDatumForName(argument, currentCallFrame()->currentParameterName_);
                }
            }
        }
    }

    currentCallFrame()->isReadingArgs_ = false;

    // We have processed all the parameters, so we can move to the first line of the procedure.
    nextOperation_ = &Kernel::ece_decideEmptyEvaluationStack;
}

/***************************************************
 * Miscellaneous operations
 ***************************************************/

DatumPtr Kernel::pause()
{
    // if (isPausing)
    // {
    //     StreamManager::get().printToTerminal(QObject::tr("Already Pausing\n"));
    //     return nothing();
    // }

    // isPausing = true;
    // DatumPtr sourceNode = callStack.localFrame()->sourceNode;
    // QString sourceNodeName;
    // if (sourceNode.isASTNode())
    // {
    //     sourceNodeName = sourceNode.astnodeValue()->nodeName.toString();
    // }

    // CallFrame frame(callStack, nothing());

    // StreamManager::get().printToTerminal(QObject::tr("Pausing...\n"));

    // DatumPtr result = readEvalPrintLoop(true, sourceNodeName);

    // isPausing = false;
    // return result;
    return nothing();
}

int Kernel::run()
{
    Config::get().mainInterface()->initialize();

    LogoInterface::initSignals();

    readEvalPrintLoop(false);

    LogoInterface::restoreSignals();

    Compiler::releaseAllCompiledText();

    return 0;
}

/// @brief Print stack trace.
void bt()
{
    Kernel &k = Kernel::get();
    for (size_t i = 0; i < k.callFrameStack_.size(); ++i)
    {
        CallFrame *frame = &(k.callFrameStack_[i]);
        if (frame->sourceNode_.isNothing())
            std::cerr << "frame " << i << ": REPL Base" << std::endl;
        else if (frame->sourceNode_.isASTNode()
                 && frame->sourceNode_.astnodeValue()->countOfChildren() > 0)
            std::cerr << "frame " << i << ": "
                      << frame->sourceNode_.astnodeValue()->nodeName_
                             .toString(Datum::ToStringFlags_Key)
                             .toStdString()
                      << std::endl;
        else
            std::cerr << "frame " << i << ": ?" << std::endl;
        for (size_t j = 0; j < frame->evaluationStackSize(); ++j)
        {
            Evaluator *ev = frame->evaluatorAtIndex(j);
            std::cerr << "  list: "
                      << ev->list_.toString(Datum::ToStringFlags_Show).toStdString() << std::endl;
        }
    }
}
