#ifndef HELP_H
#define HELP_H

//===-- qlogo/help.h - Help class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Help class, which contains
/// the QLogo help text.
///
//===----------------------------------------------------------------------===//

#include "datum.h"

class Help {
  void initRsrc();
  void set(const QString &name, const QString &text);
  void alt(const QString &newName, const QString &oldName);
  void setDataStructurePrimitives();
  void setCommunication();
  void setArithmetic();
  void setGraphics();
  void setWorkspaceManagement();
  void setControlStructures();
  void setMacros();

public:
  Help();
  DatumP helpForKeyword(const QString &keyWord);
  DatumP allKeywords();
};

#endif // HELP_H
