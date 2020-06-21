#ifndef CONSTANTS_H
#define CONSTANTS_H

//===-- qlogo/constants.h - Shared constant values -------*- C++ -*-===//
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
/// This file contains constants shared between the QLogo/logo programs.
///
//===----------------------------------------------------------------------===//

#include <QChar>
#include <QDebug>

using message_t = quint8;

enum messageCategory : message_t {
    W_ZERO, // Zeroes get ignored
    C_CONSOLE_PRINT_STRING, // Print text to the GUI
    C_CONSOLE_REQUEST_LINE, // Ask the GUI for a raw line.
    C_CONSOLE_REQUEST_CHAR, // Ask the GUI for a single char.
    C_CONSOLE_RAWLINE_READ, // A line returned from the GUI
    C_CONSOLE_CHAR_READ, // A char returned from the GUI
};

const QChar escapeChar = 27;

#define dv(x) qDebug()<<#x<<'='<<x


const float initialBoundX = 150;
const float initialBoundY = 150;


#endif // CONSTANTS_H
