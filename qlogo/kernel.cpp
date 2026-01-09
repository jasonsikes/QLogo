
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
#include "compiler.h"
#include "controller/textstream.h"
#include "datum_types.h"
#include "sharedconstants.h"
#include "treeifyer.h"
#include "workspace/procedures.h"
#include <QApplication> // quit()
#include <QColor>
#include <QDebug>
#include <QDir>
#include <QFont>
#include <QImage>
#include <cstdlib> // arc4random_uniform()

#include "runparser.h"
#include "turtle.h"

#include "controller/logocontroller.h"

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
            if ((colorNum != round(colorNum)) || (colorNum < 0) || (colorNum >= palette.size()))
                return false;
            retval = palette[colorNum];
            if (!retval.isValid())
                retval = palette[0];
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
    QString localPrompt = prompt + "? ";
    forever
    {
        DatumPtr result;
        try
        {
            DatumPtr line = systemReadStream->readlistWithPrompt(localPrompt, true);
            if (line.isNothing()) // EOF
                return nothing();
            result = runList(line);
        }
        catch (FCError *e)
        {
            sysPrint(e->toString());
            sysPrint("\n");
            continue;
        }
        if ((result.datumValue()->isa & Datum::typeUnboundMask) != 0)
            continue;
        if (result.isErr())
        {
            FCError *e = result.errValue();
            if (e->tag().isWord() && (e->code == ErrCode::ERR_NO_CATCH))
            {
                if (e->tag().toString(Datum::ToStringFlags_Key) == QObject::tr("TOPLEVEL"))
                {
                    sysPrint("\n");
                    continue;
                }
                if (e->tag().toString(Datum::ToStringFlags_Key) == QObject::tr("SYSTEM"))
                {
                    sysPrint("\n");
                    Config::get().mainController()->systemStop();
                    return result;
                }
                if (e->tag().toString(Datum::ToStringFlags_Key) == QObject::tr("PAUSE") && isPausing)
                {
                    return e->output();
                }
            }
            sysPrint(e->toString());
            sysPrint("\n");
            continue;
        }

        if (result.isFlowControl())
        {
            // The other flow control types are OUTPUT/STOP and GOTO,
            // which are not allowed here.
            result = DatumPtr(FCError::notInsideProcedure(result.flowControlValue()->sourceNode));
        }

        // If we are here that means something was output, but not handled.
        sysPrint(QString("You don't say what to do with %1\n").arg(result.toString(Datum::ToStringFlags_Show)));
    }
}

Datum *Kernel::inputProcedure(ASTNode *node)
{
    Datum *retval = node;
    try
    {
        // to is the command name that initiated inputProcedure(), it is the first
        // word in the input line, 'TO' or '.MACRO'.
        // TODO: rename this to "command"
        DatumPtr to = node->nodeName;
        if (node->countOfChildren() == 0)
            throw FCError::notEnoughInputs(to);

        // procnameP is the name of the procedure, the second word in the input line,
        // following 'TO' or '.MACRO'.
        DatumPtr procnameP = node->childAtIndex(0);
        if (!procnameP.isWord())
            throw FCError::doesntLike(to, procnameP);

        procnameP.wordValue()->numberValue();
        if (procnameP.wordValue()->numberIsValid)
            throw FCError::doesntLike(to, procnameP);

        QString procname = procnameP.toString(Datum::ToStringFlags_Key);

        QChar firstChar = (procname)[0];
        if ((firstChar == '"') || (firstChar == ':') || (firstChar == '(') || (firstChar == ')'))
            throw FCError::doesntLike(to, procnameP);

        if (Config::get().mainProcedures()->isProcedure(procname))
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

        // Now read in the body
        forever
        {
            DatumPtr line = systemReadStream->readlistWithPrompt("> ", true, true);
            if (!line.isList()) // this must be the end of the input
                break;
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

        // The sourcetext is the raw text from which the procedure was defined.
        // We save it in case user executes `FULLTEXT`.
        DatumPtr sourceText = systemReadStream->recentHistory();
        Config::get().mainProcedures()->defineProcedure(to, procnameP, textP, sourceText);

        QString message = QObject::tr("%1 defined\n");
        message = message.arg(procnameP.toString());
        Config::get().mainKernel()->sysPrint(message);
    }
    catch (FCError *err)
    {
        retval = err;
    }
    return retval;
}

/***DOC ERRACT
ERRACT							(variable)

    When set to a value that is not "False"false" nor an empty list,
    the command interpreter will execute PAUSE to enable the user to
    inspect the state of the program.


COD***/

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

// Documentation is here because it doesn't fit anywhere else.

// TODO: move documentation to Compiler class

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

void Kernel::initVariables()
{
    ListBuilder builder;
    for (auto &arg : Config::get().ARGV)
    {
        builder.append(DatumPtr(arg));
    }

    DatumPtr platform(LOGOPLATFORM);
    DatumPtr version(LOGOVERSION);
    DatumPtr trueDatumPtr(QObject::tr("true"));
    DatumPtr commandLine = builder.finishedList();

    callStack.setDatumForName(commandLine, QObject::tr("COMMANDLINE"));
    callStack.setDatumForName(platform, QObject::tr("LOGOPLATFORM"));
    callStack.setDatumForName(version, QObject::tr("LOGOVERSION"));
    callStack.setDatumForName(trueDatumPtr, QObject::tr("ALLOWGETSET"));
    // TODO: Bury these variables:
    // "LOGOPLATFORM"
    // "LOGOVERSION"
    // "ALLOWGETSET"
    // "COMMANDLINE"
}

Kernel::Kernel()
{
    Config::get().setMainKernel(this);
    stdioStream = new TextStream(nullptr);
    readStream = stdioStream;
    systemReadStream = stdioStream;
    writeStream = stdioStream;
    systemWriteStream = stdioStream;

    turtle = new Turtle;
    procedures = new Procedures;
    treeifier = new Treeifier;
    theCompiler = new Compiler();

    // callStack holds a pointer to the new frame so it will be deleted when this
    // Kernel is deleted.
    new CallFrame(&callStack);

    initVariables();
    initPalette();

    filePrefix = emptyList();
}

Kernel::~Kernel()
{
    closeAll();
    delete theCompiler;
    delete treeifier;
    delete procedures;
    delete turtle;

    Q_ASSERT(callStack.size() == 1);
    callStack.stack.removeLast();
    Config::get().setMainKernel(nullptr);
}

DatumPtr Kernel::runList(const DatumPtr &listP, const QString &startTag)
{
    DatumPtr retval;

    Q_ASSERT(callStack.size() > 0);
    Evaluator e(listP, callStack.localFrame()->evalStack);

    retval = e.exec(0);

    return retval;
}

Datum *Kernel::specialVar(SpecialNames name)
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
    static bool isPausing = false;
    if (isPausing)
    {
        sysPrint(QObject::tr("Already Pausing"));
        return nothing();
    }

    isPausing = true;
    DatumPtr sourceNode = callStack.localFrame()->sourceNode;
    QString sourceNodeName;
    if (sourceNode.isASTNode())
    {
        sourceNodeName = sourceNode.astnodeValue()->nodeName.toString();
    }

    CallFrame frame(&callStack, nothing());

    sysPrint(QObject::tr("Pausing...\n"));

    DatumPtr result = readEvalPrintLoop(true, sourceNodeName);

    isPausing = false;
    return result;
}

QString Kernel::filepathForFilename(const DatumPtr &filenameP) const
{
    QString filename = filenameP.wordValue()->toString();

    if (filePrefix.isWord())
    {
        QString prefix = filePrefix.wordValue()->toString();
        return prefix + QDir::separator() + filename;
    }
    return filename;
}

void Kernel::closeAll()
{
    QStringList names = fileStreams.keys();
    for (const auto &iter : names)
    {
        // close(iter);
    }
}

void Kernel::stdPrint(const QString &text)
{
    writeStream->lprint(text);
}

void Kernel::sysPrint(const QString &text)
{
    systemWriteStream->lprint(text);
}
