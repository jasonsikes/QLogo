#include "workspace/evaluator.h"
#include "datum_types.h"
#include "workspace/kernel.h"
#include "compiler.h"
#include "flowcontrol.h"
#include "runparser.h"

NewEvaluator::NewEvaluator(const DatumPtr &aList) : list_(aList)
{
}

NewEvaluator::~NewEvaluator()
{
    // Destroy the coroutine frame if it exists.
    if (handle_ != nullptr)
    {
        LLVMCoroFrameHeader *frame = reinterpret_cast<LLVMCoroFrameHeader *>(handle_);
        Q_ASSERT(frame->resume == nullptr);
        frame->destroy(handle_);
    }

    // Release the objects in the release pool.
    for (auto &d : releasePool_)
    {
        if ((d->isa_ & Datum::typePersistentMask) == 0)
        {
            (d->retainCount_)--;
            if ((d != retval) && (d->retainCount_ <= 0))
                delete d;
        }
    }
}

bool NewEvaluator::exec(int32_t jumpLocation)
{
    if (handle_ == nullptr)
    {
        // Generate and execute the function. Might return a coroutine handle.
        if (list_.listValue() == EmptyList::instance())
        {
            retval = Datum::notADatum();
            return true;
        }
        try
        {
            fn_ = Compiler::get().functionPtrFromList(list_.listValue());
        }
        catch (FCError *e)
        {
            retval = e;
            return true;
        }
        handle_ = fn_((addr_t)this, (addr_t)&retval, jumpLocation, nullptr);
    } else {
        // Resume using the frame's resume function.
        if (handle_->resume != nullptr)
        {
            handle_->resume(handle_);
        }
    }
    return (handle_ == nullptr) || (handle_->resume == nullptr);
}

void NewEvaluator::pushSublist(Datum *aList)
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
            FCError *err = FCError::noHow(DatumPtr(aList));
            retval = err;
            return;
        }
        if (aList->listValue()->isEmpty())
        {
            retval = Datum::notADatum();
            return;
        }
    NewCallFrame *topCallFrame = Kernel::get().callFrameStack.top().get();
    topCallFrame->evaluationStack_.push(std::move(std::make_unique<NewEvaluator>(DatumPtr(aList))));
    }
    catch (FCError *err)
    {
        retval = err;
    }
}

Datum *NewEvaluator::procedureExec(ASTNode *node, Datum **paramAry, uint32_t paramCount)
{
    // CallFrameStack &frameStack = Kernel::get().callStack;
    // CallFrame frame(frameStack, DatumPtr(node));

    // return frame.exec(paramAry, paramCount);
}

Datum *NewEvaluator::watch(const DatumPtr &d)
{
    return watch(d.datumValue());
}

Datum *NewEvaluator::watch(Datum *d)
{
    (d->retainCount_)++;
    releasePool_.push_back(d);
    return d;
}

bool NewEvaluator::varCASEIGNOREDP()
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
