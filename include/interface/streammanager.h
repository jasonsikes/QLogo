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
/// role (current reader, current writer, commander, listing, dribble). It is
/// the single backing point for the Logo file/terminal I/O primitives.
///
//===----------------------------------------------------------------------===//

#include "datum_ptr.h"
#include <QHash>
#include <QList>
#include <QString>

class TextStream;
class QColor;
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
/// @details Layers:
///   1. A *pool* of open streams: the terminal (always present, never closed)
///      plus one entry per file opened via OPENREAD/OPENWRITE/etc.
///   2. *Role pointers* that alias the pool (or the terminal): SETREAD reader,
///      SETWRITE writer, commander (REPL / TO / LOAD), and listing destination
///      (SAVE / EDIT / PO). Roles are non-owning and always valid; unassigned
///      roles point at the terminal. Closing a pool stream that fills the
///      reader or writer reverts that role to the terminal. Program
///      source/destination roles restore their previous target when a
///      LOAD/SAVE/EDIT redirect ends.
///   3. A *REPL stack* of interactive sessions (top-level commander and
///      PAUSE). Each frame points the commander at the terminal so the human
///      is reached even during LOAD, and overlays the writer onto the terminal
///      until SETWRITE is called in that session. LOAD may nest inside a REPL
///      frame: while the commander is a file, SETWRITE applies immediately;
///      when LOAD ends the REPL frame is current again and the overlay
///      returns if it was still active.
///
/// Dribbling is separate: an independently owned sink that mirrors terminal
/// I/O. All terminal bytes flow through this class (see the terminal transport
/// methods), so dribbling is applied in one place and file reads/writes are
/// never dribbled. The interface is a dumb transport with no dribble awareness.
/// @note Singleton, consistent with Config, Kernel, Procedures, etc.
class StreamManager
{
    /// @brief One open file: the QFile/QTextStream backing plus the wrapper and mode.
    /// @details Defined in the .cpp so this header needs no Qt device includes.
    struct OpenStream;

    /// @brief One interactive commander session (top-level REPL or PAUSE).
    struct ReplFrame
    {
        /// @brief Commander stream to restore when this session ends.
        TextStream *savedCommander = nullptr;
        /// @brief True until SETWRITE in this session, while the commander is
        /// the terminal: PRINT/SHOW go to the terminal and WRITER reports empty
        /// even if ostreamWrite_ is still a file.
        bool overlayActive = false;
    };

    /// @brief The pool of open *file* streams, keyed by the name used to open them.
    QHash<QString, OpenStream *> openStreams_;

    /// @brief The terminal stream (stdin/stdout via the interface). Never closed.
    TextStream *terminalStream_;

    // Role pointers: non-owning aliases into the pool (or the terminal). Every
    // role is always valid; a role not otherwise assigned points at the terminal.

    /// @brief The current reader (SETREAD / READWORD / READLIST / ...).
    /// @details Terminal by default; a file while SETREAD is in effect.
    TextStream *istreamRead_;

    /// @brief The commander: REPL lines, TO...END bodies, and LOAD text.
    /// @details Terminal by default; a file during LOAD / EDIT / `qlogo script.lg`;
    /// the terminal again for the duration of a REPL/PAUSE session even if a
    /// load is in progress.
    TextStream *istreamProgram_;

    /// @brief The current writer (SETWRITE). PRINT/TYPE/SHOW use this unless a
    /// REPL overlay is hiding it.
    /// @details Terminal by default; a file while SETWRITE is in effect.
    TextStream *ostreamWrite_;

    /// @brief The listing destination: procedure listings (SAVE / EDIT / PO).
    /// @details Terminal by default; the file during SAVE / EDIT.
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
    // stack (or the terminal when empty), except that a REPL frame forces the
    // commander to the terminal until it ends. Stacks so redirects can nest.
    QList<OpenStream *> programReadStack_;
    QList<OpenStream *> programWriteStack_;

    /// @brief Active interactive sessions, innermost last.
    QList<ReplFrame> replStack_;

    /// @brief Look up the name a pool stream is registered under (reverse lookup).
    /// @param stream The stream to find.
    /// @return The registered name, or an empty string for the terminal / unknown.
    QString nameForStream(const TextStream *stream) const;

    /// @brief Open a file as an owned stream bundle (used for the pool and for
    /// program redirects).
    /// @param filepath The resolved path to open.
    /// @param mode How to open the file.
    /// @param filename The Logo name to report if the file cannot be opened.
    /// @return The new bundle; throws FCError::cantOpen on failure.
    OpenStream *openStream(const QString &filepath, OpenMode mode, const DatumPtr &filename);

    /// @brief Flush, close, and delete an owned stream bundle.
    void closeStream(OpenStream *stream);

    /// @brief Copy terminal text to the dribble file, if dribbling is active.
    /// @param text The text (input echo or output) to mirror.
    /// @details Best-effort: a write failure is ignored rather than interrupting
    /// the in-progress read or print.
    void teeToDribble(const QString &text);

    /// @brief True when a REPL/PAUSE session is active and the commander is the
    /// terminal (not a nested LOAD file).
    bool isInteractiveCommander() const;

    /// @brief True when PRINT/WRITER should treat the writer as the terminal
    /// because the current REPL session is still overlaying SETWRITE.
    bool writerOverlayActive() const;

    /// @brief The stream PRINT/TYPE/SHOW and WRITER currently use.
    TextStream *activeWriter() const;

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

    /************ workspace file primitives ************/

    /// @brief OPENREAD / OPENWRITE / OPENAPPEND / OPENUPDATE for a file.
    /// @param name The name to register the stream under (also the filename).
    /// @param mode How to open the file.
    /// @param spec The original Logo input, used in error messages.
    /// @note Throws FCError::alreadyOpen if already open, FCError::cantOpen if
    ///       the file system operation fails.
    void open(const QString &name, OpenMode mode, const DatumPtr &spec);

    /// @brief OPENREAD / OPENWRITE / OPENAPPEND / OPENUPDATE for a character buffer.
    /// @param name The variable name that identifies the buffer (also the stream key).
    /// @param initial Starting contents, typically the variable's current word value.
    /// @param mode How to open the buffer.
    /// @param spec The original Logo input, used in error messages.
    /// @note Throws FCError::alreadyOpen if a stream is already registered under this name.
    void openBuffer(const QString &name, const QString &initial, OpenMode mode, const DatumPtr &spec);

    /// @brief CLOSE: close one open stream; reverts reader/writer to the terminal
    /// if needed. Program and REPL stacks are not affected.
    /// @param name The stream to close.
    /// @param spec The original Logo input, used in error messages.
    /// @return A Word of the buffer contents if this was a character buffer
    ///         (to assign to the named variable); nothing() if it was a file.
    /// @note Throws FCError::notOpen if the name is not open.
    DatumPtr close(const QString &name, const DatumPtr &spec);

    /// @brief CLOSEALL: close every open file stream. Reader/writer revert to
    /// the terminal. Program and REPL stacks are not affected.
    void closeAll();

    /// @brief ALLOPEN: names of all currently open file streams as a Logo list.
    /// @details Does not include the dribble file, if any. The terminal is never listed.
    DatumPtr allOpen() const;

    /// @brief True if a stream is open under the given name.
    bool isOpen(const QString &name) const;

    /// @brief ERASEFILE: delete a file from disk (must not be open).
    void eraseFile(const QString &name);

    /// @brief SETREAD: make the named open stream the current reader.
    /// @param name The open stream's name; empty with a nothing spec selects
    ///        the terminal (SETREAD []).
    /// @param spec The original Logo input, used in error messages.
    /// @note Throws FCError::notOpen if the name is not open. Throws
    ///       FCError::fileSystem if the stream was not opened for reading.
    void setReader(const QString &name, const DatumPtr &spec = DatumPtr());

    /// @brief READER: name of the current reader (empty for the terminal).
    QString reader() const;

    /// @brief SETWRITE: make the named open stream the current writer.
    /// @param name The open stream's name; empty with a nothing spec selects
    ///        the terminal (SETWRITE []).
    /// @param spec The original Logo input, used in error messages.
    /// @details If a REPL/PAUSE session is currently talking to the human,
    /// this also ends that session's writer overlay so PRINT follows the new
    /// writer immediately. SETWRITE during LOAD does not clear the overlay;
    /// when the load ends and the REPL is current again, a still-active
    /// overlay hides the writer until the user SETWRITEs at the prompt.
    /// @note Throws FCError::notOpen if the name is not open. Throws
    ///       FCError::fileSystem if the stream was not opened for writing.
    void setWriter(const QString &name, const DatumPtr &spec = DatumPtr());

    /// @brief WRITER: name of the stream PRINT currently uses (empty for the
    /// terminal, including while a REPL overlay is hiding a file writer).
    QString writer() const;

    /// @brief Read a list from the current reader.
    /// @param prompt Prompt shown only when the reader is the terminal.
    /// @param shouldRemoveComments Strip QLogo-formatted comments.
    /// @return The list, or nothing at end of input.
    DatumPtr readList(const QString &prompt, bool shouldRemoveComments);

    /// @brief Read a word from the current reader.
    DatumPtr readWord(const QString &prompt);

    /// @brief Read a raw line (no tokenizing) from the current reader.
    DatumPtr readRawline(const QString &prompt);

    /// @brief Read up to count characters from the current reader.
    /// @return A word of the characters read, or an empty list if none were available.
    DatumPtr readChars(int32_t count);

    /// @brief The rawlines consumed to build the most recent list read.
    QList<DatumPtr> readerLineHistory() const;

    /// @brief EOFP: true if the current reader is at end of input.
    bool readerAtEnd() const;

    /// @brief KEYP: true if a character is available on the current reader.
    bool keyAvailable() const;

    /// @brief Write to the active writer (PRINT / TYPE / SHOW).
    /// @param text The text to write.
    /// @details During a REPL/PAUSE session whose overlay is still active,
    /// this goes to the terminal rather than the SETWRITE file. Dribbling, if
    /// active, is applied by the terminal layer, so output that actually
    /// reaches a file is not dribbled.
    void print(const QString &text);

    /// @brief READPOS: current position of the reader.
    qint64 readPos() const;

    /// @brief SETREADPOS: seek the reader.
    void setReadPos(qint64 pos);

    /// @brief WRITEPOS: current position of the active writer.
    /// @details Flushes the writer first so the reported position includes
    /// buffered output. Returns 0 if the writer is the terminal.
    qint64 writePos();

    /// @brief SETWRITEPOS: seek the active writer.
    void setWritePos(qint64 pos);

    /// @brief SETPREFIX: set the prefix prepended to relative filenames.
    void setPrefix(const DatumPtr &prefix);

    /// @brief PREFIX: the current filename prefix.
    DatumPtr prefix() const;

    /// @brief Resolve a filename against the current prefix.
    /// @param filenameP The filename to resolve.
    /// @return The full path.
    QString filepathForFilename(const DatumPtr &filenameP) const;

    /// @brief DRIBBLE: begin copying terminal I/O to the named file (Append mode).
    /// @param filename The dribble filename (resolved against the current prefix).
    /// @note Throws FCError::alreadyDribbling if a dribble file is already
    ///       active, FCError::cantOpen if the file cannot be opened.
    void dribble(const DatumPtr &filename);

    /// @brief NODRIBBLE: stop dribbling and close the dribble file.
    void noDribble();

    /// @brief True while a dribble file is active.
    bool isDribbling() const;

    /************ human / system output ************/

    /// @brief Write to the terminal (errors, prompts, REPL echo, notices).
    /// Always goes to the console; unaffected by SETWRITE and the REPL overlay.
    /// @param text The text to write.
    void printToTerminal(const QString &text);

    /// @brief CLEARTEXT / CT: clear the text console.
    /// @details In Psychi this empties the console widget. The command-line
    /// interface has no text window, so this is a no-op. Not dribbled.
    void clearText();

    /// @brief SETCURSOR: move the text console cursor to the given row and column.
    /// @details In Psychi this moves the console widget cursor. The command-line
    /// interface has no text window, so this is a no-op. Not dribbled.
    void setTextCursor(int row, int col);

    /// @brief CURSOR: current text console cursor row and column.
    /// @details On the command-line interface both values are 0.
    void getTextCursor(int &row, int &col);

    /// @brief SETTEXTCOLOR / SETTC: set the text console foreground and background.
    /// @details In Psychi this changes future printing in the console widget.
    /// The command-line interface has no text window, so this is a no-op.
    /// An invalid background leaves the existing background unchanged.
    void setTextColor(const QColor &foreground, const QColor &background);

    /// @brief SETTEXTSIZE: set the text console font point size.
    /// @details In Psychi this changes future printing in the console widget.
    /// The command-line interface has no text window, so this is a no-op.
    void setTextFontSize(double size);

    /// @brief TEXTSIZE: current text console font point size.
    /// @details On the command-line interface this is 12.
    double textFontSize() const;

    /************ command interpreter ************/
    //
    // The commander (istreamProgram_) is always valid (terminal by default).
    // LOAD / EDIT / a startup script redirect it to a file for the duration of
    // the operation, then restore the previous target. Redirects nest, so a
    // loaded file may itself LOAD another. Prefer the RAII scopes below.
    // readProgramList follows the commander, including a REPL frame that has
    // pointed it at the terminal.

    /// @brief Redirect the commander to a file (LOAD / EDIT / startup script).
    /// @details Saves the current target and points istreamProgram_ at the file.
    /// @note Prefer ProgramReadScope for exception-safe restore.
    void beginProgramRead(const DatumPtr &filename);

    /// @brief Read the next list from the commander (terminal unless redirected).
    /// @param prompt Prompt shown only when the source is the terminal.
    /// @param shouldRemoveComments Strip QLogo-formatted comments.
    DatumPtr readProgramList(const QString &prompt, bool shouldRemoveComments);

    /// @brief Rawlines consumed to build the most recent program list.
    QList<DatumPtr> programReaderLineHistory() const;

    /// @brief True if the commander is exhausted (never true for the terminal).
    bool programReaderAtEnd() const;

    /// @brief Close the redirected file and restore the previous commander.
    void endProgramRead();

    /// @brief Redirect the listing destination to a file (SAVE / EDIT).
    /// @details Saves the current target and points ostreamProgram_ at the file.
    /// @note Prefer ProgramWriteScope for exception-safe restore.
    void beginProgramWrite(const DatumPtr &filename, bool append = false);

    /// @brief Write text to the listing destination (terminal unless redirected).
    void writeProgram(const QString &text);

    /// @brief Close the redirected file and restore the previous listing destination.
    void endProgramWrite();

    /************ REPL / PAUSE ************/
    //
    // beginRepl/endRepl nest independently of LOAD/SAVE. Each session reads
    // commander input from the terminal and overlays PRINT onto the terminal
    // until SETWRITE in that session. Prefer ReplScope.

    /// @brief Begin an interactive commander session (top-level REPL or PAUSE).
    /// @details Saves the current commander, points it at the terminal, and
    /// starts a writer overlay if SETWRITE currently names a file.
    /// @note Prefer ReplScope for exception-safe restore.
    void beginRepl();

    /// @brief End the innermost interactive session and restore the commander.
    /// @details The SETWRITE target is left as-is: a SETWRITE from before this
    /// session remains unless the user changed it, and the overlay of this
    /// frame is discarded.
    void endRepl();

    /************ terminal transport ************/
    //
    // The terminal-backed TextStream (RawStream with a null device) routes its
    // low-level I/O through these methods rather than calling the interface
    // directly. Centralizing here makes StreamManager the single place terminal
    // bytes flow through, so dribbling is applied in one spot and the interface
    // stays a dumb transport.

    /// @brief Write text to the terminal (via the interface) and mirror to dribble.
    void writeTerminal(const QString &text);

    /// @brief Read a raw line from the terminal, echoing the prompt and line to
    /// dribble when a line is actually returned.
    /// @param prompt The prompt to display.
    /// @return The line, or a null QString at end of input.
    QString readTerminalRawline(const QString &prompt);

    /// @brief Read a single character from the terminal.
    /// @return The character, or nothing at end of input.
    DatumPtr readTerminalChar();
};

/// @brief RAII guard: redirects the commander to a file on construction and
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

/// @brief RAII guard: redirects the listing destination to a file on construction
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

/// @brief RAII guard: begins an interactive commander session on construction
/// and ends it on destruction (including when a FCError unwinds the stack).
/// @details Wrap Kernel::readEvalPrintLoop so the top-level REPL and PAUSE each
/// get a session: commander input from the terminal, and PRINT overlay until
/// SETWRITE.
class ReplScope
{
  public:
    ReplScope()
    {
        StreamManager::get().beginRepl();
    }
    ~ReplScope()
    {
        StreamManager::get().endRepl();
    }
    ReplScope(const ReplScope &) = delete;
    ReplScope &operator=(const ReplScope &) = delete;
};

#endif // STREAMMANAGER_H
