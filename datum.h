#ifndef DATUM_H
#define DATUM_H

//===-- qlogo/datum.h - Datum class and subclasses definition -------*- C++
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
class Object;

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
/// Each Word, List, and Array is a node. So, for example, a list of two words is three
/// nodes (1 List + 2 Words). Furthermore, if the list is "RUN" (e.g. "RUN [forward 100]")
/// then ASTNodes will be created, adding to the number of nodes.
DatumP nodes();

/// The unit of data for QLogo. The base class for Word, List, Array, ASTNode, etc.
class Datum {
  friend class Iterator;

protected:
  int retainCount;
  bool isDestroyable = true; // trueWord, falseWord, and notADatum are internal constants and cannot be destroyed.

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
    objectType,
    errorType
  };

  /// \brief Constructs a Datum
  ///
  /// The Datum class is the superclass for all model objects.
  /// The Datum superclass maintains retain counts (manipulated by the DatumP class).
  /// Datum may be instantiated, but it is only useful as a NULL value. To hold data,
  /// use one of the subclasses.
  Datum();
  virtual ~Datum();

  Datum &operator=(const Datum &);

  /// Increment the retain count.
  void retain() { ++retainCount; }

  /// Decrement the retain count.
  void release() { --retainCount; }

  /// Query to determine if all references to this object are destroyed and it is destructable.
  bool shouldDelete() { return (retainCount <= 0) && isDestroyable; }

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

/// A pointer to a Datum. Incorporates convenience methods, reference-counting, and automatic destruction.
class DatumP {
protected:

  Datum *d;

  void destroy();

public:

  /// Copy constructor. Increases retain count of the referred object.
  DatumP(const DatumP &other) noexcept;

  /// Default constructor. Points to notADatum (like NULL)
  DatumP();

  /// \brief Creates a pointer to a Datum object. Begins reference counting.
  ///
  /// Creates a pointer to the referenced object and increases its retain count.
  /// The referred object will be destroyed when the last object referring to it
  /// is destroyed.
  DatumP(Datum *);

  /// \brief Convenience constructor for "true" and "false".
  ///
  /// For efficiency of boolean operations, the Word objects "true" and "false"
  /// are static. This constructor simply creates a pointer to one of the two
  /// words depending on the value b.
  explicit DatumP(bool b);

  /// \brief Destructor.
  ///
  /// Decreases the retain count of the referred object. If this is the last
  /// pointer to the referred object (if its retain count reaches zero) the
  /// object is destroyed, if possible. Static objects (notADatum, trueWord, and
  /// falseWord) are not destroyed.
  ~DatumP();

  /// Returns a pointer to the referred Datum or any of Datum's subclasses.
  Datum *datumValue() { return d; }

  /// Returns a pointer to the referred Datum as a Word.
  Word *wordValue();

  /// Returns a pointer to the referred Datum as a List.
  List *listValue();

  /// Returns a pointer to the referred Datum as a ListNodeValue.
  ListNode *listNodeValue();

  /// Returns a pointer to the referred Datum as a Procedure.
  Procedure *procedureValue();

  /// Returns a pointer to the referred Datum as an ASTNode.
  ASTNode *astnodeValue();

  /// Returns a pointer to the referred Datum as an Array.
  Array *arrayValue();

  /// Returns a pointer to the referred Datum as an Object.
  Object *objectValue();

  /// Returns a pointer to the referred Datum as an Error.
  Error *errorValue();

  /// Returns true if the referred Datum is a Word, false otherwise.
  bool isWord();

  /// Returns true if the referred Datum is a List, false otherwise.
  bool isList();

  /// Returns true if the referred Datum is an ASTNode, false otherwise.
  bool isASTNode();

  /// Returns true if the referred Datum is an Array, false otherwise.
  bool isArray();

  /// Returns true if the referred Datum is an Object, false otherwise.
  bool isObject();

  /// Returns true if the referred Datum is an Error, false otherwise.
  bool isError();

  /// Returns true if the referred Datum is a notADatum, false otherwise.
  bool isNothing();

  /// Reassign the pointer to refer to the other object.
  DatumP &operator=(const DatumP &other) noexcept;

  /// Reassign the pointer to refer to the other object.
  DatumP &operator=(DatumP *other) noexcept;

  /// Return true if and only if other points to the same object as this.
  bool operator==(DatumP *other);

  /// Return true if and only if other points to the same object as this.
  bool operator==(const DatumP &other);

  /// Return true if and only if other does not point to the same object as this.
  bool operator!=(DatumP *other);

  /// Return true if and only if other does not point to the same object as this.
  bool operator!=(const DatumP &other);

  /// Return true if and only if the other object is equal to this in the manner suitable for EQUALP.
  bool isEqual(DatumP other, bool ignoreCase);

  /// Return true if and only if the other object is equal to this in the manner suitable for .EQ.
  bool isDotEqual(DatumP other);

  /// Return a string suitable for the PRINT command
  QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                     int printWidthLimit = -1);

  /// Return a string suitable for the SHOW command
  QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                    int printWidthLimit = -1);

  /// returns a DatumType enumerated value which is the DatumType of the referenced object.
  Datum::DatumType isa();
};

// If/when List is implemented using QList, this will increase efficiency.
// Since we're using linked lists, this is a noop for now.
Q_DECLARE_TYPEINFO(DatumP, Q_MOVABLE_TYPE);

/// \brief A node of QLogo's Abstract Syntax Tree.
///
/// Before execution, a list is parsed into a list of executable nodes. Each node
/// contains a name, a pointer to the KernelMethod that does the actual execution,
/// and an array of zero or more children.
class ASTNode : public Datum {
protected:
  QVector<DatumP> children;

public:

  /// A human-readable string. Usually the command name.
  DatumP nodeName;

  // TODO: This is badly misnamed! Should be called "method".
  // (This got caught in the mass renaming.)
  /// A pointer to the kernel method that should be called when executing this node.
  KernelMethod kernel;

  /// Add a child to the node.
  void addChild(DatumP aChild);

  /// Returns the child at the specified index.
  DatumP childAtIndex(unsigned index);

  /// Returns the number of children that this node owns.
  int countOfChildren();

  ASTNode(DatumP aNodeName);
  ASTNode(const char *aNodeName);
  ~ASTNode();
  DatumType isa();

  /// For debugging. To be used when printing out the AST.
  QString name();

  /// For debugging. To be used when printing out the AST.
  QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                     int printWidthLimit = -1);

  /// For debugging. To be used when printing out the AST.
  QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                    int printWidthLimit = -1);

  /// Not used. Returns nothing.
  DatumP first(void);
};

/// \brief The basic unit of data in QLogo. A Word is a string or number.
///
/// A word can be a string or number. String operations can be used on numbers.
///
/// e.g. "FIRST 23 + 34" outputs "5"
///
/// Words that are initially defined as strings may be parsed as numbers.
///
/// e.g. "SUM WORD 3 4 2" outputs "36".
class Word : public Datum {
protected:

  friend class WordIterator;

  // TODO: the nomenclature assumes words are mutable. They are not.
  enum DatumWordDirtyFlag { noValue, stringIsDirty, numberIsDirty, allClean };
  DatumWordDirtyFlag dirtyFlag;

  QString rawString;
  QString keyString;
  QString printableString;
  double number;
  bool numberConversionSucceeded;

public:

  bool isForeverSpecial = false;

  /// Create a Word object with a string.
  ///
  /// \param other the string value of this word
  /// \param aIsForeverSpecial characters defined with vertical bars will retain their special use.
  /// \param canBeDestroyed Allow this object to be destroyed if its retain count falls below 1.
  Word(const QString other, bool aIsForeverSpecial = false,
       bool canBeDestroyed = true);

  /// Create a Word object with a number.
  Word(double other);

  /// Create a Word object with an empty string.
  Word();
  ~Word();

  /// returns the number representation of the Word. Use didNumberConversionSucceed() to check.
  double numberValue(void);

  DatumType isa();
  QString name();

  // print() and show() will convert encoded chars to their displayable
  // equivalents
  QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                     int printWidthLimit = -1);
  QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                    int printWidthLimit = -1);

  /// Returns a string that can be used as the name of a procedure, variable, or property list.
  QString keyValue(); // for use as key for procedure names and variable names

  /// Returns the string with the special character encoding intact.
  QString rawValue();

  /// Returns true if the value pointed to by other is equal to this Word's value.
  /// \param other the value to be tested against.
  /// \param ignoreCase if true use case-insensitive compare.
  bool isEqual(DatumP other, bool ignoreCase);

  /// Returns the first character of the string value.
  DatumP first(void);

  /// Returns all but the first character of the string value
  DatumP butfirst(void);

  /// Returns the last character of the string value
  DatumP last(void);

  /// Returns all but the last character of the string value
  DatumP butlast(void);

  /// Returns true if the last call to numberValue() returned a valid number.
  bool didNumberConversionSucceed();

  /// returns the length of the string in characters.
  int size();

  /// Returns the character at position indicated by anIndex.
  DatumP datumAtIndex(int anIndex);

  /// Retruns true if anIndex is between 1 and the length of the string.
  bool isIndexInRange(int anIndex);

  /// Returns true if aDatum is a substring.
  bool containsDatum(DatumP aDatum, bool ignoreCase);

  /// Returns true if aDatum is a substring.
  bool isMember(DatumP aDatum, bool ignoreCase);

  /// Returns a substring starting at the first occurrence of aDatum to the end of the string.
  DatumP fromMember(DatumP aDatum, bool ignoreCase);

  Iterator newIterator(void);
};

/// An element of a List.
class ListNode : public Datum {
public:
    DatumType isa() { return listNodeType; }

    /// The item at this position in the list.
    DatumP item;

    /// A pointer to the next ListNode.
    DatumP next;
};

/// The container for data. The QLogo List is implemented as a linked list.
class List : public Datum {
  friend class ListIterator;
  friend class Parser; // Parser needs access to astList and astParseTimeStamp

protected:
  DatumP head;
  DatumP lastNode;
  int listSize;
  QList<DatumP> astList;
  qint64 astParseTimeStamp;
  void setListSize();

public:

  /// Create an empty List
  List();

  /// Create a new List using the elements of the source List.
  List(List *source);

  /// Create a new List using the elements of the source Array.
  List(Array *source);

  ~List();
  DatumType isa();
  QString name();
  QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                     int printWidthLimit = -1);
  QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                    int printWidthLimit = -1);
  bool isEqual(DatumP other, bool ignoreCase);

  /// Return the first item of the List.
  DatumP first(void);

  /// Return a new List starting with the second item of the List.
  DatumP butfirst(void);

  /// Empty the List
  void clear();

  /// Add an element to the end of the list.
  /// DO NOT USE after the List has been modified by any other method.
  void append(DatumP element);

  /// Returns the count of elements in the List.
  int size() { return listSize; }

  /// Returns the last elements of the List.
  DatumP last();

  /// Creates a new List using all but the last element of this List.
  DatumP butlast(void);

  /// Adds an element to the head of this List.
  void prepend(DatumP element);

  /// Creates a new List by adding an element to the head of this List.
  DatumP fput(DatumP item);

  /// Returns the element pointed to by anIndex.
  DatumP datumAtIndex(int anIndex);

  /// Returns true if anIndex is between 1 and the count of elements in the List.
  bool isIndexInRange(int anIndex);

  /// Replaces the item pointed to by anIndex with aValue.
  void setItem(int anIndex, DatumP aValue);

  /// Replaces the first item in the List with aValue.
  void setFirstItem(DatumP aValue);

  /// Replaces everything but the first item in the List with aValue.
  void setButfirstItem(DatumP aValue);

  /// Recursively searches List for aDatum. Returns true if found.
  bool containsDatum(DatumP aDatum, bool ignoreCase);

  /// Returns true if aDatum is a member of this List.
  bool isMember(DatumP aDatum, bool ignoreCase);

  /// Non-recursively searches this List for aDatum. Returns a new List starting
  /// from where aDaum was found to the end of this List.
  DatumP fromMember(DatumP aDatum, bool ignoreCase);

  ListIterator newIterator(void);
};

/// The container that allows efficient read and write access to its elements.
class Array : public Datum {
  friend class ArrayIterator;

protected:
  QVector<DatumP> array;

public:

  /// Create an Array containing aSize empty List with starting index at aOrigin.
  Array(int aOrigin, int aSize);

  /// Create an Array containing items copied from source with index starting at aOrigin.
  Array(int aOrigin, List *source);
  ~Array();
  DatumType isa();
  QString name();
  QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                     int printWidthLimit = -1);
  QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                    int printWidthLimit = -1);

  /// Returns true if items in other Array are equal to this Array's items.
  bool isEqual(DatumP other, bool ignoreCase);

  /// The starting index of this Array.
  int origin = 1;

  /// Returns the number of items in this Array.
  int size();

  /// Returns the item pointed to by anIndex.
  DatumP datumAtIndex(int anIndex);

  /// Returns true if anIndex can point to a valid element in the Array.
  bool isIndexInRange(int anIndex);

  /// Replace item at anIndex with aValue.
  void setItem(int anIndex, DatumP aValue);

  /// Replace the first item in the Array with aValue.
  void setFirstItem(DatumP aValue);

  /// Replace all but the first item in the Array with elements from aValue Array.
  void setButfirstItem(DatumP aValue);

  /// Recursively searches Array for aDatum. Returns true if found.
  bool containsDatum(DatumP aDatum, bool ignoreCase);

  /// Returns true if aDatum is a member of Array.
  bool isMember(DatumP aDatum, bool ignoreCase);

  /// Returns a new Array beginning with the first occurrence of aDatum to the end of the Array.
  DatumP fromMember(DatumP aDatum, bool ignoreCase);

  /// Returns the first element of the Array.
  DatumP first();

  /// Returns a new Array which is everything but the first element of this Array.
  DatumP butfirst();

  /// Returns the last element of the Array.
  DatumP last(void);

  /// Returns a new Array which is everything but the last element of this Array.
  DatumP butlast(void);

  /// Add an element to the end of this Array.
  void append(DatumP value);

  ArrayIterator newIterator();
};


/// The Object data type.
class Object : public Datum {

protected:

  static int counter;

  QHash<QString, DatumP> variables;
  QHash<QString, DatumP> procedures;
  QList<DatumP> parents;

  void initLicenseplate();
  const QString licenseplate();

public:

  /// constructor for Logo Object. Should only be used once.
  Object();

  /// Creates an object whose parent is aParent
  Object(DatumP aParent);

  /// Creates an object whose parents are aParents
  Object(List *aParents);

  DatumType isa();
  QString name();
  QString printValue(bool fullPrintp = false, int printDepthLimit = -1,
                     int printWidthLimit = -1);
  QString showValue(bool fullPrintp = false, int printDepthLimit = -1,
                    int printWidthLimit = -1);

  /// Returns true if this object is the same as 'other'
  bool isEqual(DatumP other, bool ignoreCase);

  /// tells object to create variable with 'name' and assign 'value'
  void havemake(const QString name, DatumP value);

  /// Check if variable name exists in this class. Optionally check parents.
  /// Returns pointer to Object that owns variable, or NULL.
  Object* hasVar(const QString varname, bool shouldSearchParents = false);

  /// Get value for given name.
  /// Calling program should check that name actually exists in this (not
  /// parents) instance.
  DatumP valueForName(const QString varname);

  /// Return the list of immediate parents
  List* getParents();

  /// Return a list of variable names from this (not parents) object.
  List* getVarnames();

  /// Check if procedure name exists in this class. Optionally check parents.
  /// Returns pointer to Object that owns procedure, or NULL.
  Object* hasProc(const QString procname, bool shouldSearchParents = false);

  /// Return a list of procedure names from this (not parents) object.
  List *getProcNames();
};

/// A very simple iterator. Base class does nothing. Meant to be subclassed.
class Iterator {
public:
  virtual DatumP element(); /// Returns current DatumP in collection, advances pointer.
  virtual bool elementExists(); /// Returns true if not at end.
};

class ListIterator : public Iterator {
protected:
    DatumP ptr;

public:
  ListIterator();

  /// Create a new ListIterator pointing to the head of the List.
  ListIterator(DatumP head);

  /// Return the element at the current location. Advance Iterator to the next location.
  DatumP element();

  /// Returns true if pointer references a valid element.
  bool elementExists();
};

/// Iterator for an Array.
class ArrayIterator : public Iterator {
protected:
  QVector<DatumP>::iterator arrayIter;
  QVector<DatumP>::iterator end;

public:
  ArrayIterator();

  /// Create a new ArrayIterator pointing to the first element of the Array.
  ArrayIterator(QVector<DatumP> *aArray);

  /// Return the element at the current index. Advance the index pointer.
  DatumP element();

  /// Returns true if the index points to an element in the Array.
  bool elementExists();
};

/// Iterator for a Word. Each element is a character in the Word.
class WordIterator : public Iterator {
  QString::iterator charIter;
  QString::iterator end;

public:
  WordIterator();

  /// Create a new Iterator pointing to the first char of a Word.
  WordIterator(Word *aWord);

  /// Returns the character at the current index as a Word. Advances the index.
  DatumP element();

  /// Returns true if there is a character at the curent index.
  bool elementExists();
};

/// A datum that has no value.
extern Datum notADatum;

/// A pointer to notADatum, like NULL.
extern DatumP nothing;

extern Word trueWord;
extern Word falseWord;

#endif // DATUM_H
