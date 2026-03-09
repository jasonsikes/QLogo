
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
#include "interface/textstream.h"
#include "datum_types.h"
#include "sharedconstants.h"
#include "workspace/procedures.h"
#include "runparser.h"
#include "workspace/turtle.h"
#include "interface/logointerface.h"
#include <QApplication> // quit()
#include <QColor>
#include <QDebug>
#include <QDir>
#include <QFont>
#include <QImage>
#include <iostream>
#include <cstdlib> // arc4random_uniform()

void ece_trace(const QString &msg)
{
    static bool doTrace = Config::get().traceEvaluator_;
    if (doTrace)
    {
        qInfo() << "ece_trace: " << msg;
    }
}

/// @brief Clean a procedure parameter by removing ':' and '"' from the parameter name if it is a word.
/// @param parameter The parameter to clean.
/// @return The cleaned parameter.
DatumPtr cleanParameterWord(const DatumPtr &parameter)
{
    Q_ASSERT (parameter.isWord());
    QString parameterName = parameter.wordValue()->toString();
    if (parameterName.startsWith(':') || parameterName.startsWith('"'))
    {
        parameterName.remove(0, 1);
        return DatumPtr(parameterName);
    }
    return parameter;
}

/// Clean a procedure parameter by removing ':' and '"' from the parameter name if it is a word or list.
DatumPtr cleanParameter(ASTNode *node, const DatumPtr &parameter)
{
    DatumPtr first;

    // The parameter can be a word.
    if (parameter.isWord())
    {
        return cleanParameterWord(parameter);
    }

    // If not a word, the parameter must be a list with the head being a word.
    if ( ! parameter.isList())
    {
        goto error;
    }

    first = parameter.listValue()->head;
    if ( ! first.isWord())
    {
        goto error;
    }
    first = cleanParameterWord(first);
    return new List(first, parameter.listValue()->tail.listValue());

error:
    throw FCError::doesntLike(node, parameter);
}

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
    callFrameStack_.push_back(std::move(std::make_unique<CallFrame>(nullptr, nullptr, 0)));
    QString localPrompt = prompt + "? ";
    DatumPtr result;
    forever
    {
        try
        {
            DatumPtr line = systemReadStream_->readListWithPrompt(localPrompt, true);

            Q_ASSERT(callFrameStack_.back()->evaluationStackSize() == 0);
            Q_ASSERT(callFrameStack_.back()->sourceNode_.isNothing());

            if (line.isNothing())
            {
                 // EOF
                result = nothing();
                goto bailout;
            }
            callFrameStack_.back()->pushEvaluator(line);
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
                    sysPrint("\n");
                    continue;
                }
                if (e->tag().toString(Datum::ToStringFlags_Key) == QObject::tr("SYSTEM"))
                {
                    sysPrint("\n");
                    Config::get().mainInterface()->closeInterface();
                    QApplication::quit();
                    result = nothing();
                    goto bailout;
                }
                if (e->tag().toString(Datum::ToStringFlags_Key) == QObject::tr("PAUSE") && isPausing)
                {
                    result = e->output();
                    goto bailout;
                }
            }
            sysPrint(e->toString() + "\n");
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
        paramListBuilder.append(cleanParameter(node, node->childAtIndex(i)));
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

        QList<DatumPtr> sourceText = systemReadStream_->listReaderLineHistory();
        // Now read in the body
        ListBuilder textBuilder;
        textBuilder.append(parameterList);

        forever
        {
            DatumPtr line = systemReadStream_->readListWithPrompt("> ", true);
            if (!line.isList()) // this must be the end of the input
                break;
            sourceText.append(systemReadStream_->listReaderLineHistory());
            if (line.listValue()->isEmpty())
                continue;

            DatumPtr first = line.listValue()->head;
            if (first.isWord())
            {
                QString firstWord = first.toString(Datum::ToStringFlags_Key);
                if (firstWord == QObject::tr("END"))
                    break;
            }
            textBuilder.append(line);
        }
        DatumPtr textP = textBuilder.finishedList();

        Procedures::get().defineProcedure(command, procnameP, textP, sourceText);

        QString message = QObject::tr("%1 defined\n");
        message = message.arg(procnameP.toString());
        Kernel::get().sysPrint(message);
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

    setDatumForName(commandLine, QObject::tr("COMMANDLINE"));
    setDatumForName(platform, QObject::tr("LOGOPLATFORM"));
    setDatumForName(version, QObject::tr("LOGOVERSION"));
    setDatumForName(trueDatumPtr, QObject::tr("ALLOWGETSET"));
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

void Kernel::setTest(bool isTrue)
{
    currentCallFrame()->testState_ = isTrue ? 3 : 2;
}

int8_t Kernel::testedState() const
{
    for (auto it = callFrameStack_.crbegin(); it != callFrameStack_.crend(); ++it)
    {
        auto *frame = it->get();
        if (frame->testState_ != 0)
        {
            return frame->testState_;
        }
    }
    return 0;
}


Kernel::Kernel()
{
    stdioStream_ = new TextStream(nullptr);
    readStream_ = stdioStream_;
    systemReadStream_ = stdioStream_;
    writeStream_ = stdioStream_;
    systemWriteStream_ = stdioStream_;

    initVariables();
    initPalette();

    filePrefix_ = emptyList();
    isPausing_ = false;
}

Kernel::~Kernel()
{
    closeAll();

    Q_ASSERT(callFrameStack_.size() == 0);
}

/***************************************************
 * ECE operations
 ***************************************************/

void Kernel::beginProcedure(ASTNode *node, Datum **paramAry, uint32_t paramCount)
{
    ece_trace("beginProcedure: " + node->nodeName_.toString());
    callFrameStack_.push_back(std::make_unique<CallFrame>(node, paramAry, paramCount));
    nextOperation_ = &Kernel::ece_decideEmptyEvaluationStack;
}

CallFrame *Kernel::currentCallFrame() const
{
    return callFrameStack_.back().get();
}

Evaluator *Kernel::currentEvaluator() const
{
    return currentCallFrame()->topEvaluator();
}

 DatumPtr Kernel::runECE()
 {
     nextOperation_ = &Kernel::ece_evaluateStack;
     jumpLocation_ = 0;
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
    if (currentEvaluator()->exec(jumpLocation_))
    {
        nextOperation_ = &Kernel::ece_popEvaluator;
    }
    jumpLocation_ = 0;
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

            // Any error here gets converted.
            if (defaultValue.isErr())
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
    currentCallFrame()->runningSourceList_ = currentCallFrame()->runningSourceList_.listValue()->tail;
    nextOperation_ = &Kernel::ece_evaluateStack;
}

void Kernel::ece_popEvaluator()
{
    ece_trace("ece_popEvaluator");
    currentCallFrame()->retvalToParent_ = DatumPtr(currentEvaluator()->retvalToParent_);
    retvalSourceList_ = currentEvaluator()->list_;
    ece_trace("ece_popEvaluator: returning value " + currentCallFrame()->retvalToParent_.toString() + " from source list " + retvalSourceList_.toString());

    currentCallFrame()->popEvaluator();

    if (currentCallFrame()->evaluationStackSize() > 0)
    {
        currentCallFrame()->topEvaluator()->retvalFromChild_ = currentCallFrame()->retvalToParent_;
        nextOperation_ = &Kernel::ece_evaluateStack;
    }
    else
    {
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
        // TODO: consider the case if child is a macro...
        auto *continuation = static_cast<FCContinuation*>(currentCallFrame()->retvalToParent_.flowControlValue());
        auto *procedure = continuation->procedure().astnodeValue();
        auto arguments = continuation->params().first();
        currentCallFrame()->applyContinuation(procedure, arguments);
        nextOperation_ = &Kernel::ece_processParameters;
        break;
    }
    case Datum::typeReturn:
    {
        // The return value is passed through to the caller.
        // TODO: consider the case if child is a macro...
        DatumPtr retval = currentCallFrame()->retvalToParent_.flowControlValue()->data_;
        ece_trace("ece_exitProcedure with return: popping call frame");
        callFrameStack_.pop_back();
        currentCallFrame()->topEvaluator()->retvalFromChild_ = retval;
        nextOperation_ = &Kernel::ece_evaluateStack;
        break;
    }
    case Datum::typeASTNode:
    {
        // TODO: consider using the procedure's ASTNode for the return value.
        DatumPtr retval = currentCallFrame()->retvalToParent_;
        ece_trace("ece_exitProcedure with no return value: popping call frame");
        callFrameStack_.pop_back();
        currentCallFrame()->topEvaluator()->retvalFromChild_ = retval;
        nextOperation_ = &Kernel::ece_evaluateStack;
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

    // TODO: if is macro...
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

Datum *Kernel::specialVar(SpecialNames name) const
{
    switch (name)
    {
    case ERRACT:
    {
        static DatumPtr erract = DatumPtr(new Word("ERRACT"));
        return erract.datumValue();
    }
    default:
        return nullptr;
    }
}

DatumPtr Kernel::pause()
{
    // if (isPausing)
    // {
    //     sysPrint(QObject::tr("Already Pausing\n"));
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

    // sysPrint(QObject::tr("Pausing...\n"));

    // DatumPtr result = readEvalPrintLoop(true, sourceNodeName);

    // isPausing = false;
    // return result;
    return nothing();
}

QString Kernel::filepathForFilename(const DatumPtr &filenameP) const
{
    QString filename = filenameP.wordValue()->toString();

    if (filePrefix_.isWord())
    {
        QString prefix = filePrefix_.wordValue()->toString();
        return prefix + QDir::separator() + filename;
    }
    return filename;
}

void Kernel::closeAll()
{
    QStringList names = fileStreams_.keys();
    for (const auto &filename : names)
    {
        // close(filename);
    }
}

void Kernel::stdPrint(const QString &text) const
{
    writeStream_->lprint(text);
}

void Kernel::sysPrint(const QString &text) const
{
    systemWriteStream_->lprint(text);
}

int Kernel::run()
{
    Config::get().mainInterface()->initialize();

    LogoInterface::initSignals();

    readEvalPrintLoop(false);

    LogoInterface::restoreSignals();

    return 0;
}

/// @brief Print stack trace.
void bt()
{
    Kernel &k = Kernel::get();
    const std::deque<std::unique_ptr<CallFrame>> &stack = k.callFrameStack_;
    for (size_t i = 0; i < stack.size(); ++i)
    {
        CallFrame *frame = stack[i].get();
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
        for (size_t j = 0; j < frame->evaluationStack_.size(); ++j)
        {
            Evaluator *ev = frame->evaluationStack_[j].get();
            std::cerr << "  list: "
                      << ev->list_.toString(Datum::ToStringFlags_Show).toStdString() << std::endl;
        }
    }
}
