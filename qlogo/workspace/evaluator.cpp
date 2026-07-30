#include "workspace/evaluator.h"
#include "datum_types.h"
#include "workspace/kernel.h"
#include "compiler.h"
#include "flowcontrol.h"
#include "runparser.h"
#include <QObject>

Evaluator::Evaluator(CallFrame *aOwningFrame, const DatumPtr &aList) : owningFrame_(aOwningFrame), list_(aList)
{
}

namespace {

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

} // namespace

Evaluator::~Evaluator()
{
    // Suspended frames should not reach destruction (ECE only pops when complete).
    // Still destroy whatever remains so the heap allocation cannot outlive us.
    destroyCoroFrame(handle_);
    // Drop JIT ownership after the frame is gone (resume/destroy point into the module).
    compiledText_.reset();

    // Release the objects in the release pool.
    for (auto &d : releasePool_)
    {
        if ((d->isa_ & Datum::typePersistentMask) == 0)
        {
            (d->retainCount_)--;
            if (d->retainCount_ <= 0)
                delete d;
        }
    }
}

bool Evaluator::exec(int32_t jumpLocation)
{
    if (handle_ == nullptr)
    {
        // Generate and execute the function. Might return a coroutine handle.
        if (list_.listValue() == EmptyList::instance())
        {
            retvalToParent_ = Datum::notADatum();
            return true;
        }
        try
        {
            // When compiling a procedure body line, register TAGs against the body
            // suffix that starts at this line. Skip for RUN/sublists (list_ is not
            // the head of runningSourceList_).
            DatumPtr lineLoc = owningFrame_->runningSourceList_;
            if (lineLoc.isList() && !lineLoc.listValue()->isEmpty() &&
                lineLoc.listValue()->head.datumValue() == list_.datumValue())
            {
                Compiler::get().setTagLineLocation(lineLoc);
            }
            compiledText_ = Compiler::get().compiledTextFromList(list_.listValue());
            Compiler::get().clearTagLineLocation();
        }
        catch (FCError *e)
        {
            Compiler::get().clearTagLineLocation();
            compiledText_.reset();
            retvalToParent_ = e;
            return true;
        }

        CompiledFunctionPtr fn = compiledText_ ? compiledText_->functionPtr_ : nullptr;
        if (fn == nullptr)
        {
            compiledText_.reset();
            retvalToParent_ = FCError::fatalInternal();
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
        compiledText_.reset();
        return true;
    }
    return false;
}

void Evaluator::pushSublist(Datum *aList)
{
    try
    {
        if (aList->isWord())
        {
            DatumPtr runparsedList = runparse(DatumPtr(aList));
            aList = runparsedList.datumValue();
            watch(aList);
        }
        if (!aList->isList())
        {
            retvalToParent_ = FCError::noHow(DatumPtr(aList));
            return;
        }
        if (aList->listValue()->isEmpty())
        {
            retvalToParent_ = Datum::notADatum();
            return;
        }
    owningFrame_->pushEvaluator(DatumPtr(aList));
    }
    catch (FCError *err)
    {
        retvalToParent_ = err;
    }
}

void Evaluator::beginProcedure(ASTNode *node, Datum **paramAry, uint32_t paramCount)
{
    Kernel::get().beginProcedure(node, paramAry, paramCount);
}

Datum *Evaluator::watch(const DatumPtr &d)
{
    return watch(d.datumValue());
}

Datum *Evaluator::watch(Datum *d)
{
    (d->retainCount_)++;
    releasePool_.push_back(d);
    return d;
}

bool Evaluator::varCASEIGNOREDP()
{
    QString name = QObject::tr("CASEIGNOREDP");
    DatumPtr val = Kernel::get().datumForName(name);
    bool retval = false;
    if (val.isWord())
    {
        QString word = val.toString(Datum::ToStringFlags_Key);
        retval = word == QObject::tr("TRUE");
    }
    return retval;
}
