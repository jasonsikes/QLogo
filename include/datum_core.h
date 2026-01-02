#ifndef DATUM_CORE_H
#define DATUM_CORE_H

#include <QString>

class VisitedSet;
class Word;
class List;
class Array;

/// @brief The unit of data for QLogo. The base class for Word, List, Array, ASTNode, etc.
class Datum
{
    friend class ListIterator;
    friend class DatumPtr;
    friend struct Evaluator;

  protected:
    /// @brief Protected constructor to prevent direct instantiation.
    ///
    /// @details The Datum class uses the singleton pattern. Only one instance
    /// of Datum can exist (accessed via getInstance()). Subclasses can still
    /// be instantiated multiple times because they can call this deleted constructor.
    Datum();

  public:
    Datum &operator=(const Datum &) = delete;
    Datum &operator=(Datum &&) = delete;
    Datum &operator=(Datum *) = delete;
    Datum(const Datum &) = delete;
    Datum(Datum &&) = delete;

    /// @brief Value stored in isa.
    enum DatumType : uint32_t
    {
        // These are the three data types that are made available to the user.
        typeWord = 0x00000001,
        typeList = 0x00000002,
        typeArray = 0x00000004,
        typeEmptyList = 0x00010002,      // Singleton instance of EmptyList
        typeDataMask = 0x00000007,       // Word + List + Array
        typeWordOrListMask = 0x00000003, // Word + List
        // These are the types that control the flow of the program.
        typeError = 0x00000010,
        typeGoto = 0x00000020,
        typeContinuation = 0x00000040,
        typeReturn = 0x00000080,
        typeFlowControlMask = 0x000000F0,
        // These are the types that are used internally by QLogo.
        typeNothing = 0x00000100,
        typeASTNode = 0x00000200,
        typeProcedure = 0x00000400,
        typeNothingPersistent = 0x00010100, // Singleton instance of Nothing
        typeUnboundMask = 0x00000300,       // typeASTNode + typeNothing
        typePersistentMask = 0x00010000,    // OR this value to prevent the datum from being destroyed
    };

    DatumType isa; // Subclasses must set this to a valid value.

    int retainCount;

    /// @brief If set to 'true', DatumPtr will send qDebug message when this is deleted.
    bool alertOnDelete = false;

    /// @brief Get the singleton instance of Datum.
    ///
    /// @details Returns the single instance of Datum. This instance represents
    /// "nothing" (similar to nullptr). Subclasses like Word, List, Array, etc.
    /// can still be instantiated multiple times.
    ///
    /// @return A pointer to the singleton Datum instance.
    static Datum *getInstance();

    /// @brief Destructor.
    virtual ~Datum();

    /// @brief This enum specifies flags that can be used to affect various aspects
    /// of the string representation of the Datum.
    enum ToStringFlags : int
    {
        ToStringFlags_None = 0x00,
        ToStringFlags_FullPrint = 0x01, // Show backslashes and vertical bars in words
        ToStringFlags_Show = 0x02,      // Show list brackets
        ToStringFlags_Source = 0x04,    // Format for parsing as qlogo source code
        ToStringFlags_Key = 0x08,       // Format for use as a key in a map
        ToStringFlags_Raw = 0x10,       // Format for use as a raw string (no special decoding of mapped characters)
    };

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
    virtual QString toString(ToStringFlags flags = ToStringFlags_None,
                             int printDepthLimit = -1,
                             int printWidthLimit = -1,
                             VisitedSet *visited = nullptr) const;

    /// @brief Returns true if the referred Datum is a List, false otherwise.
    ///
    /// @return True if the referred Datum is a List, false otherwise.
    bool isList() const
    {
        return (isa & Datum::typeList) != 0;
    }

    /// @brief Returns true if the referred Datum is an Array, false otherwise.
    ///
    /// @return True if the referred Datum is an Array, false otherwise.
    bool isArray() const
    {
        return (isa & Datum::typeArray) != 0;
    }

    /// @brief Returns true if the referred Datum is a Word, false otherwise.
    ///
    /// @return True if the referred Datum is a Word, false otherwise.
    bool isWord() const
    {
        return (isa & Datum::typeWord) != 0;
    }

    /// @brief Performs an assertion check that the referred Datum is a Word. Returns a pointer to the referred Datum as
    /// a Word.
    ///
    /// @return A pointer to the referred Datum as a Word.
    Word *wordValue() const
    {
        Q_ASSERT(isWord());
        return reinterpret_cast<Word *>(const_cast<Datum *>(this));
    }

    /// @brief Performs an assertion check that the referred Datum is a List. Returns a pointer to the referred Datum as
    /// a List.
    ///
    /// @return A pointer to the referred Datum as a List.
    List *listValue() const
    {
        Q_ASSERT(isList());
        return reinterpret_cast<List *>(const_cast<Datum *>(this));
    }

    /// @brief Performs an assertion check that the referred Datum is an Array. Returns a pointer to the referred Datum
    /// as an Array.
    ///
    /// @return A pointer to the referred Datum as an Array.
    Array *arrayValue() const
    {
        Q_ASSERT(isArray());
        return reinterpret_cast<Array *>(const_cast<Datum *>(this));
    }
};

#endif // DATUM_CORE_H