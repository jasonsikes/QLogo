#ifndef STREAMMANAGER_H
#define STREAMMANAGER_H

//===-- qlogo/streammanager.h - StreamManager class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the StreamManager class, which owns
/// every open text stream and tracks which stream currently fills each I/O
/// role (current reader, current writer, dribble, etc.). It is the single
/// backing point for the Logo file/terminal I/O primitives.
///
//===----------------------------------------------------------------------===//

#include "datum_ptr.h"
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

class TextStream;
class QFile;
class QTextStream;

/// @brief The mode a file was opened with.
/// @details Determines which roles a stream may fill: a Read stream may be the
/// current reader, a Write/Append stream may be the current writer, and an
/// Update stream may be either.
enum class OpenMode
{
    Read,   ///< OPENREAD: read from the beginning; must exist.
    Write,  ///< OPENWRITE: truncate (or create) for writing.
    Append, ///< OPENAPPEND: write at the end; created if absent.
    Update  ///< OPENUPDATE: read and write, positioned at the end.
};

/// @brief Owns the pool of open text streams and the current-role assignments.
/// @details There are two layers here:
///   1. A *pool* of open streams: the terminal (always present, never closed)
///      plus one entry per file opened via OPENREAD/OPENWRITE/etc.
///   2. A small set of *role pointers* that alias entries in the pool: the
///      current reader/writer, the program source/destination, and the
///      terminal-only std streams. Role pointers are non-owning and always
///      valid; a role not otherwise assigned points at the terminal. Closing a
///      pool stream that fills a role reverts that role to the terminal; the
///      program roles instead restore their previous target when a
///      LOAD/SAVE/EDIT redirect ends.
///
/// Dribbling is separate from both layers: it is an independently owned sink
/// that mirrors terminal I/O, applied at the terminal layer so that file
/// reads/writes are never dribbled.
/// @note Singleton, consistent with Config, Kernel, Procedures, etc.
class StreamManager
{
    /// @brief One open file: the QFile/QTextStream backing plus the wrapper and mode.
    /// @details Defined in the .cpp so this header needs no Qt device includes.
    struct OpenStream;

    /// @brief The pool of open *file* streams, keyed by the name used to open them.
    QHash<QString, OpenStream *> openStreams_;

    /// @brief The terminal stream (stdin/stdout via the interface). Never closed.
    TextStream *terminalStream_;

    // Role pointers: non-owning aliases into the pool (or the terminal). Every
    // role is always valid; a role not otherwise assigned points at the terminal.

    /// @brief Forced-interactive input (PAUSE). Always the terminal.
    /// @details Distinct from istreamProgram_ so PAUSE reaches the human even
    /// while a file is being loaded.
    TextStream *istreamStd_;

    /// @brief The current reader (SETREAD / READWORD / READLIST / ...).
    /// @details Terminal by default; a file while SETREAD is in effect.
    TextStream *istreamRead_;

    /// @brief The program-text source: top-level REPL lines and TO...END bodies.
    /// @details Terminal by default; the file during LOAD / EDIT / `qlogo script.lg`.
    /// Kept non-transient so the REPL and procedure definition can always read
    /// from it without checking whether a load is in progress.
    TextStream *istreamProgram_;

    /// @brief Output for errors, prompts, and REPL echo. Always the terminal.
    TextStream *ostreamStd_;

    /// @brief The current writer (SETWRITE / PRINT / TYPE / SHOW).
    /// @details Terminal by default; a file while SETWRITE is in effect.
    TextStream *ostreamWrite_;

    /// @brief The program-text destination: procedure listings (SAVE / EDIT / PO).
    /// @details Terminal by default; the file during SAVE / EDIT. Kept
    /// non-transient for symmetry with istreamProgram_.
    TextStream *ostreamProgram_;

    /// @brief Prefix prepended to relative filenames (SETPREFIX / PREFIX).
    DatumPtr filePrefix_;

    // Dribble backing: the file and its text stream, owned directly (not pooled).
    // Both are nullptr when not dribbling. Opened in Append mode; writes are
    // best-effort so a dribble problem never aborts a print.
    QFile *dribbleFile_;
    QTextStream *dribbleStream_;

    // Active program-source/destination redirects, innermost last. Each entry is
    // an owned open file; istreamProgram_/ostreamProgram_ alias the top of the
    // stack (or the terminal when empty). Stacks so redirects can nest.
    QList<OpenStream *> programReadStack_;
    QList<OpenStream *> programWriteStack_;

    /// @brief Look up the name a pool stream is registered under (reverse lookup).
    /// @param stream The stream to find.
    /// @return The registered name, or an empty string for the terminal / unknown.
    QString nameForStream(const TextStream *stream) const;

    /// @brief Open a file as an owned stream bundle (used for the pool and for
    /// program redirects).
    /// @param filepath The resolved path to open.
    /// @param mode How to open the file.
    /// @return The new bundle; throws FCError on failure.
    OpenStream *openStream(const QString &filepath, OpenMode mode);

    /// @brief Flush, close, and delete an owned stream bundle.
    void closeStream(OpenStream *stream);

    /// @brief Create the terminal stream and point every role at it.
    StreamManager();

  public:
    /// @brief Get the singleton instance of the StreamManager class.
    static StreamManager &get()
    {
        static StreamManager instance;
        return instance;
    }

    StreamManager(const StreamManager &) = delete;
    StreamManager(StreamManager *) = delete;
    StreamManager(StreamManager &&) = delete;
    StreamManager &operator=(const StreamManager &) = delete;
    StreamManager &operator=(StreamManager &&) = delete;

    /// @brief Close all open file streams. Roles revert to the terminal.
    ~StreamManager();

    /************ current-reader input ************/

    /// @brief Read a list from the current reader.
    /// @param prompt Prompt shown only when the reader is the terminal.
    /// @param shouldRemoveComments Strip QLogo-formatted comments.
    /// @return The list, or nothing at end of input.
    DatumPtr readList(const QString &prompt, bool shouldRemoveComments);

    /// @brief Read a word from the current reader.
    DatumPtr readWord(const QString &prompt);

    /// @brief Read a raw line (no tokenizing) from the current reader.
    DatumPtr readRawline(const QString &prompt);

    /// @brief Read a single character from the current reader.
    DatumPtr readChar();

    /// @brief The rawlines consumed to build the most recent list read.
    QList<DatumPtr> readerLineHistory() const;

    /// @brief EOFP: true if the current reader is at end of input.
    bool readerAtEnd() const;

    /// @brief KEYP: true if a character is available on the current reader.
    bool keyAvailable() const;

    /************ current-writer output ************/

    /// @brief Write to the current writer (PRINT / TYPE / SHOW).
    /// @param text The text to write.
    /// @note Dribbling, if active, is applied by the terminal layer, so output
    /// redirected to a file is not dribbled.
    void print(const QString &text);

    /// @brief Write to the standard/system output (errors, prompts, REPL echo).
    /// Unaffected by SETWRITE.
    /// @param text The text to write.
    void printSystem(const QString &text);

    /************ read/write positions ************/

    /// @brief READPOS: current position of the reader.
    qint64 readPos() const;

    /// @brief SETREADPOS: seek the reader.
    void setReadPos(qint64 pos);

    /// @brief WRITEPOS: current position of the writer.
    qint64 writePos() const;

    /// @brief SETWRITEPOS: seek the writer.
    void setWritePos(qint64 pos);

    /************ roles ************/

    /// @brief SETREAD: make the named open stream the current reader.
    /// @param name The open stream's name; empty selects the terminal.
    /// @note Throws if the name is not open or was not opened for reading.
    void setReader(const QString &name);

    /// @brief READER: name of the current reader (empty for the terminal).
    QString reader() const;

    /// @brief SETWRITE: make the named open stream the current writer.
    /// @param name The open stream's name; empty selects the terminal.
    /// @note Throws if the name is not open or was not opened for writing.
    void setWriter(const QString &name);

    /// @brief WRITER: name of the current writer (empty for the terminal).
    QString writer() const;

    /************ pool management ************/

    /// @brief OPENREAD / OPENWRITE / OPENAPPEND / OPENUPDATE.
    /// @param name The name to register the stream under (also the filename).
    /// @param mode How to open the file.
    /// @note Throws if already open or the file system operation fails.
    void open(const QString &name, OpenMode mode);

    /// @brief CLOSE: close one open stream; reverts roles to the terminal if needed.
    /// @param name The stream to close.
    void close(const QString &name);

    /// @brief CLOSEALL: close every open file stream. Roles revert to the terminal.
    void closeAll();

    /// @brief ALLOPEN: names of all currently open file streams.
    QStringList allOpen() const;

    /// @brief True if a stream is open under the given name.
    bool isOpen(const QString &name) const;

    /// @brief ERASEFILE: delete a file from disk (must not be open).
    void eraseFile(const QString &name);

    /************ dribble ************/

    /// @brief DRIBBLE: begin copying terminal I/O to the named file (Append mode).
    /// @param name The dribble filename (resolved against the current prefix).
    /// @note Throws FCError if the file cannot be opened.
    void dribble(const QString &name);

    /// @brief NODRIBBLE: stop dribbling and close the dribble file.
    void noDribble();

    /// @brief True while a dribble file is active.
    bool isDribbling() const;

    /// @brief Copy terminal text to the dribble file, if dribbling is active.
    /// @param text The text (input echo or output) to mirror.
    /// @details Called by the terminal I/O layer. Best-effort: a write failure is
    /// ignored rather than interrupting the in-progress read or print.
    void teeToDribble(const QString &text);

    /************ program source / destination ************/
    //
    // istreamProgram_ and ostreamProgram_ are always valid (terminal by
    // default). LOAD / SAVE / EDIT redirect them to a file for the duration of
    // the operation, then restore the previous target. Redirects nest, so a
    // loaded file may itself LOAD another. Prefer the RAII scopes below.

    /// @brief Redirect the program source to a file (LOAD / EDIT / startup script).
    /// @details Saves the current target and points istreamProgram_ at the file.
    /// @note Prefer ProgramReadScope for exception-safe restore.
    void beginProgramRead(const DatumPtr &filename);

    /// @brief Read the next list from the program source (terminal unless redirected).
    /// @param prompt Prompt shown only when the source is the terminal.
    /// @param shouldRemoveComments Strip QLogo-formatted comments.
    DatumPtr readProgramList(const QString &prompt, bool shouldRemoveComments);

    /// @brief Rawlines consumed to build the most recent program list.
    QList<DatumPtr> programReaderLineHistory() const;

    /// @brief True if the program source is exhausted (never true for the terminal).
    bool programReaderAtEnd() const;

    /// @brief Close the redirected file and restore the previous program source.
    void endProgramRead();

    /// @brief Redirect the program destination to a file (SAVE / EDIT).
    /// @details Saves the current target and points ostreamProgram_ at the file.
    /// @note Prefer ProgramWriteScope for exception-safe restore.
    void beginProgramWrite(const DatumPtr &filename, bool append = false);

    /// @brief Write text to the program destination (terminal unless redirected).
    void writeProgram(const QString &text);

    /// @brief Close the redirected file and restore the previous program destination.
    void endProgramWrite();

    /************ file naming ************/

    /// @brief SETPREFIX: set the prefix prepended to relative filenames.
    void setPrefix(const DatumPtr &prefix);

    /// @brief PREFIX: the current filename prefix.
    DatumPtr prefix() const;

    /// @brief Resolve a filename against the current prefix.
    /// @param filenameP The filename to resolve.
    /// @return The full path.
    QString filepathForFilename(const DatumPtr &filenameP) const;
};

/// @brief RAII guard: redirects the program source to a file on construction and
/// restores the previous target on destruction (including when a FCError unwinds
/// the stack).
class ProgramReadScope
{
  public:
    explicit ProgramReadScope(const DatumPtr &filename)
    {
        StreamManager::get().beginProgramRead(filename);
    }
    ~ProgramReadScope()
    {
        StreamManager::get().endProgramRead();
    }
    ProgramReadScope(const ProgramReadScope &) = delete;
    ProgramReadScope &operator=(const ProgramReadScope &) = delete;
};

/// @brief RAII guard: redirects the program destination to a file on construction
/// and restores the previous target on destruction (including when a FCError
/// unwinds the stack).
class ProgramWriteScope
{
  public:
    explicit ProgramWriteScope(const DatumPtr &filename, bool append = false)
    {
        StreamManager::get().beginProgramWrite(filename, append);
    }
    ~ProgramWriteScope()
    {
        StreamManager::get().endProgramWrite();
    }
    ProgramWriteScope(const ProgramWriteScope &) = delete;
    ProgramWriteScope &operator=(const ProgramWriteScope &) = delete;
};

#endif // STREAMMANAGER_H
