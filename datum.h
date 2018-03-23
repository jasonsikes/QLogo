#ifndef DATUM_H
#define DATUM_H

//===-- qlogo/datum.h - Datum class and subclasses definition -------*- C++
//-*-===//
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
/// This file contains the declaration of the Datum class and its subclasses,
/// Word, List, and Array, which are the data units of QLogo. This file also
/// contains the declaration of DatumP, a pointer to a Datum.
///
//===----------------------------------------------------------------------===//

#include <QHash>
#include <QString>
#include <QVector>
#include <QList>

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

QChar rawToChar(const QChar &src);
QChar charToRaw(const QChar &src);

void rawToChar(QString &src);

DatumP nodes();

class Datum {
  friend class Iterator;

protected:
  int retainCount;
  bool isMutable;
  bool isDestroyable = true;

public:
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

  Datum();
  virtual ~Datum();
  void setImmutable();

  Datum &operator=(const Datum &);

  void retain() { ++retainCount; }
  void release() { --retainCount; }
  bool shouldDelete() { return (retainCount <= 0) && isDestroyable; }

  virtual DatumType isa();
  virtual QString name(); // for internal use and testing

  virtual QString printValue(
      bool = false, int printDepthLimit = -1,
      int printWidthLimit = -1); // return a value suitable for print command
  virtual QString showValue(
      bool = false, int printDepthLimit = -1,
      int printWidthLimit = -1); // return a value suitable for show command
  virtual DatumP first(void);
  virtual DatumP butfirst(void);
  virtual DatumP last(void);
  virtual DatumP butlast(void);
  virtual bool isEqual(DatumP other, bool);
  virtual int size();
  Iterator newIterator(void);
  virtual DatumP datumAtIndex(int);
  virtual bool isIndexInRange(int);
  virtual void setItem(int anIndex, DatumP aValue);
  virtual void setFirstItem(DatumP aValue);
  virtual void setButfirstItem(DatumP aValue);
  virtual bool containsDatum(DatumP, bool);
  virtual bool isMember(DatumP aDatum, bool);
  virtual DatumP fromMember(DatumP aDatum, bool ignoreCase);
};

class DatumP {
protected:
  Datum *d;

  void destroy();

public:
  DatumP(const DatumP &other) noexcept;
  DatumP();
  DatumP(Datum *);
  explicit DatumP(bool b);
  ~DatumP();

  Datum *datumValue() { return d; }
  Word *wordValue();
  List *listValue();
  ListNode *listNodeValue();
  Procedure *procedureValue();
  ASTNode *astnodeValue();
  Array *arrayValue();
  Error *errorValue();

  bool isWord();
  bool isList();
  bool isASTNode();
  bool isArray();
  bool isError();
  bool isNothing();

  DatumP &operator=(const DatumP &other) noexcept;
  DatumP &operator=(DatumP *other) noexcept;
  bool operator==(DatumP *other);
  bool operator==(const DatumP &other);
  bool operator!=(DatumP *other);
  bool operator!=(const DatumP &other);

  bool isEqual(DatumP other, bool ignoreCase);
  bool isDotEqual(DatumP other);

  QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                     int printWidthLimit = -1);
  QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                    int printWidthLimit = -1);
  Datum::DatumType isa();
};

Q_DECLARE_TYPEINFO(DatumP, Q_MOVABLE_TYPE);

class ASTNode : public Datum {
protected:
  std::vector<DatumP> children;

public:
  DatumP nodeName;
  DatumP sourceList;
  KernelMethod kernel;

  void addChild(DatumP aChild);
  DatumP childAtIndex(unsigned index);
  int countOfChildren();

  ASTNode(DatumP aNodeName);
  ASTNode(const char *aNodeName);
  ~ASTNode();
  DatumType isa();
  QString name();

  QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                     int printWidthLimit = -1);
  QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                    int printWidthLimit = -1);
  DatumP first(void);
};

class Word : public Datum {
protected:
  enum DatumWordDirtyFlag { noValue, stringIsDirty, numberIsDirty, allClean };

  friend class WordIterator;
  DatumWordDirtyFlag dirtyFlag;
  QString rawString;
  QString keyString;
  QString printableString;
  double number;
  bool numberConversionSucceeded;

public:
  Word(const QString other, bool aIsForeverSpecial = false,
       bool canBeDestroyed = true);
  Word(double other);
  Word();
  ~Word();

  bool isForeverSpecial = false;

  double numberValue(void);
  DatumType isa();
  QString name();

  // print() and show() will convert encoded chars to their displayable
  // equivalents
  QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                     int printWidthLimit = -1);
  QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                    int printWidthLimit = -1);
  QString keyValue(); // for use as key for procedure names and variable names
  QString rawValue();
  bool isEqual(DatumP other, bool ignoreCase);
  DatumP first(void);
  DatumP butfirst(void);
  DatumP last(void);
  DatumP butlast(void);
  bool didNumberConversionSucceed();
  int size();
  DatumP datumAtIndex(int anIndex);
  bool isIndexInRange(int anIndex);
  bool containsDatum(DatumP aDatum, bool ignoreCase);
  bool isMember(DatumP aDatum, bool ignoreCase);
  DatumP fromMember(DatumP aDatum, bool ignoreCase);

  Iterator newIterator(void);
};

class ListNode : public Datum {
    DatumType isa() { return listNodeType; }
public:
    DatumP item;
    DatumP next;
};

class List : public Datum {
  friend class ListIterator;
  friend class Array;
  friend class Parser;

protected:
  DatumP head;
  int listSize;
  QList<DatumP> astList;
  qint64 astParseTimeStamp;
  void setListSize();

public:
  List();
  List(List *source);
  List(Array *source);
  ~List();
  DatumType isa();
  QString name();
  QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                     int printWidthLimit = -1);
  QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                    int printWidthLimit = -1);
  bool isEqual(DatumP other, bool ignoreCase);
  DatumP first(void);
  DatumP butfirst(void);

  void clear();
  void append(DatumP element);
  int size() { return listSize; }
  DatumP last();
  DatumP butlast(void);
  void prepend(DatumP element);
  DatumP fput(DatumP item);
  DatumP datumAtIndex(int anIndex);
  bool isIndexInRange(int anIndex);
  void setItem(int anIndex, DatumP aValue);
  void setFirstItem(DatumP aValue);
  void setButfirstItem(DatumP aValue);
  bool containsDatum(DatumP aDatum, bool ignoreCase);
  bool isMember(DatumP aDatum, bool ignoreCase);
  DatumP fromMember(DatumP aDatum, bool ignoreCase);

  ListIterator newIterator(void);
};

class Array : public Datum {
  friend class ArrayIterator;
  friend class List;

protected:
  QVector<DatumP> array;

public:
  Array(int aOrigin, int aSize);
  Array(int aOrigin, List *source);
  ~Array();
  DatumType isa();
  QString name();
  QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                     int printWidthLimit = -1);
  QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                    int printWidthLimit = -1);
  bool isEqual(DatumP other, bool ignoreCase);
  int origin = 1;
  int size();
  DatumP datumAtIndex(int anIndex);
  bool isIndexInRange(int anIndex);
  void setItem(int anIndex, DatumP aValue);
  void setFirstItem(DatumP aValue);
  void setButfirstItem(DatumP aValue);
  bool containsDatum(DatumP aDatum, bool ignoreCase);
  bool isMember(DatumP aDatum, bool ignoreCase);
  DatumP fromMember(DatumP aDatum, bool ignoreCase);
  DatumP first();
  DatumP butfirst();
  DatumP last(void);
  DatumP butlast(void);
  void append(DatumP value);

  ArrayIterator newIterator();
};

class Procedure : public Datum {

public:
  QStringList requiredInputs;
  QStringList optionalInputs;
  QList<DatumP> optionalDefaults;
  QString restInput;
  int defaultNumber;
  int countOfMinParams;
  int countOfMaxParams;
  QHash<const QString, DatumP> tagToLine;
  bool isMacro;
  DatumP sourceText;

  DatumP instructionList;
  DatumType isa() { return Datum::procedureType; }

  Procedure() {
    instructionList = DatumP(new List);
    countOfMaxParams = -1;
    countOfMinParams = 0;
  }
};

class Iterator {
public:
  virtual DatumP
  element(); // Returns current DatumP in collection, advances pointer
  virtual bool elementExists(); // returns true if not at end
};

class ListIterator : public Iterator {
protected:
    DatumP ptr;

public:
  ListIterator();
  ListIterator(DatumP head);
  DatumP element();
  bool elementExists();
};

class ArrayIterator : public Iterator {
protected:
  QVector<DatumP>::iterator arrayIter;
  QVector<DatumP>::iterator end;

public:
  ArrayIterator();
  ArrayIterator(QVector<DatumP> *aArray);
  DatumP element();
  bool elementExists();
};

class WordIterator : public Iterator {
  QString::iterator charIter;
  QString::iterator end;

public:
  WordIterator();
  WordIterator(Word *aWord);
  DatumP element();
  bool elementExists();
  bool isBetweenBars = false;
};

extern Datum notADatum;
extern DatumP nothing;

#endif // DATUM_H
