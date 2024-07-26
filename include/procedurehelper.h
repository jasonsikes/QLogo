#ifndef PROCEDUREHELPER_H
#define PROCEDUREHELPER_H

//===-- qlogo/procedurehelper.h - ProcedureHelper class definition -------*- C++
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
/// This file contains the declaration of the ProcedureHelper class, which
/// provides the functionality required by QLogo primative functions
///
//===----------------------------------------------------------------------===//

#include "datum.h"
#include <QVector>
#include <functional>

class Kernel;
class Parser;
class Procedures;

/// @brief A validator function for a procedure parameter.
/// @param aVal The value to validate.
/// @return True if the value is valid, false otherwise.
typedef std::function<bool(DatumPtr)> validatorP;

/// @brief A validator function for a procedure parameter.
/// @param aVal The value to validate.
/// @return True if the value is valid, false otherwise.
typedef std::function<bool(double)> validatorD;

/// @brief A validator function for a procedure parameter.
/// @param aVal The value to validate.
/// @return True if the value is valid, false otherwise.
typedef std::function<bool(int)> validatorI;

/// @brief A validator function for a procedure parameter.
/// @param aVal The value to validate.
/// @return True if the value is valid, false otherwise.
typedef std::function<bool(bool)> validatorB;

/// @brief A validator function for a procedure parameter.
/// @param aVal The value to validate.
/// @return True if the value is valid, false otherwise.
typedef std::function<bool(List *)> validatorL;

/// @brief The ProcedureHelper class provides the functionality required by QLogo
/// primative functions.
///
/// This class is responsible for computing, validating, and handling procedure
/// parameters. It also provides utility functions for tracing parameters and
/// return values.
class ProcedureHelper
{
    ASTNode *node;
    Kernel *parent;
    QVector<DatumPtr> parameters;
    DatumPtr returnValue;

  public:
    /// @brief Whether the procedure is being traced.
    bool isTraced;

    /// @brief Outputs a string of spaces based on the indentation level of the
    /// procedure.
    /// @return A string of spaces based on the indentation level of the
    /// procedure.
    QString indent();

    /// @brief Constructs a new ProcedureHelper. Performs child node execution and
    /// debugging operations.
    /// @param aParent The parent kernel.
    /// @param sourceNode The source node.
    /// @note This constructor performs all common operations required by
    /// QLogo primative functions.
    ProcedureHelper(Kernel *aParent, DatumPtr sourceNode);

    /// @brief Destructs the ProcedureHelper and performs any debugging
    /// operations required by QLogo primative functions.
    ~ProcedureHelper();

    /// @brief Returns the number of children of the procedure.
    /// @return The number of children of the procedure.
    int countOfChildren()
    {
        return parameters.size();
    }

    /// @brief Returns a validated datum at a given child index.
    /// @param index The index of the child datum to return.
    /// @param v The validator function.
    /// @return The validated datum.
    /// @note This function is used to get the datum at a given child index.
    /// Performs the validation function on the datum. Throws an error if the
    /// validation function returns false.
    DatumPtr validatedDatumAtIndex(int index, validatorP v);

    /// @brief Returns a datum at a given child index.
    /// @param index The index of the child datum to return.
    /// @param canRunlist Whether to run the list function if the datum is a list.
    /// @return The datum at the given index.
    DatumPtr datumAtIndex(int index, bool canRunlist = false);

    /// @brief Returns a word at a given child index.
    /// @param index The index of the child to return.
    /// @param canRunlist Whether to run the list function if the datum is a list.
    /// @return The word at the given index.
    /// @note This function is used to get the word at a given index. If the
    /// datum at the index is a list, the list is executed. Throws an error if the
    /// resulting datum is not a word.
    DatumPtr wordAtIndex(int index, bool canRunlist = false);

    /// @brief Returns a list at a given child index.
    /// @param index The index of the child to return.
    /// @return The list at the given index.
    /// @note Ensures the datum at the given index is a list. Throws an error if
    /// the datum is not a list.
    DatumPtr listAtIndex(int index);

    /// @brief Returns a validated list at a given child index.
    /// @param index The index of the child to validate.
    /// @param v The validator function.
    /// @return The validated list.
    /// @note Performs the validation function on the list. Throws an error if
    /// the validation function returns false.
    DatumPtr validatedListAtIndex(int index, validatorL v);

    /// @brief Returns an array at a given child index.
    /// @param index The index of the child to return.
    /// @return The array at the given index.
    /// @note Ensures the datum at the given index is an array. Throws an error if
    /// the datum is not an array.
    DatumPtr arrayAtIndex(int index);

    /// @brief Returns a number at a given child index.
    /// @param index The index of the child to return.
    /// @param canRunList Whether to run the list function if the datum is a list.
    /// @return The number at the given index.
    /// @note Ensures the datum at the given index is a number. Throws an error if
    /// the datum is not a number.
    double numberAtIndex(int index, bool canRunList = false);

    /// @brief Returns a validated number at a given child index.
    /// @param index The index of the child to validate.
    /// @param v The validator function.
    /// @param canRunList Whether to run the list function if the datum is a list.
    /// @return The validated number.
    /// @note Performs the validation function on the number. Throws an error if
    /// the validation function returns false.
    double validatedNumberAtIndex(int index, validatorD v, bool canRunList = false);

    /// @brief Returns an integer at a given child index.
    /// @param index The index of the child to return.
    /// @return The integer at the given index.
    /// @note Ensures the datum at the given index is an integer. Throws an error
    /// if the datum is not an integer.
    int integerAtIndex(int index);

    /// @brief Returns a validated integer at a given child index.
    /// @param index The index of the child to validate.
    /// @param v The validator function.
    /// @return The validated integer.
    /// @note Performs the validation function on the integer. Throws an error if
    /// the validation function returns false.
    int validatedIntegerAtIndex(int index, validatorI v);

    /// @brief Returns a boolean at a given child index.
    /// @param index The index of the child to return.
    /// @param canRunList Whether to run the list function if the datum is a list.
    /// @return The boolean at the given index.
    /// @note Ensures the resulting datum is a boolean. Throws an error if the
    /// datum is not a boolean.
    bool boolAtIndex(int index, bool canRunList = false);

    /// @brief Throws an error indicating that a datum at a given child index is
    /// invalid.
    /// @param index The index of the child to reject.
    /// @param allowErract Whether to allow the user to perform debugging at the
    /// state the procedure is in when the datum is rejected.
    /// @param allowRecovery Whether to allow the user to provide a replacement
    /// datum and continue execution after debugging. Requires allowErract to be
    /// true.
    /// @return The replacement datum if allowRecovery is true and the user
    /// provided a replacement datum. Otherwise, throws an error.
    DatumPtr reject(int index, bool allowErract = false, bool allowRecovery = false);

    /// @brief Throws an error indicating that a datum is invalid.
    /// @param value The datum to reject.
    /// @param allowErract Whether to allow the user to perform debugging at the
    /// state the procedure is in when the datum is rejected.
    /// @param allowRecovery Whether to allow the user to provide a replacement
    /// datum and continue execution after debugging. Requires allowErract to be
    /// true.
    /// @return The replacement datum if allowRecovery is true and the user
    /// provided a replacement datum. Otherwise, throws an error.
    DatumPtr reject(DatumPtr value, bool allowErract = false, bool allowRecovery = false);

    /// @brief Returns a datum containing a word containing a number.
    /// @param aVal The integer to return.
    /// @return The datum to the caller.
    /// @note This is a convenience function for returning an integer to a parent
    /// node. It creates a new DatumPtr containing a word containing a number. It
    /// registers the datum as the return value of the procedure for debugging
    /// purposes.
    DatumPtr ret(int aVal);

    /// @brief Returns a datum containing a word containing a number.
    /// @param aVal The number to return.
    /// @return The datum to the caller.
    /// @note This is a convenience function for returning a double to a parent
    /// node. It creates a new DatumPtr containing a word containing a number. It
    /// registers the datum as the return value of the procedure for debugging
    /// purposes.
    DatumPtr ret(double aVal);

    /// @brief Returns a datum containing a string.
    /// @param aVal The string to return.
    /// @return The datum to the caller.
    /// @note This is a convenience function for returning a string to a parent
    /// node. It creates a new DatumPtr containing a word containing a string. It
    /// registers the datum as the return value of the procedure for debugging
    /// purposes.
    DatumPtr ret(QString aVal);

    /// @brief Returns a datum.
    /// @param aVal The datum to return.
    /// @return The datum to the caller.
    /// @note This is a convenience function for returning a datum to a parent
    /// node. It creates a new DatumPtr containing a datum. It registers the datum
    /// as the return value of the procedure for debugging purposes.
    DatumPtr ret(Datum *aVal);

    /// @brief Returns a datum.
    /// @param aVal The datum to return.
    /// @return The datum to the caller.
    /// @note This is a convenience function for returning a datum to a parent
    /// node. It registers the datum as the return value of the procedure for
    /// debugging purposes.
    DatumPtr ret(DatumPtr aVal);

    /// @brief Returns a datum containing a boolean.
    /// @param aVal The boolean to return.
    /// @return The datum to the caller.
    /// @note This is a convenience function for returning a boolean to a parent
    /// node. It creates a new DatumPtr containing a boolean. It registers the
    /// datum as the return value of the procedure for debugging purposes.
    DatumPtr ret(bool aVal);

    /// @brief Returns a datum containing 'nothing'.
    /// @return The datum to the caller.
    /// @note This is a convenience function for returning 'nothing' to a parent
    /// node. It registers 'nothing' as the return value of the procedure for
    /// debugging purposes.
    DatumPtr ret(void);

    /// @brief Sets the isErroring flag.
    /// @param aIsErroring The new value of the isErroring flag.
    /// @note When set to true, the destructor will not write debug messages when
    /// deconstructing.
    static void setIsErroring(bool aIsErroring);
};

#endif // PROCEDUREHELPER_H
