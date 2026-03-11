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

Evaluator::~Evaluator()
{
    // Destroy the coroutine frame if it exists.
    if (handle_ != nullptr)
    {
        Q_ASSERT(handle_->resume == nullptr);
        if (handle_->destroy != nullptr)
            handle_->destroy(handle_);
    }

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
            fn_ = Compiler::get().functionPtrFromList(list_.listValue());
        }
        catch (FCError *e)
        {
            retvalToParent_ = e;
            return true;
        }
        handle_ = fn_((addr_t)this, (addr_t)&retvalToParent_, jumpLocation, nullptr);
    } else {
        // Resume using the frame's resume function.
        if (handle_->resume != nullptr)
        {
            handle_->resume(handle_);
        }
    }
    return (handle_ == nullptr) || (handle_->resume == nullptr);
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
