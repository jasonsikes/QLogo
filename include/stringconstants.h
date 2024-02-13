#ifndef STRINGCONSTANTS_H
#define STRINGCONSTANTS_H

//===-- qlogo/stringconstants.h - StringContants class definition -------*- C++
//-*-===//
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
/// This file contains the declaration of the StringConstants class, which
/// provides translatable string contants for QLogo.
///
//===----------------------------------------------------------------------===//

#include <qobject.h>

// Here, the s is a dummy so that we can copy and paste the block of definitions
#define ck(n,s) const QString n();


class StringConstants : public QObject {
    Q_OBJECT

public:
    StringConstants(QObject *parent = 0);

#include "stringblock.h"

};

extern StringConstants k;

#endif // STRINGCONSTANTS_H
