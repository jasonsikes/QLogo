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

Datum *NewEvaluator::exec(int32_t jumpLocation)
{
    if (list.listValue() == EmptyList::instance())
    {
        return Datum::notADatum();
    }
    try
    {
        fn = Compiler::get().functionPtrFromList(list.listValue());
    }
    catch (FCError *e)
    {
        return watch(e);
    }
    // Start: call the ramp once to get the coroutine handle (LLVM frame).
    coroutine_handle_t handle = fn((addr_t)this, (addr_t)&retval, jumpLocation, nullptr);
    // Resume using the frame's resume function, not the ramp. Repeat until the coroutine completes.
    while (handle != nullptr)
    {
        reinterpret_cast<LLVMCoroFrameHeader *>(handle)->resume(handle);
        // TODO: detect completion (e.g. frame's resume ptr nulled, or done flag) and set handle = nullptr.
        // For now assume single run to completion.
        handle = nullptr;
    }
    return retval;
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
