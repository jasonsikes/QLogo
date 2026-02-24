
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
#include <cstdlib> // arc4random_uniform()


// The maximum depth of procedure iterations before error is thrown.
const int maxIterationDepth = 1000;

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
    callFrameStack_.push(std::move(std::make_unique<NewCallFrame>(nothing())));
    QString localPrompt = prompt + "? ";
    DatumPtr result;
    forever
    {
        try
        {
            DatumPtr line = systemReadStream_->readListWithPrompt(localPrompt, true);
            Q_ASSERT(callFrameStack_.top()->evaluationStackSize() == 0);
            Q_ASSERT(callFrameStack_.top()->sourceNode_.isNothing());
            callFrameStack_.top()->pushEvaluator(line);
            result = runECE();
        }
        catch (FCError *e)
        {
            // While FCError objects can be returned from an execution in the form of a DatumPtr,
            // Sometimes exceptions are thrown by a user interface action.
            // Wrap it into a DatumPtr.
            result = DatumPtr(e);
            goto bailout;
        }
        if ((result.datumValue()->isa_ & Datum::typeUnboundMask) != 0)
        {
            continue;
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

        if (result.isFlowControl())
        {
            // The other flow control types are OUTPUT/STOP and GOTO,
            // which are not allowed here.
            result = DatumPtr(FCError::notInsideProcedure(result.flowControlValue()->sourceNode_));
        }

        // If we are here that means something was output, but not handled.
        sysPrint(QString("You don't say what to do with %1\n").arg(result.toString(Datum::ToStringFlags_Show)));
    }

bailout:
    callFrameStack_.pop();
    return result;
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

        if (Procedures::get().isProcedure(procname))
            throw FCError::procDefined(procnameP);

        // Assign the procedure's parameter names and default values.
        ListBuilder firstLineBuilder;
        for (int i = 1; i < node->countOfChildren(); ++i)
        {
            firstLineBuilder.append(node->childAtIndex(i));
        }
        DatumPtr firstLine = firstLineBuilder.finishedList();
        ListBuilder textBuilder;
        textBuilder.append(firstLine);

        QList<DatumPtr> sourceText = systemReadStream_->recentHistory();
        // Now read in the body
        forever
        {
            DatumPtr line = systemReadStream_->readListWithPrompt("> ", true);
            if (!line.isList()) // this must be the end of the input
                break;
            if (line.listValue()->isEmpty())
                continue;
            sourceText.append(systemReadStream_->recentHistory());
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

DatumPtr Kernel::runECE()
{
    nextOperation_ = &Kernel::ece_evaluateList;
    jumpLocation_ = 0;
    retval_ = nothing();

    while (nextOperation_ != nullptr)
    {
        (this->*nextOperation_)();
    }

    return retval_;
}

void Kernel::ece_evaluateList()
{
    NewCallFrame *currentCallFrame = callFrameStack_.top().get();
    NewEvaluator *topEvaluator = currentCallFrame->topEvaluator();

    // exec() returns true if execution is complete.
    if (topEvaluator->exec(jumpLocation_))
    {
        // TODO: consider if popEvaluator should be its own operation.
        retval_ = DatumPtr(topEvaluator->retval);
        currentCallFrame->popEvaluator();
        if (currentCallFrame->evaluationStackSize() >= 1)
        {
            currentCallFrame->topEvaluator()->lastSubExecResult_ = retval_.datumValue();
        } else {
            nextOperation_ = &Kernel::ece_decideEmptyEvaluationStack;
        }
    }
    jumpLocation_ = 0;
}

void Kernel::ece_decideEmptyEvaluationStack()
{
    NewCallFrame *currentCallFrame = callFrameStack_.top().get();

    // TODO: move logic to callframe.
    if (currentCallFrame->sourceNode_.isNothing())
    {
        // Empty source node means this frame is REPL. Return to the caller.
        nextOperation_ = nullptr;
        return;
    }
    Q_ASSERT(false);
    // Get the next line from the procedure list.
}

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
