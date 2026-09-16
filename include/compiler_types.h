#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H

// Qt #defines "emit". llvm uses "emit" as a function name.
#ifdef emit
#undef emit
#endif

#include "datum_ptr.h"
#include <QList>

// Qt #defines "emit". llvm uses "emit" as a function name.
#ifdef emit
#undef emit
#endif

#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/IR/Value.h>

typedef uint64_t *addr_t;

#ifdef _MSC_VER
class __single_inheritance Compiler;
#else
class Compiler;
#endif


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

/// @brief Bitmask of value kinds a node may produce or a parent may expect.
///
/// Bits: N (nothing/UNBOUND), B (bool), D (datum), R (real). Combinations
/// (e.g. DN, DB) mean more than one kind is allowed.
enum TypeMask : int
{
    TypeNone = 0x00,

    TypeN = 0x01, // nothing / UNBOUND
    TypeNothing = TypeN,
    TypeB = 0x02, // bool
    TypeBool = TypeB,
    TypeD = 0x04, // datum
    TypeDatum = TypeD,
    TypeR = 0x08, // real
    TypeReal = TypeR,

    TypeBN = TypeB | TypeN,
    TypeDN = TypeD | TypeN,
    TypeDB = TypeD | TypeB,
    TypeDBN = TypeD | TypeB | TypeN,
    TypeRN = TypeR | TypeN,
    TypeRB = TypeR | TypeB,
    TypeRBN = TypeR | TypeB | TypeN,
    TypeRD = TypeR | TypeD,
    TypeRDN = TypeR | TypeD | TypeN,
    TypeRDB = TypeR | TypeD | TypeB,
    TypeRDBN = TypeR | TypeD | TypeB | TypeN,
};

// Compiled function signature: returns coroutine handle (nullptr = completed).
// Fourth arg: resume handle; nullptr = initial call (use blockId), non-null = resume from that state.
typedef LLVMCoroFrameHeader *(*CompiledFunctionPtr)(addr_t, addr_t, int32_t, LLVMCoroFrameHeader *);

/// Signature of method that generates IR code for a given node.
typedef llvm::Value *(Compiler::*Generator)(const DatumPtr &, TypeMask);

/// The information to store the generated function and to destroy later
struct CompiledText
{
    llvm::orc::ResourceTrackerSP rt_;

    CompiledFunctionPtr functionPtr_ = nullptr;

    // The AST list from which the compiled function was generated.
    // This is stored to ensure the AST nodes are kept alive as long as the compiled text exists.
    QList<QList<DatumPtr>> astList_;

    Compiler *compiler_ = nullptr;

    ~CompiledText();
};

/// The four `APPLY` forms.
enum TemplateForm : int8_t
{
    TemplateFormInvalid = 0,
    TemplateFormExplicitSlot,   // [? * ?]
    TemplateFormNamedProcedure, // "sum
    TemplateFormLambda,         // [[x y] :x + :y]
    TemplateFormProcedureText,  // [[x y] [OUTPUT :x + :y]]
    TemplateFormCount
};

#endif // COMPILER_TYPES_H
