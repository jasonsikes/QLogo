
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
#include "datum_types.h"
#include "flowcontrol.h"
#include "interface/logointerface.h"
#include "interface/streammanager.h"
#include "interface/textstream.h"
#include "runparser.h"
#include "sharedconstants.h"
#include "treeifyer.h"
#include "workspace/procedures.h"
#include "workspace/specialvars.h"
#include "workspace/turtle.h"
#include <QColor>
#include <QCoreApplication> // quit()
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QProcess>
#include <QTemporaryFile>
#include <QTextStream>
#include <QFont>
#include <QImage>
#include <cstdlib> // arc4random_uniform()
#include <iostream>

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

Kernel::TopLevelResult Kernel::finishTopLevelResult(const DatumPtr &resultIn, bool isPausing, DatumPtr *resultOut)
{
    DatumPtr result = resultIn;

    if ((result.datumValue()->isa_ & Datum::typeDataMask) != 0)
    {
        result = DatumPtr(FCError::dontSay(result));
    }

    if (result.isFlowControl())
    {
        // Flow control instructions are not allowed at this level.
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
                if (resultOut != nullptr)
                    *resultOut = result;
                return TopLevelResult::StopThenPrompt;
            }
            if (e->tag().toString(Datum::ToStringFlags_Key) == QObject::tr("SYSTEM"))
            {
                StreamManager::get().printToTerminal("\n");
                Config::get().mainInterface()->closeInterface();
                QCoreApplication::quit();
                if (resultOut != nullptr)
                    *resultOut = result;
                return TopLevelResult::ExitProcess;
            }
            if (e->tag().toString(Datum::ToStringFlags_Key) == QObject::tr("PAUSE") && isPausing)
            {
                if (resultOut != nullptr)
                    *resultOut = e->output();
                return TopLevelResult::PauseReturn;
            }
        }
        StreamManager::get().printToTerminal(e->toString() + "\n");
        if (resultOut != nullptr)
            *resultOut = result;
        return TopLevelResult::StopThenPrompt;
    }

    if (resultOut != nullptr)
        *resultOut = result;
    return TopLevelResult::Continue;
}

Kernel::TopLevelResult Kernel::executeTopLevelList(const QString &prompt, bool isPausing, DatumPtr *resultOut)
{
    DatumPtr result;
    try
    {
        // A line read from the terminal is user interaction. UCBLogo resets its
        // ERRACT loop detector here so a bad value typed at PAUSE can be tried again.
        if (StreamManager::get().programReaderIsTerminal())
            clearErractLoop();

        DatumPtr line = StreamManager::get().readProgramList(prompt, true);

        Q_ASSERT(callFrameStack_.back().evaluationStackSize() == 0);
        Q_ASSERT(callFrameStack_.back().sourceNode_.isNothing());

        if (line.isNothing())
        {
            if (resultOut != nullptr)
                *resultOut = nothing();
            return TopLevelResult::EndOfInput;
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

    return finishTopLevelResult(result, isPausing, resultOut);
}

Kernel::TopLevelResult Kernel::runAssignedStartup(const DatumPtr &startupBefore)
{
    DatumPtr startup = datumForName(SpecialVars::name(SpecialVars::Id::Startup));
    if (!startup.isList() || startup.datumValue() == startupBefore.datumValue())
        return TopLevelResult::EndOfInput;

    callFrameStack_.emplace_back(nullptr, nullptr, 0);
    DatumPtr result;
    try
    {
        Q_ASSERT(callFrameStack_.back().evaluationStackSize() == 0);
        callFrameStack_.back().pushEvaluator(startup);
        result = runECE();
    }
    catch (FCError *e)
    {
        result = DatumPtr(e);
    }
    TopLevelResult action = finishTopLevelResult(result, false, nullptr);
    callFrameStack_.pop_back();
    return action;
}

Kernel::TopLevelResult Kernel::runLoadedProgram(const DatumPtr &startupBefore)
{
    TopLevelResult action = runProgram();
    if (action != TopLevelResult::EndOfInput)
        return action;
    return runAssignedStartup(startupBefore);
}

Kernel::TopLevelResult Kernel::loadFile(const DatumPtr &filename)
{
    DatumPtr startupBefore = datumForName(SpecialVars::name(SpecialVars::Id::Startup));
    ProgramReadScope scope{filename};
    return runLoadedProgram(startupBefore);
}

void Kernel::rememberWorkspaceFile(const DatumPtr &filename)
{
    lastWorkspaceFile_ = DatumPtr(filename.wordValue()->toString());
}

DatumPtr Kernel::lastWorkspaceFile() const
{
    return lastWorkspaceFile_;
}

void Kernel::saveWorkspace(const DatumPtr &filename)
{
    const QString name = filename.wordValue()->toString();
    if (StreamManager::get().isOpen(name))
        throw FCError::alreadyOpen(filename);

    const QString text = printoutFromContentsList(unburiedContents(true, true, true));
    ProgramWriteScope scope(filename);
    StreamManager::get().writeProgram(text);
    rememberWorkspaceFile(filename);
}

Kernel::TopLevelResult Kernel::loadText(const QString &text)
{
    DatumPtr startupBefore = datumForName(SpecialVars::name(SpecialVars::Id::Startup));
    ProgramReadTextScope scope{text};
    return runLoadedProgram(startupBefore);
}

namespace
{

QString quoteForShell(const QString &s)
{
    QString q = s;
    q.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + q + QLatin1Char('\'');
}

void writeTextFile(const QString &path, const DatumPtr &spec, const QString &text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        throw FCError::cantOpen(spec);
    QTextStream out(&file);
    out << text;
    out.flush();
    if (out.status() != QTextStream::Ok)
        throw FCError::cantOpen(spec);
}

QString readTextFile(const QString &path, const DatumPtr &spec)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        throw FCError::cantOpen(spec);
    return QString::fromUtf8(file.readAll());
}

} // namespace

void Kernel::setEditor(const DatumPtr &command)
{
    editorCommand_ = command;
}

DatumPtr Kernel::editor() const
{
    return editorCommand_;
}

void Kernel::setTempLoc(const DatumPtr &path)
{
    if (path.isWord())
        tempLoc_ = path.wordValue()->toString();
    else
        tempLoc_.clear();
}

DatumPtr Kernel::tempLoc() const
{
    return DatumPtr(resolvedTempLoc());
}

QString Kernel::resolvedTempLoc() const
{
    if (tempLoc_.isEmpty())
        return QDir::tempPath();
    return tempLoc_;
}

QString Kernel::createEditorTempFile(const QString &text)
{
    if (!keepEditorTemp_ && !editorTempPath_.isEmpty())
    {
        QFile::remove(editorTempPath_);
        editorTempPath_.clear();
    }

    const QString dir = resolvedTempLoc();
    if (!QDir(dir).exists() && !QDir().mkpath(dir))
        throw FCError::cantOpen(DatumPtr(dir));

    QTemporaryFile tmp(dir + QStringLiteral("/qlogoXXXXXX.lg"));
    tmp.setAutoRemove(false);
    if (!tmp.open())
        throw FCError::cantOpen(DatumPtr(dir));
    editorTempPath_ = tmp.fileName();
    tmp.close();
    writeTextFile(editorTempPath_, DatumPtr(editorTempPath_), text);
    return editorTempPath_;
}

bool Kernel::runEditorOnFile(const QString &path, const QString &startText, const QString &title)
{
    if (Config::get().hasGUI_)
    {
        bool accepted = false;
        const QString result = Config::get().mainInterface()->editText(startText, &accepted, title);
        if (!accepted)
            return false;
        writeTextFile(path, DatumPtr(path), result);
        return true;
    }

    QProcess proc;
    if (editorCommand_.isWord())
    {
        const QString cmd = editorCommand_.wordValue()->toString();
        if (!cmd.contains(QLatin1Char(' ')))
            proc.start(cmd, QStringList{path});
        else
        {
#ifdef _WIN32
            proc.start(QStringLiteral("cmd.exe"), QStringList{QStringLiteral("/C"), cmd + QLatin1Char(' ') + path});
#else
            proc.start(QStringLiteral("/bin/sh"),
                       QStringList{QStringLiteral("-c"), cmd + QLatin1Char(' ') + quoteForShell(path)});
#endif
        }
    }
    else
    {
        ListIterator iter = editorCommand_.listValue()->newIterator();
        const QString program = iter.element().wordValue()->toString();
        QStringList args;
        while (iter.elementExists())
            args.append(iter.element().wordValue()->toString());
        args.append(path);
        proc.start(program, args);
    }

    if (!proc.waitForStarted(-1))
        throw FCError::fileSystem();
    proc.waitForFinished(-1);
    return true;
}

Kernel::TopLevelResult Kernel::loadEditedFile(const QString &path, bool deleteAfter)
{
    const TopLevelResult action = loadFile(DatumPtr(path));
    if (action == TopLevelResult::ExitProcess)
    {
        keepEditorTemp_ = true;
        return action;
    }
    if (deleteAfter)
    {
        QFile::remove(path);
        if (path == editorTempPath_)
            editorTempPath_.clear();
    }
    return action;
}

Kernel::TopLevelResult Kernel::editContents(const DatumPtr &contentslist)
{
    DatumPtr names = contentslist;
    if (names.isNothing())
        names = unburiedContents(true, false, false);

    const QString text = printoutFromContentsList(names, false);
    const QString path = createEditorTempFile(text);
    if (!runEditorOnFile(path, text, QObject::tr("Editor")))
    {
        QFile::remove(path);
        editorTempPath_.clear();
        return TopLevelResult::EndOfInput;
    }
    return loadEditedFile(path, true);
}

Kernel::TopLevelResult Kernel::editFile(const DatumPtr &filename)
{
    const QString path = StreamManager::get().filepathForFilename(filename);
    QFileInfo info(path);
    if (!info.exists())
        writeTextFile(path, filename, QString());

    const QString text = readTextFile(path, filename);
    if (!runEditorOnFile(path, text, filename.wordValue()->toString()))
        return TopLevelResult::EndOfInput;
    return loadEditedFile(path, false);
}

Kernel::TopLevelResult Kernel::runProgram()
{
    callFrameStack_.emplace_back(nullptr, nullptr, 0);
    TopLevelResult action = TopLevelResult::EndOfInput;
    forever
    {
        action = executeTopLevelList(QString(), false);
        if (action != TopLevelResult::Continue)
            break;
    }
    callFrameStack_.pop_back();
    return action;
}

bool Kernel::runFile(const QString &path, int &exitCode)
{
    try
    {
        if (loadFile(DatumPtr(path)) == TopLevelResult::ExitProcess)
            return false;
    }
    catch (FCError *e)
    {
        DatumPtr err(e);
        StreamManager::get().printToTerminal(err.errValue()->toString() + "\n");
        exitCode = 1;
        return false;
    }
    return true;
}

DatumPtr Kernel::readEvalPrintLoop(bool isPausing, const QString &prompt)
{
    ReplScope repl;
    callFrameStack_.emplace_back(nullptr, nullptr, 0);
    QString localPrompt = prompt + "? ";
    DatumPtr result;
    forever
    {
        TopLevelResult action = executeTopLevelList(localPrompt, isPausing, &result);
        if (action == TopLevelResult::Continue)
            continue;
        if (action == TopLevelResult::StopThenPrompt)
        {
            // Uncaught TOPLEVEL in a pause REPL must leave the pause, not prompt again.
            if (isPausing && result.isErr() && result.errValue()->code_ == ErrCode::ERR_NO_CATCH &&
                result.errValue()->tag().isWord() &&
                result.errValue()->tag().toString(Datum::ToStringFlags_Key) == QObject::tr("TOPLEVEL"))
                break;
            continue;
        }
        break;
    }
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

        // Interactive TO may not replace an existing procedure. LOAD / EDITFILE
        // / a startup script / library autoload may replace a user procedure;
        // defineProcedure still rejects primitives.
        if (StreamManager::get().programReaderIsTerminal() && Procedures::get().isProcedure(procname))
            throw FCError::procDefined(procnameP);

        // Assign the procedure's parameter names and default values.
        DatumPtr parameterList = procParametersFromNode(node);

        Procedures::get().validateParameters(command, parameterList);

        QList<DatumPtr> sourceText = StreamManager::get().programReaderLineHistory();
        ListBuilder textBuilder;
        textBuilder.append(parameterList);
        Procedures::readProcedureBody(
            *StreamManager::get().programReader(), textBuilder, sourceText, QStringLiteral("> "));
        DatumPtr textP = textBuilder.finishedList();

        Procedures::get().defineProcedure(command, procnameP, textP, sourceText);

        if (StreamManager::get().programReaderIsTerminal() || specialVarIsTrue(SpecialVars::Id::LoadNoisily))
        {
            QString message = QObject::tr("%1 defined\n");
            message = message.arg(procnameP.toString());
            StreamManager::get().printToTerminal(message);
        }
    }
    catch (FCError *err)
    {
        retval = err;
    }
    return retval;
}

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

bool Kernel::specialVarIsTrue(SpecialVars::Id id) const
{
    DatumPtr val = datumForName(SpecialVars::name(id));
    if (!val.isWord())
        return false;
    return val.toString(Datum::ToStringFlags_Key) == QObject::tr("TRUE");
}

DatumPtr Kernel::allVariables(bool unburiedOnly) const
{
    ListBuilder builder;
    for (auto &varname : variables_.keys())
    {
        if (unburiedOnly && containsName(NameMark::Buried, NameKind::Variables, varname))
            continue;
        builder.append(DatumPtr(varname));
    }
    return builder.finishedList();
}

DatumPtr Kernel::contentsListFromDatum(const DatumPtr &who, const DatumPtr &source) const
{
    ListBuilder procedures;
    ListBuilder variables;
    ListBuilder properties;
    ListBuilder *sublists[3] = {&procedures, &variables, &properties};

    auto reject = [&]() { throw FCError::doesntLike(who, source); };

    if (source.isWord())
    {
        procedures.append(source);
    }
    else if (source.isList())
    {
        unsigned parseLevel = 0;
        ListIterator iter = source.listValue()->newIterator();
        while (iter.elementExists())
        {
            if (parseLevel > 2)
                reject();
            DatumPtr member = iter.element();
            if (member.isWord())
            {
                sublists[parseLevel]->append(member);
            }
            else if (member.isList())
            {
                ListIterator inner = member.listValue()->newIterator();
                while (inner.elementExists())
                {
                    DatumPtr name = inner.element();
                    if (!name.isWord())
                        reject();
                    sublists[parseLevel]->append(name);
                }
                ++parseLevel;
            }
            else
            {
                reject();
            }
        }
    }
    else
    {
        reject();
    }

    ListBuilder contents;
    contents.append(procedures.finishedList());
    contents.append(variables.finishedList());
    contents.append(properties.finishedList());
    return contents.finishedList();
}

namespace
{

QString unreadDatum(const DatumPtr &value, bool isInList);

QString unreadWord(Word *word, bool isInList)
{
    word->numberValue();
    if (word->numberIsValid)
        return word->toString();

    QString retval;
    if (!isInList)
        retval = QLatin1Char('"');

    const QString src = word->toString();
    if (src.isEmpty())
        return retval + QStringLiteral("||");

    if (word->isForeverSpecial)
    {
        retval.append(QLatin1Char('|'));
        for (auto iter = src.cbegin(); iter != src.cend(); ++iter)
        {
            const QChar letter = *iter;
            if ((iter == src.cbegin()) && (letter == QLatin1Char('"')))
                retval = QStringLiteral("\"|");
            else
            {
                if (letter == QLatin1Char('|'))
                    retval.append(QLatin1Char('\\'));
                retval.append(letter);
            }
        }
        retval.append(QLatin1Char('|'));
        return retval;
    }

    for (QChar letter : src)
    {
        if ((letter == QLatin1Char(' ')) || (letter == QLatin1Char('[')) || (letter == QLatin1Char(']')) ||
            (letter == QLatin1Char('{')) || (letter == QLatin1Char('}')) || (letter == QLatin1Char('|')) ||
            (letter == QLatin1Char('\n')))
            retval.append(QLatin1Char('\\'));
        retval.append(letter);
    }
    return retval;
}

QString unreadList(List *list, bool isInList)
{
    QString retval;
    if (isInList)
        retval = QLatin1Char('[');
    ListIterator iter = list->newIterator();
    while (iter.elementExists())
    {
        if ((retval != QLatin1String("[")) && (!retval.isEmpty()))
            retval.append(QLatin1Char(' '));
        retval += unreadDatum(iter.element(), true);
    }
    if (isInList)
        retval.append(QLatin1Char(']'));
    return retval;
}

QString unreadArray(Array *array)
{
    QString retval(QLatin1Char('{'));
    for (const auto &item : array->array)
    {
        if (retval.size() > 1)
            retval.append(QLatin1Char(' '));
        retval += unreadDatum(item, true);
    }
    retval.append(QLatin1Char('}'));
    return retval;
}

QString unreadDatum(const DatumPtr &value, bool isInList)
{
    if (value.isWord())
        return unreadWord(value.wordValue(), isInList);
    if (value.isList())
        return unreadList(value.listValue(), isInList);
    if (value.isArray())
        return unreadArray(value.arrayValue());
    Q_ASSERT(false);
    return QString();
}

/// A contents list from contentsListFromDatum is always three lists of names.
struct ContentsNames
{
    List *procedures;
    List *variables;
    List *properties;
};

ContentsNames splitContentsList(const DatumPtr &contentslist)
{
    List *src = contentslist.listValue();
    ContentsNames names;
    names.procedures = src->head.listValue();
    src = src->tail.listValue();
    names.variables = src->head.listValue();
    src = src->tail.listValue();
    names.properties = src->head.listValue();
    return names;
}

/// Format a value so PRINTOUT / SAVE can be read back as Logo.
/// A number prints without a quote. A non-numeric word is quoted and escaped.
/// A list or array is shown with brackets or braces.
QString printoutDatum(const DatumPtr &value)
{
    if (value.isWord())
        return unreadWord(value.wordValue(), false);
    if (value.isList())
        return unreadList(value.listValue(), true);
    if (value.isArray())
        return unreadArray(value.arrayValue());
    Q_ASSERT(false);
    return QString();
}

} // namespace

QString Kernel::printoutFromContentsList(const DatumPtr &contentslist, bool shouldValidate) const
{
    QString retval;
    const ContentsNames names = splitContentsList(contentslist);

    ListIterator procedures = names.procedures->newIterator();
    while (procedures.elementExists())
    {
        DatumPtr procname = procedures.element();
        DatumPtr procedureText = Procedures::get().procedureFulltext(procname, shouldValidate);
        ListIterator lines = procedureText.listValue()->newIterator();
        while (lines.elementExists())
        {
            DatumPtr line = lines.element();
            retval += line.isWord() ? line.toString() : line.toString(Datum::ToStringFlags_None);
            retval += QLatin1Char('\n');
        }
    }

    ListIterator variables = names.variables->newIterator();
    while (variables.elementExists())
    {
        DatumPtr varnameP = variables.element();
        const QString varname = varnameP.wordValue()->toString(Datum::ToStringFlags_Key);
        DatumPtr value = datumForName(varname);
        if (value.isNothing())
        {
            if (shouldValidate)
                throw FCError::noValue(varnameP);
            continue;
        }
        retval += QObject::tr("Make \"%1 %2\n").arg(varname, printoutDatum(value));
    }

    ListIterator properties = names.properties->newIterator();
    while (properties.elementExists())
    {
        DatumPtr listnameP = properties.element();
        const QString listname = listnameP.wordValue()->toString(Datum::ToStringFlags_Key);
        DatumPtr proplist = plists_.getPropertyList(listname);
        ListIterator pairs = proplist.listValue()->newIterator();
        while (pairs.elementExists())
        {
            DatumPtr nameP = pairs.element();
            DatumPtr valueP = pairs.element();
            retval += QObject::tr("Pprop %1 %2 %3\n")
                          .arg(printoutDatum(listnameP), printoutDatum(nameP), printoutDatum(valueP));
        }
    }
    return retval;
}

QString Kernel::titlesFromContentsList(const DatumPtr &contentslist) const
{
    QString retval;
    const ContentsNames names = splitContentsList(contentslist);

    ListIterator procedures = names.procedures->newIterator();
    while (procedures.elementExists())
    {
        DatumPtr procname = procedures.element();
        DatumPtr title = Procedures::get().procedureTitleList(procname);
        retval += unreadList(title.listValue(), false);
        retval += QLatin1Char('\n');
    }

    ListIterator variables = names.variables->newIterator();
    while (variables.elementExists())
    {
        DatumPtr varnameP = variables.element();
        const QString varname = varnameP.wordValue()->toString(Datum::ToStringFlags_Key);
        DatumPtr value = datumForName(varname);
        if (value.isNothing())
            throw FCError::noValue(varnameP);
        retval += QObject::tr("Make \"%1 %2\n").arg(varname, printoutDatum(value));
    }

    ListIterator properties = names.properties->newIterator();
    while (properties.elementExists())
    {
        DatumPtr listnameP = properties.element();
        const QString listname = listnameP.wordValue()->toString(Datum::ToStringFlags_Key);
        DatumPtr proplist = plists_.getPropertyList(listname);
        if (proplist.listValue()->isEmpty())
            continue;
        retval += QObject::tr("Plist %1 = %2\n").arg(printoutDatum(listnameP), printoutDatum(proplist));
    }
    return retval;
}

void Kernel::eraseFromContentsList(const DatumPtr &contentslist)
{
    const ContentsNames names = splitContentsList(contentslist);

    ListIterator procedures = names.procedures->newIterator();
    while (procedures.elementExists())
        Procedures::get().eraseProcedure(procedures.element());

    ListIterator variables = names.variables->newIterator();
    while (variables.elementExists())
    {
        DatumPtr varnameP = variables.element();
        eraseVar(varnameP.wordValue()->toString(Datum::ToStringFlags_Key));
    }

    ListIterator properties = names.properties->newIterator();
    while (properties.elementExists())
    {
        DatumPtr listnameP = properties.element();
        plists_.erasePropertyList(listnameP.wordValue()->toString(Datum::ToStringFlags_Key));
    }
}

DatumPtr Kernel::unburiedContents(bool procedures, bool variables, bool plists) const
{
    ListBuilder contents;
    contents.append(procedures ? Procedures::get().allProcedureNames(true) : emptyList());
    contents.append(variables ? allVariables(true) : emptyList());
    contents.append(plists ? plists_.allPLists(true) : emptyList());
    return contents.finishedList();
}

void Kernel::eraseUnburied(bool procedures, bool variables, bool plists)
{
    eraseFromContentsList(unburiedContents(procedures, variables, plists));
}

QSet<QString> &Kernel::namesFor(NameMark mark, NameKind kind)
{
    const auto index =
        static_cast<qsizetype>(mark) * static_cast<qsizetype>(NameKind::Count) + static_cast<qsizetype>(kind);
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

void Kernel::decrementTraceDepth()
{
    Q_ASSERT(traceDepth_ > 0);
    --traceDepth_;
}

void Kernel::restoreTraceDepth(int depth)
{
    Q_ASSERT(depth >= 0);
    Q_ASSERT(depth <= traceDepth_);
    traceDepth_ = depth;
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

    QString editor = qEnvironmentVariable("EDITOR");
    if (editor.isEmpty())
    {
#ifdef _WIN32
        editor = QStringLiteral("notepad");
#else
        editor = QStringLiteral("nano");
#endif
    }
    editorCommand_ = DatumPtr(editor);
}

Kernel::~Kernel()
{
    if (!keepEditorTemp_ && !editorTempPath_.isEmpty())
        QFile::remove(editorTempPath_);
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

bool Kernel::consumePendingSignal(DatumPtr *resultOut)
{
    switch (LogoInterface::latestSignal())
    {
    case noSignal:
        if (resultOut != nullptr)
            *resultOut = nothing();
        return false;
    case toplevelSignal:
        if (resultOut != nullptr)
            *resultOut = DatumPtr(FCError::custom(DatumPtr(QObject::tr("TOPLEVEL"))));
        return true;
    case systemSignal:
        if (resultOut != nullptr)
            *resultOut = DatumPtr(FCError::custom(DatumPtr(QObject::tr("SYSTEM"))));
        return true;
    case pauseSignal:
    {
        DatumPtr paused = pause();
        if (resultOut != nullptr)
            *resultOut = paused;
        return true;
    }
    }
    if (resultOut != nullptr)
        *resultOut = nothing();
    return false;
}

DatumPtr Kernel::runECE()
{
    const auto savedOp = nextOperation_;
    nextOperation_ = &Kernel::ece_evaluateStack;
    currentCallFrame()->retvalToParent_ = nothing();

    while (nextOperation_ != nullptr)
        (this->*nextOperation_)();

    DatumPtr result = currentCallFrame()->retvalToParent_;
    nextOperation_ = savedOp;
    return result;
}

void Kernel::ece_evaluateStack()
{
    Config::get().mainInterface()->processInputMessageQueue();
    DatumPtr signalResult;
    if (consumePendingSignal(&signalResult))
    {
        if (signalResult.isErr())
        {
            currentCallFrame()->jumpLocation_ = 0;
            if (currentCallFrame()->evaluationStackSize() > 0)
            {
                currentEvaluator()->retvalToParent_ = signalResult.datumValue();
                nextOperation_ = &Kernel::ece_popEvaluator;
            }
            else
            {
                currentCallFrame()->retvalToParent_ = signalResult;
                nextOperation_ = nullptr;
            }
            return;
        }
        // PAUSE returned a normal value: continue the interrupted instruction.
    }

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
        if (!currentCallFrame()->currentParameterName_.isEmpty())
        {
            // We have finished processing a default value for an optional parameter.
            DatumPtr defaultValue = currentCallFrame()->retvalToParent_;

            // Any result that is not a data type gets converted to a bad default error.
            if ((defaultValue.isa() & Datum::typeDataMask) == 0)
            {
                currentCallFrame()->retvalToParent_ =
                    DatumPtr(FCError::badDefault(currentCallFrame()->currentArgument_));
                nextOperation_ = &Kernel::ece_exitProcedure;
            }

            currentCallFrame()->setVarAsLocal(currentCallFrame()->currentParameterName_);
            ece_trace("ece_decideEmptyEvaluationStack: setting variable " + currentCallFrame()->currentParameterName_ +
                      " to " + defaultValue.toString());
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
    if (currentCallFrame()->retvalToParent_.isFlowControl() ||
        currentCallFrame()->runningSourceList_.listValue()->isEmpty())
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
    // The remembered ERRACT output is owned by an evaluator's release pool.
    clearErractReturnGuard();
    CallFrame *cf = currentCallFrame();
    Evaluator *e = currentEvaluator();
    DatumPtr completedList = e->list_;
    DatumPtr startingList = e->startingList_;
    cf->retvalToParent_ = DatumPtr(e->retvalToParent_);
    retvalSourceList_ = completedList;
    ece_trace("ece_popEvaluator: returning value " + cf->retvalToParent_.toString() + " from source list " +
              retvalSourceList_.toString());

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
        auto *continuation = static_cast<FCContinuation *>(currentCallFrame()->retvalToParent_.flowControlValue());
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
        auto *gotoControl = static_cast<FCGoto *>(currentCallFrame()->retvalToParent_.flowControlValue());
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
    while (!currentCallFrame()->parameters_.listValue()->isEmpty())
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
            ece_trace("ece_processParameters: setting variable " + currentCallFrame()->currentParameterName_ + " to " +
                      argument.toString());
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

namespace {

/// OUTPUT inside an instruction list arrives as FCReturn. The output itself is the result.
DatumPtr unwrapInstructionOutput(const DatumPtr &result)
{
    if (result.isa() == Datum::typeReturn)
        return static_cast<FCReturn *>(result.flowControlValue())->returnValue();
    return result;
}

/// Drop call frames pushed after @p framesBefore, and any evaluators those frames still own.
void popCallFramesTo(Kernel &kernel, std::size_t framesBefore)
{
    while (kernel.callFrameStack_.size() > framesBefore)
    {
        while (!kernel.callFrameStack_.back().isEvaluationStackViewEmpty())
            kernel.callFrameStack_.back().popEvaluator();
        kernel.callFrameStack_.pop_back();
    }
}

} // namespace

DatumPtr Kernel::pause()
{
    if (isPausing_)
    {
        StreamManager::get().printToTerminal(QObject::tr("Already Pausing\n"));
        return nothing();
    }

    // ERRACT runs on its own frame, below which the procedure that failed is still active.
    QString sourceNodeName;
    for (auto it = callFrameStack_.rbegin(); it != callFrameStack_.rend(); ++it)
    {
        if (it->sourceNode_.isASTNode())
        {
            sourceNodeName = it->sourceNode_.astnodeValue()->nodeName_.toString();
            break;
        }
    }

    // UCBLogo prints the error that invoked ERRACT only when that instruction enters PAUSE.
    if (erractMessagePending_)
    {
        erractMessagePending_ = false;
        StreamManager::get().printToTerminal(erractMessage_ + "\n");
    }

    StreamManager::get().printToTerminal(QObject::tr("Pausing...\n"));

    ++pauseCount_;
    isPausing_ = true;
    DatumPtr result;
    try
    {
        result = readEvalPrintLoop(true, sourceNodeName);
    }
    catch (...)
    {
        isPausing_ = false;
        throw;
    }
    isPausing_ = false;
    return result;
}

DatumPtr Kernel::runErractInstruction(const DatumPtr &instructionIn)
{
    DatumPtr instruction = instructionIn;
    try
    {
        if (instruction.isWord() || instruction.isList())
            instruction = runparse(instruction);
    }
    catch (FCError *err)
    {
        return DatumPtr(err);
    }

    if (!instruction.isList())
        return DatumPtr(FCError::noHow(instructionIn));

    // A fresh frame makes runECE stop when this list finishes, instead of resuming the
    // procedure that is suspended inside the error. Variables stay visible: they live in
    // the kernel table, and the procedure frame underneath is not popped.
    const std::size_t framesBefore = callFrameStack_.size();
    callFrameStack_.emplace_back(nullptr, nullptr, 0);
    DatumPtr result = nothing();
    try
    {
        CallFrame *frame = currentCallFrame();
        frame->setVarAsLocal(SpecialVars::name(SpecialVars::Id::ErrAct));
        if (!instruction.listValue()->isEmpty())
        {
            frame->pushEvaluator(instruction);
            result = runECE();
        }
    }
    catch (FCError *err)
    {
        popCallFramesTo(*this, framesBefore);
        return DatumPtr(err);
    }
    catch (...)
    {
        popCallFramesTo(*this, framesBefore);
        throw;
    }
    popCallFramesTo(*this, framesBefore);
    return unwrapInstructionOutput(result);
}

void Kernel::armErractLoop(int code)
{
    erractLoopArmed_ = true;
    erractLoopCode_ = code;
}

bool Kernel::isErractLoop(int code) const
{
    return erractLoopArmed_ && erractLoopCode_ == code;
}

void Kernel::clearErractLoop()
{
    erractLoopArmed_ = false;
    erractLoopCode_ = -1;
    clearErractReturnGuard();
}

void Kernel::clearErractReturnGuard()
{
    erractReturned_ = nullptr;
    erractReturnedIsNumber_ = false;
    erractReturnedNumber_ = 0;
    erractReturnedFromUser_ = false;
}

void Kernel::noteErractReturn(Datum *value, bool fromUser)
{
    clearErractLoop();
    erractReturned_ = value;
    erractReturnedFromUser_ = fromUser;
    if (value != nullptr && value->isa_ == Datum::typeWord)
    {
        auto *word = static_cast<Word *>(value);
        double number = word->numberValue();
        if (word->numberIsValid)
        {
            erractReturnedIsNumber_ = true;
            erractReturnedNumber_ = number;
        }
    }
}

bool Kernel::isErractReturnRetry(const DatumPtr &value) const
{
    if (erractReturned_ == nullptr || erractReturnedFromUser_)
        return false;
    if (value.datumValue() == erractReturned_)
        return true;
    if (erractReturnedIsNumber_ && value.isWord())
    {
        Word *word = value.wordValue();
        double number = word->numberValue();
        if (word->numberIsValid && number == erractReturnedNumber_)
            return true;
    }
    return false;
}

void Kernel::armErractMessage(const QString &message)
{
    erractMessage_ = message;
    erractMessagePending_ = true;
}

bool Kernel::takeErractMessagePending()
{
    bool pending = erractMessagePending_;
    erractMessagePending_ = false;
    return pending;
}

int Kernel::run()
{
    Config::get().mainInterface()->initialize();

    if (!Config::get().hasGUI_)
        LogoInterface::initSignals();

    int exitCode = 0;
    bool enterPrompt = true;

    const QFileInfo homeStartup(QDir::home().filePath(QStringLiteral("startup.lg")));
    if (homeStartup.isFile())
    {
        if (!runFile(homeStartup.absoluteFilePath(), exitCode))
            enterPrompt = false;
    }

    if (enterPrompt && !Config::get().startupScriptPath_.isEmpty())
    {
        if (!runFile(Config::get().startupScriptPath_, exitCode))
            enterPrompt = false;
    }

    if (enterPrompt)
        readEvalPrintLoop(false);

    if (!Config::get().hasGUI_)
        LogoInterface::restoreSignals();

    Compiler::releaseAllCompiledText();

    return exitCode;
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
        else if (frame->sourceNode_.isASTNode() && frame->sourceNode_.astnodeValue()->countOfChildren() > 0)
            std::cerr << "frame " << i << ": "
                      << frame->sourceNode_.astnodeValue()->nodeName_.toString(Datum::ToStringFlags_Key).toStdString()
                      << std::endl;
        else
            std::cerr << "frame " << i << ": ?" << std::endl;
        for (size_t j = 0; j < frame->evaluationStackSize(); ++j)
        {
            Evaluator *ev = frame->evaluatorAtIndex(j);
            std::cerr << "  list: " << ev->list_.toString(Datum::ToStringFlags_Show).toStdString() << std::endl;
        }
    }
}
