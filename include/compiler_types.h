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

/// @brief Storage for a suspended coroutine's state (custom switched-resume).
///
/// The handle is a pointer to this struct. When the compiled procedure
/// suspends, it returns a pointer to a CoroutineState. When it completes,
/// it returns nullptr (no state). blockId is only used for initial entry
/// and GO/tag; on resume the coroutine continues from this state.
///
/// Layout must match what the compiler emits when it allocates and fills
/// the state on suspend (and what it expects when re-entered with non-null
/// resumeHandle).
struct CoroutineState
{
    /// Switch index in the compiled body for where to continue. -1 when invalid.
    int32_t resumePoint = -1;

    /// Evaluator pointer; required when resuming so the procedure can run.
    addr_t evaluator = nullptr;

    /// Address where the procedure stores its return value; same as the
    /// compiled function's second parameter.
    addr_t returnValueAddress = nullptr;

    // Reserve space for spilled locals / more state as needed.
    // char reserved[...];
};

/// @brief Handle returned by the compiled function. nullptr means "completed".
typedef CoroutineState *coroutine_handle_t;

/// @brief LLVM coroutine frame header (when using LLVM coro-split).
///
/// When the JIT uses LLVM's coroutine pass, the handle points to an
/// LLVM-allocated frame whose first two words are resume and destroy.
/// Use this to call resume/destroy; the rest of the frame is opaque.
struct LLVMCoroFrameHeader
{
    void (*resume)(void *frame);
    void (*destroy)(void *frame);
};

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
// Fourth arg: resume handle; nullptr = initial call (use blockId), non-null = resume from that state.
typedef coroutine_handle_t (*CompiledFunctionPtr)(addr_t, addr_t, int32_t, coroutine_handle_t);

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