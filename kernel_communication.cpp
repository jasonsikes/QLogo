
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

#include "datum_iterator.h"
#include "error.h"
#include "kernel.h"
#include "datum_word.h"
#include "datum_astnode.h"
#include "datum_list.h"

#include "logocontroller.h"
#include "stringconstants.h"

#include <QByteArray> // for SHELL
#include <QDir>
#include <QFile>
#include <QProcess> // for SHELL
#include "textstream.h"

QString Kernel::filepathForFilename(DatumPtr filenameP) {
  const QString &filename = filenameP.wordValue()->printValue();

  QString prefix;
  if (filePrefix.isWord()) {
    prefix = filePrefix.wordValue()->printValue();
  } else {
    prefix = QDir::homePath();
  }

  QString retval = QString("%1/%2").arg(prefix, filename);
  return retval;
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

DatumPtr Kernel::excReadlist(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval = readStream->readlistWithPrompt("", false);
  if (retval == nothing)
    return h.ret(QString(""));
  return h.ret(retval);
}

DatumPtr Kernel::excReadword(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval = readStream->readwordWithPrompt("");
  if (retval == nothing)
    return h.ret(List::alloc());
  return h.ret(retval);
}

DatumPtr Kernel::excReadrawline(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval = readStream->readrawlineWithPrompt("");
  if (retval == nothing)
    return h.ret(List::alloc());
  return h.ret(retval);
}

DatumPtr Kernel::excReadchar(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr retval = readStream->readChar();
  if (retval == nothing)
    return h.ret(List::alloc());
  return h.ret(retval);
}

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
    return h.ret(List::alloc());
  return h.ret(retval);
}

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
  List *retval = List::alloc();

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
    filePrefix = nothing;

  return nothing;
}

DatumPtr Kernel::excPrefix(DatumPtr node) {
  ProcedureHelper h(this, node);
  if (filePrefix == nothing) {
    return h.ret(List::alloc());
  }
  return h.ret(filePrefix);
}

DatumPtr Kernel::excOpenread(DatumPtr node) {
  ProcedureHelper h(this, node);
  QIODevice::OpenMode openFlags = QIODevice::ReadOnly | QIODevice::Text;
  TextStream *stream = open(h, openFlags);

  readableStreams.insert(stream);
  return nothing;
}

DatumPtr Kernel::excOpenwrite(DatumPtr node) {
  ProcedureHelper h(this, node);
  QIODevice::OpenMode openFlags =
      QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text;
  TextStream *stream = open(h, openFlags);

  writableStreams.insert(stream);
  return nothing;
}

DatumPtr Kernel::excOpenappend(DatumPtr node) {
  ProcedureHelper h(this, node);
  QIODevice::OpenMode openFlags =
      QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text;
  TextStream *stream = open(h, openFlags);

  writableStreams.insert(stream);
  return nothing;
}

DatumPtr Kernel::excOpenupdate(DatumPtr node) {
  ProcedureHelper h(this, node);
  QIODevice::OpenMode openFlags = QIODevice::ReadWrite | QIODevice::Text;
  TextStream *stream = open(h, openFlags);

  readableStreams.insert(stream);
  writableStreams.insert(stream);
  return nothing;
}

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

DatumPtr Kernel::excAllopen(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = List::alloc();
  DatumPtr retvalP = h.ret(retval);
  for (auto &filename : fileStreams.keys()) {
    retval->append(DatumPtr(filename));
  }
  return retvalP;
}

DatumPtr Kernel::excCloseall(DatumPtr node) {
  ProcedureHelper h(this, node);
  closeAll();
  return nothing;
}

DatumPtr Kernel::excErasefile(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr filenameP = h.wordAtIndex(0);

  const QString filepath = filepathForFilename(filenameP);
  QFile file(filepath);
  file.remove();

  return nothing;
}

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

DatumPtr Kernel::excNodribble(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->setDribble("");
  return nothing;
}

DatumPtr Kernel::excSetread(DatumPtr node) {
  ProcedureHelper h(this, node);
  readStream = getStream(h);
  return nothing;
}

DatumPtr Kernel::excSetwrite(DatumPtr node) {
  ProcedureHelper h(this, node);
  writeStream = getStream(h);
  return nothing;
}

DatumPtr Kernel::excReader(DatumPtr node) {
  ProcedureHelper h(this, node);
  if (readStream == stdioStream)
    return h.ret(List::alloc());

  const QString retval = fileStreams.key(readStream);
  return h.ret(retval);
}

DatumPtr Kernel::excWriter(DatumPtr node) {
  ProcedureHelper h(this, node);
  if (writeStream == stdioStream)
    return h.ret(List::alloc());

  const QString retval = fileStreams.key(writeStream);
  return h.ret(retval);
}

DatumPtr Kernel::excSetreadpos(DatumPtr node) {
  ProcedureHelper h(this, node);
  int pos = h.validatedIntegerAtIndex(
      0, [](int candidate) { return candidate >= 0; });
  if (readStream != stdioStream) {
    readStream->seek(pos);
  }
  return nothing;
}

DatumPtr Kernel::excSetwritepos(DatumPtr node) {
  ProcedureHelper h(this, node);
  int pos = h.validatedIntegerAtIndex(
      0, [](int candidate) { return candidate >= 0; });
  if (writeStream != stdioStream) {
    writeStream->seek(pos);
  }
  return nothing;
}

DatumPtr Kernel::excReadpos(DatumPtr node) {
  ProcedureHelper h(this, node);
  double retval = 0;

  if (readStream != stdioStream) {
    retval = (double)readStream->pos();
  }
  return h.ret(retval);
}

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

DatumPtr Kernel::excEofp(DatumPtr node) {
  ProcedureHelper h(this, node);
  bool retval =
      (readStream != stdioStream) ? readStream->atEnd() : mainController()->atEnd();
  return h.ret(retval);
}

// TERMINAL ACCESS

DatumPtr Kernel::excKeyp(DatumPtr node) {
  ProcedureHelper h(this, node);
  bool retval = (readStream != stdioStream) ? !readStream->atEnd()
                                            : mainController()->keyQueueHasChars();
  return h.ret(retval);
}

DatumPtr Kernel::excCleartext(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->clearScreenText();
  return nothing;
}

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

DatumPtr Kernel::excCursor(DatumPtr node) {
  ProcedureHelper h(this, node);
  int row = 0, col = 0;
  mainController()->getTextCursorPos(row, col);
  List *retval = List::alloc();
  retval->append(DatumPtr(row));
  retval->append(DatumPtr(col));
  return h.ret(retval);
}

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

DatumPtr Kernel::excDecreasefont(DatumPtr node) {
  ProcedureHelper h(this, node);
  double f = mainController()->getTextFontSize();
  f -= 2;
  if (f < 2)
    f = 2;
  mainController()->setTextFontSize(f);
  return nothing;
}

DatumPtr Kernel::excSettextsize(DatumPtr node) {
  ProcedureHelper h(this, node);
  double newSize = h.validatedNumberAtIndex(
      0, [](double candidate) { return candidate >= 1; });
  mainController()->setTextFontSize(newSize);
  return nothing;
}

DatumPtr Kernel::excTextsize(DatumPtr node) {
  ProcedureHelper h(this, node);
  double size = mainController()->getTextFontSize();
  return h.ret(size);
}

DatumPtr Kernel::excSetfont(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString fontName = h.wordAtIndex(0).wordValue()->printValue();
  mainController()->setTextFontName(fontName);
  return nothing;
}

DatumPtr Kernel::excFont(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString retval = mainController()->getTextFontName();
  return h.ret(retval);
}

DatumPtr Kernel::excAllfonts(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = List::alloc();
  QStringList fonts = mainController()->getAllFontNames();
  for (const QString &i : fonts) {
    retval->append(DatumPtr(i));
  }
  return h.ret(retval);
}

DatumPtr Kernel::excCursorInsert(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->setCursorOverwriteMode(false);
  return nothing;
}

DatumPtr Kernel::excCursorOverwrite(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->setCursorOverwriteMode(true);
  return nothing;
}

DatumPtr Kernel::excCursorMode(DatumPtr node) {
  ProcedureHelper h(this, node);
  bool mode = mainController()->cursorOverwriteMode();
  QString retval = mode ? k.overwrite() : k.insert();
  return h.ret(retval);
}
