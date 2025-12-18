#ifndef DATUM_H
#define DATUM_H

//===-- qlogo/datum.h - Datum class definition -------*- C++ -*-===//
//
// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Datum class, which is the base class
/// for all the data classes used by QLogo.
///
//===----------------------------------------------------------------------===//

#include "compiler_types.h"
#include "datum_ptr.h"
#include "datum_types.h"

// // Qt #defines "emit". llvm uses "emit" as a function name.
// #ifdef emit
// #undef emit
// #endif

// #include <llvm/IR/Value.h>
// #include <llvm/ExecutionEngine/Orc/Core.h>

// #include <QDebug>
// #include <QChar>
// #include <QStack>

// class Datum;
// class Word;
// class List;
// struct Array;

// struct FlowControl;
// struct FCError;

// class Unbound;
// class ASTNode;
// class Procedure;
// class EmptyList;

// class DatumPtr;
// class ListIterator;
// class VisitedSet;

// class Kernel;
// class Compiler;

// typedef uint64_t* addr_t;

// // Compiled function signature
// typedef Datum* (*CompiledFunctionPtr)(addr_t, int32_t);



// // The information to store the generated function and to destroy later
// struct CompiledText
// {
//     llvm::orc::ResourceTrackerSP rt;

//     CompiledFunctionPtr functionPtr = nullptr;

//     ~CompiledText();
// };



// /// @brief Expression generator request type.
// ///
// /// Request that the generator generate code that produces this output type.
// enum RequestReturnType : int
// {
//     RequestReturnVoid    = 0x00,
//     RequestReturnNothing = 0x01,
//     RequestReturnN       = 0x01, // N
//     RequestReturnBool    = 0x02, // B
//     RequestReturnB       = 0x02,
//     RequestReturnBN      = 0x03,
//     RequestReturnDatum   = 0x04, // D
//     RequestReturnD       = 0x04,
//     RequestReturnDN      = 0x05,
//     RequestReturnDB      = 0x06,
//     RequestReturnDBN     = 0x07,
//     RequestReturnReal    = 0x08, // R
//     RequestReturnR       = 0x08,
//     RequestReturnRN      = 0x09,
//     RequestReturnRB      = 0x0A,
//     RequestReturnRBN     = 0x0B,
//     RequestReturnRD      = 0x0C,
//     RequestReturnRDN     = 0x0D,
//     RequestReturnRDB     = 0x0E,
//     RequestReturnRDBN    = 0x0F,
// };

// /// Signature of method that generates IR code for a given node.
// typedef llvm::Value * (Compiler::*Generator)(DatumPtr, RequestReturnType);

// /// Convert "raw" encoding to Char encoding.
// QChar rawToChar(const QChar &src);

// /// Convert Char encoding to "raw" encoding.
// QChar charToRaw(const QChar &src);

// /// Convert a string from "raw" encoding to Char. In place.
// void rawToChar(QString &src);

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

// /// @brief The unit of data for QLogo. The base class for Word, List, Array, ASTNode, etc.
// class Datum
// {
//     friend class ListIterator;
//     friend class DatumPtr;
//     friend struct Evaluator;

//   protected:

//     /// @brief Protected constructors to prevent direct instantiation.
//     ///
//     /// @details The Datum class uses the singleton pattern. Only one instance
//     /// of Datum can exist (accessed via getInstance()). Subclasses can still
//     /// be instantiated multiple times because they can call these protected constructors.
//     Datum &operator=(const Datum &) = delete;
//     Datum &operator=(Datum &&) = delete;
//     Datum &operator=(Datum *) = delete;
//     Datum(const Datum &) = delete;
//     Datum(Datum &&) = delete;
//     Datum();

//   public:

//     /// @brief Value stored in isa.
//     enum DatumType : uint32_t
//     {
//         // These are the three data types that are made available to the user.
//         typeWord            = 0x00000001,
//         typeList            = 0x00000002,
//         typeArray           = 0x00000004,

//         typeDataMask        = 0x00000007, // Word + List + Array
//         typeWordOrListMask  = 0x00000003, // Word + List

//         // These are the types that control the flow of the program.
//         typeError           = 0x00000010,
//         typeGoto            = 0x00000020,
//         typeContinuation    = 0x00000040,
//         typeReturn          = 0x00000080,
//         typeFlowControlMask = 0x000000F0,

//         // These are the types that are used internally by QLogo.
//         typeNothing         = 0x00000100,
//         typeASTNode         = 0x00000200,
//         typeProcedure       = 0x00000400,

//         typeUnboundMask     = 0x00000300, // typeASTNode + typeNothing

//         typePersistentMask  = 0x00010000, // OR this value to prevent the datum from being destroyed
//     };

//     DatumType isa; // Subclasses must set this to a valid value.

//     int retainCount;

//     /// @brief If set to 'true', DatumPtr will send qDebug message when this is deleted.
//     bool alertOnDelete = false;

//     /// @brief Get the singleton instance of Datum.
//     ///
//     /// @details Returns the single instance of Datum. This instance represents
//     /// "nothing" (similar to nullptr). Subclasses like Word, List, Array, etc.
//     /// can still be instantiated multiple times.
//     ///
//     /// @return A pointer to the singleton Datum instance.
//     static Datum *getInstance();

//     /// @brief Destructor.
//     virtual ~Datum();

//     /// @brief This enum specifies flags that can be used to affect various aspects
//     /// of the string representation of the Datum.
//     enum ToStringFlags : int
//     {
//         ToStringFlags_None = 0x00,
//         ToStringFlags_FullPrint = 0x01, // Show backslashes and vertical bars in words
//         ToStringFlags_Show = 0x02,      // Show list brackets
//         ToStringFlags_Source = 0x04,    // Format for parsing as qlogo source code
//         ToStringFlags_Key = 0x08,       // Format for use as a key in a map
//         ToStringFlags_Raw = 0x10,       // Format for use as a raw string (no special decoding of mapped characters)
//     };

//     /// @brief Return a string representation of the Datum.
//     /// @param flags Flags to control the output. See ToStringFlags for possible values.
//     /// @param printDepthLimit Limit the depth of sublists or arrays for readability.
//     /// printDepthLimit = 1 means don't show sublists or arrays.
//     /// printDepthLimit = 2 means show sublists or arrays, but don't show sublist's sublist or array's subarray.
//     /// printDepthLimit = 0 means show '...' instead of THIS list or array.
//     /// printDepthLimit = -1 means show all sublists or arrays.
//     /// @param printWidthLimit Limit the length of a string or list or array for readability.
//     /// @param visited Set of visited nodes to prevent cycles.
//     /// @return A string representation of the Datum.
//     virtual QString toString( ToStringFlags flags = ToStringFlags_None, int printDepthLimit = -1, int printWidthLimit = -1, VisitedSet *visited = nullptr);

//     /// @brief Returns true if the referred Datum is a List, false otherwise.
//     ///
//     /// @return True if the referred Datum is a List, false otherwise.
//     bool isList()
//     {
//         return (isa & Datum::typeList) != 0;
//     }

//     /// @brief Returns true if the referred Datum is an Array, false otherwise.
//     ///
//     /// @return True if the referred Datum is an Array, false otherwise.
//     bool isArray()
//     {
//         return (isa & Datum::typeArray) != 0;
//     }

//     /// @brief Returns true if the referred Datum is a Word, false otherwise.
//     ///
//     /// @return True if the referred Datum is a Word, false otherwise.
//     bool isWord()
//     {
//         return (isa & Datum::typeWord) != 0;
//     }

//     /// @brief Performs an assertion check that the referred Datum is a Word. Returns a pointer to the referred Datum as a Word.
//     ///
//     /// @return A pointer to the referred Datum as a Word.
//     Word *wordValue()
//     {
//         Q_ASSERT(isWord());
//         return reinterpret_cast<Word *>(this);
//     }

//     /// @brief Performs an assertion check that the referred Datum is a List. Returns a pointer to the referred Datum as a List.
//     ///
//     /// @return A pointer to the referred Datum as a List.
//     List *listValue()
//     {
//         Q_ASSERT(isList());
//         return reinterpret_cast<List *>(this);
//     }

//     /// @brief Performs an assertion check that the referred Datum is an Array. Returns a pointer to the referred Datum as an Array.
//     ///
//     /// @return A pointer to the referred Datum as an Array.
//     Array *arrayValue()
//     {
//         Q_ASSERT(isArray());
//         return reinterpret_cast<Array *>(this);
//     }
// };

// /// @brief A smart pointer to a Datum.
// ///
// /// @details This class is a smart pointer to a Datum. It incorporates convenience
// /// methods, reference-counting, and automatic destruction of the referred datum.
// class DatumPtr
// {
//   protected:
//     Datum *d;

//     void destroy();

//   public:
//     /// Copy constructor. Increases retain count of the referred object.
//     DatumPtr(const DatumPtr &other) noexcept;

//     /// Default constructor. Creates a pointer to the singleton Datum instance.
//     DatumPtr();

//     /// @brief Creates a pointer to a Datum object. Begins reference counting.
//     ///
//     /// Creates a pointer to the specified Datum object and increases its retain count.
//     /// The referred object will be destroyed when the last object referring to it
//     /// is destroyed.
//     DatumPtr(Datum *);

//     /// @brief Destructor.
//     ///
//     /// Decreases the retain count of the referred object. If this is the last
//     /// pointer to the referred object (if its retain count reaches zero) the
//     /// object is destroyed.
//     ~DatumPtr();

//     /// @brief Convenience constructor for "true" and "false".
//     ///
//     /// @param b The boolean value to create the DatumPtr for.
//     /// @return A new DatumPtr pointing to a new Word containing the boolean value.
//     explicit DatumPtr(bool b);

//     /// @brief Convenience constructor for numbers.
//     ///
//     /// @param n The number to create the DatumPtr for.
//     /// @return A new DatumPtr pointing to a new Word containing the number.
//     explicit DatumPtr(double n);

//     /// @brief Convenience constructor for integers.
//     ///
//     /// @param n The integer to create the DatumPtr for.
//     /// @return A new DatumPtr pointing to a new Word containing the integer.
//     explicit DatumPtr(int n);

//     /// @brief Convenience constructor for strings.
//     ///
//     /// @param n The string to create the DatumPtr for.
//     /// @param isVBarred Whether the string is created with vertical bars.
//     /// @return A new DatumPtr pointing to a new Word containing the string.
//     explicit DatumPtr(QString n, bool isVBarred = false);

//     /// @brief Convenience constructor for const char strings.
//     ///
//     /// @param n The string to create the DatumPtr for.
//     /// @return A new DatumPtr pointing to a new Word containing the string.
//     explicit DatumPtr(const char *n);

//     /// @brief Returns a pointer to the referred Datum or any of Datum's subclasses.
//     ///
//     /// @return A pointer to the referred Datum or any of Datum's subclasses.
//     Datum *datumValue()
//     {
//         return d;
//     }

//     /// @brief Returns a pointer to the referred Datum as a Word.
//     ///
//     /// @return A pointer to the referred Datum as a Word.
//     Word *wordValue();

//     /// @brief Returns a pointer to the referred Datum as a List.
//     ///
//     /// @return A pointer to the referred Datum as a List.
//     List *listValue();

//     /// @brief Returns a pointer to the referred Datum as a Procedure.
//     ///
//     /// @return A pointer to the referred Datum as a Procedure.
//     Procedure *procedureValue();

//     /// @brief Returns a pointer to the referred Datum as an ASTNode.
//     ///
//     /// @return A pointer to the referred Datum as an ASTNode.
//     ASTNode *astnodeValue();

//     /// @brief Returns a pointer to the referred Datum as an Array.
//     ///
//     /// @return A pointer to the referred Datum as an Array.
//     Array *arrayValue();

//     /// @brief Returns a pointer to the referred Datum as a FlowControl.
//     ///
//     /// @return A pointer to the referred Datum as a FlowControl.
//     FlowControl *flowControlValue();

//     /// @brief Returns a pointer to the referred Datum as an Err.
//     ///
//     /// @return A pointer to the referred Datum as an Err.
//     FCError *errValue();

//     /// @brief Returns true if the referred Datum is a Word, false otherwise.
//     ///
//     /// @return True if the referred Datum is a Word, false otherwise.
//     bool isWord()
//     {
//         return d->isa == Datum::typeWord;
//     }

//     /// @brief Returns true if the referred Datum is a List, false otherwise.
//     ///
//     /// @return True if the referred Datum is a List, false otherwise.
//     bool isList()
//     {
//         return (d->isa & Datum::typeList) != 0;
//     }

//     /// @brief Returns true if the referred Datum is an ASTNode, false otherwise.
//     ///
//     /// @return True if the referred Datum is an ASTNode, false otherwise.
//     bool isASTNode()
//     {
//         return d->isa == Datum::typeASTNode;
//     }

//     /// @brief Returns true if the referred Datum is an Array, false otherwise.
//     ///
//     /// @return True if the referred Datum is an Array, false otherwise.
//     bool isArray()
//     {
//         return d->isa == Datum::typeArray;
//     }


//     /// @brief Returns true if the referred Datum is an Err, false otherwise.
//     ///
//     /// @return True if the referred Datum is an Err, false otherwise.
//     bool isErr()
//     {
//         return d->isa == Datum::typeError;
//     }


//     /// @brief Returns true if the referred Datum is the singleton Datum instance, false otherwise.
//     ///
//     /// @return True if the referred Datum is the singleton Datum instance, false otherwise.
//     bool isNothing() {
//         return d == Datum::getInstance();
//     }

//     /// @brief Returns true if the referred Datum is a FlowControl, false otherwise.
//     ///
//     /// @return True if the referred Datum is a FlowControl, false otherwise.
//     bool isFlowControl()
//     {
//         return (d->isa & Datum::typeFlowControlMask) != 0;
//     }


//     /// @brief Reassign the pointer to refer to the other object.
//     ///
//     /// @param other The DatumPtr to assign to this.
//     /// @return A reference to this.
//     DatumPtr &operator=(const DatumPtr &other) noexcept;

//     /// @brief Return true if and only if other points to the same object as this.
//     ///
//     /// @param other The DatumPtr to compare to this.
//     /// @return True if and only if other points to the same object as this.
//     bool operator==(const DatumPtr &other);

//     /// @brief Return true if and only if other does not point to the same object as this.
//     ///
//     /// @param other The DatumPtr to compare to this.
//     /// @return True if and only if other does not point to the same object as this.
//     bool operator!=(const DatumPtr &other);

//     /// @brief Return a string representation of the Datum.
//     /// @param flags Flags to control the output. See ToStringFlags for possible values.
//     /// @param printDepthLimit Limit the depth of sublists or arrays for readability.
//     /// printDepthLimit = 1 means don't show sublists or arrays.
//     /// printDepthLimit = 2 means show sublists or arrays, but don't show sublist's sublist or array's subarray.
//     /// printDepthLimit = 0 means show '...' instead of THIS list or array.
//     /// printDepthLimit = -1 means show all sublists or arrays.
//     /// @param printWidthLimit Limit the length of a string or list or array for readability.
//     /// @param visited Set of visited nodes to prevent cycles.
//     /// @return A string representation of the Datum.
//     QString toString( Datum::ToStringFlags flags = Datum::ToStringFlags_None, int printDepthLimit = -1, int printWidthLimit = -1, VisitedSet *visited = nullptr);

//     /// @brief returns the DatumType of the referenced object.
//     ///
//     /// @return The DatumType of the referenced object.
//     Datum::DatumType isa()
//     {
//         return d->isa;
//     }

//     /// @brief Set a mark on the datum so that debug message will print when datum is
//     /// destroyed.
//     ///
//     /// @details This is used to help debug memory leaks. The MARK command is used to set
//     /// a mark on the datum so that a debug message will be printed when the datum is destroyed.
//     void alertOnDelete()
//     {
//         qDebug() << "MARKED: " << d << " " << d->toString(Datum::ToStringFlags_Show);
//         d->alertOnDelete = true;
//     }
// };

// Q_DECLARE_TYPEINFO(DatumPtr, Q_RELOCATABLE_TYPE);

// /// @brief The basic unit of data in QLogo. A Word is a string or number.
// ///
// /// A word can be a string or number. String operations can be used on numbers.
// ///
// /// e.g. "FIRST 23 + 34" outputs "5"
// ///
// /// Words that are initially defined as strings may be parsed as numbers.
// ///
// /// e.g. "SUM WORD 3 4 2" outputs "36".
// class Word : public Datum
// {
//   protected:
//     QString rawString;
//     QString keyString;
//     QString printableString;
//     double number;
//     bool boolean;
//     bool sourceIsNumber;

//     void genRawString();
//     void genPrintString();
//     void genKeyString();

//   public:
//     /// @brief Set to true if the word was created with vertical bars as delimiters.
//     /// Words created this way will not be separated during parsing or runparsing.
//     bool isForeverSpecial;

//     /// @brief True if a number was calculated/given AND the number is valid
//     /// @note Read this AFTER calling numberValue()
//     bool numberIsValid = false;

//     /// @brief True if the word is either "true" or "false".
//     /// @note Read this AFTER calling boolValue()
//     bool boolIsValid = false;

//     /// @brief Create a Word object that is invalid.
//     Word();

//     /// @brief Create a Word object with a string.
//     ///
//     /// @param other the string value of this word
//     /// @param aIsForeverSpecial if set to 'true' characters defined with vertical bars will
//     /// not be treated as token delimiters during parsing.
//     Word(const QString other, bool aIsForeverSpecial = false);

//     /// @brief Create a Word object with a number.
//     Word(double other);

//     ~Word();

//     /// @brief returns the number representation of the Word, if possible.
//     /// @note check numberIsValid to determine validity AFTER calling this. It may seem counterintuitive,
//     /// but it's because that is the procedure of the underlying Qt toolkit.
//     double numberValue(void);

//     /// @brief returns the boolean representation of the Word, if possible.
//     /// @note check boolIsValid to determine validity AFTER calling this. It may seem counterintuitive,
//     /// but it's because that is the procedure of the underlying Qt toolkit.
//     bool boolValue(void);

//     virtual QString toString( Datum::ToStringFlags flags = Datum::ToStringFlags_None, int printDepthLimit = -1, int printWidthLimit = -1, VisitedSet *visited = nullptr) override;


//     /// @brief Return true iff this word was created with a number.
//     ///
//     /// @return True iff this word was created with a number.
//     bool isSourceNumber()
//     {
//         return sourceIsNumber;
//     }
// };

// /// @brief The container that allows efficient read and write access to its elements.
// struct Array : public Datum
// {
//     /// @brief The container that stores the elements of the Array.
//     QList<DatumPtr> array;

//     /// @brief Create an Array containing aSize empty List with starting index at aOrigin.
//     ///
//     /// @param aOrigin The starting index of the Array.
//     /// @param aSize The number of elements in the Array.
//     Array(int aOrigin = 1, int aSize = 0);

//     /// @brief Create an Array containing items copied from source with index starting at aOrigin.
//     ///
//     /// @param aOrigin The starting index of the Array.
//     /// @param source The source list to copy from.
//     Array(int aOrigin, List *source);

//     /// @brief Destructor.
//     ~Array();

//     virtual QString toString( Datum::ToStringFlags flags = Datum::ToStringFlags_None, int printDepthLimit = -1, int printWidthLimit = -1, VisitedSet *visited = nullptr) override;

//     /// @brief The starting index of this Array.
//     int origin = 1;
// };

// /// @brief The general container for data. The QLogo List is implemented as a linked list.
// ///
// /// @details The List class is a linked list of DatumPtrs. The head of the list must contain a
// /// DatumPtr to a Word, List, or Array. The tail of the list is a DatumPtr which must point to
// /// a List or EmptyList.
// class List : public Datum
// {
//     friend class ListIterator;
//     friend class Compiler;

//   public:
//     /// @brief The head of the list, also called the 'element'.
//     ///
//     /// @details The head of the list.
//     DatumPtr head;

//     /// @brief The remainder of the list after the head. Must be either List or EmptyList.
//     DatumPtr tail;

//     /// @brief The time, as returned by QDateTime::currentMSecsSinceEpoch().
//     ///
//     /// @details This is used to determine if the list needs to be reparsed.
//     /// Set when the most recent ASTList is generated from this list. Reset this
//     /// to zero when the list is modified to trigger reparsing, if needed.
//     ///
//     /// @todo It's difficult to know when the list is modified. We should consider
//     /// removing this.
//     qint64 astParseTimeStamp;

//     /// @brief Create a new list by attaching item as the head of srcList.
//     ///
//     /// @param item The item to add to the head of the list.
//     /// @param srcList The list to copy from, this will become the tail of the new list.
//     List(DatumPtr item, List *srcList);

//     /// @brief Destructor.
//     virtual ~List();

//     virtual QString toString( ToStringFlags flags = ToStringFlags_None, int printDepthLimit = -1, int printWidthLimit = -1, VisitedSet *visited = nullptr) override;

//     /// @brief Empty the List
//     void clear();

//     /// @brief Returns the count of elements in the List.
//     ///
//     /// @details This should only be used when you need a specfic number since it traverses
//     /// the list. Consider using isEmpty().
//     ///
//     /// @return The count of elements in the List.
//     int count();

//     /// @brief Returns the element pointed to by anIndex.
//     ///
//     /// @param anIndex The index of the element to return.
//     /// @return The element at the given index, starting at 1. Ensure that the index is
//     /// within the bounds of the list. Triggers an error if the index is out of bounds.
//     DatumPtr itemAtIndex(int anIndex);

//     /// @brief Returns true if this is an empty list.
//     ///
//     /// @return True if this is an empty list, false otherwise.
//     bool isEmpty();

//     /// @brief Replaces everything but the first item in the List with aValue.
//     ///
//     /// @param aValue The value to replace the first item with.
//     void setButfirstItem(DatumPtr aValue);

//     /// @brief Create a new ListIterator pointing to the head of the List.
//     /// @return A new ListIterator pointing to the head of the List.
//     ListIterator newIterator();
// };

// /// @brief A singleton class that represents an empty list.
// ///
// /// @details EmptyList is an immutable singleton. There can only be one instance
// /// of EmptyList in the entire program. All empty lists should reference this
// /// single instance. The list cannot be modified after creation.
// class EmptyList : public List
// {
//   private:
//     /// @brief Private constructor to enforce singleton pattern.
//     EmptyList();

//     /// @brief Deleted copy constructor to prevent copying.
//     EmptyList(const EmptyList &) = delete;

//     /// @brief Deleted assignment operator to prevent assignment.
//     EmptyList &operator=(const EmptyList &) = delete;

//     /// @brief Deleted move constructor to prevent moving.
//     EmptyList(EmptyList &&) = delete;

//     /// @brief Deleted move assignment operator to prevent move assignment.
//     EmptyList &operator=(EmptyList &&) = delete;

//     /// @brief Static instance pointer.
//     static EmptyList *instance_;

    
//     void clear();
//     void setButfirstItem(DatumPtr aValue);

//   public:
//     /// @brief Get the singleton instance of EmptyList.
//     ///
//     /// @return A pointer to the singleton EmptyList instance.
//     static EmptyList *instance();

//     virtual QString toString( ToStringFlags flags = ToStringFlags_None, int printDepthLimit = -1, int printWidthLimit = -1, VisitedSet *visited = nullptr) override;
// };

// /// @brief A class that simplifies iterating through a list.
// ///
// /// @details This class is used to iterate through a list. Unlike the c++ STL iterator,
// /// this class is very simple. There are only two methods: element() and elementExists().
// /// The element() method returns the current element and advances the iterator to the
// /// next element. The elementExists() method returns true if the iterator is pointing to
// /// a valid element.
// class ListIterator
// {
//   protected:
//     DatumPtr ptr;

//   public:
//     /// @brief Create an empty ListIterator.
//     ListIterator();

//     /// @brief Create a new ListIterator pointing to the head of the List.
//     ListIterator(DatumPtr aList);

//     /// @brief Return the element at the current location. Advance Iterator to the next location.
//     ///
//     /// @return The element at the current location.
//     DatumPtr element();

//     /// @brief Returns true if pointer references a valid element.
//     bool elementExists();
// };


// /// @brief A class that allows quickly building a list.
// ///
// /// @details This class is used to build a list quickly by appending elements to the end of the list.
// /// @note This class should only be used internally within the qlogo binary. It should not be available to the user.
// class ListBuilder
// {
//   private:
//     DatumPtr finishedList_;

// public:
//     List *firstNode;
//     List *lastNode;

//     ListBuilder() : firstNode(EmptyList::instance()), lastNode(EmptyList::instance()), finishedList_(EmptyList::instance())
//     {}

//     /// @brief Append an element to the end of the list.
//     /// @param element The element to append to the end of the list.
//     void append(DatumPtr element)
//     {
//         List *newList = new List(element, EmptyList::instance());
//         if (firstNode == EmptyList::instance())
//         {
//             firstNode = newList;
//             lastNode = newList;
//             finishedList_ = DatumPtr(firstNode);
//         } else {
//             lastNode->tail = DatumPtr(newList);
//             lastNode = newList;
//         }
//     }

//     /// @brief Return the finished list.
//     /// @return The finished list.
//     DatumPtr finishedList() const
//     {
//         return finishedList_;
//     }
// };


// /// @brief A pointer to the singleton Datum instance.
// extern DatumPtr nothing;

// /// @brief A pointer to an empty list.
// extern DatumPtr emptyList;

#endif // DATUM_H
