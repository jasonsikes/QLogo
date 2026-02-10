#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H

// Qt #defines "emit". llvm uses "emit" as a function name.
#ifdef emit
#undef emit
#endif

#include "datum_ptr.h"
#include <QList>

// llvm #defines "emit". Qt uses "emit" as a function name.
#ifdef emit
#undef emit
#endif

#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/IR/Value.h>

typedef uint64_t *addr_t;
class Compiler;

/// @brief Storage for a suspended coroutine's state.
///
/// When the compiled procedure suspends, it returns a handle (pointer) to
/// a CoroutineState. When it completes, it returns nullptr (no state).
/// blockId (the compiled function's third parameter) is only used for initial
/// entry and GO/tag; on resume the coroutine continues from this state, not
/// from blockId. Extend with evaluator/frame refs, etc., when adding suspend.
struct CoroutineState
{
    /// Resume point used when the coroutine is re-entered (e.g. switch index
    /// in the compiled body). Distinct from blockId; -1 when invalid.
    int32_t resumePoint = -1;
};

/// @brief Handle returned by the compiled function. nullptr means "completed".
typedef CoroutineState *coroutine_handle_t;

/// @brief Expression generator request type.
///
/// Request that the generator generate code that produces this output type.
enum RequestReturnType : int
{
    RequestReturnVoid = 0x00,
    RequestReturnNothing = 0x01,
    RequestReturnN = 0x01,    // N
    RequestReturnBool = 0x02, // B
    RequestReturnB = 0x02,
    RequestReturnBN = 0x03,
    RequestReturnDatum = 0x04, // D
    RequestReturnD = 0x04,
    RequestReturnDN = 0x05,
    RequestReturnDB = 0x06,
    RequestReturnDBN = 0x07,
    RequestReturnReal = 0x08, // R
    RequestReturnR = 0x08,
    RequestReturnRN = 0x09,
    RequestReturnRB = 0x0A,
    RequestReturnRBN = 0x0B,
    RequestReturnRD = 0x0C,
    RequestReturnRDN = 0x0D,
    RequestReturnRDB = 0x0E,
    RequestReturnRDBN = 0x0F,
};

// Compiled function signature: returns coroutine handle (nullptr = completed).
typedef coroutine_handle_t (*CompiledFunctionPtr)(addr_t, addr_t, int32_t);

/// Signature of method that generates IR code for a given node.
typedef llvm::Value *(Compiler::*Generator)(const DatumPtr &, RequestReturnType);

// The information to store the generated function and to destroy later
struct CompiledText
{
    llvm::orc::ResourceTrackerSP rt;

    CompiledFunctionPtr functionPtr = nullptr;

    // The AST list from which the compiled function was generated.
    // This is stored to ensure the AST nodes are kept alive as long as the compiled text exists.
    QList<QList<DatumPtr>> astList;

    Compiler *compiler = nullptr;

    ~CompiledText();
};

#endif // COMPILER_TYPES_H