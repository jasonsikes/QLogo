//===-- qlogo/streammanager.cpp - StreamManager class implementation ---*- C++ -*-===//
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
/// This file contains the implementation of the StreamManager class, which owns
/// every open text stream, tracks which stream fills each I/O role, and backs
/// the Logo file/terminal I/O primitives (including dribbling).
///
//===----------------------------------------------------------------------===//

#include "interface/streammanager.h"
#include "interface/logointerface.h"
#include "interface/textstream.h"
#include "datum_types.h"
#include "flowcontrol.h"
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QTextStream>

/// @brief One open stream: a file or an in-memory character buffer, the
/// QTextStream wrapping it, the TextStream wrapper, and the mode it was opened
/// with. The wrapper does not own the QFile/QString/QTextStream, so they are
/// torn down together in closeStream().
struct StreamManager::OpenStream
{
    QFile *file = nullptr;
    QString *buffer = nullptr;
    QTextStream *textStream = nullptr;
    TextStream *stream = nullptr;
    OpenMode mode = OpenMode::Read;
};

static QIODevice::OpenMode flagsForMode(OpenMode mode)
{
    switch (mode)
    {
    case OpenMode::Read:
        return QIODevice::ReadOnly;
    case OpenMode::Write:
        return QIODevice::WriteOnly | QIODevice::Truncate;
    case OpenMode::Append:
        return QIODevice::WriteOnly | QIODevice::Append;
    case OpenMode::Update:
        return QIODevice::ReadWrite;
    }
    Q_ASSERT(false);
    return QIODevice::ReadOnly;
}

StreamManager::StreamManager()
    : terminalStream_(new TextStream(nullptr))
    , dribbleFile_(nullptr)
    , dribbleStream_(nullptr)
{
    // Every role starts pointed at the terminal.
    istreamRead_ = terminalStream_;
    istreamProgram_ = terminalStream_;
    ostreamWrite_ = terminalStream_;
    ostreamProgram_ = terminalStream_;

    filePrefix_ = emptyList();
}

StreamManager::~StreamManager()
{
    closeAll();
    while (!programReadStack_.isEmpty())
        closeStream(programReadStack_.takeLast());
    while (!programWriteStack_.isEmpty())
        closeStream(programWriteStack_.takeLast());
    replStack_.clear();
    noDribble();
    delete terminalStream_;
}

bool StreamManager::isInteractiveCommander() const
{
    return !replStack_.isEmpty() && istreamProgram_ == terminalStream_;
}

bool StreamManager::writerOverlayActive() const
{
    return isInteractiveCommander() && replStack_.last().overlayActive;
}

TextStream *StreamManager::activeWriter() const
{
    return writerOverlayActive() ? terminalStream_ : ostreamWrite_;
}

StreamManager::OpenStream *StreamManager::openStream(const QString &filepath, OpenMode mode, const DatumPtr &filename)
{
    QIODevice::OpenMode flags = flagsForMode(mode);

    auto *file = new QFile(filepath);
    if (!file->open(flags))
    {
        delete file;
        throw FCError::cantOpen(filename);
    }

    // OPENUPDATE positions the file at the end for both reading and writing.
    if (mode == OpenMode::Update)
        file->seek(file->size());

    auto *textStream = new QTextStream(file);
    auto *os = new OpenStream;
    os->file = file;
    os->textStream = textStream;
    os->stream = new TextStream(textStream);
    os->mode = mode;
    return os;
}

void StreamManager::closeStream(OpenStream *stream)
{
    if (stream == nullptr)
        return;
    if (stream->textStream != nullptr)
        stream->textStream->flush();
    delete stream->stream;     // TextStream wrapper (non-owning of the below)
    delete stream->textStream; // QTextStream
    if (stream->file != nullptr)
    {
        stream->file->close();
        delete stream->file;
    }
    delete stream->buffer;
    delete stream;
}

QString StreamManager::nameForStream(const TextStream *stream) const
{
    if (stream == terminalStream_)
        return {};
    for (auto it = openStreams_.cbegin(); it != openStreams_.cend(); ++it)
    {
        if (it.value()->stream == stream)
            return it.key();
    }
    return {};
}

/************ current-reader input ************/

DatumPtr StreamManager::readList(const QString &prompt, bool shouldRemoveComments)
{
    return istreamRead_->readListWithPrompt(prompt, shouldRemoveComments);
}

DatumPtr StreamManager::readWord(const QString &prompt)
{
    return istreamRead_->readWordWithPrompt(prompt);
}

DatumPtr StreamManager::readRawline(const QString &prompt)
{
    return istreamRead_->readRawlineWithPrompt(prompt);
}

DatumPtr StreamManager::readChars(int32_t count)
{
    return istreamRead_->readChars(count);
}

QList<DatumPtr> StreamManager::readerLineHistory() const
{
    return istreamRead_->listReaderLineHistory();
}

bool StreamManager::readerAtEnd() const
{
    if (istreamRead_ == terminalStream_)
        return Config::get().mainInterface()->atEnd();
    return istreamRead_->atEnd();
}

bool StreamManager::keyAvailable() const
{
    if (istreamRead_ == terminalStream_)
        return Config::get().mainInterface()->keyQueueHasChars();
    return !istreamRead_->atEnd();
}

/************ current-writer output ************/

void StreamManager::print(const QString &text)
{
    activeWriter()->lprint(text);
}

void StreamManager::printToTerminal(const QString &text)
{
    terminalStream_->lprint(text);
}

/************ read/write positions ************/

qint64 StreamManager::readPos() const
{
    if (istreamRead_ == terminalStream_)
        return 0;
    return istreamRead_->pos();
}

void StreamManager::setReadPos(qint64 pos)
{
    if (istreamRead_ == terminalStream_)
        return;
    istreamRead_->seek(pos);
}

qint64 StreamManager::writePos() const
{
    TextStream *writer = activeWriter();
    if (writer == terminalStream_)
        return 0;
    return writer->pos();
}

void StreamManager::setWritePos(qint64 pos)
{
    TextStream *writer = activeWriter();
    if (writer == terminalStream_)
        return;
    writer->seek(pos);
}

/************ roles ************/

void StreamManager::setReader(const QString &name, const DatumPtr &spec)
{
    if (name.isEmpty() && spec.isNothing())
    {
        istreamRead_ = terminalStream_;
        return;
    }
    auto it = openStreams_.find(name);
    if (it == openStreams_.end())
        throw FCError::notOpen(spec.isNothing() ? DatumPtr(name) : spec);
    OpenStream *os = it.value();
    if ((os->mode != OpenMode::Read) && (os->mode != OpenMode::Update))
        throw FCError::fileSystem();
    istreamRead_ = os->stream;
}

QString StreamManager::reader() const
{
    return nameForStream(istreamRead_);
}

void StreamManager::setWriter(const QString &name, const DatumPtr &spec)
{
    if (name.isEmpty() && spec.isNothing())
    {
        ostreamWrite_ = terminalStream_;
    }
    else
    {
        auto it = openStreams_.find(name);
        if (it == openStreams_.end())
            throw FCError::notOpen(spec.isNothing() ? DatumPtr(name) : spec);
        OpenStream *os = it.value();
        if ((os->mode != OpenMode::Write) && (os->mode != OpenMode::Append) && (os->mode != OpenMode::Update))
            throw FCError::fileSystem();
        ostreamWrite_ = os->stream;
    }

    // SETWRITE at a human prompt ends this session's overlay so PRINT follows
    // the new writer. SETWRITE during LOAD leaves the overlay in place.
    if (isInteractiveCommander())
        replStack_.last().overlayActive = false;
}

QString StreamManager::writer() const
{
    return nameForStream(activeWriter());
}

/************ pool management ************/

void StreamManager::open(const QString &name, OpenMode mode, const DatumPtr &spec)
{
    if (isOpen(name))
        throw FCError::alreadyOpen(spec);
    QString filepath = filepathForFilename(DatumPtr(name));
    openStreams_.insert(name, openStream(filepath, mode, spec));
}

void StreamManager::openBuffer(const QString &name, const QString &initial, OpenMode mode, const DatumPtr &spec)
{
    if (isOpen(name))
        throw FCError::alreadyOpen(spec);

    auto *buffer = new QString(initial);
    auto *textStream = new QTextStream(buffer, flagsForMode(mode));
    if (mode == OpenMode::Update)
        textStream->seek(buffer->size());

    auto *os = new OpenStream;
    os->buffer = buffer;
    os->textStream = textStream;
    os->stream = new TextStream(textStream);
    os->mode = mode;
    openStreams_.insert(name, os);
}

DatumPtr StreamManager::close(const QString &name, const DatumPtr &spec)
{
    auto it = openStreams_.find(name);
    if (it == openStreams_.end())
        throw FCError::notOpen(spec);
    OpenStream *os = it.value();

    // Reader/writer aliasing this stream revert to the terminal. Program and
    // REPL stacks do not hold pool streams, so they are left alone.
    if (istreamRead_ == os->stream)
        istreamRead_ = terminalStream_;
    if (ostreamWrite_ == os->stream)
        ostreamWrite_ = terminalStream_;

    DatumPtr bufferContents = nothing();
    if (os->buffer != nullptr)
    {
        if (os->textStream != nullptr)
            os->textStream->flush();
        QString contents = *os->buffer;
        if (contents.isNull())
            contents = QStringLiteral("");
        bufferContents = DatumPtr(contents);
    }

    openStreams_.erase(it);
    closeStream(os);
    return bufferContents;
}

void StreamManager::closeAll()
{
    for (OpenStream *os : openStreams_)
        closeStream(os);
    openStreams_.clear();
    istreamRead_ = terminalStream_;
    ostreamWrite_ = terminalStream_;
}

DatumPtr StreamManager::allOpen() const
{
    ListBuilder builder;
    for (const auto &name : openStreams_.keys())
        builder.append(DatumPtr(name));
    return builder.finishedList();
}

bool StreamManager::isOpen(const QString &name) const
{
    return openStreams_.contains(name);
}

void StreamManager::eraseFile(const QString &name)
{
    if (isOpen(name))
        throw FCError::fileSystem();
    QString filepath = filepathForFilename(DatumPtr(name));
    if (!QFile::remove(filepath))
        throw FCError::fileSystem();
}

/************ dribble ************/

void StreamManager::dribble(const DatumPtr &filename)
{
    if (isDribbling())
        throw FCError::alreadyDribbling();
    QString filepath = filepathForFilename(filename);
    auto *file = new QFile(filepath);
    if (!file->open(QIODevice::Append))
    {
        delete file;
        throw FCError::cantOpen(filename);
    }
    dribbleFile_ = file;
    dribbleStream_ = new QTextStream(file);
}

void StreamManager::noDribble()
{
    if (dribbleStream_ != nullptr)
    {
        dribbleStream_->flush();
        delete dribbleStream_;
        dribbleStream_ = nullptr;
    }
    if (dribbleFile_ != nullptr)
    {
        dribbleFile_->close();
        delete dribbleFile_;
        dribbleFile_ = nullptr;
    }
}

bool StreamManager::isDribbling() const
{
    return dribbleStream_ != nullptr;
}

void StreamManager::teeToDribble(const QString &text)
{
    if (dribbleStream_ != nullptr)
        *dribbleStream_ << text;
}

/************ terminal transport ************/

void StreamManager::writeTerminal(const QString &text)
{
    Config::get().mainInterface()->printToConsole(text);
    teeToDribble(text);
}

QString StreamManager::readTerminalRawline(const QString &prompt)
{
    QString line = Config::get().mainInterface()->inputRawlineWithPrompt(prompt);
    // A null line means end of input; nothing was displayed, so nothing is
    // dribbled. Otherwise mirror the prompt and the line the user supplied.
    if (!line.isNull())
    {
        teeToDribble(prompt);
        teeToDribble(line + '\n');
    }
    return line;
}

DatumPtr StreamManager::readTerminalChar()
{
    return Config::get().mainInterface()->readchar();
}

/************ program source / destination ************/

void StreamManager::beginProgramRead(const DatumPtr &filename)
{
    QString filepath = filepathForFilename(filename);
    OpenStream *os = openStream(filepath, OpenMode::Read, filename);
    programReadStack_.append(os);
    istreamProgram_ = os->stream;
}

DatumPtr StreamManager::readProgramList(const QString &prompt, bool shouldRemoveComments)
{
    return istreamProgram_->readListWithPrompt(prompt, shouldRemoveComments);
}

QList<DatumPtr> StreamManager::programReaderLineHistory() const
{
    return istreamProgram_->listReaderLineHistory();
}

bool StreamManager::programReaderAtEnd() const
{
    if (istreamProgram_ == terminalStream_)
        return Config::get().mainInterface()->atEnd();
    return istreamProgram_->atEnd();
}

void StreamManager::endProgramRead()
{
    if (programReadStack_.isEmpty())
        return;
    closeStream(programReadStack_.takeLast());
    istreamProgram_ = programReadStack_.isEmpty() ? terminalStream_ : programReadStack_.last()->stream;
}

void StreamManager::beginProgramWrite(const DatumPtr &filename, bool append)
{
    QString filepath = filepathForFilename(filename);
    OpenStream *os = openStream(filepath, append ? OpenMode::Append : OpenMode::Write, filename);
    programWriteStack_.append(os);
    ostreamProgram_ = os->stream;
}

void StreamManager::writeProgram(const QString &text)
{
    ostreamProgram_->lprint(text);
}

void StreamManager::endProgramWrite()
{
    if (programWriteStack_.isEmpty())
        return;
    closeStream(programWriteStack_.takeLast());
    ostreamProgram_ = programWriteStack_.isEmpty() ? terminalStream_ : programWriteStack_.last()->stream;
}

/************ REPL / PAUSE ************/

void StreamManager::beginRepl()
{
    ReplFrame frame;
    frame.savedCommander = istreamProgram_;
    frame.overlayActive = (ostreamWrite_ != terminalStream_);
    replStack_.append(frame);
    istreamProgram_ = terminalStream_;
}

void StreamManager::endRepl()
{
    if (replStack_.isEmpty())
        return;
    istreamProgram_ = replStack_.last().savedCommander;
    replStack_.removeLast();
}

/************ file naming ************/

void StreamManager::setPrefix(const DatumPtr &prefix)
{
    filePrefix_ = prefix;
}

DatumPtr StreamManager::prefix() const
{
    return filePrefix_;
}

QString StreamManager::filepathForFilename(const DatumPtr &filenameP) const
{
    QString filename = filenameP.wordValue()->toString();

    if (filePrefix_.isWord())
    {
        QString prefix = filePrefix_.wordValue()->toString();
        return prefix + QDir::separator() + filename;
    }
    return filename;
}
