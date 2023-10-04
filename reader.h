#ifndef READER_H
#define READER_H

//===-- qlogo/reader.h - Reader class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Reader class, which is responsible
/// for reading text.
///
//===----------------------------------------------------------------------===//

#include "datum_datump.h"

class QTextStream;

class Reader {
    QTextStream *readStream;


public:
    Reader(QTextStream *aReadStream);

    DatumPtr readrawlineWithPrompt(const QString &prompt);
    DatumPtr readwordWithPrompt(const QString &prompt);
    DatumPtr readlistWithPrompt(const QString &prompt, bool shouldRemoveComments);
    void readProcedure(DatumPtr nodeP);
};



#endif // READER_H
