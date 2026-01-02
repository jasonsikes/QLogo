// ===-- qlogo/misc/exports.cpp - Exports for the QLogo language -------*- C++ -*-===//
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
/// This file contains the C library routines that are used by the QLogo compiler.
///
//===----------------------------------------------------------------------===//

#include "exports.h"
#include "astnode.h"
#include "compiler_private.h"
#include "controller/logocontroller.h"
#include "controller/textstream.h"
#include "datum_types.h"
#include "flowcontrol.h"
#include "kernel.h"
#include "runparser.h"
#include "sharedconstants.h"
#include "turtle.h"
#include "visited.h"
#include "workspace/callframe.h"

#include <QFile>
#include <QObject>
#include <QRandomGenerator>

#include <algorithm>

bool areDatumsEqual(VisitedMap &visited, Datum *d1, Datum *d2, Qt::CaseSensitivity cs);

/// @brief Recursively check if a datum is in an array.
/// @param visited The set of visited nodes.
/// @param value The value to check for.
/// @param array The array to check.
/// @param cs The case sensitivity to use for the comparison.
/// @return True if the value is in the array, false otherwise.
bool isDatumInArray(VisitedSet &visited, Datum *value, Array *array, Qt::CaseSensitivity cs)
{
    VisitedMap searched;
    for (auto &item : array->array)
    {
        searched.clear();
        Datum *itemPtr = item.datumValue();
        if (areDatumsEqual(searched, itemPtr, value, cs))
            return true;
        if (itemPtr->isArray() || itemPtr->isList())
        {
            if (visited.contains(itemPtr))
            {
                visited.add(itemPtr);
                if (isDatumInContainer(visited, itemPtr, value, cs))
                    return true;
            }
        }
    }
    return false;
}

/// @brief Recursively check if a datum is in a list.
/// @param visited The set of visited nodes.
/// @param value The value to check for.
/// @param list The list to check.
/// @param cs The case sensitivity to use for the comparison.
/// @return True if the value is in the list, false otherwise.
bool isDatumInList(VisitedSet &visited, Datum *value, List *list, Qt::CaseSensitivity cs)
{
    VisitedMap searched;
    while (list != EmptyList::instance())
    {
        Datum *itemPtr = list->head.datumValue();
        searched.clear();
        if (areDatumsEqual(searched, itemPtr, value, cs))
            return true;
        if (itemPtr->isArray() || itemPtr->isList())
        {
            if (!visited.contains(itemPtr))
            {
                visited.add(itemPtr);
                if (isDatumInContainer(visited, itemPtr, value, cs))
                    return true;
            }
        }
        list = list->tail.listValue();
    }
    return false;
}

/// @brief Recursively check if a datum is in a container.
/// @param visited The set of visited nodes.
/// @param value The value to check for.
/// @param container The container to check.
/// @param cs The case sensitivity to use for the comparison.
/// @return True if the value is in the container, false otherwise.
bool isDatumInContainer(VisitedSet &visited, Datum *value, Datum *container, Qt::CaseSensitivity cs)
{
    if (visited.contains(container))
        return false;
    visited.add(container);

    VisitedMap searched;

    if (container->isArray())
    {
        Array *a = container->arrayValue();
        return isDatumInArray(visited, value, a, cs);
    }
    else if (container->isList())
    {
        List *l = container->listValue();
        return isDatumInList(visited, value, l, cs);
    }
    else
    {
        Q_ASSERT(false);
    }
    return false;
}

List *listFromColor(const QColor &c)
{
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(round(static_cast<double>(c.redF() * 100.0))));
    retvalBuilder.append(DatumPtr(round(static_cast<double>(c.greenF() * 100.0))));
    retvalBuilder.append(DatumPtr(round(static_cast<double>(c.blueF() * 100.0))));
    return retvalBuilder.finishedList().listValue();
}

/// Get a reference to the random number generator instance.
static QRandomGenerator &randomGenerator()
{
    static QRandomGenerator randomGeneratorInstance;
    return randomGeneratorInstance;
}

/// Print an integer to the console.
/// @param p the integer to print.
/// @note For debugging.
// cppcheck-suppress unusedFunction
EXPORTC void printInt(int32_t p)
{
    qWarning() << "int = " << p;
}

/// Return the double value of a word object.
/// @param eAddr a pointer to the Evaluator object
/// @param datumAddr a pointer to a Datum object
/// @return the stored value as a double
/// @note the caller should check getValidityOfDoubleForDatum() afterward.
EXPORTC double getDoubleForDatum(addr_t eAddr, addr_t datumAddr)
{
    auto *w = reinterpret_cast<Word *>(datumAddr);
    if (w->isa == Datum::typeWord)
    {
        return w->numberValue();
    }
    return 0.0;
}

/// Query whether the recent number retrieved is valid.
/// @param eAddr a pointer to the Evaluator object
/// @param datumAddr a pointer to a Datum object
/// @return true iff the number retrieved is valid
/// @note the caller should getDoubleForDatum before querying validity.
EXPORTC bool getValidityOfDoubleForDatum(addr_t eAddr, addr_t datumAddr)
{
    auto *w = reinterpret_cast<Word *>(datumAddr);
    if (w->isa == Datum::typeWord)
    {
        return w->numberIsValid;
    }
    return false;
}

/// Lookup the var name and return the value as a bool
/// @param eAddr a pointer to the Evaluator object
/// @param datumAddr a pointer to a Datum object which contains the name of the variable
/// @return the stored value as a bool
EXPORTC bool getBoolForDatum(addr_t eAddr, addr_t datumAddr)
{
    auto *w = reinterpret_cast<Word *>(datumAddr);
    if (w->isa == Datum::typeWord)
    {
        return w->boolValue();
    }
    return false;
}

/// Query whether the recent boolean value retrieved is valid.
/// @param eAddr a pointer to the Evaluator object
/// @param datumAddr a pointer to a Datum object
/// @return true iff the number retrieved is valid
/// @note the caller should getBoolForDatum before querying validity.
EXPORTC bool getValidityOfBoolForDatum(addr_t eAddr, addr_t datumAddr)
{
    auto *w = reinterpret_cast<Word *>(datumAddr);
    if (w->isa == Datum::typeWord)
    {
        return w->boolIsValid;
    }
    return false;
}

/// Lookup the var name and return the value as a QLogo object (Word, List, etc)
/// @param wordAddr a pointer to a Word object which contains the name of the variable
/// @return the stored value as a QLogo object
EXPORTC addr_t getDatumForVarname(addr_t wordAddr)
{
    auto name = reinterpret_cast<Word *>(wordAddr)->toString(Datum::ToStringFlags_Key);
    Datum *val = Config::get().mainKernel()->callStack.datumForName(name).datumValue();

    return reinterpret_cast<addr_t>(val);
}

/// Write a Datum object to the standard ouput device.
/// @param datumAddr a pointer to a Datum object to print.
/// @param useShow set to true to generate output for SHOW, false for PRINT
EXPORTC addr_t stdWriteDatum(addr_t datumAddr, bool useShow)
{
    Datum::ToStringFlags writeFlags = useShow ? Datum::ToStringFlags_Show : Datum::ToStringFlags_None;
    auto *d = reinterpret_cast<Datum *>(datumAddr);
    QString output = d->toString(writeFlags) + "\n";
    Config::get().mainKernel()->stdPrint(output);
    return nullptr;
}

/// Write an array of Datum objects to the standard ouput device.
/// @param datumAddr a pointer to an array of pointers to Datum objects to print.
/// @param count the number of Datum objects in the array
/// @param useShow set to true to generate output for SHOW, false for PRINT
/// @param addWhitespace set to true to add a newline to the end of the output and spaces between datums.
EXPORTC addr_t stdWriteDatumAry(addr_t datumAddr, uint32_t count, bool useShow, bool addWhitespace)
{
    Datum::ToStringFlags writeFlags = useShow ? Datum::ToStringFlags_Show : Datum::ToStringFlags_None;
    int countOfWords = (int)count;
    auto **datumAry = reinterpret_cast<Datum **>(datumAddr);
    QString output = "";
    for (int i = 0; i < countOfWords; ++i)
    {
        Datum *d = *(datumAry + i);
        if ((i != 0) && addWhitespace)
            output += " ";
        output += d->toString(writeFlags);
    }
    if (addWhitespace)
        output += "\n";
    Config::get().mainKernel()->stdPrint(output);
    return nullptr;
}

/// Create a QLogo Word object using a double value
/// @param eAddr a pointer to the Evaluator object
/// @param val a value stored as a double
/// @return the given value as a QLogo object
EXPORTC addr_t getWordForDouble(addr_t eAddr, double val)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *w = new Word(val);
    e->watch(w);

    return reinterpret_cast<addr_t>(w);
}

/// Create a QLogo Word object using a bool value
/// @param eAddr a pointer to the Evaluator object
/// @param val a value stored as a bool
/// @return the given value as a QLogo object
EXPORTC addr_t getWordForBool(addr_t eAddr, bool val)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *w = new Word(val ? QObject::tr("true") : QObject::tr("false"));
    e->watch(w);

    return reinterpret_cast<addr_t>(w);
}

/// Store the given datum using the given word as a variable name.
/// @param datumAddr a pointer to a QLogo object which is the value to be stored
/// @param wordAddr a pointer to a Word object which contains the name of the variable
EXPORTC void setDatumForWord(addr_t datumAddr, addr_t wordAddr)
{
    auto d = DatumPtr(reinterpret_cast<Datum *>(datumAddr));
    auto *w = reinterpret_cast<Word *>(wordAddr);
    Config::get().mainKernel()->callStack.setDatumForName(d, w->toString(Datum::ToStringFlags_Key));
}

/// Run the given list. Output whatever the list outputs.
/// @param eAddr a pointer to the Evaluator object context.
/// @param listAddr a pointer to a List object which contains QLogo instructions to run
/// @return the result of the list execution
EXPORTC addr_t runList(addr_t eAddr, addr_t listAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *list = reinterpret_cast<List *>(listAddr);
    Datum *result = e->subExec(list);
    e->watch(result);

    return reinterpret_cast<addr_t>(result);
}

/// @brief Execute a procedure.
/// @param eAddr a pointer to the Evaluator object context.
/// @param astnodeAddr a pointer to the ASTNode object which is the procedure to execute.
/// @param paramAryAddr a pointer to an array of pointers to Datum objects which are the parameters to the procedure.
/// @param paramCount the number of parameters to the procedure.
/// @return the result of the procedure execution.
EXPORTC addr_t runProcedure(addr_t eAddr, addr_t astnodeAddr, addr_t paramAryAddr, uint32_t paramCount)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    auto **paramAry = reinterpret_cast<Datum **>(paramAryAddr);
    Datum *result = e->procedureExec(node, paramAry, paramCount);
    e->watch(result);

    return reinterpret_cast<addr_t>(result);
}

/// Create and return Error: "SYSTEM"
/// @param aE a pointer to the Evaluator object context.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorSystem(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    FCError *err = FCError::custom(DatumPtr(QObject::tr("SYSTEM")));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return Error: "X didn't like Y as input"
/// @param aE a pointer to the Evaluator object context.
/// @param whoAddr a pointer to the Datum object which rejected the input.
/// @param whatAddr a pointer to the Datum object that was rejected.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoLike(addr_t eAddr, addr_t whoAddr, addr_t whatAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *who = reinterpret_cast<Datum *>(whoAddr);
    auto *what = reinterpret_cast<Datum *>(whatAddr);
    FCError *err = FCError::doesntLike(who, what);
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return Error: "You don't say what to do with X"
/// @param aE a pointer to the Evaluator object context.
/// @param whatAddr a pointer to the Datum object that was orphaned.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoSay(addr_t eAddr, addr_t whatAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *what = reinterpret_cast<Datum *>(whatAddr);
    FCError *err = FCError::dontSay(DatumPtr(what->toString(Datum::ToStringFlags_Show)));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return Error: "X without TEST"
/// @param aE a pointer to the Evaluator object context.
/// @param xAddr a pointer to the Datum object that was called without a TEST.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoTest(addr_t eAddr, addr_t whoAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *who = reinterpret_cast<Datum *>(whoAddr);
    FCError *err = FCError::noTest(DatumPtr(who->toString(Datum::ToStringFlags_Show)));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return Error: "X didn't output to Y"
/// @param aE a pointer to the Evaluator object context.
/// @param xAddr a pointer to the Datum object which didn't ouput.
/// @param yAddr a pointer to the Datum object that expected input.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoOutput(addr_t eAddr, addr_t xAddr, addr_t yAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *x = reinterpret_cast<Datum *>(xAddr);
    auto *y = reinterpret_cast<Datum *>(yAddr);
    // If the thing that didn't output is an ASTNode, use the name of the ASTNode.
    if (x->isa == Datum::typeASTNode)
    {
        x = reinterpret_cast<ASTNode *>(x)->nodeName.datumValue();
    }
    FCError *err = FCError::didntOutput(DatumPtr(x->toString(Datum::ToStringFlags_Show)),
                                        DatumPtr(y->toString(Datum::ToStringFlags_Show)));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return Error: "not enough inputs to X"
/// @param aE a pointer to the Evaluator object context.
/// @param xAddr a pointer to the Datum object that is deprived of inputs.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNotEnoughInputs(addr_t eAddr, addr_t xAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *x = reinterpret_cast<Datum *>(xAddr);
    FCError *err = FCError::notEnoughInputs(DatumPtr(x->toString(Datum::ToStringFlags_Show)));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return Error: "X has no value"
/// @param aE a pointer to the Evaluator object context.
/// @param whatAddr a pointer to the Word that was looked up.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoValue(addr_t eAddr, addr_t whatAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *what = reinterpret_cast<Datum *>(whatAddr);
    FCError *err = FCError::noValue(DatumPtr(what->toString(Datum::ToStringFlags_Show)));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// Create and return an error generated by THROW
/// @param eAddr a pointer to the Evaluator object context.
/// @param tagAddr a pointer to the Datum object which is the tag of the error.
/// @param outputAddr a pointer to the Datum object which is the output of the error.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorCustom(addr_t eAddr, addr_t tagAddr, addr_t outputAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *tag = reinterpret_cast<Datum *>(tagAddr);
    auto *output = reinterpret_cast<Datum *>(outputAddr);
    FCError *err = FCError::custom(DatumPtr(tag), nothing(), DatumPtr(output));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

/// @brief Create and return a RETURN control object.
/// @param eAddr a pointer to the Evaluator object context.
/// @param astNodeAddr a pointer to the ASTNode which is the source node of the RETURN.
/// @param retvalAddr a pointer to the Datum object which is the value to return.
/// @return a pointer to the RETURN control object that was generated.
EXPORTC addr_t getCtrlReturn(addr_t eAddr, addr_t astNodeAddr, addr_t retvalAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto retval = DatumPtr(reinterpret_cast<Datum *>(retvalAddr));
    auto *control = new FCReturn(DatumPtr(reinterpret_cast<Datum *>(astNodeAddr)), retval);
    e->watch(control);
    return reinterpret_cast<addr_t>(control);
}

/// @brief Create and return a CONTINUATION control object.
/// @param eAddr a pointer to the Evaluator object context.
/// @param astNodeAddr a pointer to the ASTNode which is the source node of the CONTINUATION.
/// @param paramAryAddr a pointer to an array of pointers to Datum objects which are the parameters to the CONTINUATION.
/// @param paramCount the number of parameters to the CONTINUATION.
/// @return a pointer to the CONTINUATION control object that was generated.
EXPORTC addr_t getCtrlContinuation(addr_t eAddr, addr_t astNodeAddr, addr_t paramAryAddr, uint32_t paramCount)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(astNodeAddr);
    auto nodePtr = DatumPtr(node);

    QList<DatumPtr> paramAry;
    for (uint32_t i = 0; i < paramCount; ++i)
    {
        auto param = DatumPtr(reinterpret_cast<Datum *>(paramAryAddr[i]));
        paramAry.append(param);
    }
    auto *control = new FCContinuation(DatumPtr(reinterpret_cast<Datum *>(astNodeAddr)), nodePtr, paramAry);
    e->watch(control);
    return reinterpret_cast<addr_t>(control);
}

/// @brief Create and return a GOTO control object.
/// @param eAddr a pointer to the Evaluator object context.
/// @param astNodeAddr a pointer to the ASTNode which is the source node of the GOTO.
/// @param tagAddr a pointer to the Datum object which is the tag of the GOTO.
/// @return a pointer to the GOTO control object that was generated.
EXPORTC addr_t getCtrlGoto(addr_t eAddr, addr_t astNodeAddr, addr_t tagAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto tag = DatumPtr(reinterpret_cast<Datum *>(tagAddr));

    auto *control = new FCGoto(DatumPtr(reinterpret_cast<Datum *>(astNodeAddr)), tag);
    e->watch(control);
    return reinterpret_cast<addr_t>(control);
}

/// @brief Get the number of elements in a list.
/// @param listAddr a pointer to the List object.
/// @return the number of elements in the list.
EXPORTC int32_t getCountOfList(addr_t listAddr)
{
    auto *list = reinterpret_cast<List *>(listAddr);
    return list->count();
}

/// @brief Get a double array from a list of doubles.
/// @param eAddr a pointer to the Evaluator object context.
/// @param listAddr a pointer to the List object source
/// @param destAddr a pointer to the double array to store the values.
/// @return 0 if the list is not a list of doubles, 1 if it is.
EXPORTC int32_t getNumberAryFromList(addr_t listAddr, addr_t destAddr)
{
    auto list = DatumPtr(reinterpret_cast<Datum *>(listAddr));
    auto *dest = reinterpret_cast<double *>(destAddr);
    // Presumably, getCountOfList() has already been called so the destination size is correct.
    while (list.isList() && !list.listValue()->isEmpty())
    {
        DatumPtr d = list.listValue()->head;
        if (!d.isWord())
        {
            return 0;
        }
        *dest = d.wordValue()->numberValue();
        if (!d.wordValue()->numberIsValid)
        {
            return 0;
        }
        ++dest;
        list = list.listValue()->tail;
    }
    return 1;
}

// Functions moved from compiler*.cpp files

/// Generate a random nonnegative integer less than num given
/// @param num an upper bound (exclusive) to the random number.
/// @return the random number generated.
EXPORTC double random1(int32_t num)
{
    int result = randomGenerator().bounded(num);
    return (double)result;
}

/// Generate a random integer between start and end (both inclusive)
/// @param start a lower bound (inclusive) to the random number.
/// @param end an upper bound (inclusive) to the random number.
/// @return the random number generated.
EXPORTC double random2(int32_t start, int32_t end)
{
    int result = randomGenerator().bounded(start, end + 1);
    return (double)result;
}

/// Set the seed for the random number generator
/// @param seed the seed.
/// @return nothing.
EXPORTC addr_t setRandomWithSeed(int32_t seed)
{
    randomGenerator().seed(seed);
    return nullptr;
}

/// Set the seed for the random number generator using the system seed
/// @return nothing.
EXPORTC addr_t setRandom()
{
    QRandomGenerator *s = QRandomGenerator::system();
    quint32 seed = s->generate();
    randomGenerator().seed(seed);
    return nullptr;
}

/// Generate a Word(string) from a number that is formatted according to the
/// other parameters.
/// @param eAddr a pointer to the Evaluator object
/// @param num the number to apply formatting to.
/// @param width the minimum number of characters to use. Spaces may be added.
/// @param precision the number of digits to add after the decimal point.
/// @return A Word(string) with formatting applied.
EXPORTC addr_t getFormForNumber(addr_t eAddr, double num, uint32_t width, int32_t precision)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QString retval = QString("%1").arg(num, width, 'f', precision);
    auto *w = new Word(retval);
    e->watch(w);

    return reinterpret_cast<addr_t>(w);
}

/// @brief return the address of the repcount variable.
EXPORTC addr_t repcountAddr(void)
{
    void *retval = &Config::get().mainKernel()->callStack.repcount;
    return (addr_t)retval;
}

EXPORTC addr_t beginCatch(void)
{
    auto *erractWord = reinterpret_cast<Word *>(Config::get().mainKernel()->specialVar(SpecialNames::ERRACT));
    Datum *erractValue =
        Config::get().mainKernel()->callStack.datumForName(erractWord->toString(Datum::ToStringFlags_Key)).datumValue();

    // Save the erract value.
    if (erractValue->isa != Datum::typeNothing)
    {
        erractValue->retainCount++;
        Config::get().mainKernel()->callStack.setDatumForName(nothing(),
                                                              erractWord->toString(Datum::ToStringFlags_Key));
    }
    return reinterpret_cast<addr_t>(erractValue);
}

EXPORTC addr_t endCatch(addr_t eAddr, addr_t nodeAddr, addr_t errActAddr, addr_t resultAddr, addr_t tagAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *erractWord = reinterpret_cast<Word *>(Config::get().mainKernel()->specialVar(SpecialNames::ERRACT));
    auto *erractValue = reinterpret_cast<Datum *>(errActAddr);
    auto *result = reinterpret_cast<Datum *>(resultAddr);
    auto *tag = reinterpret_cast<Word *>(tagAddr);

    // Restore the erract value.
    if (erractValue->isa != Datum::typeNothing)
    {
        DatumPtr erractValuePtr = DatumPtr(erractValue);
        Config::get().mainKernel()->callStack.setDatumForName(erractValuePtr,
                                                              erractWord->toString(Datum::ToStringFlags_Key));
        erractValue->retainCount--;
    }

    if (result->isa == Datum::typeError)
    {
        auto *err = reinterpret_cast<FCError *>(result);
        QString tagStr = tag->toString(Datum::ToStringFlags_Key);

        if ((tagStr == QObject::tr("ERROR")) &&
            ((err->code == ErrCode::ERR_NO_CATCH) &&
                 (err->tag().toString(Datum::ToStringFlags_Key) == QObject::tr("ERROR")) ||
             (err->code != ErrCode::ERR_NO_CATCH)))
        {
            e->watch(err);
            return nodeAddr;
        }
        else if ((err->code == ErrCode::ERR_NO_CATCH) && (err->tag().toString(Datum::ToStringFlags_Key) == tagStr))
        {
            e->watch(err);
            auto retval = reinterpret_cast<addr_t>(err->output().datumValue());
            Config::get().mainKernel()->currentError = nothing();
            return retval;
        }
        return resultAddr;
    }

    return reinterpret_cast<addr_t>(result);
}

EXPORTC addr_t getCurrentError(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr errPtr = Config::get().mainKernel()->currentError;

    ListBuilder retvalBuilder;
    if (!errPtr.isNothing())
    {
        auto *err = reinterpret_cast<FCError *>(errPtr.datumValue());
        retvalBuilder.append(DatumPtr(err->code));
        retvalBuilder.append(err->message());
        retvalBuilder.append(err->procedure());
        retvalBuilder.append(err->line());
    }
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t callPause(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *retval = Config::get().mainKernel()->pause().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t generateContinue(addr_t eAddr, addr_t outputAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *output = reinterpret_cast<Datum *>(outputAddr);

    FCError *err = FCError::custom(DatumPtr(QObject::tr("PAUSE")), nothing(), DatumPtr(output));
    e->watch(err);
    return reinterpret_cast<addr_t>(err);
}

EXPORTC addr_t processRunresult(addr_t eAddr, addr_t resultAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *result = reinterpret_cast<Datum *>(resultAddr);
    Datum *retval;

    if ((result->isa & Datum::typeDataMask) != 0)
    {
        retval = new List(result, EmptyList::instance());
    }
    else if ((result->isa & Datum::typeUnboundMask) != 0)
    {
        retval = EmptyList::instance();
    }
    else
    {
        // Pass through whatever we got because it's not good.
        return resultAddr;
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC void saveTestResult(bool tf)
{
    Config::get().mainKernel()->callStack.setTest(tf);
}

EXPORTC bool getIsTested(void)
{
    return Config::get().mainKernel()->callStack.isTested();
}

EXPORTC bool getTestResult(void)
{
    return Config::get().mainKernel()->callStack.testedState();
}

/// Compare a Datum with a bool.
/// @param d a Datum
/// @param b a bool
/// @returns true iff d is a bool that equals b
EXPORTC bool cmpDatumToBool(addr_t d, bool b)
{
    auto *dD = reinterpret_cast<Datum *>(d);
    if (dD->isa != Datum::typeWord)
        return false;
    auto *dW = reinterpret_cast<Word *>(dD);
    bool dB = dW->boolValue();
    if (!dW->boolIsValid)
        return false;
    return dB == b;
}

/// Compare a Datum with a double.
/// @param d a Datum
/// @param n a number
/// @returns true iff d is a number that equals n
EXPORTC bool cmpDatumToDouble(addr_t d, double n)
{
    auto *dD = reinterpret_cast<Datum *>(d);
    if (dD->isa != Datum::typeWord)
        return false;
    auto *dW = reinterpret_cast<Word *>(dD);
    double dN = dW->numberValue();
    if (!dW->numberIsValid)
        return false;
    return dN == n;
}

/// Compare a Datum with a Datum.
/// @param eAddr The address of the Evaluator.
/// @param d1 a Datum
/// @param d2 a Datum
/// @returns true iff d1 is equal to d2 according to the help text of EQUALP
EXPORTC bool cmpDatumToDatum(addr_t eAddr, addr_t d1, addr_t d2)
{
    auto *dD1 = reinterpret_cast<Datum *>(d1);
    auto *dD2 = reinterpret_cast<Datum *>(d2);
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    Qt::CaseSensitivity cs = e->varCASEIGNOREDP() ? Qt::CaseInsensitive : Qt::CaseSensitive;
    VisitedMap visited;
    return areDatumsEqual(visited, dD1, dD2, cs);
}

EXPORTC addr_t concatWord(addr_t eAddr, addr_t aryAddr, uint32_t count)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto **wordAry = reinterpret_cast<Word **>(aryAddr);
    QString retval = "";
    for (uint32_t i = 0; i < count; ++i)
    {
        Word *w = *(wordAry + i);
        retval += w->toString(Datum::ToStringFlags_Raw);
    }
    return reinterpret_cast<addr_t>(e->watch(new Word(retval)));
}

EXPORTC bool isDatumEmpty(addr_t /* eAddr */, addr_t dAddr)
{
    auto *d = reinterpret_cast<Datum *>(dAddr);
    if (d->isWord())
    {
        return d->toString(Datum::ToStringFlags_Raw).isEmpty();
    }
    else if (d->isList())
    {
        return d->listValue()->isEmpty();
    }
    else if (d->isArray())
    {
        return false;
    }
    else
    {
        Q_ASSERT(false);
    }
    return false;
}

EXPORTC addr_t createList(addr_t eAddr, addr_t aryAddr, uint32_t count)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto **ary = reinterpret_cast<Datum **>(aryAddr);
    ListBuilder builder;
    for (uint32_t i = 0; i < count; ++i)
    {
        DatumPtr d = DatumPtr(ary[i]);
        builder.append(d);
    }
    DatumPtr retval = builder.finishedList();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC addr_t createSentence(addr_t eAddr, addr_t aryAddr, uint32_t count)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto **ary = reinterpret_cast<Datum **>(aryAddr);
    ListBuilder builder;
    for (uint32_t i = 0; i < count; ++i)
    {
        DatumPtr d = DatumPtr(ary[i]);
        if (d.isList())
        {
            ListIterator it = d.listValue()->newIterator();
            while (it.elementExists())
            {
                builder.append(it.element());
            }
        }
        else
        {
            builder.append(d);
        }
    }
    DatumPtr retval = builder.finishedList();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC addr_t fputList(addr_t eAddr, addr_t thingAddr, addr_t listAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    auto *list = reinterpret_cast<List *>(listAddr);

    auto *retval = new List(DatumPtr(thing), list);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t lputList(addr_t eAddr, addr_t thingAddr, addr_t listAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    auto *list = reinterpret_cast<List *>(listAddr);

    ListBuilder builder;
    ListIterator it = list->newIterator();

    while (it.elementExists())
    {
        builder.append(it.element());
    }
    builder.append(DatumPtr(thing));

    DatumPtr retval = builder.finishedList();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC addr_t createArray(addr_t eAddr, int32_t size, int32_t origin)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *retval = new Array(origin, size);
    for (int i = 0; i < size; ++i)
    {
        retval->array.append(emptyList());
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t listToArray(addr_t eAddr, addr_t listAddr, int32_t origin)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *list = reinterpret_cast<List *>(listAddr);
    auto *retval = new Array(origin, list->count());
    ListIterator it = list->newIterator();
    while (it.elementExists())
    {
        retval->array.append(it.element());
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t arrayToList(addr_t eAddr, addr_t arrayAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    const auto *array = reinterpret_cast<const Array *>(arrayAddr);
    ListBuilder builder;
    for (const auto &i : array->array)
    {
        builder.append(i);
    }
    DatumPtr retval = builder.finishedList();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC addr_t firstOfDatum(addr_t eAddr, addr_t thingAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    Datum *retval = nullptr;
    if (thing->isWord())
    {
        Word *w = thing->wordValue();
        retval = new Word(w->toString(Datum::ToStringFlags_Raw).left(1));
    }
    else if (thing->isList())
    {
        List *l = thing->listValue();
        retval = l->head.datumValue();
    }
    else if (thing->isArray())
    {
        Array *a = thing->arrayValue();
        retval = new Word(QString::number(a->origin));
    }
    else
    {
        Q_ASSERT(false);
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t lastOfDatum(addr_t eAddr, addr_t thingAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    Datum *retval = nullptr;
    if (thing->isWord())
    {
        Word *w = thing->wordValue();
        retval = new Word(w->toString(Datum::ToStringFlags_Raw).right(1));
    }
    else if (thing->isList())
    {
        List *l = thing->listValue();
        ListIterator iter = l->newIterator();
        DatumPtr lastElement;
        while (iter.elementExists())
        {
            lastElement = iter.element();
        }
        retval = lastElement.datumValue();
    }
    else if (thing->isArray())
    {
        Array *a = thing->arrayValue();
        retval = new Word(a->origin);
    }
    else
    {
        Q_ASSERT(false);
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t butFirstOfDatum(addr_t eAddr, addr_t thingAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    Datum *retval = nullptr;
    if (thing->isWord())
    {
        Word *w = thing->wordValue();
        QString rawValue = w->toString(Datum::ToStringFlags_Raw);
        retval = new Word(rawValue.sliced(1, rawValue.size() - 1));
    }
    else if (thing->isList())
    {
        List *l = thing->listValue();
        retval = l->tail.datumValue();
    }
    else
    {
        Q_ASSERT(false);
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t butLastOfDatum(addr_t eAddr, addr_t thingAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);

    if (thing->isWord())
    {
        Word *w = thing->wordValue();
        QString rawValue = w->toString(Datum::ToStringFlags_Raw);
        Datum *retval = new Word(rawValue.sliced(0, rawValue.size() - 1));
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval);
    }
    else if (thing->isList())
    {
        List *l = thing->listValue();
        ListIterator iter = l->newIterator();
        ListBuilder builder;
        while (iter.elementExists())
        {
            DatumPtr element = iter.element();
            if (iter.elementExists())
            {
                builder.append(element);
            }
        }
        DatumPtr retval = builder.finishedList();
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval.datumValue());
    }
    else
    {
        Q_ASSERT(false);
    }
    Q_ASSERT(false);
    return nullptr;
}

EXPORTC bool isDatumIndexValid(addr_t /* eAddr */, addr_t thingAddr, double dIndex, addr_t listItemPtrAddr)
{
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    auto **listItemPtr = reinterpret_cast<Datum **>(listItemPtrAddr);
    auto index = static_cast<qsizetype>(dIndex);
    if (index != dIndex)
        return false;

    if (thing->isWord())
    {
        Word *w = thing->wordValue();
        QString rawValue = w->toString(Datum::ToStringFlags_Raw);
        return (index >= 1) && (index <= rawValue.size());
    }
    else if (thing->isList())
    {
        List *l = thing->listValue();
        if (index < 1)
            return false;
        ListIterator iter = l->newIterator();
        while (iter.elementExists())
        {
            *listItemPtr = iter.element().datumValue();
            index--;
            if (index == 0)
                return true;
        }
        return false;
    }
    else if (thing->isArray())
    {
        Array *a = thing->arrayValue();
        auto size = static_cast<int32_t>(a->array.size());
        index = index - a->origin;
        return (index >= 0) && (index < size);
    }
    else
    {
        Q_ASSERT(false);
    }
    return false;
}

EXPORTC addr_t itemOfDatum(addr_t eAddr, addr_t thingAddr, double dIndex, addr_t listItemPtrAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    Datum *retval = nullptr;
    auto index = static_cast<qsizetype>(dIndex);

    if (thing->isWord())
    {
        Word *w = thing->wordValue();
        QString rawValue = w->toString(Datum::ToStringFlags_Raw);
        retval = new Word(rawValue[index - 1]);
    }
    else if (thing->isList())
    {
        auto **retvalPtr = reinterpret_cast<Datum **>(listItemPtrAddr);
        retval = *retvalPtr;
    }
    else if (thing->isArray())
    {
        Array *a = thing->arrayValue();
        index = index - a->origin;
        retval = a->array[index].datumValue();
    }
    else
    {
        Q_ASSERT(false);
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC bool isDatumContainerOrInContainer(addr_t eAddr, addr_t valueAddr, addr_t containerAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *value = reinterpret_cast<Datum *>(valueAddr);
    auto *container = reinterpret_cast<Datum *>(containerAddr);

    // If not a container then there's no container to search.
    if (container->isa == Datum::typeWord)
        return false;

    // If value and container are the same then it's in the container.
    if (value == container)
        return true;

    Qt::CaseSensitivity cs = e->varCASEIGNOREDP() ? Qt::CaseInsensitive : Qt::CaseSensitive;
    VisitedSet visited;

    return isDatumInContainer(visited, value, container, cs);
}

EXPORTC void setDatumAtIndexOfContainer(addr_t /* eAddr */, addr_t valueAddr, double dIndex, addr_t containerAddr)
{
    auto *container = reinterpret_cast<Datum *>(containerAddr);
    DatumPtr value(reinterpret_cast<Datum *>(valueAddr));
    auto index = static_cast<qsizetype>(dIndex);

    if (container->isList())
    {
        List *l = container->listValue();
        for (qsizetype i = 1; i < index; ++i)
        {
            l = l->tail.listValue();
        }
        l->head = value;
    }
    else if (container->isArray())
    {
        Array *a = container->arrayValue();
        a->array[index - a->origin] = value;
    }
    else
    {
        Q_ASSERT(false);
    }
}

EXPORTC void setFirstOfList(addr_t /* eAddr */, addr_t listAddr, addr_t valueAddr)
{
    auto *l = reinterpret_cast<List *>(listAddr);
    l->head = DatumPtr(reinterpret_cast<Datum *>(valueAddr));
    l->astParseTimeStamp = 0;
}

EXPORTC void setButfirstOfList(addr_t /* eAddr */, addr_t listAddr, addr_t valueAddr)
{
    auto *l = reinterpret_cast<List *>(listAddr);
    l->tail = DatumPtr(reinterpret_cast<Datum *>(valueAddr));
    l->astParseTimeStamp = 0;
}

EXPORTC bool isEmpty(addr_t /* eAddr */, addr_t thingAddr)
{
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    if (thing->isWord())
    {
        Word *word = thing->wordValue();
        return word->toString(Datum::ToStringFlags_Raw).isEmpty();
    }
    else if (thing->isList())
    {
        List *list = thing->listValue();
        return list->isEmpty();
    }
    return false;
}

EXPORTC bool isBefore(addr_t eAddr, addr_t word1Addr, addr_t word2Addr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *word1 = reinterpret_cast<Word *>(word1Addr);
    auto *word2 = reinterpret_cast<Word *>(word2Addr);

    Qt::CaseSensitivity cs = e->varCASEIGNOREDP() ? Qt::CaseInsensitive : Qt::CaseSensitive;

    QString value1 = word1->toString();
    QString value2 = word2->toString();
    return value1.compare(value2, cs) < 0;
}

EXPORTC bool isMember(addr_t eAddr, addr_t thingAddr, addr_t containerAddr)
{
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    auto *container = reinterpret_cast<Datum *>(containerAddr);

    if (container->isWord())
    {
        Word *word = container->wordValue();
        QString containerString = word->toString(Datum::ToStringFlags_Key);
        if (thing->isWord())
        {
            Word *word2 = thing->wordValue();
            QString thingString = word2->toString(Datum::ToStringFlags_Key);
            if (thingString.length() != 1)
            {
                return false;
            }
            return containerString.contains(thingString);
        }
        return false;
    }
    else if (container->isList())
    {
        List *list = container->listValue();
        ListIterator iter(list);
        while (iter.elementExists())
        {
            Datum *item = iter.element().datumValue();
            auto itemAddr = reinterpret_cast<addr_t>(item);
            if (cmpDatumToDatum(eAddr, thingAddr, itemAddr))
            {
                return true;
            }
        }
        return false;
    }
    else if (container->isArray())
    {
        const Array *array = container->arrayValue();
        return std::any_of(array->array.begin(), array->array.end(), [eAddr, thingAddr](const auto &item) {
            return cmpDatumToDatum(eAddr, thingAddr, reinterpret_cast<addr_t>(item.datumValue()));
        });
    }
    else
    {
        Q_ASSERT(false);
    }
    Q_ASSERT(false);
    return false;
}

EXPORTC bool isSubstring(addr_t /* eAddr */, addr_t thing1Addr, addr_t thing2Addr)
{
    auto *thing1 = reinterpret_cast<Datum *>(thing1Addr);
    auto *thing2 = reinterpret_cast<Datum *>(thing2Addr);

    if (thing1->isa == Datum::typeWord && thing2->isa == Datum::typeWord)
    {
        auto *word1 = reinterpret_cast<Word *>(thing1);
        auto *word2 = reinterpret_cast<Word *>(thing2);
        QString string1 = word1->toString(Datum::ToStringFlags_Key);
        QString string2 = word2->toString(Datum::ToStringFlags_Key);
        return string2.contains(string1);
    }
    return false;
}

EXPORTC bool isNumber(addr_t /* eAddr */, addr_t thingAddr)
{
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    if (thing->isa != Datum::typeWord)
    {
        return false;
    }
    Word *word = reinterpret_cast<Word *>(thing);
    word->numberValue();
    return word->numberIsValid;
}

EXPORTC bool isSingleCharWord(addr_t /* eAddr */, addr_t candidateAddr)
{
    auto *candidate = reinterpret_cast<Datum *>(candidateAddr);
    if (candidate->isa != Datum::typeWord)
    {
        return false;
    }
    auto *word = reinterpret_cast<Word *>(candidate);
    return word->toString(Datum::ToStringFlags_Key).length() == 1;
}

EXPORTC bool isVbarred(addr_t /* eAddr */, addr_t cAddr)
{
    auto *word = reinterpret_cast<Word *>(cAddr);

    // A character is vbarred IFF it's print value is different from its raw value.
    char16_t rawC = word->toString(Datum::ToStringFlags_Raw).front().unicode();
    char16_t c = word->toString().front().unicode();
    return rawC != c;
}

EXPORTC double datumCount(addr_t /* eAddr */, addr_t thingAddr)
{
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    if (thing->isWord())
    {
        Word *word = thing->wordValue();
        return word->toString(Datum::ToStringFlags_Raw).length();
    }
    else if (thing->isList())
    {
        List *list = thing->listValue();
        return list->count();
    }
    else if (thing->isArray())
    {
        Array *array = thing->arrayValue();
        return array->array.size();
    }
    Q_ASSERT(false);
    return 0;
}

EXPORTC double ascii(addr_t /* eAddr */, addr_t cAddr)
{
    auto *word = reinterpret_cast<Word *>(cAddr);
    return word->toString().front().unicode();
}

EXPORTC double rawascii(addr_t /* eAddr */, addr_t cAddr)
{
    auto *word = reinterpret_cast<Word *>(cAddr);
    return word->toString(Datum::ToStringFlags_Raw).front().unicode();
}

EXPORTC addr_t chr(addr_t eAddr, uint32_t c)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto qstr = QString(QChar(static_cast<uint16_t>(c)));
    auto *retval = new Word(qstr);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t member(addr_t eAddr, addr_t thing1Addr, addr_t thing2Addr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing1 = reinterpret_cast<Datum *>(thing1Addr);
    auto *thing2 = reinterpret_cast<Datum *>(thing2Addr);
    if (thing2->isWord())
    {
        Word *word1 = thing1->wordValue();
        Word *word2 = thing2->wordValue();
        QString retval = "";
        if (thing1->isWord())
        {
            QString thing2Str = word2->toString(Datum::ToStringFlags_Raw);
            QString thing1Str = word1->toString(Datum::ToStringFlags_Raw);
            if (!thing1Str.isEmpty())
            {
                int index = thing2Str.indexOf(thing1Str);
                if (index != -1)
                {
                    retval = thing2Str.sliced(index);
                }
            }
        }
        Word *retvalWord = new Word(retval);
        e->watch(retvalWord);
        return reinterpret_cast<addr_t>(retvalWord);
    }
    else if (thing2->isList())
    {
        List *list = thing2->listValue();
        while (!list->isEmpty())
        {
            auto listAddr = reinterpret_cast<addr_t>(list->head.datumValue());
            if (cmpDatumToDatum(eAddr, listAddr, thing1Addr))
            {
                return reinterpret_cast<addr_t>(list);
            }
            list = list->tail.listValue();
        }
        // If we get here, thing1 was not found in thing2.
        // Return an empty list.
        List *retval = EmptyList::instance();
        e->watch(retval);
        return reinterpret_cast<addr_t>(retval);
    }
    else
    {
        Q_ASSERT(false);
    }
    Q_ASSERT(false);
    return nullptr;
}

EXPORTC addr_t lowercase(addr_t eAddr, addr_t wordAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *word = reinterpret_cast<Word *>(wordAddr);
    QString retval = word->toString(Datum::ToStringFlags_Raw).toLower();
    auto *retvalWord = new Word(retval);
    e->watch(retvalWord);
    return reinterpret_cast<addr_t>(retvalWord);
}

EXPORTC addr_t uppercase(addr_t eAddr, addr_t wordAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *word = reinterpret_cast<Word *>(wordAddr);
    QString retval = word->toString(Datum::ToStringFlags_Raw).toUpper();
    auto *retvalWord = new Word(retval);
    e->watch(retvalWord);
    return reinterpret_cast<addr_t>(retvalWord);
}

EXPORTC addr_t standout(addr_t eAddr, addr_t thingAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *thing = reinterpret_cast<Datum *>(thingAddr);
    QString phrase = thing->toString();
    QString retval = Config::get().mainController()->addStandoutToString(phrase);
    auto *retvalWord = new Word(retval);
    e->watch(retvalWord);
    return reinterpret_cast<addr_t>(retvalWord);
}

EXPORTC addr_t parse(addr_t eAddr, addr_t wordAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *word = reinterpret_cast<Word *>(wordAddr);
    QString phrase = word->toString(Datum::ToStringFlags_Raw);
    QTextStream stream(&phrase, QIODevice::ReadOnly);
    TextStream ts(&stream);
    DatumPtr retvalPtr = ts.readlistWithPrompt("", false);
    List *retval = retvalPtr.listValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t runparseDatum(addr_t eAddr, addr_t wordorlistAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto wordorlist = DatumPtr(reinterpret_cast<Datum *>(wordorlistAddr));

    DatumPtr retvalPtr = runparse(wordorlist);
    List *retval = retvalPtr.listValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC void moveTurtleForward(addr_t /* eAddr */, double distance)
{
    Config::get().mainTurtle()->forward(distance);
}

EXPORTC void moveTurtleRotate(addr_t /* eAddr */, double angle)
{
    Config::get().mainTurtle()->rotate(angle);
}

EXPORTC void setTurtleXY(addr_t /* eAddr */, double x, double y)
{
    Config::get().mainTurtle()->setxy(x, y);
}

EXPORTC void setTurtleX(addr_t /* eAddr */, double x)
{
    Config::get().mainTurtle()->setx(x);
}

EXPORTC void setTurtleY(addr_t /* eAddr */, double y)
{
    Config::get().mainTurtle()->sety(y);
}

EXPORTC void setTurtlePos(addr_t /* eAddr */, addr_t posAddr)
{
    const auto *pos = reinterpret_cast<const double *>(posAddr);
    double x = pos[0];
    double y = pos[1];
    Config::get().mainTurtle()->setxy(x, y);
}

EXPORTC void setTurtleHeading(addr_t /* eAddr */, double newHeading)
{
    double oldHeading = Config::get().mainTurtle()->getHeading();

    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    newHeading = 360 - newHeading;

    double adjustment = oldHeading - newHeading;
    Config::get().mainTurtle()->rotate(adjustment);
}

EXPORTC void setTurtleMoveToHome(addr_t /* eAddr */)
{
    Config::get().mainTurtle()->moveToHome();
}

EXPORTC void drawTurtleArc(addr_t /* eAddr */, double angle, double radius)
{
    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    angle = 0 - angle;

    if ((angle < -360) || (angle > 360))
        angle = 360;

    if ((angle != 0) && (radius != 0))
        Config::get().mainTurtle()->drawArc(angle, radius);
}

EXPORTC addr_t getTurtlePos(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    double x = 0, y = 0;
    Config::get().mainTurtle()->getxy(x, y);
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(x));
    retvalBuilder.append(DatumPtr(y));
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC double getTurtleHeading(addr_t /* eAddr */)
{
    double retval = Config::get().mainTurtle()->getHeading();

    // Heading should only show two decimal places.
    retval = round(retval * 100.0) / 100.0;

    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    if (retval > 0)
        retval = 360 - retval;

    return retval;
}

EXPORTC double getTurtleTowards(addr_t /* eAddr */, addr_t posAddr)
{
    double x = 0, y = 0;
    Config::get().mainTurtle()->getxy(x, y);
    const auto *pos = reinterpret_cast<const double *>(posAddr);
    double vx = pos[0];
    double vy = pos[1];
    double retval = atan2(x - vx, vy - y) * (180 / PI);

    // Heading should only show two decimal places.
    retval = round(retval * 100.0) / 100.0;

    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    retval = 0 - retval;
    if (retval < 0)
        retval = 360 + retval;

    return retval;
}

EXPORTC addr_t getScrunch(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(1));
    retvalBuilder.append(DatumPtr(1));
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC void setTurtleVisible(addr_t /* eAddr */, int visible)
{
    Config::get().mainTurtle()->setIsTurtleVisible(visible);
}

EXPORTC void clean(addr_t /* eAddr */)
{
    Config::get().mainController()->clearCanvas();
}

EXPORTC void setTurtleMode(addr_t /* eAddr */, int mode)
{
    auto newMode = static_cast<TurtleModeEnum>(mode);
    if (Config::get().mainTurtle()->getMode() != newMode)
    {
        bool isCanvasBounded = (newMode == turtleWindow);
        Config::get().mainTurtle()->setMode(newMode);
        Config::get().mainController()->setIsCanvasBounded(isCanvasBounded);
    }
}

EXPORTC addr_t getBounds(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    double x = Config::get().mainController()->boundX();
    double y = Config::get().mainController()->boundY();

    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(x));
    retvalBuilder.append(DatumPtr(y));
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC void setBounds(addr_t /* eAddr */, double x, double y)
{
    Config::get().mainController()->setBounds(x, y);
}

EXPORTC int32_t beginFilledWithColor(addr_t /* eAddr */, addr_t colorAddr)
{
    auto *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Config::get().mainKernel()->colorFromDatumPtr(color, DatumPtr(d)))
        return 0;
    Config::get().mainTurtle()->beginFillWithColor(color);
    return 1;
}

EXPORTC void endFilled(addr_t /* eAddr */)
{
    Config::get().mainTurtle()->endFill();
}

EXPORTC void addLabel(addr_t /* eAddr */, addr_t textAddr)
{
    auto *d = reinterpret_cast<Datum *>(textAddr);
    Config::get().mainController()->drawLabel(d->toString());
}

EXPORTC void setLabelHeight(addr_t /* eAddr */, double height)
{
    Config::get().mainController()->setLabelFontSize(height);
}

EXPORTC void setScreenMode(addr_t /* eAddr */, int mode)
{
    Config::get().mainController()->setScreenMode(static_cast<ScreenModeEnum>(mode));
}

EXPORTC bool isTurtleVisible(addr_t /* eAddr */)
{
    return Config::get().mainTurtle()->isTurtleVisible();
}

EXPORTC addr_t getScreenMode(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    ScreenModeEnum mode = Config::get().mainController()->getScreenMode();
    QString modeStr;
    switch (mode)
    {
    case textScreenMode:
    case initScreenMode:
        modeStr = QObject::tr("textscreen");
        break;
    case splitScreenMode:
        modeStr = QObject::tr("splitscreen");
        break;
    case fullScreenMode:
        modeStr = QObject::tr("fullscreen");
        break;
    }
    auto *retval = new Word(modeStr);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t getTurtleMode(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    TurtleModeEnum mode = Config::get().mainTurtle()->getMode();
    QString modeStr;
    switch (mode)
    {
    case turtleWrap:
        modeStr = QObject::tr("wrap");
        break;
    case turtleFence:
        modeStr = QObject::tr("fence");
        break;
    case turtleWindow:
        modeStr = QObject::tr("window");
        break;
    }
    auto *retval = new Word(modeStr);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t getLabelSize(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    double height = Config::get().mainController()->getLabelFontSize();
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(height));
    retvalBuilder.append(DatumPtr(height));
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC void setPenIsDown(addr_t /* eAddr */, bool isDown)
{
    Config::get().mainTurtle()->setPenIsDown(isDown);
}

EXPORTC void setPenMode(addr_t /* eAddr */, int32_t mode)
{
    Config::get().mainTurtle()->setPenMode(static_cast<PenModeEnum>(mode));
}

EXPORTC bool setPenColor(addr_t /* eAddr */, addr_t colorAddr)
{
    auto *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Config::get().mainKernel()->colorFromDatumPtr(color, DatumPtr(d)))
        return false;
    Config::get().mainTurtle()->setPenColor(color);
    return true;
}

EXPORTC addr_t getAllColors(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    ListBuilder lb;
    QStringList colors = QColor::colorNames();
    for (const QString &i : colors)
    {
        lb.append(DatumPtr(new Word(i)));
    }
    DatumPtr retval = lb.finishedList();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC bool isColorIndexGood(addr_t /* eAddr */, addr_t colorIndexAddr, double lowerLimit)
{
    auto *w = reinterpret_cast<Word *>(colorIndexAddr);
    double colorIndex = w->numberValue();

    return (w->numberIsValid) && (colorIndex == floor(colorIndex)) && (colorIndex >= lowerLimit) &&
           (colorIndex < Config::get().mainKernel()->palette.size());
}

EXPORTC bool setPalette(addr_t /* eAddr */, addr_t colorIndexAddr, addr_t colorAddr)
{
    auto colorIndex = static_cast<int>((reinterpret_cast<Word *>(colorIndexAddr))->numberValue());
    auto *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Config::get().mainKernel()->colorFromDatumPtr(color, DatumPtr(d)))
        return false;
    Config::get().mainKernel()->palette[colorIndex] = color;
    return true;
}

EXPORTC void setPenSize(addr_t /* eAddr */, double size)
{
    Config::get().mainTurtle()->setPenSize(size);
}

EXPORTC bool setBackground(addr_t /* eAddr */, addr_t colorAddr)
{
    auto *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Config::get().mainKernel()->colorFromDatumPtr(color, DatumPtr(d)))
        return false;
    Config::get().mainController()->setCanvasBackgroundColor(color);
    return true;
}

EXPORTC bool isPenDown(addr_t /* eAddr */)
{
    return Config::get().mainTurtle()->isPenDown();
}

EXPORTC addr_t getPenMode(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    PenModeEnum pm = Config::get().mainTurtle()->getPenMode();
    QString retval;
    switch (pm)
    {
    case penModePaint:
        retval = QObject::tr("paint");
        break;
    case penModeReverse:
        retval = QObject::tr("reverse");
        break;
    case penModeErase:
        retval = QObject::tr("erase");
        break;
    }
    auto *w = new Word(retval);
    e->watch(w);
    return reinterpret_cast<addr_t>(w);
}

EXPORTC addr_t getPenColor(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    const QColor &color = Config::get().mainTurtle()->getPenColor();
    List *retval = listFromColor(color);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t getPaletteColor(addr_t eAddr, addr_t colorIndexAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto colorIndex = static_cast<int>((reinterpret_cast<Word *>(colorIndexAddr))->numberValue());
    const QColor &color = Config::get().mainKernel()->palette[colorIndex];
    List *retval = listFromColor(color);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC double getPenSize(addr_t /* eAddr */)
{
    return Config::get().mainTurtle()->getPenSize();
}

EXPORTC addr_t getBackground(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    const QColor &color = Config::get().mainController()->getCanvasBackgroundColor();
    List *retval = listFromColor(color);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t savePict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QString filename = reinterpret_cast<Word *>(filenameAddr)->toString();
    QString filepath = Config::get().mainKernel()->filepathForFilename(DatumPtr(filename));
    QImage image = Config::get().mainController()->getCanvasImage();
    bool isSuccessful = image.save(filepath);
    auto *retval = reinterpret_cast<Datum *>(nodeAddr);
    if (!isSuccessful)
    {
        retval = FCError::fileSystem();
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t saveSvgpict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QString filename = reinterpret_cast<Word *>(filenameAddr)->toString();
    QString filepath = Config::get().mainKernel()->filepathForFilename(DatumPtr(filename));
    QByteArray svgImage = Config::get().mainController()->getSvgImage();

    auto *retval = reinterpret_cast<Datum *>(nodeAddr);
    QFile file(filepath);
    bool isSuccessful = file.open(QIODevice::WriteOnly);
    if (!isSuccessful)
    {
        retval = FCError::fileSystem();
    }

    qint64 bytesWritten = file.write(svgImage);
    if (bytesWritten != svgImage.size())
        retval = FCError::fileSystem();

    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t loadPict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *dFilename = reinterpret_cast<Datum *>(filenameAddr);
    auto *retval = reinterpret_cast<Datum *>(nodeAddr);
    if (dFilename->isa == Datum::typeWord)
    {
        QString filename = reinterpret_cast<Word *>(filenameAddr)->toString();
        QString filepath = Config::get().mainKernel()->filepathForFilename(DatumPtr(filename));
        QImage image = QImage(filepath);
        if (image.isNull())
        {
            retval = FCError::fileSystem();
        }
        Config::get().mainController()->setCanvasBackgroundImage(image);
        goto done;
    }
    if (dFilename->isList())
    {
        if (dFilename->listValue()->isEmpty())
        {
            Config::get().mainController()->setCanvasBackgroundImage(QImage());
            goto done;
        }
    }
    retval = FCError::doesntLike(DatumPtr(reinterpret_cast<ASTNode *>(nodeAddr)->nodeName), DatumPtr(dFilename));
done:
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t getMousePos(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QVector2D position = Config::get().mainController()->mousePosition();
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(position.x()));
    retvalBuilder.append(DatumPtr(position.y()));
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t getClickPos(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QVector2D position = Config::get().mainController()->lastMouseclickPosition();
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(position.x()));
    retvalBuilder.append(DatumPtr(position.y()));
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC bool isMouseButtonDown(addr_t /* eAddr */)
{
    return Config::get().mainController()->getIsMouseButtonDown();
}

EXPORTC double getMouseButton(addr_t /* eAddr */)
{
    return static_cast<double>(Config::get().mainController()->getAndResetButtonID());
}

/// @brief Get the value of the ERRORACT variable.
/// @param eAddr the address of the evaluator.
/// @return the value of the ERRORACT variable as a boolean.
/// @note In QLogo, ERRACT is a pseudo-boolean variable. For compatibility with
/// UCBLogo, we accept any word or list. However, in QLogo, ERRACT is considered TRUE
/// only if the following conditions are met:
/// 1. The value EXISTS, and:
/// 2 a. The value is a word AND the word is not "FALSE" or the empty string, or
/// 2 b. The value is a list AND the list is not empty.
EXPORTC bool getvarErroract(addr_t /* eAddr */)
{
    QString name = QObject::tr("ERRACT");
    DatumPtr val = Config::get().mainKernel()->callStack.datumForName(name);
    if (val.isWord())
    {
        QString word = val.toString(Datum::ToStringFlags_Key);
        return (word != "FALSE") && (word != "");
    }
    if (val.isList())
    {
        return !val.listValue()->isEmpty();
    }
    return false;
}

/// @brief input a procedure using the system read stream.
/// @param eAddr a pointer to the Evaluator object
/// @param nodeAddr a pointer to the node calling this procedure.
/// @return ASTNode on success, else Err.
EXPORTC addr_t inputProcedure(addr_t eAddr, addr_t nodeAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *node = reinterpret_cast<ASTNode *>(nodeAddr);
    const CallFrame *currentFrame = Config::get().mainKernel()->callStack.localFrame();
    DatumPtr currentProc = currentFrame->sourceNode;
    if (currentProc.isASTNode())
    {
        FCError *err = FCError::toInProc(node->nodeName);
        e->watch(err);
        return reinterpret_cast<addr_t>(err);
    }

    return reinterpret_cast<addr_t>(Config::get().mainKernel()->inputProcedure(node));
}

// TODO: Should the executor be passed in here instead of getting the local frame from the call stack?
EXPORTC void setVarAsLocal(addr_t varname)
{
    auto *varName = reinterpret_cast<Word *>(varname);
    QString varNameStr = varName->toString(Datum::ToStringFlags_Key);
    CallFrame *currentFrame = Config::get().mainKernel()->callStack.localFrame();
    currentFrame->setVarAsLocal(varNameStr);
}
