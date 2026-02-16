#include "workspace/evaluator.h"
#include "datum_types.h"
#include "workspace/kernel.h"
#include "compiler.h"
#include "flowcontrol.h"

NewEvaluator::NewEvaluator(const DatumPtr &aList) : list(aList)
{
}

NewEvaluator::~NewEvaluator()
{
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
        if (list.listValue() == EmptyList::instance())
        {
            retval = Datum::notADatum();
            return false;
        }
        try
        {
            fn = Compiler::get().functionPtrFromList(list.listValue());
        }
        catch (FCError *e)
        {
            retval = e;
            return false;
        }
        // Start: call the ramp once to get the coroutine handle (LLVM frame).
        handle = fn((addr_t)this, (addr_t)&retval, jumpLocation, nullptr);
    } else {
        // Resume using the frame's resume function. Completion: compiler nulls the frame's
        // resume pointer; we check it after resume() returns, then call destroy to free.
        LLVMCoroFrameHeader *frame = reinterpret_cast<LLVMCoroFrameHeader *>(handle);
        frame->resume(handle);
        if (frame->resume == nullptr)
        {
            frame->destroy(handle);
            handle = nullptr;
        }
    }
    return handle == nullptr;
}

Datum *NewEvaluator::subExec(Datum *aList)
{
    // try
    // {
    //     if (aList->isWord())
    //     {
    //         DatumPtr runparsedList = runparse(DatumPtr(aList));
    //         aList = runparsedList.datumValue();
    //         watch(aList);
    //     }
    //     if (!aList->isList())
    //     {
    //         FCError *err = FCError::noHow(DatumPtr(aList));
    //         watch(err);
    //         return err;
    //     }
    //     if (aList->listValue()->isEmpty())
    //     {
    //         return Datum::notADatum();
    //     }
    //     NewEvaluator e(aList, evalStack);
    //     retval = e.exec();
    // }
    // catch (FCError *err)
    // {
    //     retval = err;
    // }
    // return retval;
    return nullptr;
}

Datum *NewEvaluator::procedureExec(ASTNode *node, Datum **paramAry, uint32_t paramCount)
{
    CallFrameStack &frameStack = Kernel::get().callStack;
    CallFrame frame(frameStack, DatumPtr(node));

    return frame.exec(paramAry, paramCount);
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
    QString name = QObject::tr("CASEIGNOREDP");
    DatumPtr val = Kernel::get().callStack.datumForName(name);
    bool retval = false;
    if (val.isWord())
    {
        QString word = val.toString(Datum::ToStringFlags_Key);
        retval = word == QObject::tr("TRUE");
    }
    return retval;
}
