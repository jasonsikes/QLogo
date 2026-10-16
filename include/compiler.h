#ifndef COMPILER_H
#define COMPILER_H

//===-- qlogo/compiler.h - Compiler class definition -------*- C++ -*-===//
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
/// This file contains the declarations of the Compiler class, which
/// compiles an ASTList into LLVM JIT code.
///
//===----------------------------------------------------------------------===//

#include "compiler_types.h"
#include "datum_ptr.h"
#include "compiler_internal.h"
#include "workspace/jit_export_traits.h"

// Qt #defines "emit". llvm uses "emit" as a function name.
#ifdef emit
#undef emit
#endif

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/TargetParser/Triple.h"

#include <array>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

struct Scaffold;
class List;

template <std::size_t N>
class CallSite;

namespace llvm
{
class Value;
class AllocaInst;
class TargetMachine;
namespace orc
{
class LLJIT;
}
}; // namespace llvm

/// @brief A function that validates a value.
/// @param parent The parent ASTNode of the value to validate.
/// @param value The value to validate.
/// @return The result of a comparison operation, e.g. CreateICmpEQ().
/// @note The retval of this function will immediately be passed to CreateCondBr().
typedef std::function<llvm::Value *(llvm::Value *)> validatorFunction;

class Compiler
{
  public:
    /// @brief Per-compile IR/CFG dump request.
    struct DumpRequest
    {
        bool showIR = false;
        bool showCFG = false;
        bool showModuleIR = false;

        bool any() const { return showIR || showCFG || showModuleIR; }
    };

  private:
    template <std::size_t N>
    friend class CallSite;

    // Scaffold is a collection of objects needed to compile a function (JIT module).
    // Null when no compile is in progress. Nested compiles save and restore this.
    Scaffold *scaff_ = nullptr;

    friend std::string dbgName(const char *enclosing, const char *name);

    std::unique_ptr<llvm::TargetMachine> targetMachine_;
    std::unique_ptr<llvm::orc::LLJIT> lljit_;

    // Weak refs so shutdown can release ResourceTrackers while LLJIT is still alive.
    static std::vector<std::weak_ptr<CompiledText>> liveCompiledTexts_;

    static void trackCompiledText(const std::shared_ptr<CompiledText> &compiledText);

    /// @brief Procedure-body suffix whose head is the line currently being compiled.
    /// @details Used when registering TAG locations so GOTO can jump to the correct line.
    /// Must be set by the caller of compiledTextFromList() when compiling a procedure line.
    DatumPtr tagLineLocation_;

    /// @brief DEFINE/COPYDEF/ERASE and TRACE/UNTRACE. Cached JIT older than this is dropped.
    qint64 lastCodeChangeTimestamp_ = 0;

    /// @brief Which dumps to emit for the current compile (IR, CFG, module IR).
    /// @details Nested compiles (library autoload during treeify, RUN inside a
    /// dump command) must save and restore via DumpScope so they do not inherit
    /// the outer request.
    DumpRequest dumpRequest_;

    // Child node generation.

    // Generate code for all children of the given node and cast them to the expected type.
    std::vector<llvm::Value *> generateChildren(ASTNode *node, TypeMask expected);

    // Generate code for all children of the given node and cast each to its expected type.
    std::vector<llvm::Value *> generateChildren(ASTNode *node, std::vector<TypeMask> expectedAry);

    // Generate code for all children of the given node and cast them to the expected type.
    llvm::AllocaInst *generateChildrenAlloca(ASTNode *node, TypeMask expected, const std::string &name = "");

    // Generate code to save a vector of values to an alloca array.
    llvm::AllocaInst *generateAllocaAry(const std::vector<llvm::Value *> &values, const std::string &name = "");

    // Box an LLVM double/bool/datum as a Datum pointer for TRACE messages.
    llvm::Value *generateDatumForTrace(llvm::Value *value);

    // If the node's name is traced, emit stdTraceBegin with the evaluated inputs.
    void generateTraceBegin(ASTNode *node, const std::vector<llvm::Value *> &children);

    // If the node's name is traced, emit stdTraceEnd with the call result.
    void generateTraceEnd(ASTNode *node, llvm::Value *result);

    // Glue to cast child's produced type to the type the parent expects.
    llvm::Value *generateCast(llvm::Value *child, ASTNode *parent, const DatumPtr &, TypeMask expected);

    // Cast helpers: source type is known, emit code to produce expected.
    llvm::Value *generateCastFromDouble(llvm::Value *src, ASTNode *parent, TypeMask expected);
    llvm::Value *generateCastFromBool(llvm::Value *src, ASTNode *parent, TypeMask expected);
    llvm::Value *generateCastFromDatum(llvm::Value *src, ASTNode *parent, TypeMask expected);
    llvm::Value *generateCastFromNothing(llvm::Value *src, ASTNode *parent, TypeMask expected);
    llvm::Value *generateCastFromDN(llvm::Value *src, ASTNode *parent, TypeMask expected);

    // Generate code for a child node and cast it to the expected type.
    llvm::Value *generateChild(ASTNode *parent, const DatumPtr &, TypeMask expected);

    // Generate code for the child node at the given index and cast it to the expected type.
    llvm::Value *generateChild(ASTNode *parent, unsigned int index, TypeMask expected);

    // Generate a query to return a datum type (isa) of a given object.
    llvm::Value *generateGetDatumIsa(llvm::Value *objAddr);

    // Generate a child node in its native `produces_` kind. `expected` is what
    // the parent wants; generateCast converts after this returns.
    llvm::Value *generateChildOfNode(ASTNode *parent, const DatumPtr &, TypeMask expected);

    // Generate a void return value using the ASTNode to represent the source (for blame).
    llvm::Value *generateVoidRetval(const DatumPtr &node);

    // Ensure coroutine frame exists (create once per function if needed). Returns handle; use before
    // control flow that may take a path with no suspend so the handle is defined on all paths.
    llvm::Value *ensureCoroutineFrame();

    // llvm.coro.suspend + switch (0 continue, 1 cleanup, default suspend).
    // Leaves the builder in the continue block.
    void generateCoroSuspend();

    // Generate a call to execute a list.
    llvm::Value *generateCallList(llvm::Value *list);

    // Generate a call to execute a list continuation.
    llvm::Value *generateCallListContinuation(llvm::Value *list);

    // Named-procedure and procedure-text APPLY. builder is buildApplyNamedCall
    // or buildApplyProcedureText. NFC the builder result, then generateCallList
    // or Continuation from inSequenceTail_.
    llvm::Value *generateApplyBuiltCall(const DatumPtr &node, TypeMask expected,
                                        llvm::Value *aTemplate, llvm::Value *inputList,
                                        addr_t (*builder)(addr_t, addr_t, addr_t, addr_t),
                                        const char *builderName);

    // Generate explicit-slot APPLY. Insert point must be the form's entry block.
    // Pushes inputList, runs the template with generateCallList, then pops.
    // Always uses generateCallList (not a continuation) so the slot list can be
    // restored. Returns the list result, including flow control; genApply NFCs
    // after the merge so TRACE end is success-only.
    llvm::Value *generateApplyExplicitSlot(const DatumPtr &node, TypeMask expected,
                                           llvm::Value *aTemplate, llvm::Value *inputList);

    // Generate lambda APPLY. Insert point must be the form's entry block.
    // Binds template names to inputList, runs the template tail, then restores.
    // Always uses generateCallList (not a continuation) so bindings can be
    // restored. Returns the list result, including flow control; genApply NFCs
    // after the merge so TRACE end is success-only.
    llvm::Value *generateApplyLambda(const DatumPtr &node, TypeMask expected,
                                     llvm::Value *aTemplate, llvm::Value *inputList);

    // Generate a call to return a value immediately.
    llvm::Value *generateImmediateReturn(llvm::Value *retval);

    // Generate a call to construct error: SYSTEM
    llvm::Value *generateErrorSystem();

    // Generate a call to construct error: TOPLEVEL
    llvm::Value *generateErrorToplevel();

    // Generate a call to construct error: "X doesn't like Y as input"
    llvm::Value *generateErrorNoLike(ASTNode *who, llvm::Value *what);

    // Generate a call to construct error: "You don't say what to do with X"
    llvm::Value *generateErrorNoSay(llvm::Value *what);

    // Generate a call to construct error: "X without TEST"
    llvm::Value *generateErrorNoTest(llvm::Value *who);

    // Generate a call to construct error: "X didn't output to Y"
    llvm::Value *generateErrorNoOutput(llvm::Value *x, ASTNode *y);

    /// Run ERRACT for "X didn't output to Y". Errors and other flow control return
    /// immediately. A datum from ERRACT replaces the expression that did not output.
    llvm::Value *generateRecoverDidntOutput(llvm::Value *who, ASTNode *parent);

    // Generate a call to construct error: "X has no value"
    llvm::Value *generateErrorNoValue(llvm::Value *what);

    // Generate a call to construct error: "Graphics not initialized"
    llvm::Value *generateErrorNoGraphics();

    // Generate a call to construct error: "Not enough inputs to X"
    llvm::Value *generateErrorNotEnoughInputs(ASTNode *x);

    // Validated conversion generators

    llvm::Value *generateValidationDouble(ASTNode *parent, llvm::Value *val, const validatorFunction &validator);

    llvm::Value *generateValidationDatum(ASTNode *parent, llvm::Value *val, const validatorFunction &validator);

    // Generate a call to convert a double to a Word object
    llvm::Value *generateWordFromDouble(llvm::Value *val);

    // Generate a call to convert a boolean to a Word object
    llvm::Value *generateWordFromBool(llvm::Value *val);

    // Convert a Datum pointer to a number.
    // Emit return "doesn't like" error if conversion not possible.
    llvm::Value *generateDoubleFromDatum(ASTNode *parent, llvm::Value *src);

    // Emit i1: src is a finite integer in [lo, hi] inclusive (ordered fcmp).
    // Does not convert; llvm.trunc is used so the check itself cannot poison.
    llvm::Value *generateIsExactIntegerInRange(llvm::Value *src, double lo, double hi);

    // Emit i1: src is a finite integer that fits in signed or unsigned i32.
    llvm::Value *generateIsExactInt32(llvm::Value *src, bool isSigned);

    // Convert a double to an int32. Must be an exact integer that fits in
    // signed or unsigned i32. `extra`, if set, is ANDed into the ERRACT
    // predicate so extra constraints share one trip.
    llvm::Value *generateInt32FromDouble(ASTNode *parent, llvm::Value *src, bool isSigned,
                                        const validatorFunction &extra = {});

    // Shift an i32 by a signed count. Negative count shifts the other way.
    // |count| >= 32 saturates: shl/lshr → 0, ashr → sign fill. Never poison.
    llvm::Value *generateI32Shift(llvm::Value *value, llvm::Value *count, bool arithmetic);

    // Signed i32 remainder. INT_MIN % -1 is 0 (LLVM srem of that pair is poison).
    llvm::Value *generateI32SRem(llvm::Value *num, llvm::Value *denom);

    // Convert a Datum pointer to a boolean.
    // Emit return "doesn't like" error if conversion not possible.
    llvm::Value *generateBoolFromDatum(ASTNode *parent, llvm::Value *src);

    // If the given Datum is a list, execute it and return the result.
    // Otherwise, simply pass through the Datum.
    llvm::Value *generateListExecIfList(ASTNode *parent, llvm::Value *src);

    // Validate that given double is not zero.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateNotZeroFromDouble(ASTNode *parent, llvm::Value *src);

    // Validate that given double is greater than or equal to zero.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateNotNegativeFromDouble(ASTNode *parent, llvm::Value *src);

    // Convert a double to an int32 and validate that it is greater than or equal to zero.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateNotNegativeInt32FromDouble(ASTNode *parent, llvm::Value *src);

    // Convert a double to an int32 and validate that it is not equal to zero.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateNotZeroInt32FromDouble(ASTNode *parent, llvm::Value *src);

    // Validate that given double is greater than zero.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateGTZeroFromDouble(ASTNode *parent, llvm::Value *src);

    // Validate that Datum is Word.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateWordFromDatum(ASTNode *parent, llvm::Value *src);

    // Validate that Datum is List.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateListFromDatum(ASTNode *parent, llvm::Value *src);

    // Validate that Datum is Array.
    // Emit return "doesn't like" error if not.
    llvm::Value *generateArrayFromDatum(ASTNode *parent, llvm::Value *src);

    // Validate that Datum describes a color (palette index, RGB/RGBA list, name, or hex).
    // Run ERRACT if not; a usable color from it replaces the input.
    llvm::Value *generateColorFromDatum(ASTNode *parent, llvm::Value *src);

    // Palette index: exact integer at or above lowerLimit and below palette size (in C).
    // src is a double. Run ERRACT if not; a usable index from it replaces the input.
    llvm::Value *generateColorIndexFromDouble(ASTNode *parent, llvm::Value *src, double lowerLimit);

    // PENPAINT / PENERASE / PENREVERSE: pen down, then setPenMode.
    llvm::Value *generateSetPenMode(const DatumPtr &node, int32_t mode);

    // WRAP / WINDOW / FENCE.
    llvm::Value *generateSetTurtleMode(const DatumPtr &node, int32_t mode);

    // TEXTSCREEN / FULLSCREEN / SPLITSCREEN.
    llvm::Value *generateSetScreenMode(const DatumPtr &node, int32_t mode);

    // Common methodology for the generate*FromDatum() methods.
    llvm::Value *generateFromDatum(Datum::DatumType t, ASTNode *parent, llvm::Value *src);

    // Emit i1: src is a Word.
    llvm::Value *generateIsWord(llvm::Value *src);

    // Emit i1: src is an empty List.
    llvm::Value *generateIsEmptyList(llvm::Value *src);

    // Emit i1: src is a Word or an empty List.
    llvm::Value *generateIsWordOrEmptyList(llvm::Value *src);

    // Word, or []. Run ERRACT if not; a usable datum from it replaces the input.
    llvm::Value *generateWordOrEmptyListFromDatum(ASTNode *parent, llvm::Value *src);

    // Word, list of words, or list of up to three lists of words (a contents list).
    // Run ERRACT if not; a usable contents list from it replaces the input.
    llvm::Value *generateContentsListFromDatum(ASTNode *parent, llvm::Value *src);

    // Contents list that names at least one procedure, variable, or property list.
    // Run ERRACT if not; a usable named contents list from it replaces the input.
    llvm::Value *generateNamedContentsListFromDatum(ASTNode *parent, llvm::Value *src);

    // Word that is not a number and does not start with '"' or ':'.
    // Run ERRACT if not; a usable name from it replaces the input.
    llvm::Value *generateDefinableProcNameFromDatum(ASTNode *parent, llvm::Value *src);

    // Non-empty list whose members are lists (DEFINE / .DEFMACRO text).
    // Run ERRACT if not; a usable text from it replaces the input.
    llvm::Value *generateDefineTextFromDatum(ASTNode *parent, llvm::Value *src);

    // Ensure src is a non-empty datum whose isa matches typeMask (ERRACT if not).
    llvm::Value *generateNotEmptyFromDatum(ASTNode *parent, llvm::Value *src, Datum::DatumType typeMask);

    // Ensure src is a one-character word (ERRACT if not).
    llvm::Value *generateSingleCharWordFromDatum(ASTNode *parent, llvm::Value *src);

    // Ensure that the Datum is NOT nothing (NOT ASTNode).
    // If it is nothing, run ERRACT; a datum from it replaces the expression.
    llvm::Value *generateNotNothingFromDatum(ASTNode *parent, llvm::Value *src);

    // If src is flow control (error, STOP, OUTPUT, ...), return it immediately.
    // Otherwise pass src through.
    llvm::Value *generateNotFlowControl(llvm::Value *src);

    // Ensure that the Datum is nothing (ASTNode).
    // Emit return "don't say" error if not.
    llvm::Value *generateNothingFromDatum(ASTNode *parent, llvm::Value *src);

    // Common methodology for the genAnd()/genOr() methods.
    // @param isAnd true if AND, false if OR
    llvm::Value *generateAndOr(const DatumPtr &node, TypeMask expected, bool isAnd);

    // BITAND / BITOR / BITXOR. `op` is And, Or, or Xor.
    llvm::Value *generateBitOp(const DatumPtr &node, TypeMask expected, llvm::Instruction::BinaryOps op);

    // ASHIFT / LSHIFT. `arithmetic` selects sign-fill vs zero-fill on a right shift.
    llvm::Value *generateShift(const DatumPtr &node, TypeMask expected, bool arithmetic);

    // ARCTAN / RADARCTAN. `toDegrees` converts the radian result.
    llvm::Value *generateArctan(const DatumPtr &node, TypeMask expected, bool toDegrees);

    // SIN / COS / RADSIN / RADCOS. `id` is sin or cos; `fromDegrees` converts the input.
    llvm::Value *generateTrig(const DatumPtr &node, TypeMask expected, llvm::Intrinsic::ID id, bool fromDegrees);

    // Emit i1: src is a word, or a non-empty list whose head is a word.
    llvm::Value *generateIsValidOpenSpec(llvm::Value *src);

    // OPENWRITE / CLOSE argument. Run ERRACT if not; a usable datum replaces the input.
    llvm::Value *generateOpenSpecFromDatum(ASTNode *parent, llvm::Value *src);

    // OPENREAD / OPENWRITE / OPENAPPEND / OPENUPDATE. `mode` is OpenMode as int32.
    llvm::Value *generateOpen(const DatumPtr &node, TypeMask expected, int32_t mode);

    // PRINT / SHOW / TYPE. `useShow` brackets lists; `addWhitespace` adds spaces and a newline.
    llvm::Value *generateWrite(const DatumPtr &node, TypeMask expected, bool useShow, bool addWhitespace);

    // SHOWIR / SHOWCFG / SHOWMODULEIR. `dumpFn` is the host export that compiles and prints.
    llvm::Value *generateDumpInstructionList(const DatumPtr &node, TypeMask expected,
                                             addr_t (*dumpFn)(addr_t, addr_t, addr_t),
                                             const char *dumpFnName);

    // Group consecutive expressions of the same type (tag or non-tag) together.
    QList<QList<DatumPtr>> groupConsecutiveExpressions(const QList<DatumPtr> &expressions);

    // Compile a list of ASTNode trees into a new CompiledText (JIT module) and store it on aList.
    std::shared_ptr<CompiledText> generateFunctionPtrFromASTList(QList<QList<DatumPtr>> parsedList, List *aList);

    // Generate a number array from an already-evaluated datum.
    // `elementOk`, if set, is applied to each stored double and ANDed into the
    // list predicate, so those constraints share the list's ERRACT trip.
    // A failure blames the list, not the extracted number.
    llvm::AllocaInst *generateNumberAryFromDatum(ASTNode *parent, llvm::Value *src, int32_t size,
                                                 const validatorFunction &elementOk = {});

    // Get the tag name from a tag node.
    QString getTagNameFromNode(const DatumPtr &node) const;

    // Process the tags in a block of ASTNodes.
    void setTagToBlockIdInProcedure(const QList<DatumPtr> &tagList, int32_t blockId);

    // Generate the TagId-to-Block Table of Contents.
    llvm::BasicBlock *generateTOC(QList<llvm::BasicBlock *> blocks, llvm::Function *theFunction);

    // Generate a return operation for the function.
    void generateReturn(llvm::Value *retval);

    // Generate the wrapup code for the function.
    void generateWrapup();

    /// @brief Private constructor for singleton pattern.
    Compiler();

  public:
    /// @brief Get the singleton instance of the Compiler class.
    /// @return The singleton instance of the Compiler class.
    static Compiler &get()
    {
        static Compiler instance;
        return instance;
    }

    Compiler(const Compiler &) = delete;
    Compiler(Compiler &&) = delete;
    Compiler &operator=(const Compiler &) = delete;
    Compiler &operator=(Compiler &&) = delete;

    /// @brief Destructor.
    ~Compiler();

    /// Get the compiled text for a list (compiling if stale, missing, or a dump was requested).
    std::shared_ptr<CompiledText> compiledTextFromList(List *aList);

    /// @brief Timestamp of the last procedure or TRACE change.
    qint64 lastCodeChangeTimestamp() const { return lastCodeChangeTimestamp_; }

    /// @brief Record DEFINE/COPYDEF/ERASE or TRACE/UNTRACE so lists recompile.
    void noteCodeChange();

    /// @brief Set the procedure-body suffix for TAG registration during the next compile.
    /// @param location List whose head is the line being compiled; tail is the remaining body.
    void setTagLineLocation(const DatumPtr &location) { tagLineLocation_ = location; }

    /// @brief The current TAG registration line location.
    /// @details Nested compiles (library autoload during treeify) must save and
    /// restore this so an inner compile does not drop the outer line.
    DatumPtr tagLineLocation() const { return tagLineLocation_; }

    /// @brief Clear the TAG registration line location after compiling.
    void clearTagLineLocation() { tagLineLocation_ = nothing(); }

    /// @brief Replace the dump request for this object's lifetime, then restore the previous value.
    class DumpScope
    {
        DumpRequest saved_;

      public:
        explicit DumpScope(const DumpRequest &request)
            : saved_(Compiler::get().dumpRequest_)
        {
            Compiler::get().dumpRequest_ = request;
        }
        ~DumpScope() { Compiler::get().dumpRequest_ = saved_; }
        DumpScope(const DumpScope &) = delete;
        DumpScope &operator=(const DumpScope &) = delete;
    };

    /// Release JIT resources for all live CompiledText.
    /// Safe to call during shutdown; does not require walking every List.
    static void releaseAllCompiledText();

    // The generators for the different ASTNodes. Since this list changes often during development
    // and it needs to be consistent across several files, we keep the master list in the compiler
    // implementation files. Here, primitives.inc is one of the generated files.
#include "primitives.inc"

    // Generate a noop expression.
    llvm::Value *genNoop(const DatumPtr &node, TypeMask expected);

    // generate the common code for IFTRUE and IFFALSE.
    llvm::Value *generateIftruefalse(const DatumPtr &node, TypeMask expected, bool testForTrue);

    // generate the common code for ERALL, ERPS, ERNS, and ERPLS.
    llvm::Value *generateEraseUnburied(const DatumPtr &node, TypeMask expected,
                                       bool procedures, bool variables, bool plists);

    // generate the common code for LOCAL and GLOBAL.
    llvm::Value *generateLocalGlobal(const DatumPtr &node, TypeMask expected,
                                     addr_t (*fn)(addr_t, addr_t, addr_t, uint32_t),
                                     const std::string &fnName, const std::string &aryName);

    // stdLocal/stdGlobal for names that are already evaluated. May return an Error.
    llvm::Value *generateLocalGlobal(const DatumPtr &node, const std::vector<llvm::Value *> &names,
                                     addr_t (*fn)(addr_t, addr_t, addr_t, uint32_t),
                                     const std::string &fnName, const std::string &aryName);

    // If the child is a genLiteral array, copy it so SETITEM cannot mutate AST.
    // Runtime arrays (ARRAY, variables) are left shared.
    llvm::Value *generateCopyIfLiteralArray(const DatumPtr &node, unsigned childIndex, llvm::Value *value);

    // Look up a variable by name.
    llvm::Value *generateLookupVarname(llvm::Value *nameAddr);

    // generate the common code for FPUT and LPUT.
    llvm::Value *generateFputlput(const DatumPtr &node, TypeMask expected, bool isLput);

    // WORD / LIST / SENTENCE: allocate children and call concatWord / createList / createSentence.
    llvm::Value *generateVariadicConstruct(const DatumPtr &node, addr_t (*ctor)(addr_t, addr_t, uint32_t),
                                           const std::string &ctorName, const std::string &aryName, bool requireWords);

    // generate the common comparison for EQUALP and NOTEQUALP.
    llvm::Value *generateEqualpCompare(llvm::Value *thing1, llvm::Value *thing2);

    // generate the common code for SETITEM.
    llvm::Value *generateSetitem(const DatumPtr &node, TypeMask expected, bool isDangerous);

    // generate the common code for OUTPUT, STOP, and .MAYBEOUTPUT.
    llvm::Value *generateProcedureExit(const DatumPtr &node,
                                       TypeMask expected,
                                       TypeMask childExpected);

    // Shared REPEAT / FOREVER loop. count is the validated trip count, or
    // nullptr for FOREVER. Shadows the process-global repcount. REPEAT
    // returns void or the last iteration's datum; FOREVER only returns
    // via generateImmediateReturn (flow control or "You don't say").
    llvm::Value *generateRepeatLike(const DatumPtr &node, llvm::Value *list, llvm::Value *count);

    llvm::Value *genValueOf(const DatumPtr &node, TypeMask expected);
    llvm::Value *genLiteral(const DatumPtr &node, TypeMask expected);

    llvm::Value *genExecProcedure(const DatumPtr &node, TypeMask expected);

    /// Implicit getter for a variable of the same name when ALLOWGETSET is TRUE.
    llvm::Value *genFoo(const DatumPtr &node, TypeMask expected);

    /// Implicit setter: SETFOO value is MAKE of FOO when ALLOWGETSET is TRUE.
    llvm::Value *genSetfoo(const DatumPtr &node, TypeMask expected);

    template <typename Ret, typename... Args>
    llvm::Value *generateCallExternTyped(Ret (*)(Args...),
                                        const std::string &name,
                                        const std::vector<std::pair<llvm::Type *, llvm::Value *>> &args)
    {
        llvm::LLVMContext &ctx = *scaff_->theContext_;
        llvm::FunctionType *fType = hostExportFunctionType(ctx, name);
        llvm::Type *cRetType = fType->getReturnType();
        std::vector<llvm::Value *> argsV;
        argsV.reserve(args.size());

        Q_ASSERT(cRetType == CTypeToLLVM<Ret>::get(ctx));
        Q_ASSERT(static_cast<size_t>(args.size()) == sizeof...(Args));
        Q_ASSERT(static_cast<size_t>(args.size()) == fType->getNumParams());

        std::array<bool, sizeof...(Args)> isBoolParam{CTypeABI<Args>::isBool...};
        for (size_t i = 0; i < args.size(); ++i)
        {
            llvm::Type *cTy = fType->getParamType(i);
            llvm::Value *v = args[i].second;
            if (isBoolParam[i])
            {
                Q_ASSERT(args[i].first->isIntegerTy(1));
                Q_ASSERT(v->getType()->isIntegerTy(1));
                Q_ASSERT(cTy->isIntegerTy(8));
                v = scaff_->builder_.CreateZExt(v, cTy, name + "_zext");
            }
            else
            {
                Q_ASSERT(args[i].first == cTy);
                Q_ASSERT(v->getType() == cTy);
            }
            argsV.push_back(v);
        }

        llvm::FunctionCallee calleeF = scaff_->theModule_->getOrInsertFunction(name, fType);
        Q_ASSERT(calleeF.getFunctionType() == fType);

        const bool extI32 = llvm::Triple(scaff_->theModule_->getTargetTriple()).isAArch64();
        llvm::AttributeList attrs = hostExportAttributeList<Ret, Args...>(ctx, extI32);
        if (auto *F = llvm::dyn_cast<llvm::Function>(calleeF.getCallee()))
            F->setAttributes(attrs);

        llvm::CallInst *CI = scaff_->builder_.CreateCall(
            calleeF, argsV, cRetType->isVoidTy() ? "" : name + "_result");
        CI->setAttributes(attrs);

        if constexpr (CTypeABI<Ret>::isBool)
            return scaff_->builder_.CreateTrunc(CI, llvm::Type::getInt1Ty(ctx), name + "_i1");
        return CI;
    }
    
}; // class Compiler

/// @brief Evaluates a gen* node's children and is the future TRACE enter/exit site.
template <std::size_t N>
class CallSite
{
    Compiler *compiler_;
    DatumPtr node_;
    std::vector<llvm::Value *> children_;

    void initAll(TypeMask expectedAll)
    {
        Q_ASSERT(compiler_ != nullptr);
        children_ = compiler_->generateChildren(node_.astnodeValue(), expectedAll);
        compiler_->generateTraceBegin(node_.astnodeValue(), children_);
    }

    void initEach(const std::vector<TypeMask> &expected)
    {
        Q_ASSERT(compiler_ != nullptr);
        children_ = compiler_->generateChildren(node_.astnodeValue(), expected);
        compiler_->generateTraceBegin(node_.astnodeValue(), children_);
    }

public:
    template <std::size_t M = N, std::enable_if_t<M == 0, int> = 0>
    CallSite(Compiler *compiler, const DatumPtr &node, TypeMask expectedAll)
        : compiler_(compiler)
        , node_(node)
    {
        initAll(expectedAll);
    }

    template <std::size_t M = N, std::enable_if_t<M == 0, int> = 0>
    CallSite(Compiler *compiler, const DatumPtr &node, const std::vector<TypeMask> &expected)
        : compiler_(compiler)
        , node_(node)
    {
        initEach(expected);
    }

    template <std::size_t M = N, std::enable_if_t<M == 0, int> = 0>
    CallSite(Compiler *compiler, const DatumPtr &node, std::initializer_list<TypeMask> expected)
        : CallSite(compiler, node, std::vector<TypeMask>(expected))
    {
    }

    template <typename... Masks, std::size_t M = N, std::enable_if_t<(M == sizeof...(Masks) && M > 0), int> = 0>
    CallSite(Compiler *compiler, const DatumPtr &node, Masks... masks)
        : compiler_(compiler)
        , node_(node)
    {
        initEach(std::vector<TypeMask>{static_cast<TypeMask>(masks)...});
    }

    CallSite(const CallSite &) = delete;
    CallSite &operator=(const CallSite &) = delete;
    CallSite(CallSite &&) = delete;
    CallSite &operator=(CallSite &&) = delete;

    ~CallSite() = default;

    const DatumPtr &node() const
    {
        return node_;
    }

    unsigned count() const
    {
        return static_cast<unsigned>(children_.size());
    }

    llvm::Value *child(unsigned index) const
    {
        Q_ASSERT(index < children_.size());
        return children_[index];
    }

    std::conditional_t<N == 0, const std::vector<llvm::Value *> &, std::array<llvm::Value *, N>> children() const
    {
        if constexpr (N == 0)
        {
            return children_;
        }
        else
        {
            Q_ASSERT(children_.size() == N);
            std::array<llvm::Value *, N> out{};
            for (std::size_t i = 0; i < N; ++i)
                out[i] = children_[i];
            return out;
        }
    }

    llvm::AllocaInst *allocaAry(const std::string &name = "")
    {
        return compiler_->generateAllocaAry(children_, name);
    }

    llvm::Value *ret(llvm::Value *value)
    {
        Q_ASSERT(value != nullptr);
        compiler_->generateTraceEnd(node_.astnodeValue(), value);
        return value;
    }
};

CallSite(Compiler *, const DatumPtr &, TypeMask) -> CallSite<0>;
CallSite(Compiler *, const DatumPtr &, const std::vector<TypeMask> &) -> CallSite<0>;
CallSite(Compiler *, const DatumPtr &, std::initializer_list<TypeMask>) -> CallSite<0>;

template <typename M1, typename M2, typename... Rest>
CallSite(Compiler *, const DatumPtr &, M1, M2, Rest...) -> CallSite<2 + sizeof...(Rest)>;

#define generateCallExtern(FUNC, ...) \
generateCallExternTyped(FUNC, #FUNC, {__VA_ARGS__})


#endif // COMPILER_H
