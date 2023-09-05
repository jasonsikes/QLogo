#ifndef DATUM_DATUMP_H
#define DATUM_DATUMP_H

//===-- qlogo/datum_datump.h - Datump class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Datump class, a reference-counting
/// pointer to a Datum.
///
//===----------------------------------------------------------------------===//



#include "datum.h"


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

    /// Convenience constructor for numbers.
    explicit DatumP(double n);

    /// Convenience constructor for integers.
    explicit DatumP(int n);

    /// Convenience constructor for strings.
    explicit DatumP(const QString n);



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


/// A pointer to notADatum, like NULL.
extern DatumP nothing;


#endif // DATUM_DATUMP_H
