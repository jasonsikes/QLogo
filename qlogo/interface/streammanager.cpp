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

/// @brief One open file: the QFile and QTextStream backing plus the TextStream
/// wrapper and the mode it was opened with. The wrapper does not own the
/// QFile/QTextStream, so all three are torn down together in closeStream().
struct StreamManager::OpenStream
{
    QFile *file = nullptr;
    QTextStream *textStream = nullptr;
    TextStream *stream = nullptr;
    OpenMode mode = OpenMode::Read;
};

StreamManager::StreamManager()
    : terminalStream_(new TextStream(nullptr))
    , dribbleFile_(nullptr)
    , dribbleStream_(nullptr)
{
    // Every role starts pointed at the terminal.
    istreamStd_ = terminalStream_;
    istreamRead_ = terminalStream_;
    istreamProgram_ = terminalStream_;
    ostreamStd_ = terminalStream_;
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
    noDribble();
    delete terminalStream_;
}

StreamManager::OpenStream *StreamManager::openStream(const QString &filepath, OpenMode mode)
{
    QIODevice::OpenMode flags;
    switch (mode)
    {
    case OpenMode::Read:
        flags = QIODevice::ReadOnly;
        break;
    case OpenMode::Write:
        flags = QIODevice::WriteOnly | QIODevice::Truncate;
        break;
    case OpenMode::Append:
        flags = QIODevice::WriteOnly | QIODevice::Append;
        break;
    case OpenMode::Update:
        flags = QIODevice::ReadWrite;
        break;
    }

    auto *file = new QFile(filepath);
    if (!file->open(flags))
    {
        delete file;
        throw FCError::fileSystem();
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
    stream->textStream->flush();
    delete stream->stream;     // TextStream wrapper (non-owning of the below)
    delete stream->textStream; // QTextStream
    stream->file->close();
    delete stream->file; // QFile
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

DatumPtr StreamManager::readChar()
{
    return istreamRead_->readChar();
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
    ostreamWrite_->lprint(text);
}

void StreamManager::printSystem(const QString &text)
{
    ostreamStd_->lprint(text);
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
    if (ostreamWrite_ == terminalStream_)
        return 0;
    return ostreamWrite_->pos();
}

void StreamManager::setWritePos(qint64 pos)
{
    if (ostreamWrite_ == terminalStream_)
        return;
    ostreamWrite_->seek(pos);
}

/************ roles ************/

void StreamManager::setReader(const QString &name)
{
    if (name.isEmpty())
    {
        istreamRead_ = terminalStream_;
        return;
    }
    auto it = openStreams_.find(name);
    if (it == openStreams_.end())
        throw FCError::fileSystem();
    OpenStream *os = it.value();
    if ((os->mode != OpenMode::Read) && (os->mode != OpenMode::Update))
        throw FCError::fileSystem();
    istreamRead_ = os->stream;
}

QString StreamManager::reader() const
{
    return nameForStream(istreamRead_);
}

void StreamManager::setWriter(const QString &name)
{
    if (name.isEmpty())
    {
        ostreamWrite_ = terminalStream_;
        return;
    }
    auto it = openStreams_.find(name);
    if (it == openStreams_.end())
        throw FCError::fileSystem();
    OpenStream *os = it.value();
    if ((os->mode != OpenMode::Write) && (os->mode != OpenMode::Append) && (os->mode != OpenMode::Update))
        throw FCError::fileSystem();
    ostreamWrite_ = os->stream;
}

QString StreamManager::writer() const
{
    return nameForStream(ostreamWrite_);
}

/************ pool management ************/

void StreamManager::open(const QString &name, OpenMode mode)
{
    if (isOpen(name))
        throw FCError::fileSystem();
    QString filepath = filepathForFilename(DatumPtr(name));
    openStreams_.insert(name, openStream(filepath, mode));
}

void StreamManager::close(const QString &name)
{
    auto it = openStreams_.find(name);
    if (it == openStreams_.end())
        throw FCError::fileSystem();
    OpenStream *os = it.value();

    // A role aliasing this stream reverts to the terminal.
    if (istreamRead_ == os->stream)
        istreamRead_ = terminalStream_;
    if (ostreamWrite_ == os->stream)
        ostreamWrite_ = terminalStream_;

    openStreams_.erase(it);
    closeStream(os);
}

void StreamManager::closeAll()
{
    for (OpenStream *os : openStreams_)
        closeStream(os);
    openStreams_.clear();
    istreamRead_ = terminalStream_;
    ostreamWrite_ = terminalStream_;
}

QStringList StreamManager::allOpen() const
{
    return openStreams_.keys();
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

void StreamManager::dribble(const QString &name)
{
    noDribble();
    QString filepath = filepathForFilename(DatumPtr(name));
    auto *file = new QFile(filepath);
    if (!file->open(QIODevice::Append))
    {
        delete file;
        throw FCError::fileSystem();
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

/************ program source / destination ************/

void StreamManager::beginProgramRead(const DatumPtr &filename)
{
    QString filepath = filepathForFilename(filename);
    OpenStream *os = openStream(filepath, OpenMode::Read);
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
    OpenStream *os = openStream(filepath, append ? OpenMode::Append : OpenMode::Write);
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
