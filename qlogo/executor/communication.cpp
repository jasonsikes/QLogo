
//===-- qlogo/kernel.cpp - Kernel class implementation -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// QLogo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with QLogo.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the Kernel class, which is the
/// executor proper of the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "datum/iterator.h"
#include "error.h"
#include "kernel.h"
#include "datum/word.h"
#include "datum/astnode.h"
#include "datum/list.h"

#include "controller/logocontroller.h"
#include "stringconstants.h"

#include <QByteArray> // for SHELL
#include <QDir>
#include <QFile>
#include <QProcess> // for SHELL
#include "controller/textstream.h"

QString Kernel::filepathForFilename(DatumPtr filenameP) {
  QString filename = filenameP.wordValue()->printValue();

  QString prefix;
  if (filePrefix.isWord()) {
    prefix = filePrefix.wordValue()->printValue();
  } else {
    return filename;
  }

  return prefix
         + QDir::separator()
         + filename;
}

TextStream *Kernel::openFileStream(DatumPtr filenameP,
                                    QIODevice::OpenMode mode) {
  QString filepath = filepathForFilename(filenameP.wordValue());
  QString filename = filenameP.wordValue()->keyValue();
  if (fileStreams.contains(filename)) {
    Error::alreadyOpen(filenameP);
  }

  QFile *file = new QFile(filepath);
  if (!file->open(mode)) {
    Error::cantOpen(filenameP);
  }

  TextStream *stream = new TextStream(new QTextStream(file));
  fileStreams[filename] = stream;

  return stream;
}

TextStream* Kernel::createStringStream(DatumPtr filenameP,
                                        QIODevice::OpenMode mode) {
  QString filename = filenameP.datumValue()->first().wordValue()->keyValue();
  if (fileStreams.contains(filename)) {
    Error::alreadyOpen(filenameP);
  }

  QString *buffer = NULL;
  DatumPtr value = variables.datumForName(filename);
  if (value.isWord()) {
    // buffer will be deleted when stream is closed
    buffer = new QString(value.wordValue()->printValue());
  }
  if (buffer == NULL)
    buffer = new QString;

  TextStream *stream = new TextStream(new QTextStream(buffer, mode));
  fileStreams[filename] = stream;

  return stream;
}

TextStream* Kernel::open(ProcedureHelper &h, QIODevice::OpenMode openFlags) {
  DatumPtr filenameP = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    if (candidate.isWord())
      return true;
    if (!candidate.isList() || (candidate.listValue()->size() == 0))
      return false;
    return candidate.listValue()->first().isWord();
  });
  TextStream *stream;
  if (filenameP.isWord()) {
    stream = openFileStream(filenameP, openFlags);
  } else {
    stream = createStringStream(filenameP, openFlags);
  }
  return stream;
}

TextStream* Kernel::getStream(ProcedureHelper &h) {
  DatumPtr filenameP = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    if (candidate.isList() && (candidate.listValue()->size() != 0))
      return false;
    return candidate.isWord() || candidate.isList();
  });
  if (filenameP.isList() && (filenameP.listValue()->size() == 0)) {
    return stdioStream;
  }

  if (!filenameP.isWord()) {
    return stdioStream;
  }
  QString filename = filenameP.wordValue()->keyValue();

  if (!fileStreams.contains(filename)) {
    Error::notOpen(filenameP);
  }

  return fileStreams[filename];
}

void Kernel::close(const QString &filename) {
  TextStream *stream = fileStreams[filename];
  if (readStream == stream)
    readStream = stdioStream;
  if (writeStream == stream)
    writeStream = stdioStream;

  QIODevice *device = stream->device();
  QString *buffer = stream->string();

  delete stream;
  if (buffer != NULL) {
    DatumPtr w = DatumPtr(*buffer);
    variables.setDatumForName(w, filename);
    delete buffer;
  }
  if (device != NULL)
    delete device;
  fileStreams.remove(filename);
  readableStreams.remove(stream);
  writableStreams.remove(stream);
}

void Kernel::closeAll() {
  QStringList names = fileStreams.keys();
  for (auto &iter : names) {
    close(iter);
  }
}


void Kernel::stdPrint(const QString &text) { writeStream->lprint(text); }

void Kernel::sysPrint(const QString &text) { systemWriteStream->lprint(text); }

// TRANSMITTERS


/***DOC PRINT PR
PRINT thing
PR thing
(PRINT thing1 thing2 ...)
(PR thing1 thing2 ...)

    command.  Prints the input or inputs to the current write stream
    (initially the screen).  All the inputs are printed on a single
    line, separated by spaces, ending with a newline.  If an input is a
    list, square brackets are not printed around it, but brackets are
    printed around sublists.  Braces are always printed around arrays.

COD***/

DatumPtr Kernel::excPrint(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString printString = "";
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumPtr value = h.datumAtIndex(i);
    if (i > 0)
      printString.append(' ');
    printString.append(value.printValue(varFULLPRINTP(), varPRINTDEPTHLIMIT(),
                                        varPRINTWIDTHLIMIT()));
  }
  printString.append('\n');
  stdPrint(printString);
  return nothing;
}


/***DOC TYPE
TYPE thing
(TYPE thing1 thing2 ...)

    command.  Prints the input or inputs like PRINT, except that no
    newline character is printed at the end and multiple inputs are not
    separated by spaces.  Note: printing to the terminal is ordinarily
    "line buffered"; that is, the characters you print using TYPE will
    not actually appear on the screen until either a newline character
    is printed (for example, by PRINT or SHOW) or Logo tries to read
    from the keyboard (either at the request of your program or after an
    instruction prompt).  This buffering makes the program much faster
    than it would be if each character appeared immediately, and in most
    cases the effect is not disconcerting.  To accommodate programs that
    do a lot of positioned text display using TYPE, Logo will force
    printing whenever CURSOR or SETCURSOR is invoked.  This solves most
    buffering problems.  Still, on occasion you may find it necessary to
    force the buffered characters to be printed explicitly; this can be
    done using the WAIT command.  WAIT 0 will force printing without
    actually waiting.

COD***/

DatumPtr Kernel::excType(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString printString = "";
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumPtr value = h.datumAtIndex(i);
    printString.append(value.showValue(varFULLPRINTP(), varPRINTDEPTHLIMIT(),
                                       varPRINTWIDTHLIMIT()));
  }
  stdPrint(printString);
  return nothing;
}


/***DOC SHOW
SHOW thing
(SHOW thing1 thing2 ...)

    command.  Prints the input or inputs like PRINT, except that
    if an input is a list it is printed inside square brackets.


COD***/

DatumPtr Kernel::excShow(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString printString = "";
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumPtr value = h.datumAtIndex(i);
    if (i > 0)
      printString.append(' ');
    printString.append(value.showValue(varFULLPRINTP(), varPRINTDEPTHLIMIT(),
                                       varPRINTWIDTHLIMIT()));
  }
  printString.append('\n');
  stdPrint(printString);
  return nothing;
}

// RECEIVERS


/***DOC READLIST RL
READLIST
RL

    reads a line from the read stream (initially the keyboard) and
    outputs that line as a list.  The line is separated into members as
    though it were typed in square brackets in an instruction.  If the
    read stream is a file, and the end of file is reached, READLIST
    outputs the empty word (not the empty list).  READLIST processes
    backslash, vertical bar, and tilde characters in the read stream;
    the output list will not contain these characters but they will have
    had their usual effect.  READLIST does not, however, treat semicolon
    as a comment character.

COD***/

DatumPtr Kernel::excReadlist(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval = readStream->readlistWithPrompt("", false);
  if (retval == nothing)
    return h.ret(QString(""));
  return h.ret(retval);
}


/***DOC READWORD RW
READWORD
RW

    reads a line from the read stream and outputs that line as a word.
    The output is a single word even if the line contains spaces,
    brackets, etc.  If the read stream is a file, and the end of file is
    reached, READWORD outputs the empty list (not the empty word).
    READWORD processes backslash, vertical bar, and tilde characters in
    the read stream.  In the case of a tilde used for line continuation,
    the output word DOES include the tilde and the newline characters, so
    that the user program can tell exactly what the user entered.
    Vertical bars in the line are also preserved in the output.
    Backslash characters are not preserved in the output.

COD***/

DatumPtr Kernel::excReadword(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval = readStream->readwordWithPrompt("");
  if (retval == nothing)
    return h.ret(new List());
  return h.ret(retval);
}


/***DOC READRAWLINE
READRAWLINE

    reads a line from the read stream and outputs that line as a word.
    The output is a single word even if the line contains spaces,
    brackets, etc.  If the read stream is a file, and the end of file is
    reached, READRAWLINE outputs the empty list (not the empty word).
    READRAWLINE outputs the exact string of characters as they appear
    in the line, with no special meaning for backslash, vertical bar,
    tilde, or any other formatting characters.

COD***/

DatumPtr Kernel::excReadrawline(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval = readStream->readrawlineWithPrompt("");
  if (retval == nothing)
    return h.ret(new List());
  return h.ret(retval);
}


/***DOC READCHAR RC
READCHAR
RC

    reads a single character from the read stream and outputs that
    character as a word.  If the read stream is a file, and the end of
    file is reached, READCHAR outputs the empty list (not the empty
    word).  If the read stream is the keyboard, echoing is turned off
    when READCHAR is invoked, and remains off until READLIST or READWORD
    is invoked or a Logo prompt is printed.  Backslash, vertical bar,
    and tilde characters have no special meaning in this context.

COD***/

DatumPtr Kernel::excReadchar(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval = readStream->readChar();
  if (retval == nothing)
    return h.ret(new List());
  return h.ret(retval);
}


/***DOC READCHARS RCS
READCHARS num
RCS num

    reads "num" characters from the read stream and outputs those
    characters as a word.  If the read stream is a file, and the end of
    file is reached, READCHARS outputs the empty list (not the empty
    word).  If the read stream is a terminal, echoing is turned off
    when READCHARS is invoked, and remains off until READLIST or READWORD
    is invoked or a Logo prompt is printed.  Backslash, vertical bar,
    and tilde characters have no special meaning in this context.

COD***/

DatumPtr Kernel::excReadchars(DatumPtr node) {
  ProcedureHelper h(this, node);
  int count = h.validatedIntegerAtIndex(
      0, [](int candidate) { return candidate >= 0; });

  QString retval;
  retval.reserve(count);
  while (count > 0) {
    DatumPtr c = readStream->readChar();
    if (c == nothing)
      break;
    retval += c.wordValue()->rawValue();
    --count;
  }

  if (retval == "")
    return h.ret(new List());
  return h.ret(retval);
}


/***DOC SHELL
SHELL command
(SHELL command wordflag)

    Under Unix, outputs the result of running "command" as a shell
    command.  (The command is sent to /bin/sh, not csh or other
    alternatives.)  If the command is a literal list in the instruction
    line, and if you want a backslash character sent to the shell, you
    must use \\ to get the backslash through Logo's reader intact.  The
    output is a list containing one member for each line generated by
    the shell command.  Ordinarily each such line is represented by a
    list in the output, as though the line were read using READLIST.  If
    a second input is given, regardless of the value of the input, each
    line is represented by a word in the output as though it were read
    with READWORD.  Example:

            to dayofweek
            output first first shell [date]
            end

    This is "first first" to extract the first word of the first (and
    only) line of the shell output.

    Under MacOS X, SHELL works as under Unix.  SHELL is not available
    under Mac Classic.

    Under DOS, SHELL is a command, not an operation; it sends its
    input to a DOS command processor but does not collect the result
    of the command.

    Under Windows, the wxWidgets version of Logo behaves as under Unix (except
    that DOS-style commands are understood; use "dir" rather than "ls").
    The non-wxWidgets version behaves like the DOS version.


COD***/

DatumPtr Kernel::excShell(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr commandP = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    if (candidate.isWord())
      return true;
    if (!candidate.isList() || (candidate.listValue()->size() == 0))
      return false;
    ListIterator iter = candidate.listValue()->newIterator();
    while (iter.elementExists()) {
      if (!iter.element().isWord())
        return false;
    }
    return true;
  });
  QStringList commandList;
#ifdef _WIN32
  commandList << "cmd.exe"
              << "/C";
#endif

  if (commandP.isWord()) {
    commandList << commandP.wordValue()->printValue();
  } else {
    ListIterator iter = commandP.listValue()->newIterator();
    while (iter.elementExists()) {
      commandList << iter.element().wordValue()->printValue();
    }
  }

  QString command = commandList.first();
  commandList.pop_front();

  QProcess proc;
  proc.start(command, commandList);

  proc.waitForStarted(-1);
  proc.closeWriteChannel();
  proc.waitForFinished(-1);

  QByteArray result = proc.readAll();
  List *retval = new List();

  if ((result.size() > 0) && (result.at(result.size() - 1) == '\n')) {
    result.chop(1);

    QList<QByteArray> resultAry = result.split('\n');

    for (auto &line : resultAry) {
#ifdef _WIN32
      if (line.endsWith((char)13))
        line.chop(1);
#endif
      QString text(line);
      DatumPtr rawline = DatumPtr(text);
      if (node.astnodeValue()->countOfChildren() == 2) {
        retval->append(rawline);
      } else {
        QTextStream stream(&text, QIODevice::ReadOnly);
        TextStream streamParser(&stream);
        retval->append(streamParser.readlistWithPrompt("", false));
      }
    }
  }
  return h.ret(retval);
}

// FILE ACCESS


/***DOC SETPREFIX
SETPREFIX string

    command.  Sets a prefix that will be used as the implicit beginning
    of filenames in OPENREAD, OPENWRITE, OPENAPPEND, OPENUPDATE, LOAD,
    and SAVE commands.  Logo will put the appropriate separator
    character (slash for Unix, backslash for DOS/Windows, colon for
    MacOS Classic) between the prefix and the filename entered by the user.
    The input to SETPREFIX must be a word, unless it is the empty list,
    to indicate that there should be no prefix.

COD***/

DatumPtr Kernel::excSetprefix(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr newPrefix = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
    if (candidate.isList() && (candidate.listValue()->size() == 0))
      return true;
    return candidate.isWord();
  });

  if (newPrefix.isWord())
    filePrefix = newPrefix;
  else
    filePrefix = new List();

  return nothing;
}


/***DOC PREFIX
PREFIX

    outputs the current file prefix, or [] if there is no prefix.
    See SETPREFIX.

COD***/

DatumPtr Kernel::excPrefix(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(filePrefix);
}


/***DOC OPENREAD
OPENREAD filename

    command.  Opens the named file for reading.  The read position is
    initially at the beginning of the file.

COD***/

DatumPtr Kernel::excOpenread(DatumPtr node) {
  ProcedureHelper h(this, node);
  QIODevice::OpenMode openFlags = QIODevice::ReadOnly | QIODevice::Text;
  TextStream *stream = open(h, openFlags);

  readableStreams.insert(stream);
  return nothing;
}


/***DOC OPENWRITE
OPENWRITE filename

    command.  Opens the named file for writing.  If the file already
    existed, the old version is deleted and a new, empty file created.

    OPENWRITE, but not the other OPEN variants, will accept as input
    a two-element list, in which the first element must be a variable
    name, and the second must be a positive integer.  A character
    buffer of the specified size will be created.  When a SETWRITE is
    done with this same list (in the sense of .EQ, not a copy, so
    you must do something like
        ? make "buf [foo 100]
        ? openwrite :buf
        ? setwrite :buf
            [...]
        ? close :buf
    and not just
        ? openwrite [foo 100]
        ? setwrite [foo 100]
    and so on), the printed characters are stored in the buffer;
    when a CLOSE is done with the same list as input, the characters
    from the buffer (treated as one long word, even if spaces and
    newlines are included) become the value of the specified variable.

COD***/

DatumPtr Kernel::excOpenwrite(DatumPtr node) {
  ProcedureHelper h(this, node);
  QIODevice::OpenMode openFlags =
      QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text;
  TextStream *stream = open(h, openFlags);

  writableStreams.insert(stream);
  return nothing;
}


/***DOC OPENAPPEND
OPENAPPEND filename

    command.  Opens the named file for writing.  If the file already
    exists, the write position is initially set to the end of the old
    file, so that newly written data will be appended to it.

COD***/

DatumPtr Kernel::excOpenappend(DatumPtr node) {
  ProcedureHelper h(this, node);
  QIODevice::OpenMode openFlags =
      QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text;
  TextStream *stream = open(h, openFlags);

  writableStreams.insert(stream);
  return nothing;
}


/***DOC OPENUPDATE
OPENUPDATE filename

    command.  Opens the named file for reading and writing.  The read and
    write position is initially set to the end of the old file, if any.
    Note: each open file has only one position, for both reading and
    writing.  If a file opened for update is both READER and WRITER at
    the same time, then SETREADPOS will also affect WRITEPOS and vice
    versa.  Also, if you alternate reading and writing the same file,
    you must SETREADPOS between a write and a read, and SETWRITEPOS
    between a read and a write.

COD***/

DatumPtr Kernel::excOpenupdate(DatumPtr node) {
  ProcedureHelper h(this, node);
  QIODevice::OpenMode openFlags = QIODevice::ReadWrite | QIODevice::Text;
  TextStream *stream = open(h, openFlags);

  readableStreams.insert(stream);
  writableStreams.insert(stream);
  return nothing;
}


/***DOC CLOSE
CLOSE filename

    command.  Closes the named file.  If the file was currently the
    reader or writer, then the reader or writer is changed to the
    keyboard or screen, as if SETREAD [] or SETWRITE [] had been done.

COD***/

DatumPtr Kernel::excClose(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr filenameP = h.wordAtIndex(0);
  QString filename = filenameP.wordValue()->keyValue();

  if (!fileStreams.contains(filename)) {
    Error::notOpen(filenameP);
  }

  close(filename);
  return nothing;
}


/***DOC ALLOPEN
ALLOPEN

    outputs a list whose members are the names of all files currently open.
    This list does not include the dribble file, if any.

COD***/

DatumPtr Kernel::excAllopen(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = new List();
  DatumPtr retvalP = h.ret(retval);
  for (const auto &filename : fileStreams.asKeyValueRange()) {
    retval->append(DatumPtr(filename.first));
  }
  return retvalP;
}


/***DOC CLOSEALL
CLOSEALL

    command.  Closes all open files.

COD***/

DatumPtr Kernel::excCloseall(DatumPtr node) {
  ProcedureHelper h(this, node);
  closeAll();
  return nothing;
}


/***DOC ERASEFILE ERF
ERASEFILE filename
ERF filename

    command.  Erases (deletes, removes) the named file, which should not
    currently be open.

COD***/

DatumPtr Kernel::excErasefile(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr filenameP = h.wordAtIndex(0);

  const QString filepath = filepathForFilename(filenameP);
  QFile file(filepath);
  file.remove();

  return nothing;
}


/***DOC DRIBBLE
DRIBBLE filename

    command.  Creates a new file whose name is the input, like OPENWRITE,
    and begins recording in that file everything that is read from the
    keyboard or written to the terminal.  That is, this writing is in
    addition to the writing to WRITER.  The intent is to create a
    transcript of a Logo session, including things like prompt
    characters and interactions.

COD***/

DatumPtr Kernel::excDribble(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr filenameP = h.wordAtIndex(0);

  const QString filepath = filepathForFilename(filenameP);

  if (mainController()->isDribbling())
    Error::alreadyDribbling();

  if (!mainController()->setDribble(filepath)) {
    Error::cantOpen(filenameP);
  }
  return nothing;
}


/***DOC NODRIBBLE
NODRIBBLE

    command.  Stops copying information into the dribble file, and
    closes the file.

COD***/

DatumPtr Kernel::excNodribble(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->setDribble("");
  return nothing;
}


/***DOC SETREAD
SETREAD filename

    command.  Makes the named file the read stream, used for READLIST,
    etc.  The file must already be open with OPENREAD or OPENUPDATE.  If
    the input is the empty list, then the read stream becomes the
    keyboard, as usual.  Changing the read stream does not close the
    file that was previously the read stream, so it is possible to
    alternate between files.

COD***/

DatumPtr Kernel::excSetread(DatumPtr node) {
  ProcedureHelper h(this, node);
  readStream = getStream(h);
  return nothing;
}


/***DOC SETWRITE
SETWRITE filename

    command.  Makes the named file the write stream, used for PRINT,
    etc.  The file must already be open with OPENWRITE, OPENAPPEND, or
    OPENUPDATE.  If the input is the empty list, then the write stream
    becomes the screen, as usual.  Changing the write stream does
    not close the file that was previously the write stream, so it is
    possible to alternate between files.

    If the input is a list, then its first element must be a variable
    name, and its second and last element must be a positive integer; a
    buffer of that many characters will be allocated, and will become the
    writestream.  If the same list (same in the .EQ sense, not a copy)
    has been used as input to OPENWRITE, then the already-allocated
    buffer will be used, and the writer can be changed to and from this
    buffer, with all the characters accumulated as in a file.  When the
    same list is used as input to CLOSE, the contents of the buffer
    (as an unparsed word, which may contain newline characters) will
    become the value of the named variable.  For compatibility with
    earlier versions, if the list has not been opened when the SETWRITE
    is done, it will be opened implicitly, but the first SETWRITE after
    this one will implicitly close it, setting the variable and freeing
    the allocated buffer.

COD***/

DatumPtr Kernel::excSetwrite(DatumPtr node) {
  ProcedureHelper h(this, node);
  writeStream = getStream(h);
  return nothing;
}


/***DOC READER
READER

    outputs the name of the current read stream file, or the empty list
    if the read stream is the terminal.

COD***/

DatumPtr Kernel::excReader(DatumPtr node) {
  ProcedureHelper h(this, node);
  if (readStream == stdioStream)
    return h.ret(new List());

  const QString retval = fileStreams.key(readStream);
  return h.ret(retval);
}


/***DOC WRITER
WRITER

    outputs the name of the current write stream file, or the empty list
    if the write stream is the screen.

COD***/

DatumPtr Kernel::excWriter(DatumPtr node) {
  ProcedureHelper h(this, node);
  if (writeStream == stdioStream)
    return h.ret(new List());

  const QString retval = fileStreams.key(writeStream);
  return h.ret(retval);
}


/***DOC SETREADPOS
SETREADPOS charpos

    command.  Sets the file pointer of the read stream file so that the
    next READLIST, etc., will begin reading at the "charpos"th character
    in the file, counting from 0.  (That is, SETREADPOS 0 will start
    reading from the beginning of the file.)  Meaningless if the read
    stream is the screen.

COD***/

DatumPtr Kernel::excSetreadpos(DatumPtr node) {
  ProcedureHelper h(this, node);
  int pos = h.validatedIntegerAtIndex(
      0, [](int candidate) { return candidate >= 0; });
  if (readStream != stdioStream) {
    readStream->seek(pos);
  }
  return nothing;
}


/***DOC SETWRITEPOS
SETWRITEPOS charpos

    command.  Sets the file pointer of the write stream file so that the
    next PRINT, etc., will begin writing at the "charpos"th character
    in the file, counting from 0.  (That is, SETWRITEPOS 0 will start
    writing from the beginning of the file.)  Meaningless if the write
    stream is the screen.

COD***/

DatumPtr Kernel::excSetwritepos(DatumPtr node) {
  ProcedureHelper h(this, node);
  int pos = h.validatedIntegerAtIndex(
      0, [](int candidate) { return candidate >= 0; });
  if (writeStream != stdioStream) {
    writeStream->seek(pos);
  }
  return nothing;
}


/***DOC READPOS
READPOS

    outputs the file position of the current read stream file.

COD***/

DatumPtr Kernel::excReadpos(DatumPtr node) {
  ProcedureHelper h(this, node);
  double retval = 0;

  if (readStream != stdioStream) {
    retval = (double)readStream->pos();
  }
  return h.ret(retval);
}


/***DOC WRITEPOS
WRITEPOS

    outputs the file position of the current write stream file.

COD***/

DatumPtr Kernel::excWritepos(DatumPtr node) {
  ProcedureHelper h(this, node);
  double retval = 0;

  if (writeStream != stdioStream) {
    writeStream
        ->flush(); // pos() won't return a valid value unless we flush first.
    retval = (double)writeStream->pos();
  }
  return h.ret(retval);
}


/***DOC EOFP EOF?
EOFP
EOF?

    predicate, outputs TRUE if there are no more characters to be
    read in the read stream file, FALSE otherwise.

COD***/

DatumPtr Kernel::excEofp(DatumPtr node) {
  ProcedureHelper h(this, node);
  bool retval =
      (readStream != stdioStream) ? readStream->atEnd() : mainController()->atEnd();
  return h.ret(retval);
}

// TERMINAL ACCESS


/***DOC KEYP KEY?
KEYP
KEY?

    predicate, outputs TRUE if there are characters waiting to be
    read from the read stream.  If the read stream is a file, this
    is equivalent to NOT EOFP.  If the read stream is the terminal,
    then echoing is turned off and the terminal is set to CBREAK
    (character at a time instead of line at a time) mode.  It
    remains in this mode until some line-mode reading is requested
    (e.g., READLIST).  The Unix operating system forgets about any
    pending characters when it switches modes, so the first KEYP
    invocation will always output FALSE.

COD***/

DatumPtr Kernel::excKeyp(DatumPtr node) {
  ProcedureHelper h(this, node);
  bool retval = (readStream != stdioStream) ? !readStream->atEnd()
                                            : mainController()->keyQueueHasChars();
  return h.ret(retval);
}


/***DOC CLEARTEXT CT
CLEARTEXT
CT

    command.  Clears the text window.

COD***/

DatumPtr Kernel::excCleartext(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->clearScreenText();
  return nothing;
}


/***DOC SETCURSOR
SETCURSOR vector

    command.  The input is a list of two numbers, the row and column
    coordinates of the text cursor position in the text console portion of the
    GUI terminal.  The text cursor is moved to the requested position. The
    text console may scroll to reveal the requested position.

COD***/

DatumPtr Kernel::excSetcursor(DatumPtr node) {
  ProcedureHelper h(this, node);
  QVector<double> v;
  h.validatedDatumAtIndex(0, [&v, this](DatumPtr candidate) {
    if (!numbersFromList(v, candidate))
      return false;
    if (v.size() != 2)
      return false;
    if ((v[0] != floor(v[0])) || (v[0] < 0))
      return false;
    if ((v[1] != floor(v[1])) || (v[1] < 0))
      return false;
    return true;
  });
  mainController()->setTextCursorPos(v[0], v[1]);
  return nothing;
}


/***DOC CURSOR
CURSOR

    outputs a list containing the current row and column coordinates of
    the text cursor.

COD***/

DatumPtr Kernel::excCursor(DatumPtr node) {
  ProcedureHelper h(this, node);
  int row = 0, col = 0;
  mainController()->getTextCursorPos(row, col);
  List *retval = new List();
  retval->append(DatumPtr(row));
  retval->append(DatumPtr(col));
  return h.ret(retval);
}


/***DOC SETTEXTCOLOR SETTC
SETTEXTCOLOR foreground background
SETTC foreground background

    The inputs are color numbers, as for turtle graphics.  Future printing
    to the text window will use the specified colors for foreground (the
    characters printed) and background (the space under those characters).

COD***/

DatumPtr Kernel::excSettextcolor(DatumPtr node) {
  ProcedureHelper h(this, node);
  QColor foreground;
  QColor background;
  DatumPtr foregroundP =
      h.validatedDatumAtIndex(0, [&foreground, this](DatumPtr candidate) {
        return colorFromDatumPtr(foreground, candidate);
      });

  if (h.countOfChildren() > 1) {
    DatumPtr backgroundP =
        h.validatedDatumAtIndex(1, [&background, this](DatumPtr candidate) {
          return colorFromDatumPtr(background, candidate);
        });
  }

  mainController()->setTextColor(foreground, background);
  return nothing;
}

DatumPtr Kernel::excIncreasefont(DatumPtr node) {
  ProcedureHelper h(this, node);
  double f = mainController()->getTextFontSize();
  f += 2;
  // There doesn't appear to be a maximum font size.
  mainController()->setTextFontSize(f);
  return nothing;
}


/***DOC INCREASEFONT DECREASEFONT
INCREASEFONT
DECREASEFONT

    Increase or decrease the size of the font  used in the text and edit
    windows to the next larger or smaller available size.

COD***/

DatumPtr Kernel::excDecreasefont(DatumPtr node) {
  ProcedureHelper h(this, node);
  double f = mainController()->getTextFontSize();
  f -= 2;
  if (f < 2)
    f = 2;
  mainController()->setTextFontSize(f);
  return nothing;
}


/***DOC SETTEXTSIZE
SETTEXTSIZE height

    Set the "point size" of the font used in the text and edit windows
    to the given integer input.  See SETLABELHEIGHT for a different approach
    used for the graphics window.

COD***/

DatumPtr Kernel::excSettextsize(DatumPtr node) {
  ProcedureHelper h(this, node);
  double newSize = h.validatedNumberAtIndex(
      0, [](double candidate) { return candidate >= 1; });
  mainController()->setTextFontSize(newSize);
  return nothing;
}


/***DOC TEXTSIZE
TEXTSIZE

    outputs the "point size" of the font used in the text and edit windows.
    See SETTEXTSIZE for a discussion of font sizing.  See LABELSIZE for a
    different approach used for the graphics window.

COD***/

DatumPtr Kernel::excTextsize(DatumPtr node) {
  ProcedureHelper h(this, node);
  double size = mainController()->getTextFontSize();
  return h.ret(size);
}

/***DOC SETFONT
SETFONT fontname

    Set the "font family name" of the font used in the text and edit windows.
    See ALLFONTS for a list of all fonts available on your system.

COD***/

DatumPtr Kernel::excSetfont(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString fontName = h.wordAtIndex(0).wordValue()->printValue();
  mainController()->setTextFontName(fontName);
  return nothing;
}

/***DOC FONTNAME
FONTNAME

    outputs the "font family name" of the font used in the text and edit
    windows.

COD***/

DatumPtr Kernel::excFont(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString retval = mainController()->getTextFontName();
  return h.ret(retval);
}

/***DOC ALLFONTS
ALLFONTS

    outputs a list of all the font names available on your system. Note that
    simply printing the list may not be the best representation since font
    names usually contain spaces. You may instead wish to print each font
    name on a separate line:

        foreach allfonts [print ?]

COD***/

DatumPtr Kernel::excAllfonts(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = new List();
  QStringList fonts = mainController()->getAllFontNames();
  for (const QString &i : fonts) {
    retval->append(DatumPtr(i));
  }
  return h.ret(retval);
}

/***DOC CURSORINSERT
CURSORINSERT

    Sets the cursor mode to "insert". Future output to the text window will
    cause any text that was already positioned after the cursor to be pushed
    forward to make room for the inserted text.

COD***/

DatumPtr Kernel::excCursorInsert(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->setCursorOverwriteMode(false);
  return nothing;
}

/***DOC CURSOROVERWRITE
CURSOROVERWRITE

    Sets the cursor mode to "overwrite". Future output to the text window will
    overwrite any text that was already positioned after the cursor.

COD***/

DatumPtr Kernel::excCursorOverwrite(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->setCursorOverwriteMode(true);
  return nothing;
}

/***DOC CURSORMODE
CURSORMODE

    Outputs the current cursor mode, either "OVERWRITE" or "INSERT".

COD***/

DatumPtr Kernel::excCursorMode(DatumPtr node) {
  ProcedureHelper h(this, node);
  bool mode = mainController()->cursorOverwriteMode();
  QString retval = mode ? k.overwrite() : k.insert();
  return h.ret(retval);
}
