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

#include "compiler_private.h"
#include "flowcontrol.h"
#include "astnode.h"
#include "workspace/callframe.h"
#include "sharedconstants.h"
#include "kernel.h"
#include <QObject>

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
    Word *w = reinterpret_cast<Word *>(datumAddr);
    if(w->isa == Datum::typeWord) {
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
    Word *w = reinterpret_cast<Word *>(datumAddr);
    if(w->isa == Datum::typeWord) {
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
    Word *w = reinterpret_cast<Word *>(datumAddr);
    if(w->isa == Datum::typeWord) {
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
    Word *w = reinterpret_cast<Word *>(datumAddr);
    if(w->isa == Datum::typeWord) {
        return w->boolIsValid;
    }
    return false;
}

/// Lookup the var name and return the value as a QLogo object (Word, List, etc)
/// @param wordAddr a pointer to a Word object which contains the name of the variable
/// @return the stored value as a QLogo object
EXPORTC addr_t getDatumForVarname(addr_t wordAddr)
{
    QString name = reinterpret_cast<Word *>(wordAddr)->keyValue();
    Datum *val = Config::get().mainKernel()->callStack.datumForName(name).datumValue();

    return reinterpret_cast<addr_t >(val);
}


/// Write a Datum object to the standard ouput device.
/// @param datumAddr a pointer to a Datum object to print.
/// @param useShow set to true to generate output for SHOW, false for PRINT
EXPORTC addr_t stdWriteDatum(addr_t datumAddr, bool useShow)
{
    auto writeMethod = useShow ? &Datum::showValue : &Datum::printValue;
    Datum *d = reinterpret_cast<Datum *>(datumAddr);
    QString output = (d->*writeMethod)(false, -1, -1) + "\n";
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
    auto writeMethod = useShow ? &Datum::showValue : &Datum::printValue;
    int countOfWords = (int)count;
    Datum **datumAry = reinterpret_cast<Datum **>(datumAddr);
    QString output = "";
    for (int i = 0; i < countOfWords; ++i)
    {
        Datum *d = *(datumAry + i);
        if ((i != 0) && addWhitespace)
            output += " ";
        output += (d->*writeMethod)(false, -1, -1);
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
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Word *w = new Word(val);
    e->watch(w);

    return reinterpret_cast<addr_t >(w);
}

/// Create a QLogo Word object using a bool value
/// @param eAddr a pointer to the Evaluator object
/// @param val a value stored as a bool
/// @return the given value as a QLogo object
EXPORTC addr_t getWordForBool(addr_t eAddr, bool val)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Word *w = new Word(val ? QObject::tr("true") : QObject::tr("false"));
    e->watch(w);

    return reinterpret_cast<addr_t >(w);
}


/// Store the given datum using the given word as a variable name.
/// @param datumAddr a pointer to a QLogo object which is the value to be stored
/// @param wordAddr a pointer to a Word object which contains the name of the variable
EXPORTC void setDatumForWord(addr_t datumAddr, addr_t wordAddr)
{
    DatumPtr d = DatumPtr(reinterpret_cast<Datum *>(datumAddr));
    Word *w = reinterpret_cast<Word *>(wordAddr);
    Config::get().mainKernel()->callStack.setDatumForName(d, w->keyValue());
}


/// Run the given list. Output whatever the list outputs.
/// @param eAddr a pointer to the Evaluator object context.
/// @param listAddr a pointer to a List object which contains QLogo instructions to run
/// @return the result of the list execution
EXPORTC addr_t runList(addr_t eAddr, addr_t listAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    List *list = reinterpret_cast<List *>(listAddr);
    Datum *result = e->subExec(list);
    e->watch(result);

    return reinterpret_cast<addr_t >(result);
}

/// @brief Execute a procedure.
/// @param eAddr a pointer to the Evaluator object context.
/// @param astnodeAddr a pointer to the ASTNode object which is the procedure to execute.
/// @param paramAryAddr a pointer to an array of pointers to Datum objects which are the parameters to the procedure.
/// @param paramCount the number of parameters to the procedure.
/// @return the result of the procedure execution.
EXPORTC addr_t runProcedure(addr_t eAddr, addr_t astnodeAddr, addr_t paramAryAddr, uint32_t paramCount)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    ASTNode *node = reinterpret_cast<ASTNode *>(astnodeAddr);
    Datum **paramAry = reinterpret_cast<Datum **>(paramAryAddr);
    Datum *result = e->procedureExec(node, paramAry, paramCount);
    e->watch(result);

    return reinterpret_cast<addr_t >(result);
}

/// Create and return Error: "SYSTEM"
/// @param aE a pointer to the Evaluator object context.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorSystem(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    FCError *err = FCError::custom(DatumPtr(QObject::tr("SYSTEM")));
    e->watch(err);
    return reinterpret_cast<addr_t >(err);
}

/// Create and return Error: "X didn't like Y as input"
/// @param aE a pointer to the Evaluator object context.
/// @param whoAddr a pointer to the Datum object which rejected the input.
/// @param whatAddr a pointer to the Datum object that was rejected.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoLike(addr_t eAddr, addr_t whoAddr, addr_t whatAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *who = reinterpret_cast<Datum *>(whoAddr);
    Datum *what = reinterpret_cast<Datum *>(whatAddr);
    FCError *err = FCError::doesntLike(who,what);
    e->watch(err);
    return reinterpret_cast<addr_t >(err);
}

/// Create and return Error: "You don't say what to do with X"
/// @param aE a pointer to the Evaluator object context.
/// @param whatAddr a pointer to the Datum object that was orphaned.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoSay(addr_t eAddr, addr_t whatAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *what = reinterpret_cast<Datum *>(whatAddr);
    FCError *err = FCError::dontSay(DatumPtr(what->showValue()));
    e->watch(err);
    return reinterpret_cast<addr_t >(err);
}

/// Create and return Error: "X without TEST"
/// @param aE a pointer to the Evaluator object context.
/// @param xAddr a pointer to the Datum object that was called without a TEST.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoTest(addr_t eAddr, addr_t whoAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *who = reinterpret_cast<Datum *>(whoAddr);
    FCError *err = FCError::noTest(DatumPtr(who->showValue()));
    e->watch(err);
    return reinterpret_cast<addr_t >(err);
}

/// Create and return Error: "X didn't output to Y"
/// @param aE a pointer to the Evaluator object context.
/// @param xAddr a pointer to the Datum object which didn't ouput.
/// @param yAddr a pointer to the Datum object that expected input.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoOutput(addr_t eAddr, addr_t xAddr, addr_t yAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *x = reinterpret_cast<Datum *>(xAddr);
    Datum *y = reinterpret_cast<Datum *>(yAddr);
    // If the thing that didn't output is an ASTNode, use the name of the ASTNode.
    if (x->isa == Datum::typeASTNode)
    {
        x = static_cast<ASTNode *>(x)->nodeName.datumValue();
    }
    FCError *err = FCError::didntOutput(DatumPtr(x->showValue()), DatumPtr(y->showValue()));
    e->watch(err);
    return reinterpret_cast<addr_t >(err);
}

/// Create and return Error: "not enough inputs to X"
/// @param aE a pointer to the Evaluator object context.
/// @param xAddr a pointer to the Datum object that is deprived of inputs.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNotEnoughInputs(addr_t eAddr, addr_t xAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *x = reinterpret_cast<Datum *>(xAddr);
    FCError *err = FCError::notEnoughInputs(DatumPtr(x->showValue()));
    e->watch(err);
    return reinterpret_cast<addr_t >(err);
}

/// Create and return Error: "X has no value"
/// @param aE a pointer to the Evaluator object context.
/// @param whatAddr a pointer to the Word that was looked up.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorNoValue(addr_t eAddr, addr_t whatAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *what = reinterpret_cast<Datum *>(whatAddr);
    FCError *err = FCError::noValue(DatumPtr(what->showValue()));
    e->watch(err);
    return reinterpret_cast<addr_t >(err);
}

/// Create and return an error generated by THROW
/// @param eAddr a pointer to the Evaluator object context.
/// @param tagAddr a pointer to the Datum object which is the tag of the error.
/// @param outputAddr a pointer to the Datum object which is the output of the error.
/// @return a pointer to the Error object that was generated.
EXPORTC addr_t getErrorCustom(addr_t eAddr, addr_t tagAddr, addr_t outputAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *tag = reinterpret_cast<Datum *>(tagAddr);
    Datum *output = reinterpret_cast<Datum *>(outputAddr);
    FCError *err = FCError::custom(DatumPtr(tag), nothing, DatumPtr(output));
    e->watch(err);
    return reinterpret_cast<addr_t >(err);
}

/// @brief Create and return a RETURN control object.
/// @param eAddr a pointer to the Evaluator object context.
/// @param astNodeAddr a pointer to the ASTNode which is the source node of the RETURN.
/// @param retvalAddr a pointer to the Datum object which is the value to return.
/// @return a pointer to the RETURN control object that was generated.
EXPORTC addr_t getCtrlReturn(addr_t eAddr, addr_t astNodeAddr, addr_t retvalAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr retval = DatumPtr(reinterpret_cast<Datum *>(retvalAddr));
    FCReturn *control = new FCReturn(DatumPtr(reinterpret_cast<Datum *>(astNodeAddr)), retval);
    e->watch(control);
    return reinterpret_cast<addr_t >(control);
}

/// @brief Create and return a CONTINUATION control object.
/// @param eAddr a pointer to the Evaluator object context.
/// @param astNodeAddr a pointer to the ASTNode which is the source node of the CONTINUATION.
/// @param paramAryAddr a pointer to an array of pointers to Datum objects which are the parameters to the CONTINUATION.
/// @param paramCount the number of parameters to the CONTINUATION.
/// @return a pointer to the CONTINUATION control object that was generated.
EXPORTC addr_t getCtrlContinuation(addr_t eAddr, addr_t astNodeAddr, addr_t paramAryAddr, uint32_t paramCount)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    ASTNode *node = reinterpret_cast<ASTNode *>(astNodeAddr);
    DatumPtr nodePtr = DatumPtr(node);

    QList<DatumPtr> paramAry;
    for (uint32_t i = 0; i < paramCount; ++i) {
        DatumPtr param = DatumPtr(reinterpret_cast<Datum *>(paramAryAddr[i]));
        paramAry.append(param);
    }
    FCContinuation *control = new FCContinuation(DatumPtr(reinterpret_cast<Datum *>(astNodeAddr)), nodePtr, paramAry);
    e->watch(control);
    return reinterpret_cast<addr_t >(control);
}

/// @brief Create and return a GOTO control object.
/// @param eAddr a pointer to the Evaluator object context.
/// @param astNodeAddr a pointer to the ASTNode which is the source node of the GOTO.
/// @param tagAddr a pointer to the Datum object which is the tag of the GOTO.
/// @return a pointer to the GOTO control object that was generated.
EXPORTC addr_t getCtrlGoto(addr_t eAddr, addr_t astNodeAddr, addr_t tagAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr tag = DatumPtr(reinterpret_cast<Datum *>(tagAddr));

    FCGoto *control = new FCGoto(DatumPtr(reinterpret_cast<Datum *>(astNodeAddr)), tag);
    e->watch(control);
    return reinterpret_cast<addr_t >(control);
}

/// @brief Get the number of elements in a list.
/// @param listAddr a pointer to the List object.
/// @return the number of elements in the list.
EXPORTC int32_t getCountOfList(addr_t listAddr)
{
    List *list = reinterpret_cast<List *>(listAddr);
    return list->count();
}

/// @brief Get a double array from a list of doubles.
/// @param eAddr a pointer to the Evaluator object context.
/// @param listAddr a pointer to the List object source
/// @param destAddr a pointer to the double array to store the values.
/// @return 0 if the list is not a list of doubles, 1 if it is.
EXPORTC int32_t getNumberAryFromList(addr_t eAddr, addr_t listAddr, addr_t destAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    DatumPtr list = DatumPtr(reinterpret_cast<Datum *>(listAddr));
    double *dest = reinterpret_cast<double *>(destAddr);
    // Presumably, getCountOfList() has already been called so the destination size is correct.
    while ((list.datumValue()->isa == Datum::typeList) && ! list.listValue()->isEmpty())
    {
        DatumPtr d = list.listValue()->head;
        if ( ! d.isWord())
        {
            return 0;
        }
        *dest = d.wordValue()->numberValue();
        if ( ! d.wordValue()->numberIsValid)
        {
            return 0;
        }
        ++dest;
        list = list.listValue()->tail;
    }
    return 1;
}
