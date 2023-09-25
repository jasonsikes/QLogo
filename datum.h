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
#include "QDebug"

#ifndef dv
#define dv(x) qDebug()<<#x<<'='<<x
#endif

class ASTNode;
class Word;
class List;
class ListNode;
class Array;
class Error;
class DatumP;
class Procedure;

class Iterator;
class WordIterator;
class ListIterator;
class ArrayIterator;

class Kernel;
typedef DatumP (Kernel::*KernelMethod)(DatumP);

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
DatumP nodes();

/// The unit of data for QLogo. The base class for Word, List, Array, ASTNode, etc.
class Datum {
  friend class Iterator;
    friend class DatumPool;

protected:
  int retainCount;
    Datum *nextInPool;
  QList<DatumP *> retainers;

  virtual void addToPool();

public:
  /// Value returned by isa().
  enum DatumType {
    noType,
    wordType,
    listType,
      listNodeType,
    arrayType,
    astnodeType,
    procedureType,
    errorType
  };

  /// \brief Constructs a Datum
  ///
  /// The Datum class is the superclass for all data.
  /// The Datum superclass maintains retain counts (manipulated by the DatumP class).
  ///
  /// To hold data:
  /// 1. use one of the subclasses, NOT Datum, and
  /// 2. use alloc(), NOT the default constructor.
  Datum();

  ~Datum();

  Datum &operator=(const Datum &);

  /// Increment the retain count.
  inline void retain(void)
  {
      ++retainCount;
  }

  /// Decrement the retain count.
  inline void release(void) {
      --retainCount;
      if (retainCount <= 0) {
          addToPool();
      }
  }

  /// Return type of this object.
  virtual DatumType isa();

  /// For debugging.
  virtual QString name();

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
  virtual DatumP first(void);

  /// Return everything but the first element.
  virtual DatumP butfirst(void);

  /// Return the last element.
  virtual DatumP last(void);

  /// Return everything but the last element.
  virtual DatumP butlast(void);

  /// Determine if the object pointed to by other is equal to this object.
  virtual bool isEqual(DatumP other, bool);

  /// return the number of elements in the object.
  virtual int size();

  /// creates and returns a new Iterator object for this object.
  Iterator newIterator(void);

  /// returns the element at the index given.
  virtual DatumP datumAtIndex(int);

  /// returns true if the index given is valid for this object.
  virtual bool isIndexInRange(int);

  /// replaces the item at the index of this object with aValue.
  virtual void setItem(int anIndex, DatumP aValue);

  /// replaces the first element of this object with aValue.
  virtual void setFirstItem(DatumP aValue);

  /// replaces everything but the first element of this object with aValue.
  virtual void setButfirstItem(DatumP aValue);

  /// recursively search this object for an instance of a Datum.
  virtual bool containsDatum(DatumP, bool);

  /// nonrecursively search this object for an instance of a Datum.
  virtual bool isMember(DatumP aDatum, bool);

  /// return a new Datum beginning with the first occurrence of aDatum.
  virtual DatumP fromMember(DatumP aDatum, bool ignoreCase);
};


class DatumPool
{
  protected:
  Datum *top;

  // For debugging
  int allocCount = 0; // Count of objects in the pool
  int poolCount = 0; // Count of objecs allocated that are outside the pool

  /// Acquire a new block of objects to be allocated.
  void fillPool();

  // Needs to be implemented by subclasses.
  // /param box is a vector of pointers, initially empty.
  // Subclass needs to create a block of Datums, and then
  // fill the box with pointers to each Datum.
  virtual void createNewDatums(QVector<Datum*> &box);

  // Return the size of a memory page in bytes.
  int getPageSize();

  public:

  /// acquire a Datum from its pool.
  Datum * alloc();

  /// place a datum into its pool.
  void dealloc(Datum *);

  /// For debugging: show the status of pool
  void debugMessage()
  {
      dv(allocCount);
      dv(poolCount);
  }
};


/// A datum that has no value.
extern Datum notADatum;

#endif // DATUM_H
