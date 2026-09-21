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
#include "datum_types.h"
#include "workspace/kernel.h"
#include "flowcontrol.h"
#include "sharedconstants.h"

#include <QFile>
#include <QObject>
#include <QProcess>
#include <QStringBuilder>
#include <QTextStream>

/// Write a Datum object to the standard output device.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param datumAddr a pointer to a Datum object to print.
/// @param useShow set to true to generate output for SHOW, false for PRINT
/// @return a pointer to the owning ASTNode object or a pointer to the Error object if an error occurs.
EXPORTC addr_t stdWriteDatum(addr_t eAddr, addr_t astnodeAddr, addr_t datumAddr, bool useShow)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    Datum::ToStringFlags writeFlags = useShow ? Datum::ToStringFlags_Show : Datum::ToStringFlags_None;
    auto *d = reinterpret_cast<Datum *>(datumAddr);
    QString output = d->toString(writeFlags) + "\n";
    try {
        StreamManager::get().print(output);
    } catch (FCError *err) {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(astnodeAddr);
}

/// Write an array of Datum objects to the standard output device.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param datumAddr a pointer to an array of pointers to Datum objects to print.
/// @param count the number of Datum objects in the array
/// @param useShow set to true to generate output for SHOW, false for PRINT
/// @param addWhitespace set to true to add a newline to the end of the output and spaces between datums.
EXPORTC addr_t stdWriteDatumAry(addr_t eAddr, addr_t astnodeAddr, addr_t datumAddr, uint32_t count, bool useShow, bool addWhitespace)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    Datum::ToStringFlags writeFlags = useShow ? Datum::ToStringFlags_Show : Datum::ToStringFlags_None;
    auto **datumAry = reinterpret_cast<Datum **>(datumAddr);
    QString output;
    for (uint32_t i = 0; i < count; ++i)
    {
        Datum *d = datumAry[i];
        if ((i != 0) && addWhitespace)
            output = output % " ";
        output = output % d->toString(writeFlags);
    }
    if (addWhitespace)
        output = output % "\n";
        try {
            StreamManager::get().print(output);
        } catch (FCError *err) {
            e->watch(err);
            return reinterpret_cast<addr_t>(err);
        }
        return reinterpret_cast<addr_t>(astnodeAddr);
    }

/// Read a list from the current reader (READLIST / RL).
/// @param eAddr a pointer to the Evaluator object
/// @return a list, the empty word at end of input, or an Error on I/O failure.
EXPORTC addr_t stdReadList(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    try
    {
        DatumPtr retval = StreamManager::get().readList(QString(), false);
        if (retval.isNothing())
        {
            auto *emptyWord = new Word(QStringLiteral(""));
            e->watch(emptyWord);
            return reinterpret_cast<addr_t>(emptyWord);
        }
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// Read a word from the current reader (READWORD / RW).
/// @param eAddr a pointer to the Evaluator object
/// @return a word, the empty list at end of input, or an Error on I/O failure.
EXPORTC addr_t stdReadWord(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    try
    {
        DatumPtr retval = StreamManager::get().readWord(QString());
        if (retval.isNothing())
        {
            List *empty = EmptyList::instance();
            e->watch(empty);
            return reinterpret_cast<addr_t>(empty);
        }
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// Read a raw line from the current reader (READRAWLINE).
/// @param eAddr a pointer to the Evaluator object
/// @return a word, the empty list at end of input, or an Error on I/O failure.
EXPORTC addr_t stdReadRawline(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    try
    {
        DatumPtr retval = StreamManager::get().readRawline(QString());
        if (retval.isNothing())
        {
            List *empty = EmptyList::instance();
            e->watch(empty);
            return reinterpret_cast<addr_t>(empty);
        }
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// Read characters from the current reader (READCHAR / RC / READCHARS / RCS).
/// @param eAddr a pointer to the Evaluator object
/// @param count the number of characters to read
/// @return a word of the characters read, the empty list at end of input, or an Error on I/O failure.
EXPORTC addr_t stdReadChars(addr_t eAddr, int32_t count)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    try
    {
        DatumPtr retval = StreamManager::get().readChars(count);
        if (retval.isNothing())
        {
            List *empty = EmptyList::instance();
            e->watch(empty);
            return reinterpret_cast<addr_t>(empty);
        }
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// Present a modal file dialog (FILEDIALOG). GUI only.
/// @param eAddr a pointer to the Evaluator object
/// @return a word containing the selected path, the empty list if cancelled, or an Error.
EXPORTC addr_t stdFileDialog(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    try
    {
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
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// Print the QLogo copyright message to the current writer (COPYRIGHT).
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @return the owning ASTNode, or an Error on I/O failure.
EXPORTC addr_t stdCopyright(addr_t eAddr, addr_t astnodeAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    QString printString = QStringLiteral(
        "QLogo and Psychi source Copyright 2017-2026 Jason Sikes\n"
        "Help and library texts Copyright (C) 1993 by the Regents of the University of California\n");
    try
    {
        StreamManager::get().print(printString);
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(node);
}

/// Return true if the datum is a valid SHELL command: a word, or a non-empty list of words.
EXPORTC bool isValidShellCommand(addr_t commandAddr)
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
EXPORTC addr_t stdShell(addr_t eAddr, addr_t commandAddr, bool wordflag)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *command = reinterpret_cast<Datum *>(commandAddr);
    try
    {
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
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
}

/// SETPREFIX: store the filename prefix (a word, or the empty list for none).
EXPORTC void stdSetPrefix(addr_t prefixAddr)
{
    auto *d = reinterpret_cast<Datum *>(prefixAddr);
    StreamManager::get().setPrefix(DatumPtr(d));
}

/// PREFIX: return the current filename prefix, or the empty list if none.
EXPORTC addr_t stdPrefix(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr retval = StreamManager::get().prefix();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

/// Return true if the datum is a valid OPEN* argument: a word, or a non-empty
/// list whose first element is a word (the buffer's variable name).
EXPORTC bool isValidOpenSpec(addr_t specAddr)
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
/// @param specAddr a word filename, or a list whose first element is a variable name
/// @param mode OpenMode as int32 (Read=0, Write=1, Append=2, Update=3)
/// @return the owning ASTNode, or an Error if the stream is already open, the
///         open fails, or OPENREAD names a variable that has no value.
EXPORTC addr_t stdOpen(addr_t eAddr, addr_t astnodeAddr, addr_t specAddr, int32_t mode)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *spec = reinterpret_cast<Datum *>(specAddr);
    auto openMode = static_cast<OpenMode>(mode);
    try
    {
        if (spec->isWord())
        {
            StreamManager::get().open(spec->wordValue()->toString(), openMode, DatumPtr(spec));
        }
        else
        {
            DatumPtr nameWord = spec->listValue()->head;
            QString name = nameWord.wordValue()->toString(Datum::ToStringFlags_Key);
            if (openMode == OpenMode::Read && !Kernel::get().doesExist(name))
            {
                throw FCError::noValue(nameWord);
            }
            QString initial;
            DatumPtr existing = Kernel::get().datumForName(name);
            if (existing.isWord())
                initial = existing.wordValue()->toString();
            StreamManager::get().openBuffer(name, initial, openMode, DatumPtr(spec));
        }
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(node);
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
EXPORTC addr_t stdClose(addr_t eAddr, addr_t astnodeAddr, addr_t specAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *spec = reinterpret_cast<Datum *>(specAddr);
    try
    {
        const QString name = streamNameFromSpec(spec);
        DatumPtr bufferContents = StreamManager::get().close(name, DatumPtr(spec));
        if (!bufferContents.isNothing())
            Kernel::get().setDatumForName(bufferContents, name);
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(node);
}

/// ALLOPEN: names of all currently open streams, not including dribble.
/// @param eAddr a pointer to the Evaluator object
/// @return a list of names, or the empty list if none are open.
EXPORTC addr_t stdAllOpen(addr_t eAddr)
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
EXPORTC addr_t stdEraseFile(addr_t eAddr, addr_t astnodeAddr, addr_t filenameAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *filename = reinterpret_cast<Datum *>(filenameAddr);
    try
    {
        StreamManager::get().eraseFile(filename->wordValue()->toString());
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(node);
}

/// DRIBBLE: begin copying terminal I/O to the named file.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param filenameAddr a word naming the dribble file
/// @return the owning ASTNode, or an Error if already dribbling or the file
///         cannot be opened.
EXPORTC addr_t stdDribble(addr_t eAddr, addr_t astnodeAddr, addr_t filenameAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *filename = reinterpret_cast<Datum *>(filenameAddr);
    try
    {
        StreamManager::get().dribble(DatumPtr(filename));
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(node);
}

/// NODRIBBLE: stop dribbling and close the dribble file.
EXPORTC void stdNoDribble(void)
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
/// @param specAddr a word filename, a list whose first element is a buffer
///        variable name, or the empty list for the terminal
/// @return the owning ASTNode, or an Error if the name is not open for reading.
EXPORTC addr_t stdSetRead(addr_t eAddr, addr_t astnodeAddr, addr_t specAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *spec = reinterpret_cast<Datum *>(specAddr);
    try
    {
        if (spec->isList() && spec->listValue()->isEmpty())
        {
            StreamManager::get().setReader(QString());
        }
        else
        {
            const QString name = spec->isWord() ? openStreamNameFromWord(spec) : streamNameFromSpec(spec);
            StreamManager::get().setReader(name, DatumPtr(spec));
        }
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(node);
}

/// READER: name of the current reader, or the empty list if it is the terminal.
/// @param eAddr a pointer to the Evaluator object
EXPORTC addr_t stdReader(addr_t eAddr)
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
EXPORTC addr_t stdSetWrite(addr_t eAddr, addr_t astnodeAddr, addr_t specAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto *spec = reinterpret_cast<Datum *>(specAddr);
    try
    {
        if (spec->isList() && spec->listValue()->isEmpty())
        {
            StreamManager::get().setWriter(QString());
        }
        else
        {
            const QString name = spec->isWord() ? openStreamNameFromWord(spec) : streamNameFromSpec(spec);
            StreamManager::get().setWriter(name, DatumPtr(spec));
        }
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(node);
}

/// WRITER: name of the current writer, or the empty list if it is the terminal
/// (including while a REPL overlay is hiding a file writer).
/// @param eAddr a pointer to the Evaluator object
EXPORTC addr_t stdWriter(addr_t eAddr)
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
EXPORTC void stdSetReadPos(int32_t pos)
{
    StreamManager::get().setReadPos(pos);
}

/// READPOS: current position of the reader, or 0 if the reader is the terminal.
EXPORTC double stdReadPos(void)
{
    return static_cast<double>(StreamManager::get().readPos());
}

/// SETWRITEPOS: seek the current writer. No-op if the writer is the terminal.
EXPORTC void stdSetWritePos(int32_t pos)
{
    StreamManager::get().setWritePos(pos);
}

/// WRITEPOS: current position of the writer, or 0 if the writer is the terminal.
EXPORTC double stdWritePos(void)
{
    return static_cast<double>(StreamManager::get().writePos());
}

/// EOFP / EOF?: true if the current reader has no more characters.
EXPORTC bool stdEofp(void)
{
    return StreamManager::get().readerAtEnd();
}

/// KEYP / KEY?: true if a character is waiting on the current reader.
EXPORTC bool stdKeyp(void)
{
    return StreamManager::get().keyAvailable();
}

/// CLEARTEXT / CT: clear the text console. No-op on the command-line interface.
EXPORTC void stdClearText(void)
{
    StreamManager::get().clearText();
}

/// SETCURSOR: move the text console cursor. Logo order is [column row];
/// the interface uses row, then column.
EXPORTC void stdSetCursor(int32_t row, int32_t col)
{
    StreamManager::get().setTextCursor(row, col);
}

/// CURSOR: current text console cursor as a two-number list [column row].
EXPORTC addr_t stdCursor(addr_t eAddr)
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
EXPORTC bool isValidColor(addr_t colorAddr)
{
    auto *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    return Kernel::get().colorFromDatumPtr(color, DatumPtr(d));
}

/// SETTEXTCOLOR / SETTC: set the text console colors.
/// @param foregroundAddr a color datum
/// @param backgroundAddr a color datum, or null to leave the background unchanged
EXPORTC void stdSetTextColor(addr_t foregroundAddr, addr_t backgroundAddr)
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
EXPORTC void stdSetTextSize(double size)
{
    StreamManager::get().setTextFontSize(size);
}

/// TEXTSIZE: current text console font point size. 12 on the command-line interface.
EXPORTC double stdTextSize(void)
{
    return StreamManager::get().textFontSize();
}

/// SETFONT: set the text console font family. No-op on the command-line interface.
EXPORTC void stdSetFont(addr_t nameAddr)
{
    auto *w = reinterpret_cast<Word *>(nameAddr);
    StreamManager::get().setTextFontName(w->toString());
}

/// FONT: current text console font family. Empty word on the command-line interface.
EXPORTC addr_t stdFont(addr_t eAddr)
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
EXPORTC addr_t stdAllFonts(addr_t eAddr)
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

/// CURSORINSERT / CURSOROVERWRITE: set the text console cursor mode.
/// @param overwrite true for overwrite mode, false for insert mode.
EXPORTC void stdSetCursorOverwriteMode(bool overwrite)
{
    StreamManager::get().setCursorOverwriteMode(overwrite);
}

/// CURSORMODE: current text console cursor mode as INSERT or OVERWRITE.
EXPORTC addr_t stdCursorMode(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    const bool overwrite = StreamManager::get().cursorOverwriteMode();
    auto *word = new Word(overwrite ? QObject::tr("OVERWRITE") : QObject::tr("INSERT"));
    e->watch(word);
    return reinterpret_cast<addr_t>(word);
}

/// WAIT: delay for the given number of 60ths of a second.
/// @param eAddr a pointer to the Evaluator object
/// @param astnodeAddr a pointer to the owning ASTNode object
/// @param sixtieths delay in 60ths of a second, already validated as >= 0
/// @return the owning ASTNode, or an Error if the GUI wait is interrupted.
EXPORTC addr_t stdWait(addr_t eAddr, addr_t astnodeAddr, double sixtieths)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    try
    {
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
    }
    catch (FCError *err)
    {
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }
    return reinterpret_cast<addr_t>(node);
}

