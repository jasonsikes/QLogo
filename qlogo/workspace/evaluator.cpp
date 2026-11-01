#include "workspace/evaluator.h"
#include "compiler.h"
#include "datum_types.h"
#include "flowcontrol.h"
#include "runparser.h"
#include "workspace/kernel.h"
#include "workspace/specialvars.h"
#include <QObject>

namespace
{

bool retvalIsError(Datum *retval)
{
    return retval != nullptr && retval->isa_ == Datum::typeError;
}

void unwindTraceDepth(Evaluator &e)
{
    Kernel::get().restoreTraceDepth(e.traceDepthAtEntry_);
}

/// Restore TRACE indent on error. On success, every begin in this list should have ended.
void finishTraceDepth(Evaluator &e)
{
    if (retvalIsError(e.retvalToParent_))
        unwindTraceDepth(e);
    else
        Q_ASSERT(Kernel::get().traceDepth() == e.traceDepthAtEntry_);
}

/// Free the LLVM coro frame if destroy is present, then drop the host handle.
void destroyCoroFrame(LLVMCoroFrameHeader *&handle)
{
    if (handle == nullptr)
        return;
    if (handle->destroy != nullptr)
        handle->destroy(handle);
    handle = nullptr;
}

bool coroFrameIsComplete(LLVMCoroFrameHeader *handle)
{
    return handle == nullptr || handle->resume == nullptr;
}

/// Runparse a word if needed; require a non-empty list.
/// @return nullptr if @p aList is ready to run; otherwise error or nothing (also in e.retvalToParent_).
Datum *normalizeListToRun(Evaluator &e, Datum *&aList)
{
    if (aList->isWord())
    {
        DatumPtr runparsedList = runparse(DatumPtr(aList));
        aList = runparsedList.datumValue();
        e.watch(aList);
    }
    if (!aList->isList())
    {
        e.retvalToParent_ = FCError::noHow(DatumPtr(aList));
        return e.retvalToParent_;
    }
    if (aList->listValue()->isEmpty())
    {
        e.retvalToParent_ = Datum::notADatum();
        return e.retvalToParent_;
    }
    return nullptr;
}

} // namespace

Evaluator::Evaluator(CallFrame *aOwningFrame, const DatumPtr &aList)
    : owningFrame_(aOwningFrame), list_(aList), startingList_(aList), traceDepthAtEntry_(Kernel::get().traceDepth())
{
}

Evaluator::~Evaluator()
{
    // Suspended frames should not reach destruction (ECE only pops when complete).
    // Still destroy whatever remains so the heap allocation cannot outlive us.
    destroyCoroFrame(handle_);
    // Drop JIT ownership after the frame is gone (resume/destroy point into the module).
    compiledText_.reset();

    if (Kernel::get().traceDepth() > traceDepthAtEntry_)
        unwindTraceDepth(*this);

    // Release the objects in the release pool.
    releasePool_.clear();
}

bool Evaluator::exec(int32_t jumpLocation)
{
    if (handle_ == nullptr)
    {
        // Generate and execute the function. Might return a coroutine handle.
        if (list_.listValue() == EmptyList::instance())
        {
            retvalToParent_ = Datum::notADatum();
            finishTraceDepth(*this);
            return true;
        }
        DatumPtr savedTagLocation = Compiler::get().tagLineLocation();
        try
        {
            // When compiling a procedure body line, register TAGs against the body
            // suffix that starts at this line. Skip for RUN/sublists (list_ is not
            // the head of runningSourceList_). Nested compiles (library autoload
            // during treeify) must not keep this location, and must restore it.
            DatumPtr lineLoc = owningFrame_->runningSourceList_;
            if (lineLoc.isList() && !lineLoc.listValue()->isEmpty() &&
                lineLoc.listValue()->head.datumValue() == list_.datumValue())
            {
                Compiler::get().setTagLineLocation(lineLoc);
            }
            else
            {
                Compiler::get().clearTagLineLocation();
            }
            compiledText_ = Compiler::get().compiledTextFromList(list_.listValue());
            Compiler::get().setTagLineLocation(savedTagLocation);
        }
        catch (FCError *e)
        {
            Compiler::get().setTagLineLocation(savedTagLocation);
            compiledText_.reset();
            retvalToParent_ = e;
            finishTraceDepth(*this);
            return true;
        }

        CompiledFunctionPtr fn = compiledText_ ? compiledText_->functionPtr_ : nullptr;
        if (fn == nullptr)
        {
            compiledText_.reset();
            retvalToParent_ = FCError::fatalInternal();
            finishTraceDepth(*this);
            return true;
        }

        handle_ = fn((addr_t)this, (addr_t)&retvalToParent_, jumpLocation, nullptr);
    }
    else
    {
        if (handle_->resume != nullptr)
            handle_->resume(handle_);
    }

    // Completed: destroy immediately so handle_ never dangles across later ECE steps.
    if (coroFrameIsComplete(handle_))
    {
        destroyCoroFrame(handle_);
        if (retvalToParent_ != nullptr)
            watch(retvalToParent_);
        compiledText_.reset();
        finishTraceDepth(*this);
        return true;
    }
    return false;
}

void Evaluator::pushSublist(Datum *aList)
{
    try
    {
        if (normalizeListToRun(*this, aList) != nullptr)
            return;
        owningFrame_->pushEvaluator(DatumPtr(aList));
    }
    catch (FCError *err)
    {
        retvalToParent_ = err;
    }
}

void Evaluator::replaceRunningList(const DatumPtr &aList)
{
    destroyCoroFrame(handle_);
    compiledText_.reset();
    unwindTraceDepth(*this);
    list_ = aList;
    listToRunAfterCurrentList_ = DatumPtr();
    retvalToParent_ = Datum::notADatum();
    retvalFromChild_ = DatumPtr();
}

Datum *Evaluator::runListAfterCurrentList(Datum *aList)
{
    try
    {
        if (Datum *status = normalizeListToRun(*this, aList))
            return status;
        listToRunAfterCurrentList_ = DatumPtr(aList);
        // Continuation will supply the real result, but we have to return something. So return nothing.
        return Datum::notADatum();
    }
    catch (FCError *err)
    {
        retvalToParent_ = err;
        return err;
    }
}

void Evaluator::beginProcedure(ASTNode *node, Datum **paramAry, uint32_t paramCount, OutputRequired outputRequired)
{
    Kernel::get().beginProcedure(node, paramAry, paramCount, outputRequired);
}

Datum *Evaluator::watch(const DatumPtr &d)
{
    return watch(d.datumValue());
}

Datum *Evaluator::watch(Datum *d)
{
    releasePool_.emplace_back(d);
    return d;
}

bool Evaluator::varCASEIGNOREDP()
{
    return Kernel::get().specialVarIsTrue(SpecialVars::Id::CaseIgnoredP);
}

bool Evaluator::varALLOWGETSET()
{
    return Kernel::get().specialVarIsTrue(SpecialVars::Id::AllowGetSet);
}
