#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H

// Qt #defines "emit". llvm uses "emit" as a function name.
#ifdef emit
#undef emit
#endif

#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/IR/Value.h>

typedef uint64_t *addr_t;
class Datum;
class DatumPtr;
class Compiler;

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

// Compiled function signature
typedef Datum *(*CompiledFunctionPtr)(addr_t, int32_t);

/// Signature of method that generates IR code for a given node.
typedef llvm::Value *(Compiler::*Generator)(const DatumPtr &, RequestReturnType);

// The information to store the generated function and to destroy later
struct CompiledText
{
    llvm::orc::ResourceTrackerSP rt;

    CompiledFunctionPtr functionPtr = nullptr;

    ~CompiledText();
};

#endif // COMPILER_TYPES_H