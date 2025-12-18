#ifndef DATUM_PTR_H
#define DATUM_PTR_H

#include "datum_core.h"
#include <QString>
#include <QDebug>

class Procedure;
class ASTNode;
class FlowControl;
class FCError;
class VisitedSet;


/// @brief A smart pointer to a Datum.
///
/// @details This class is a smart pointer to a Datum. It incorporates convenience
/// methods, reference-counting, and automatic destruction of the referred datum.
class DatumPtr
{
  protected:
    Datum *d;

    void destroy();

  public:
    /// Copy constructor. Increases retain count of the referred object.
    DatumPtr(const DatumPtr &other) noexcept;

    /// Default constructor. Creates a pointer to the singleton Datum instance.
    DatumPtr();

    /// @brief Creates a pointer to a Datum object. Begins reference counting.
    ///
    /// Creates a pointer to the specified Datum object and increases its retain count.
    /// The referred object will be destroyed when the last object referring to it
    /// is destroyed.
    DatumPtr(Datum *);

    /// @brief Destructor.
    ///
    /// Decreases the retain count of the referred object. If this is the last
    /// pointer to the referred object (if its retain count reaches zero) the
    /// object is destroyed.
    ~DatumPtr();

    /// @brief Convenience constructor for "true" and "false".
    ///
    /// @param b The boolean value to create the DatumPtr for.
    /// @return A new DatumPtr pointing to a new Word containing the boolean value.
    explicit DatumPtr(bool b);

    /// @brief Convenience constructor for numbers.
    ///
    /// @param n The number to create the DatumPtr for.
    /// @return A new DatumPtr pointing to a new Word containing the number.
    explicit DatumPtr(double n);

    /// @brief Convenience constructor for integers.
    ///
    /// @param n The integer to create the DatumPtr for.
    /// @return A new DatumPtr pointing to a new Word containing the integer.
    explicit DatumPtr(int n);

    /// @brief Convenience constructor for strings.
    ///
    /// @param n The string to create the DatumPtr for.
    /// @param isVBarred Whether the string is created with vertical bars.
    /// @return A new DatumPtr pointing to a new Word containing the string.
    explicit DatumPtr(QString n, bool isVBarred = false);

    /// @brief Convenience constructor for const char strings.
    ///
    /// @param n The string to create the DatumPtr for.
    /// @return A new DatumPtr pointing to a new Word containing the string.
    explicit DatumPtr(const char *n);

    /// @brief Returns a pointer to the referred Datum or any of Datum's subclasses.
    ///
    /// @return A pointer to the referred Datum or any of Datum's subclasses.
    Datum *datumValue() const
    {
        return d;
    }

    /// @brief Returns a pointer to the referred Datum as a Word.
    ///
    /// @return A pointer to the referred Datum as a Word.
    Word *wordValue() const;

    /// @brief Returns a pointer to the referred Datum as a List.
    ///
    /// @return A pointer to the referred Datum as a List.
    List *listValue() const;

    /// @brief Returns a pointer to the referred Datum as a Procedure.
    ///
    /// @return A pointer to the referred Datum as a Procedure.
    Procedure *procedureValue() const;

    /// @brief Returns a pointer to the referred Datum as an ASTNode.
    ///
    /// @return A pointer to the referred Datum as an ASTNode.
    ASTNode *astnodeValue() const;

    /// @brief Returns a pointer to the referred Datum as an Array.
    ///
    /// @return A pointer to the referred Datum as an Array.
    Array *arrayValue() const;

    /// @brief Returns a pointer to the referred Datum as a FlowControl.
    ///
    /// @return A pointer to the referred Datum as a FlowControl.
    FlowControl *flowControlValue() const;

    /// @brief Returns a pointer to the referred Datum as an Err.
    ///
    /// @return A pointer to the referred Datum as an Err.
    FCError *errValue() const;

    /// @brief Returns true if the referred Datum is a Word, false otherwise.
    ///
    /// @return True if the referred Datum is a Word, false otherwise.
    bool isWord() const
    {
        return d->isa == Datum::typeWord;
    }

    /// @brief Returns true if the referred Datum is a List, false otherwise.
    ///
    /// @return True if the referred Datum is a List, false otherwise.
    bool isList() const
    {
        return (d->isa & Datum::typeList) != 0;
    }

    /// @brief Returns true if the referred Datum is an ASTNode, false otherwise.
    ///
    /// @return True if the referred Datum is an ASTNode, false otherwise.
    bool isASTNode() const
    {
        return d->isa == Datum::typeASTNode;
    }

    /// @brief Returns true if the referred Datum is an Array, false otherwise.
    ///
    /// @return True if the referred Datum is an Array, false otherwise.
    bool isArray() const
    {
        return d->isa == Datum::typeArray;
    }


    /// @brief Returns true if the referred Datum is an Err, false otherwise.
    ///
    /// @return True if the referred Datum is an Err, false otherwise.
    bool isErr() const
    {
        return d->isa == Datum::typeError;
    }


    /// @brief Returns true if the referred Datum is the singleton Datum instance, false otherwise.
    ///
    /// @return True if the referred Datum is the singleton Datum instance, false otherwise.
    bool isNothing() const {
        return d == Datum::getInstance();
    }

    /// @brief Returns true if the referred Datum is a FlowControl, false otherwise.
    ///
    /// @return True if the referred Datum is a FlowControl, false otherwise.
    bool isFlowControl() const
    {
        return (d->isa & Datum::typeFlowControlMask) != 0;
    }


    /// @brief Reassign the pointer to refer to the other object.
    ///
    /// @param other The DatumPtr to assign to this.
    /// @return A reference to this.
    DatumPtr &operator=(const DatumPtr &other) noexcept;

    /// @brief Return true if and only if other points to the same object as this.
    ///
    /// @param other The DatumPtr to compare to this.
    /// @return True if and only if other points to the same object as this.
    bool operator==(const DatumPtr &other) const;

    /// @brief Return true if and only if other does not point to the same object as this.
    ///
    /// @param other The DatumPtr to compare to this.
    /// @return True if and only if other does not point to the same object as this.
    bool operator!=(const DatumPtr &other) const;

    /// @brief Return a string representation of the Datum.
    /// @param flags Flags to control the output. See ToStringFlags for possible values.
    /// @param printDepthLimit Limit the depth of sublists or arrays for readability.
    /// printDepthLimit = 1 means don't show sublists or arrays.
    /// printDepthLimit = 2 means show sublists or arrays, but don't show sublist's sublist or array's subarray.
    /// printDepthLimit = 0 means show '...' instead of THIS list or array.
    /// printDepthLimit = -1 means show all sublists or arrays.
    /// @param printWidthLimit Limit the length of a string or list or array for readability.
    /// @param visited Set of visited nodes to prevent cycles.
    /// @return A string representation of the Datum.
    QString toString( Datum::ToStringFlags flags = Datum::ToStringFlags_None, int printDepthLimit = -1, int printWidthLimit = -1, VisitedSet *visited = nullptr) const;

    /// @brief returns the DatumType of the referenced object.
    ///
    /// @return The DatumType of the referenced object.
    Datum::DatumType isa() const
    {
        return d->isa;
    }

    /// @brief Set a mark on the datum so that debug message will print when datum is
    /// destroyed.
    ///
    /// @details This is used to help debug memory leaks. The MARK command is used to set
    /// a mark on the datum so that a debug message will be printed when the datum is destroyed.
    void alertOnDelete()
    {
        qDebug() << "MARKED: " << d << " " << d->toString(Datum::ToStringFlags_Show);
        d->alertOnDelete = true;
    }
};

Q_DECLARE_TYPEINFO(DatumPtr, Q_RELOCATABLE_TYPE);

/// @brief A pointer to the singleton Datum instance.
extern DatumPtr nothing;

/// @brief A pointer to an empty list.
extern DatumPtr emptyList;

#endif // DATUM_PTR_H