// ===-- qlogo/exports/exports_communication.cpp - Exports for the QLogo language -------*- C++ -*-===//
//
// Copyright 2017-2026 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the C library routines that are used by the QLogo compiler
/// for file and terminal input and output.
///
//===----------------------------------------------------------------------===//

#include "interface/streammanager.h"
#include "interface/logointerface.h"
#include "interface/textstream.h"
#include "workspace/exports_internal.h"
#include <limits>
#include "astnode.h"
#include "compiler.h"
#include "datum_types.h"
#include "runparser.h"
#include "workspace/evaluator.h"
#include "workspace/kernel.h"
#include "workspace/specialvars.h"
#include "flowcontrol.h"
#include "sharedconstants.h"

#include <QFile>
#include <QObject>
#include <QProcess>
#include <QStringBuilder>
#include <QTextStream>

namespace
{

Datum::ToStringFlags writeToStringFlags(bool useShow)
{
    Datum::ToStringFlags flags = useShow ? Datum::ToStringFlags_Show : Datum::ToStringFlags_None;
    if (Kernel::get().specialVarIsTrue(SpecialVars::Id::FullPrintP))
        flags = static_cast<Datum::ToStringFlags>(flags | Datum::ToStringFlags_FullPrint);
    return flags;
}

void currentPrintLimits(int &printDepthLimit, int &printWidthLimit)
{
    printDepthLimit = Kernel::get().printLimitFromSpecialVar(SpecialVars::Id::PrintDepthLimit);
    printWidthLimit = Kernel::get().printLimitFromSpecialVar(SpecialVars::Id::PrintWidthLimit);
}

} // namespace

/// Write a Datum object to the standard output device.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param datumAddr a pointer to a Datum object to print.
/// @param useShow set to true to generate output for SHOW, false for PRINT
/// @return a pointer to the owning ASTNode object or a pointer to the Error object if an error occurs.
EXPORTC addr_t stdWriteDatum(addr_t eAddr, addr_t astnodeAddr, addr_t datumAddr, bool useShow) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    Datum::ToStringFlags writeFlags = writeToStringFlags(useShow);
    auto *d = reinterpret_cast<Datum *>(datumAddr);
    int printDepthLimit = -1;
    int printWidthLimit = -1;
    currentPrintLimits(printDepthLimit, printWidthLimit);
    QString output = d->toString(writeFlags, printDepthLimit, printWidthLimit) + "\n";
    return printOrError(e, reinterpret_cast<addr_t>(node), output);
}

/// Write an array of Datum objects to the standard output device.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param datumAddr a pointer to an array of pointers to Datum objects to print.
/// @param count the number of Datum objects in the array
/// @param useShow set to true to generate output for SHOW, false for PRINT
/// @param addWhitespace set to true to add a newline to the end of the output and spaces between datums.
EXPORTC addr_t stdWriteDatumAry(addr_t eAddr, addr_t astnodeAddr, addr_t datumAddr, uint32_t count, bool useShow, bool addWhitespace) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    Datum::ToStringFlags writeFlags = writeToStringFlags(useShow);
    auto **datumAry = reinterpret_cast<Datum **>(datumAddr);
    int printDepthLimit = -1;
    int printWidthLimit = -1;
    currentPrintLimits(printDepthLimit, printWidthLimit);
    QString output;
    for (uint32_t i = 0; i < count; ++i)
    {
        Datum *d = datumAry[i];
        if ((i != 0) && addWhitespace)
            output = output % " ";
        output = output % d->toString(writeFlags, printDepthLimit, printWidthLimit);
    }
    if (addWhitespace)
        output = output % "\n";
    return printOrError(e, reinterpret_cast<addr_t>(node), output);
}

/// Read a list from the current reader (READLIST / RL).
/// @param eAddr a pointer to the Evaluator object
/// @return a list, the empty word at end of input, or an Error on I/O failure.
EXPORTC addr_t stdReadList(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    return catchError(e, [&]() -> addr_t {
        DatumPtr retval = StreamManager::get().readList(QString(), false);
        if (retval.isNothing())
        {
            auto *emptyWord = new Word(QStringLiteral(""));
            e->watch(emptyWord);
            return reinterpret_cast<addr_t>(emptyWord);
        }
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    });
}

/// Read a word from the current reader (READWORD / RW).
/// @param eAddr a pointer to the Evaluator object
/// @return a word, the empty list at end of input, or an Error on I/O failure.
EXPORTC addr_t stdReadWord(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    return catchError(e, [&]() -> addr_t {
        DatumPtr retval = StreamManager::get().readWord(QString());
        if (retval.isNothing())
        {
            List *empty = EmptyList::instance();
            e->watch(empty);
            return reinterpret_cast<addr_t>(empty);
        }
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    });
}

/// Read a raw line from the current reader (READRAWLINE).
/// @param eAddr a pointer to the Evaluator object
/// @return a word, the empty list at end of input, or an Error on I/O failure.
EXPORTC addr_t stdReadRawline(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    return catchError(e, [&]() -> addr_t {
        DatumPtr retval = StreamManager::get().readRawline(QString());
        if (retval.isNothing())
        {
            List *empty = EmptyList::instance();
            e->watch(empty);
            return reinterpret_cast<addr_t>(empty);
        }
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    });
}

/// Read characters from the current reader (READCHAR / RC / READCHARS / RCS).
/// @param eAddr a pointer to the Evaluator object
/// @param count the number of characters to read
/// @return a word of the characters read, the empty list at end of input, or an Error on I/O failure.
EXPORTC addr_t stdReadChars(addr_t eAddr, int32_t count) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    return catchError(e, [&]() -> addr_t {
        DatumPtr retval = StreamManager::get().readChars(count);
        if (retval.isNothing())
        {
            List *empty = EmptyList::instance();
            e->watch(empty);
            return reinterpret_cast<addr_t>(empty);
        }
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    });
}

/// Present a modal file dialog (FILEDIALOG). GUI only.
/// @param eAddr a pointer to the Evaluator object
/// @return a word containing the selected path, the empty list if cancelled, or an Error.
EXPORTC addr_t stdFileDialog(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    return catchError(e, [&]() -> addr_t {
        QString path = Config::get().mainInterface()->fileDialogModal();
        if (path.isEmpty())
        {
            List *empty = EmptyList::instance();
            e->watch(empty);
            return reinterpret_cast<addr_t>(empty);
        }
        auto *word = new Word(path);
        e->watch(word);
        return reinterpret_cast<addr_t>(word);
    });
}

/// Print the QLogo copyright message to the current writer (COPYRIGHT).
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @return the owning ASTNode, or an Error on I/O failure.
EXPORTC addr_t stdCopyright(addr_t eAddr, addr_t astnodeAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    QString printString = QStringLiteral(
        "QLogo and Psychi source Copyright 2017-2026 Jason Sikes\n"
        "Help and library texts Copyright (C) 1993 by the Regents of the University of California\n");
    return printOrError(e, reinterpret_cast<addr_t>(node), printString);
}

/// Compile an instruction list under a dump request. Does not execute the list.
/// A word is runparsed first, matching RUN. An empty list prints nothing.
static addr_t dumpInstructionList(addr_t eAddr, addr_t astnodeAddr, addr_t datumAddr, Compiler::DumpRequest dump)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *input = reinterpret_cast<Datum *>(datumAddr);
    return catchError(e, [&]() -> addr_t {
        if (input->isWord())
        {
            DatumPtr parsed = runparse(DatumPtr(input));
            input = parsed.datumValue();
            e->watch(input);
        }
        if (!input->isList())
        {
            FCError *err = FCError::doesntLike(node->nodeName_, DatumPtr(input));
            e->watch(err);
            return reinterpret_cast<addr_t>(err);
        }
        List *list = input->listValue();
        if (list->isEmpty())
            return reinterpret_cast<addr_t>(node);

        Compiler::DumpScope scope(dump);
        Compiler::get().compiledTextFromList(list);
        return reinterpret_cast<addr_t>(node);
    });
}

/// SHOWIR: compile the instruction list and print its IR. Does not run it.
EXPORTC addr_t stdShowIR(addr_t eAddr, addr_t astnodeAddr, addr_t datumAddr) noexcept
{
    Compiler::DumpRequest dump;
    dump.showIR = true;
    return dumpInstructionList(eAddr, astnodeAddr, datumAddr, dump);
}

/// SHOWCFG: compile the instruction list and print its CFG as DOT. Does not run it.
EXPORTC addr_t stdShowCFG(addr_t eAddr, addr_t astnodeAddr, addr_t datumAddr) noexcept
{
    Compiler::DumpRequest dump;
    dump.showCFG = true;
    return dumpInstructionList(eAddr, astnodeAddr, datumAddr, dump);
}

/// SHOWMODULEIR: compile the instruction list and print the module IR. Does not run it.
EXPORTC addr_t stdShowModuleIR(addr_t eAddr, addr_t astnodeAddr, addr_t datumAddr) noexcept
{
    Compiler::DumpRequest dump;
    dump.showModuleIR = true;
    return dumpInstructionList(eAddr, astnodeAddr, datumAddr, dump);
}

/// Return true if the datum is a valid SHELL command: a word, or a non-empty list of words.
EXPORTC bool isValidShellCommand(addr_t commandAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(commandAddr);
    if (d->isWord())
        return true;
    if (!d->isList())
        return false;
    List *list = d->listValue();
    if (list->isEmpty())
        return false;
    ListIterator iter = list->newIterator();
    while (iter.elementExists())
    {
        if (!iter.element().isWord())
            return false;
    }
    return true;
}

static QString shellCommandString(Datum *command)
{
    if (command->isWord())
        return command->wordValue()->toString();

    QString joined;
    ListIterator iter = command->listValue()->newIterator();
    while (iter.elementExists())
    {
        if (!joined.isEmpty())
            joined += QLatin1Char(' ');
        joined += iter.element().wordValue()->toString();
    }
    return joined;
}

/// Run a shell command (SHELL).
/// @param eAddr a pointer to the Evaluator object
/// @param commandAddr a word or a list of words naming the command
/// @param wordflag if true, each output line is a word; otherwise each line is parsed as a list
/// @return a list of lines, or an Error
EXPORTC addr_t stdShell(addr_t eAddr, addr_t commandAddr, bool wordflag) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *command = reinterpret_cast<Datum *>(commandAddr);
    return catchError(e, [&]() -> addr_t {
        const QString commandString = shellCommandString(command);

        QProcess proc;
#ifdef _WIN32
        proc.start(QStringLiteral("cmd.exe"), QStringList{QStringLiteral("/C"), commandString});
#else
        proc.start(QStringLiteral("/bin/sh"), QStringList{QStringLiteral("-c"), commandString});
#endif
        proc.waitForStarted(-1);
        proc.closeWriteChannel();
        proc.waitForFinished(-1);

        QByteArray result = proc.readAllStandardOutput();
        if (result.isEmpty())
        {
            List *empty = EmptyList::instance();
            e->watch(empty);
            return reinterpret_cast<addr_t>(empty);
        }

        if (result.endsWith('\n'))
            result.chop(1);

        ListBuilder builder;
        const QList<QByteArray> lines = result.split('\n');
        for (QByteArray line : lines)
        {
#ifdef _WIN32
            if (line.endsWith('\r'))
                line.chop(1);
#endif
            QString text = QString::fromLocal8Bit(line);
            if (wordflag)
            {
                builder.append(DatumPtr(text));
            }
            else
            {
                QTextStream stream(&text, QIODevice::ReadOnly);
                TextStream ts(&stream);
                DatumPtr parsed = ts.readListWithPrompt(QString(), false);
                if (parsed.isNothing())
                    builder.append(emptyList());
                else
                    builder.append(parsed);
            }
        }

        DatumPtr retval = builder.finishedList();
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    });
}

/// SETPREFIX: store the filename prefix (a word, or the empty list for none).
EXPORTC void stdSetPrefix(addr_t prefixAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(prefixAddr);
    StreamManager::get().setPrefix(DatumPtr(d));
}

/// PREFIX: return the current filename prefix, or the empty list if none.
EXPORTC addr_t stdPrefix(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr retval = StreamManager::get().prefix();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

/// Return true if the datum is a valid OPENWRITE / CLOSE / SETWRITE argument:
/// a word filename, or a non-empty list whose first element is a word (the
/// character buffer's variable name). OPENREAD, OPENAPPEND, and OPENUPDATE
/// accept only a word.
EXPORTC bool isValidOpenSpec(addr_t specAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(specAddr);
    if (d->isWord())
        return true;
    if (!d->isList())
        return false;
    List *list = d->listValue();
    if (list->isEmpty())
        return false;
    return list->head.isWord();
}

/// OPENREAD / OPENWRITE / OPENAPPEND / OPENUPDATE.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param specAddr a word filename, or (OPENWRITE only) a list whose first
///        element is a variable name for a character buffer
/// @param mode OpenMode as int32 (Read=0, Write=1, Append=2, Update=3)
/// @return the owning ASTNode, or an Error if the stream is already open or
///         the open fails.
EXPORTC addr_t stdOpen(addr_t eAddr, addr_t astnodeAddr, addr_t specAddr, int32_t mode) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *spec = reinterpret_cast<Datum *>(specAddr);
    auto openMode = static_cast<OpenMode>(mode);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        if (spec->isWord())
        {
            StreamManager::get().open(spec->wordValue()->toString(), openMode, DatumPtr(spec));
        }
        else if (openMode == OpenMode::Write)
        {
            DatumPtr nameWord = spec->listValue()->head;
            QString name = nameWord.wordValue()->toString(Datum::ToStringFlags_Key);
            StreamManager::get().openBuffer(name, DatumPtr(spec));
        }
        else
        {
            throw FCError::doesntLike(node->nodeName_, DatumPtr(spec));
        }
    });
}

static QString streamNameFromSpec(Datum *spec)
{
    if (spec->isWord())
        return spec->wordValue()->toString();
    return spec->listValue()->head.wordValue()->toString(Datum::ToStringFlags_Key);
}

/// CLOSE: close one open stream. If it was a character buffer, assign the
/// buffer contents to the named variable.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param specAddr a word filename, or a list whose first element is a variable name
/// @return the owning ASTNode, or an Error if the name is not open.
EXPORTC addr_t stdClose(addr_t eAddr, addr_t astnodeAddr, addr_t specAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *spec = reinterpret_cast<Datum *>(specAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        const QString name = streamNameFromSpec(spec);
        DatumPtr bufferContents = StreamManager::get().close(name, DatumPtr(spec));
        if (!bufferContents.isNothing())
            Kernel::get().setDatumForName(bufferContents, name);
    });
}

/// ALLOPEN: names of all currently open streams, not including dribble.
/// @param eAddr a pointer to the Evaluator object
/// @return a list of names, or the empty list if none are open.
EXPORTC addr_t stdAllOpen(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr retval = StreamManager::get().allOpen();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

/// ERASEFILE / ERF: delete a file from disk. The file must not be open.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param filenameAddr a word naming the file
/// @return the owning ASTNode, or an Error if the file is open or cannot be removed.
EXPORTC addr_t stdEraseFile(addr_t eAddr, addr_t astnodeAddr, addr_t filenameAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *filename = reinterpret_cast<Datum *>(filenameAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        StreamManager::get().eraseFile(filename->wordValue()->toString());
    });
}

/// DRIBBLE: begin copying terminal I/O to the named file.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param filenameAddr a word naming the dribble file
/// @return the owning ASTNode, or an Error if already dribbling or the file
///         cannot be opened.
EXPORTC addr_t stdDribble(addr_t eAddr, addr_t astnodeAddr, addr_t filenameAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *filename = reinterpret_cast<Datum *>(filenameAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        StreamManager::get().dribble(DatumPtr(filename));
    });
}

/// NODRIBBLE: stop dribbling and close the dribble file.
EXPORTC void stdNoDribble(void) noexcept
{
    StreamManager::get().noDribble();
}

/// Resolve a SETREAD/SETWRITE word to the name stored in the open-stream pool.
/// Files are stored under the original word text; buffers under the Key form.
static QString openStreamNameFromWord(Datum *spec)
{
    QString name = spec->wordValue()->toString();
    if (StreamManager::get().isOpen(name))
        return name;
    QString key = spec->wordValue()->toString(Datum::ToStringFlags_Key);
    if (StreamManager::get().isOpen(key))
        return key;
    return name;
}

/// SETREAD: make the named open stream the current reader, or [] for the terminal.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param specAddr a word filename, or the empty list for the terminal
/// @return the owning ASTNode, or an Error if the name is not open for reading.
EXPORTC addr_t stdSetRead(addr_t eAddr, addr_t astnodeAddr, addr_t specAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *spec = reinterpret_cast<Datum *>(specAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        if (spec->isList() && spec->listValue()->isEmpty())
        {
            StreamManager::get().setReader(QString());
        }
        else
        {
            StreamManager::get().setReader(openStreamNameFromWord(spec), DatumPtr(spec));
        }
    });
}

/// READER: name of the current reader, or the empty list if it is the terminal.
/// @param eAddr a pointer to the Evaluator object
EXPORTC addr_t stdReader(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QString name = StreamManager::get().reader();
    if (name.isEmpty())
    {
        List *empty = EmptyList::instance();
        e->watch(empty);
        return reinterpret_cast<addr_t>(empty);
    }
    auto *word = new Word(name);
    e->watch(word);
    return reinterpret_cast<addr_t>(word);
}

/// SETWRITE: make the named open stream the current writer, or [] for the terminal.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param specAddr a word filename, a list whose first element is a buffer
///        variable name, or the empty list for the terminal
/// @return the owning ASTNode, or an Error if the name is not open for writing.
EXPORTC addr_t stdSetWrite(addr_t eAddr, addr_t astnodeAddr, addr_t specAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *spec = reinterpret_cast<Datum *>(specAddr);
    return catchError(e, reinterpret_cast<addr_t>(node), [&] {
        if (spec->isList() && spec->listValue()->isEmpty())
        {
            StreamManager::get().setWriter(QString());
        }
        else
        {
            const QString name = spec->isWord() ? openStreamNameFromWord(spec) : streamNameFromSpec(spec);
            StreamManager::get().setWriter(name, DatumPtr(spec));
        }
    });
}

/// WRITER: name of the current writer, or the empty list if it is the terminal
/// (including while a REPL overlay is hiding a file writer).
/// @param eAddr a pointer to the Evaluator object
EXPORTC addr_t stdWriter(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QString name = StreamManager::get().writer();
    if (name.isEmpty())
    {
        List *empty = EmptyList::instance();
        e->watch(empty);
        return reinterpret_cast<addr_t>(empty);
    }
    auto *word = new Word(name);
    e->watch(word);
    return reinterpret_cast<addr_t>(word);
}

/// SETREADPOS: seek the current reader. No-op if the reader is the terminal.
EXPORTC void stdSetReadPos(int32_t pos) noexcept
{
    StreamManager::get().setReadPos(pos);
}

/// READPOS: current position of the reader, or 0 if the reader is the terminal.
EXPORTC double stdReadPos(void) noexcept
{
    return static_cast<double>(StreamManager::get().readPos());
}

/// SETWRITEPOS: seek the current writer. No-op if the writer is the terminal.
EXPORTC void stdSetWritePos(int32_t pos) noexcept
{
    StreamManager::get().setWritePos(pos);
}

/// WRITEPOS: current position of the writer, or 0 if the writer is the terminal.
EXPORTC double stdWritePos(void) noexcept
{
    return static_cast<double>(StreamManager::get().writePos());
}

/// EOFP / EOF?: true if the current reader has no more characters.
EXPORTC bool stdEofp(void) noexcept
{
    return StreamManager::get().readerAtEnd();
}

/// KEYP / KEY?: true if a character is waiting on the current reader.
EXPORTC bool stdKeyp(void) noexcept
{
    return StreamManager::get().keyAvailable();
}

/// LINEP / LINE?: true if a line is waiting on the current reader.
/// A file is the same as not EOFP. The terminal stays in line mode.
EXPORTC bool stdLinep(void) noexcept
{
    return StreamManager::get().lineAvailable();
}

/// CLEARTEXT / CT: clear the text console. No-op on the command-line interface.
EXPORTC void stdClearText(void) noexcept
{
    StreamManager::get().clearText();
}

/// SETCURSOR: move the text console cursor. Logo order is [column row];
/// the interface uses row, then column.
EXPORTC void stdSetCursor(int32_t row, int32_t col) noexcept
{
    StreamManager::get().setTextCursor(row, col);
}

/// CURSOR: current text console cursor as a two-number list [column row].
EXPORTC addr_t stdCursor(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    int row = 0;
    int col = 0;
    StreamManager::get().getTextCursor(row, col);
    ListBuilder builder;
    builder.append(DatumPtr(col));
    builder.append(DatumPtr(row));
    DatumPtr retval = builder.finishedList();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

/// Return true if the datum names a color (palette index, RGB/RGBA list, name, or hex).
EXPORTC bool isValidColor(addr_t colorAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    return Kernel::get().colorFromDatumPtr(color, DatumPtr(d));
}

/// SETTEXTCOLOR / SETTC: set the text console colors.
/// @param foregroundAddr a color datum
/// @param backgroundAddr a color datum, or null to leave the background unchanged
EXPORTC void stdSetTextColor(addr_t foregroundAddr, addr_t backgroundAddr) noexcept
{
    auto *foreground = reinterpret_cast<Datum *>(foregroundAddr);
    QColor fg;
    Kernel::get().colorFromDatumPtr(fg, DatumPtr(foreground));
    QColor bg;
    if (backgroundAddr)
    {
        auto *background = reinterpret_cast<Datum *>(backgroundAddr);
        Kernel::get().colorFromDatumPtr(bg, DatumPtr(background));
    }
    StreamManager::get().setTextColor(fg, bg);
}

/// SETTEXTSIZE: set the text console font point size. No-op on the command-line interface.
EXPORTC void stdSetTextSize(double size) noexcept
{
    StreamManager::get().setTextFontSize(size);
}

/// TEXTSIZE: current text console font point size. 12 on the command-line interface.
EXPORTC double stdTextSize(void) noexcept
{
    return StreamManager::get().textFontSize();
}

/// SETFONT: set the text console font family. No-op on the command-line interface.
EXPORTC void stdSetFont(addr_t nameAddr) noexcept
{
    auto *w = reinterpret_cast<Word *>(nameAddr);
    StreamManager::get().setTextFontName(w->toString());
}

/// FONT: current text console font family. Empty word on the command-line interface.
EXPORTC addr_t stdFont(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QString name = StreamManager::get().textFontName();
    if (name.isNull())
        name = QStringLiteral("");
    auto *word = new Word(name);
    e->watch(word);
    return reinterpret_cast<addr_t>(word);
}

/// ALLFONTS: names of fonts available on the system, as a Logo list of words.
EXPORTC addr_t stdAllFonts(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    ListBuilder builder;
    const QStringList fonts = StreamManager::get().allFontNames();
    for (const QString &name : fonts)
    {
        builder.append(DatumPtr(new Word(name)));
    }
    DatumPtr retval = builder.finishedList();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

/// WAIT: delay for the given number of 60ths of a second.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param sixtieths delay in 60ths of a second, already validated as >= 0
/// @return the owning ASTNode, or an Error if the GUI wait is interrupted.
EXPORTC addr_t stdWait(addr_t eAddr, addr_t astnodeAddr, double sixtieths) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    return catchError(e, [&]() -> addr_t {
        double msecs = (1000.0 / 60.0) * sixtieths;
        unsigned long waitMs = 0;
        if (msecs > 0)
        {
            if (msecs >= static_cast<double>(std::numeric_limits<unsigned long>::max()))
                waitMs = std::numeric_limits<unsigned long>::max();
            else
                waitMs = static_cast<unsigned long>(msecs);
        }
        Config::get().mainInterface()->mwait(waitMs);
        DatumPtr signalResult;
        if (Kernel::get().consumePendingSignal(&signalResult) && signalResult.isErr())
        {
            e->watch(signalResult.datumValue());
            return reinterpret_cast<addr_t>(signalResult.datumValue());
        }
        return reinterpret_cast<addr_t>(node);
    });
}

