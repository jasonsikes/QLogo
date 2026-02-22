#include "workspace/evaluator.h"
#include "datum_types.h"
#include "workspace/kernel.h"
#include "compiler.h"
#include "flowcontrol.h"
#include "runparser.h"

NewEvaluator::NewEvaluator(const DatumPtr &aList) : list(aList)
{
}

NewEvaluator::~NewEvaluator()
{
    // Destroy the coroutine frame if it exists.
    if (handle != nullptr)
    {
        LLVMCoroFrameHeader *frame = reinterpret_cast<LLVMCoroFrameHeader *>(handle);
        Q_ASSERT(frame->resume == nullptr);
        frame->destroy(handle);
    }

    // Release the objects in the release pool.
    for (auto &d : releasePool)
    {
        if ((d->isa & Datum::typePersistentMask) == 0)
        {
            (d->retainCount)--;
            if ((d != retval) && (d->retainCount <= 0))
                delete d;
        }
    }
}

bool NewEvaluator::exec(int32_t jumpLocation)
{
    if (handle == nullptr)
    {
        // Generate and execute the function. Might return a coroutine handle.
        if (list.listValue() == EmptyList::instance())
        {
            retval = Datum::notADatum();
            return true;
        }
        try
        {
            fn = Compiler::get().functionPtrFromList(list.listValue());
        }
        catch (FCError *e)
        {
            retval = e;
            return true;
        }
        handle = fn((addr_t)this, (addr_t)&retval, jumpLocation, nullptr);
    } else {
        // Resume using the frame's resume function.
        if (handle->resume != nullptr)
        {
            handle->resume(handle);
        }
    }
    return (handle == nullptr) || (handle->resume == nullptr);
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
        Kernel::get().pushListOntoEvaluationStack(DatumPtr(aList));
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
    (d->retainCount)++;
    releasePool.push_back(d);
    return d;
}

bool NewEvaluator::varCASEIGNOREDP()
{
    // QString name = QObject::tr("CASEIGNOREDP");
    // DatumPtr val = Kernel::get().callStack.datumForName(name);
    // bool retval = false;
    // if (val.isWord())
    // {
    //     QString word = val.toString(Datum::ToStringFlags_Key);
    //     retval = word == QObject::tr("TRUE");
    // }
    // return retval;
}
