#ifndef DATUM_H
#define DATUM_H

//===-- qlogo/datum.h - Datum class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Datum class, which is the base class
/// for all the data classes used by QLogo.
///
//===----------------------------------------------------------------------===//

#include <QChar>
#include <QStack>
#include "QDebug"

#ifndef dv
#define dv(x) qDebug()<<#x<<'='<<x
#endif

class ASTNode;
class Word;
class List;
class Array;
class Error;
class DatumPtr;
class Procedure;

class Iterator;
class WordIterator;
class ListIterator;
class ArrayIterator;

class Kernel;
typedef DatumPtr (Kernel::*KernelMethod)(DatumPtr);

/// Convert "raw" encoding to Char encoding.
QChar rawToChar(const QChar &src);

/// Convert Char encoding to "raw" encoding.
QChar charToRaw(const QChar &src);

/// Convert a string from "raw" encoding to Char. In place.
void rawToChar(QString &src);

/// \brief Return a list of two words for the NODES command.
///
/// Returns a list of two words (numbers). The first represents the number of Datums
/// currently in use. The second shows the maximum number of Datums in use at any one
/// time since the last invocation of this function.
///
/// Each Word, List, and Array is a node. So, for example, an array of two words is three
/// nodes (1 Array + 2 Words). Furthermore, if the list is "RUN" (e.g. "RUN [forward 100]")
/// then ASTNodes will be created, adding to the number of nodes.
DatumPtr nodes();

/// The unit of data for QLogo. The base class for Word, List, Array, ASTNode, etc.
class Datum {
  friend class Iterator;
  friend class DatumPtr;
  int retainCount;

  // If set to 'true', DatumPtr will send qDebug message when this is deleted.
  bool alertOnDelete = false;

public:
  /// Value returned by isa().
  enum DatumType {
    noType,
    wordType,
    listType,
    arrayType,
    astnodeType,
    procedureType,
    errorType
  };

  /// \brief Constructs a Datum
  ///
  /// The Datum class is the superclass for all data.
  /// The Datum superclass maintains retain counts (manipulated by the DatumPtr class).
  Datum();

  virtual ~Datum();

  Datum &operator=(const Datum &);

  /// Return type of this object.
  virtual DatumType isa();

  /// \brief Return a string suitable for the PRINT command
  ///
  /// \param fullPrintp if true print with backslashes and vertical bars
  /// \param printDepthLimit limit the depth of sublists for readability.
  /// printDepthLimit = 1 means don't show sublists.
  /// printDepthLimit = 2 means show sublists, but don't show sublist's sublist.
  /// printDepthLimit = 0 means show '...' instead of this list.
  /// printDepthLimit = -1 means show all sublists.
  /// \param printWidthLimit limit the length of a string or list for readability.
  virtual QString printValue(
      bool = false, int printDepthLimit = -1,
      int printWidthLimit = -1);

  /// \brief Return a string suitable for the SHOW command
  ///
  /// \param fullPrintp if true print with backslashes and vertical bars
  /// \param printDepthLimit limit the depth of sublists for readability.
  /// printDepthLimit = 1 means don't show sublists.
  /// printDepthLimit = 2 means show sublists, but don't show sublist's sublist.
  /// printDepthLimit = 0 means show '...' instead of this list.
  /// printDepthLimit = -1 means show all sublists.
  /// \param printWidthLimit limit the length of a string or list for readability.
  virtual QString showValue(
      bool = false, int printDepthLimit = -1,
      int printWidthLimit = -1);

  /// Return the first element.
  virtual DatumPtr first(void);

  /// Return everything but the first element.
  virtual DatumPtr butfirst(void);

  /// Return the last element.
  virtual DatumPtr last(void);

  /// Determine if the object pointed to by other is equal to this object.
  virtual bool isEqual(DatumPtr other, bool);

  /// return the number of elements in the object.
  virtual int size();

  /// creates and returns a new Iterator object for this object.
  Iterator newIterator(void);

  /// returns true if the index given is valid for this object.
  virtual bool isIndexInRange(int);

  /// recursively search this object for an instance of a Datum.
  virtual bool containsDatum(DatumPtr, bool);

  /// nonrecursively search this object for an instance of a Datum.
  virtual bool isMember(DatumPtr aDatum, bool);

  /// return a new Datum beginning with the first occurrence of aDatum.
  virtual DatumPtr fromMember(DatumPtr aDatum, bool ignoreCase);
};



/// A datum that has no value.
extern Datum notADatum;

#endif // DATUM_H
