
//===-- qlogo/kernel.cpp - Kernel class implementation -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
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

#include "error.h"
#include "kernel.h"
#include "parser.h"

#include "logocontroller.h"
#include "qlogocontroller.h"

#include <QByteArray> // for SHELL
#include <QDir>
#include <QFile>
#include <QProcess> // for SHELL
#include <QTextStream>

QString Kernel::filepathForFilename(DatumP filenameP) {
  const QString &filename = filenameP.wordValue()->printValue();

  QString prefix;
  if (filePrefix.isWord()) {
    prefix = filePrefix.wordValue()->printValue();
  } else {
    prefix = QDir::homePath();
  }

  QString retval = QString("%1/%2").arg(prefix).arg(filename);
  return retval;
}

QTextStream *Kernel::openFileStream(DatumP filenameP,
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

  QTextStream *stream = new QTextStream(file);
  fileStreams[filename] = stream;

  return stream;
}

QTextStream *Kernel::createStringStream(DatumP filenameP,
                                        QIODevice::OpenMode mode) {
  QString filename = filenameP.datumValue()->first().wordValue()->keyValue();
  if (fileStreams.contains(filename)) {
    Error::alreadyOpen(filenameP);
  }

  QString *buffer = NULL;
  DatumP value = variables.datumForName(filename);
  if (value.isWord()) {
    // buffer will be deleted when stream is closed
    buffer = new QString(value.wordValue()->printValue());
  }
  if (buffer == NULL)
    buffer = new QString;
  QTextStream *stream = new QTextStream(buffer, mode);
  fileStreams[filename] = stream;

  return stream;
}

QTextStream *Kernel::open(ProcedureHelper &h, QIODevice::OpenMode openFlags) {
  DatumP filenameP = h.validatedDatumAtIndex(0, [](DatumP candidate) {
    if (candidate.isWord())
      return true;
    if (!candidate.isList() || (candidate.listValue()->size() == 0))
      return false;
    return candidate.listValue()->first().isWord();
  });
  QTextStream *stream;
  if (filenameP.isWord()) {
    stream = openFileStream(filenameP, openFlags);
  } else {
    stream = createStringStream(filenameP, openFlags);
  }
  return stream;
}

QTextStream *Kernel::getStream(ProcedureHelper &h) {
  DatumP filenameP = h.validatedDatumAtIndex(0, [](DatumP candidate) {
    if (candidate.isList() && (candidate.listValue()->size() != 0))
      return false;
    return candidate.isWord() || candidate.isList();
  });
  if (filenameP.isList() && (filenameP.listValue()->size() == 0)) {
    return NULL;
  }

  if (!filenameP.isWord()) {
    return NULL;
  }
  QString filename = filenameP.wordValue()->keyValue();

  if (!fileStreams.contains(filename)) {
    Error::notOpen(filenameP);
  }

  return fileStreams[filename];
}

void Kernel::close(const QString &filename) {
  QTextStream *stream = fileStreams[filename];
  if (readStream == stream)
    readStream = NULL;
  if (writeStream == stream)
    writeStream = NULL;

  QIODevice *device = stream->device();
  QString *buffer = stream->string();

  delete stream;
  if (buffer != NULL) {
    DatumP w = DatumP(new Word(*buffer));
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

void Kernel::lprint(QTextStream *stream, const QString &text) {
  if (stream == NULL) {
    mainController()->printToConsole(text);
  } else {
    *stream << text;
    if (stream->status() != QTextStream::Ok)
      Error::fileSystem();
  }
}

void Kernel::stdPrint(const QString &text) { lprint(writeStream, text); }

void Kernel::sysPrint(const QString &text) { lprint(systemWriteStream, text); }

// TRANSMITTERS

DatumP Kernel::excPrint(DatumP node) {
  ProcedureHelper h(this, node);
  QString printString = "";
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumP value = h.datumAtIndex(i);
    if (i > 0)
      printString.append(' ');
    printString.append(value.printValue(varFULLPRINTP(), varPRINTDEPTHLIMIT(),
                                        varPRINTWIDTHLIMIT()));
  }
  printString.append("\n");
  stdPrint(printString);
  return nothing;
}

DatumP Kernel::excType(DatumP node) {
  ProcedureHelper h(this, node);
  QString printString = "";
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumP value = h.datumAtIndex(i);
    printString.append(value.showValue(varFULLPRINTP(), varPRINTDEPTHLIMIT(),
                                       varPRINTWIDTHLIMIT()));
  }
  stdPrint(printString);
  return nothing;
}

DatumP Kernel::excShow(DatumP node) {
  ProcedureHelper h(this, node);
  QString printString = "";
  for (int i = 0; i < h.countOfChildren(); ++i) {
    DatumP value = h.datumAtIndex(i);
    if (i > 0)
      printString.append(' ');
    printString.append(value.showValue(varFULLPRINTP(), varPRINTDEPTHLIMIT(),
                                       varPRINTWIDTHLIMIT()));
  }
  printString.append("\n");
  stdPrint(printString);
  return nothing;
}

// RECEIVERS

DatumP Kernel::excReadlist(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP retval = readlistWithPrompt("", false, readStream);
  if (retval == nothing)
    return h.ret(new Word(""));
  return h.ret(retval);
}

DatumP Kernel::excReadword(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP retval = readWordWithPrompt("", readStream);
  if (retval == nothing)
    return h.ret(new List);
  return h.ret(retval);
}

DatumP Kernel::excReadrawline(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP retval = readRawLineWithPrompt("", readStream);
  if (retval == nothing)
    return h.ret(new List);
  return h.ret(retval);
}

DatumP Kernel::excReadchar(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP retval = readChar();
  if (retval == nothing)
    return h.ret(new List);
  return h.ret(retval);
}

DatumP Kernel::excReadchars(DatumP node) {
  ProcedureHelper h(this, node);
  int count = h.validatedIntegerAtIndex(
      0, [](long candidate) { return candidate >= 0; });

  QString retval;
  retval.reserve(count);
  while (count > 0) {
    DatumP c = readChar();
    if (c == nothing)
      break;
    retval += c.wordValue()->rawValue();
    --count;
  }

  if (retval == "")
    return h.ret(new List);
  return h.ret(new Word(retval));
}

DatumP Kernel::excShell(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP commandP = h.validatedDatumAtIndex(0, [](DatumP candidate) {
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
  List *retval = new List;

  if ((result.size() > 0) && (result.at(result.size() - 1) == '\n')) {
    result.chop(1);

    QList<QByteArray> resultAry = result.split('\n');

    for (auto &line : resultAry) {
#ifdef _WIN32
      if (line.endsWith((char)13))
        line.chop(1);
#endif
      QString text(line);
      DatumP rawline = DatumP(new Word(text));
      if (node.astnodeValue()->countOfChildren() == 2) {
        retval->append(rawline);
      } else {
        QTextStream stream(&text, QIODevice::ReadOnly);
        retval->append(parser->readlistWithPrompt("", false, &stream));
      }
    }
  }
  return h.ret(retval);
}

// FILE ACCESS

DatumP Kernel::excSetprefix(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP newPrefix = h.validatedDatumAtIndex(0, [](DatumP candidate) {
    if (candidate.isList() && (candidate.listValue()->size() == 0))
      return true;
    return candidate.isWord();
  });

  if (newPrefix.isWord())
    filePrefix = newPrefix;
  else
    filePrefix = nothing;

  return nothing;
}

DatumP Kernel::excPrefix(DatumP node) {
  ProcedureHelper h(this, node);
  if (filePrefix == nothing) {
    return h.ret(new List);
  }
  return h.ret(filePrefix);
}

DatumP Kernel::excOpenread(DatumP node) {
  ProcedureHelper h(this, node);
  QIODevice::OpenMode openFlags = QIODevice::ReadOnly | QIODevice::Text;
  QTextStream *stream = open(h, openFlags);

  readableStreams.insert(stream);
  return nothing;
}

DatumP Kernel::excOpenwrite(DatumP node) {
  ProcedureHelper h(this, node);
  QIODevice::OpenMode openFlags =
      QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text;
  QTextStream *stream = open(h, openFlags);

  writableStreams.insert(stream);
  return nothing;
}

DatumP Kernel::excOpenappend(DatumP node) {
  ProcedureHelper h(this, node);
  QIODevice::OpenMode openFlags =
      QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text;
  QTextStream *stream = open(h, openFlags);

  writableStreams.insert(stream);
  return nothing;
}

DatumP Kernel::excOpenupdate(DatumP node) {
  ProcedureHelper h(this, node);
  QIODevice::OpenMode openFlags = QIODevice::ReadWrite | QIODevice::Text;
  QTextStream *stream = open(h, openFlags);

  readableStreams.insert(stream);
  writableStreams.insert(stream);
  return nothing;
}

DatumP Kernel::excClose(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP filenameP = h.wordAtIndex(0);
  QString filename = filenameP.wordValue()->keyValue();

  if (!fileStreams.contains(filename)) {
    Error::notOpen(filenameP);
  }

  close(filename);
  return nothing;
}

DatumP Kernel::excAllopen(DatumP node) {
  ProcedureHelper h(this, node);
  List *retval = new List;
  DatumP retvalP = h.ret(retval);
  for (auto &filename : fileStreams.keys()) {
    retval->append(DatumP(new Word(filename)));
  }
  return retvalP;
}

DatumP Kernel::excCloseall(DatumP node) {
  ProcedureHelper h(this, node);
  closeAll();
  return h.ret();
}

DatumP Kernel::excErasefile(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP filenameP = h.wordAtIndex(0);

  const QString filepath = filepathForFilename(filenameP);
  QFile file(filepath);
  file.remove();

  return nothing;
}

DatumP Kernel::excDribble(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP filenameP = h.wordAtIndex(0);

  const QString filepath = filepathForFilename(filenameP);

  if (mainController()->isDribbling())
    Error::alreadyDribbling();

  if (!mainController()->setDribble(filepath)) {
    Error::cantOpen(filenameP);
  }
  return nothing;
}

DatumP Kernel::excNodribble(DatumP node) {
  ProcedureHelper h(this, node);
  mainController()->setDribble("");
  return h.ret();
}

DatumP Kernel::excSetread(DatumP node) {
  ProcedureHelper h(this, node);
  readStream = getStream(h);
  return nothing;
}

DatumP Kernel::excSetwrite(DatumP node) {
  ProcedureHelper h(this, node);
  writeStream = getStream(h);
  return nothing;
}

DatumP Kernel::excReader(DatumP node) {
  ProcedureHelper h(this, node);
  if (readStream == NULL)
    return h.ret(new List);

  const QString retval = fileStreams.key(readStream);
  return h.ret(new Word(retval));
}

DatumP Kernel::excWriter(DatumP node) {
  ProcedureHelper h(this, node);
  if (writeStream == NULL)
    return h.ret(new List);

  const QString retval = fileStreams.key(writeStream);
  return h.ret(new Word(retval));
}

DatumP Kernel::excSetreadpos(DatumP node) {
  ProcedureHelper h(this, node);
  long pos = h.validatedIntegerAtIndex(
      0, [](long candidate) { return candidate >= 0; });
  if (readStream != NULL) {
    readStream->seek(pos);
  }
  return nothing;
}

DatumP Kernel::excSetwritepos(DatumP node) {
  ProcedureHelper h(this, node);
  long pos = h.validatedIntegerAtIndex(
      0, [](long candidate) { return candidate >= 0; });
  if (writeStream != NULL) {
    writeStream->seek(pos);
  }
  return nothing;
}

DatumP Kernel::excReadpos(DatumP node) {
  ProcedureHelper h(this, node);
  double retval = 0;

  if (readStream != NULL) {
    retval = (double)readStream->pos();
  }
  return h.ret(new Word(retval));
}

DatumP Kernel::excWritepos(DatumP node) {
  ProcedureHelper h(this, node);
  double retval = 0;

  if (writeStream != NULL) {
    writeStream
        ->flush(); // pos() won't return a valid value unless we flush first.
    retval = (double)writeStream->pos();
  }
  return h.ret(new Word(retval));
}

DatumP Kernel::excEofp(DatumP node) {
  ProcedureHelper h(this, node);
  bool retval =
      (readStream != NULL) ? readStream->atEnd() : mainController()->atEnd();
  return h.ret(retval);
}

// TERMINAL ACCESS

DatumP Kernel::excKeyp(DatumP node) {
  ProcedureHelper h(this, node);
  bool retval = (readStream != NULL) ? !readStream->atEnd()
                                     : mainController()->keyQueueHasChars();
  return h.ret(retval);
}

DatumP Kernel::excCleartext(DatumP node) {
  ProcedureHelper h(this, node);
  mainController()->clearScreenText();
  return nothing;
}

DatumP Kernel::excSetcursor(DatumP node) {
  ProcedureHelper h(this, node);
  QVector<double> v;
  h.validatedDatumAtIndex(0, [&v, this](DatumP candidate) {
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

DatumP Kernel::excCursor(DatumP node) {
  ProcedureHelper h(this, node);
  int row = 0, col = 0;
  mainController()->getTextCursorPos(row, col);
  List *retval = new List;
  DatumP retvalP = h.ret(retval);
  retval->append(DatumP(new Word(row)));
  retval->append(DatumP(new Word(col)));
  return retvalP;
}

DatumP Kernel::excSettextcolor(DatumP node) {
  ProcedureHelper h(this, node);
  QColor foreground;
  QColor background = QColor("white");
  DatumP foregroundP =
      h.validatedDatumAtIndex(0, [&foreground, this](DatumP candidate) {
        return colorFromDatumP(foreground, candidate);
      });

  if (h.countOfChildren() > 1) {
    DatumP backgroundP =
        h.validatedDatumAtIndex(1, [&background, this](DatumP candidate) {
          return colorFromDatumP(background, candidate);
        });
  }

  mainController()->setTextColor(foreground, background);
  return nothing;
}

DatumP Kernel::excIncreasefont(DatumP node) {
  ProcedureHelper h(this, node);
  double f = mainController()->getTextSize();
  f += 2;
  // There doesn't appear to be a maximum font size.
  mainController()->setTextSize(f);
  return h.ret();
}

DatumP Kernel::excDecreasefont(DatumP node) {
  ProcedureHelper h(this, node);
  double f = mainController()->getTextSize();
  f -= 2;
  if (f < 2)
    f = 2;
  mainController()->setTextSize(f);
  return h.ret();
}

DatumP Kernel::excSettextsize(DatumP node) {
  ProcedureHelper h(this, node);
  double newSize = h.validatedNumberAtIndex(
      0, [](double candidate) { return candidate >= 1; });
  mainController()->setTextSize(newSize);
  return nothing;
}

DatumP Kernel::excTextsize(DatumP node) {
  ProcedureHelper h(this, node);
  double size = mainController()->getTextSize();
  return h.ret(new Word(size));
}

DatumP Kernel::excSetfont(DatumP node) {
  ProcedureHelper h(this, node);
  QString fontName = h.wordAtIndex(0).wordValue()->printValue();
  mainController()->setFontName(fontName);
  return nothing;
}

DatumP Kernel::excFont(DatumP node) {
  ProcedureHelper h(this, node);
  QString retval = mainController()->getFontName();
  return h.ret(new Word(retval));
}

DatumP Kernel::excAllfonts(DatumP node) {
  ProcedureHelper h(this, node);
  List *retval = new List;
  QStringList fonts = mainController()->getAllFontNames();
  for (auto &i : fonts) {
    retval->append(new Word(i));
  }
  return h.ret(retval);
}

DatumP Kernel::excCursorInsert(DatumP node) {
  ProcedureHelper h(this, node);
  cursorOverwrite = false;
  mainController()->setCursorOverwriteMode(false);
  return h.ret(nothing);
}

DatumP Kernel::excCursorOverwrite(DatumP node) {
  ProcedureHelper h(this, node);
  cursorOverwrite = true;
  mainController()->setCursorOverwriteMode(true);
  return h.ret(nothing);
}

DatumP Kernel::excCursorMode(DatumP node) {
  ProcedureHelper h(this, node);
  QString retval = cursorOverwrite ? "OVERWRITE" : "INSERT";
  DatumP retvalP(new Word(retval));
  return h.ret(retvalP);
}
