
//===-- qlogo/procedures.cpp - Procedures class implementation -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// QLogo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with QLogo.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the Procedures class, which is
/// responsible for for organizing all procedures in QLogo: primitives,
/// user-defined, and library.
///
//===----------------------------------------------------------------------===//

#include "workspace/procedures.h"
#include "datum.h"
#include "kernel.h"
#include "error.h"
#include "astnode.h"
#include <QDateTime>
#include "QApplication"


Procedures::Procedures() {
    Config::get().setMainProcedures(this);

    // The procedure table code is generated by util/generate_command_table.py:
#include "workspace/primitivetable.h"

}

Procedures::~Procedures() {
    Config::get().setMainProcedures(NULL);
}

void Procedures::defineProcedure(DatumPtr cmd, DatumPtr procnameP, DatumPtr text,
                             DatumPtr sourceText) {
    if ( ! std::isnan(procnameP.wordValue()->numberValue()))
        Error::doesntLike(cmd, procnameP);

    QString procname = procnameP.wordValue()->keyValue();

    QChar firstChar = (procname)[0];
    if ((firstChar == '"') || (firstChar == ':'))
        Error::doesntLike(cmd, procnameP);

    if (stringToCmd.contains(procname))
        Error::isPrimative(procnameP);

    DatumPtr procBody = createProcedure(cmd, text, sourceText);

    procedures[procname] = procBody;

    if (Config::get().mainKernel()->isInputRedirected()
        && Config::get().mainKernel()->varUNBURYONEDIT()) {
        unbury(procname);
    }
}

DatumPtr Procedures::createProcedure(DatumPtr cmd, DatumPtr text, DatumPtr sourceText) {
    Procedure *body = new Procedure();
    body->init();
    DatumPtr bodyP(body);

    lastProcedureCreatedTimestamp = QDateTime::currentMSecsSinceEpoch();

    QString cmdString = cmd.wordValue()->keyValue();
    bool isMacro = ((cmdString == QObject::tr(".MACRO")) || (cmdString == QObject::tr(".DEFMACRO")));

    body->countOfDefaultParams = 0;
    body->countOfMinParams = 0;
    body->countOfMaxParams = 0;
    body->isMacro = isMacro;
    body->sourceText = sourceText;

    bool isOptionalDefined = false;
    bool isRestDefined = false;
    bool isDefaultDefined = false;

    // Required Inputs :FOO
    // Optional inputs [:BAZ 87]
    // Rest input      [:GARPLY]
    // default number  5

    ListIterator paramIter = text.listValue()->head.listValue()->newIterator();

    while (paramIter.elementExists()) {
        DatumPtr currentParam = paramIter.element();

        if (currentParam.isWord()) { // default 5 OR required :FOO
            double paramAsNumber = currentParam.wordValue()->numberValue();
            if ( ! std::isnan(paramAsNumber)) { // default 5
                if (isDefaultDefined)
                    Error::doesntLike(cmd, currentParam);
                if ((paramAsNumber != floor(paramAsNumber)) ||
                    (paramAsNumber < body->countOfMinParams) ||
                    ((paramAsNumber > body->countOfMaxParams) &&
                     (body->countOfMaxParams >= 0)))
                    Error::doesntLike(cmd, currentParam);
                body->countOfDefaultParams = paramAsNumber;
                isDefaultDefined = true;
            } else {
                if (isDefaultDefined || isRestDefined || isOptionalDefined)
                    Error::doesntLike(cmd, currentParam);
                QString paramName =
                    currentParam.wordValue()->keyValue(); // required :FOO
                if (paramName.startsWith(':') || paramName.startsWith('"'))
                    paramName.remove(0, 1);
                if (paramName.size() < 1)
                    Error::doesntLike(cmd, currentParam);
                body->requiredInputs.append(paramName);
                body->countOfDefaultParams += 1;
                body->countOfMinParams += 1;
                body->countOfMaxParams += 1;
            }
        } else if (currentParam.isList()) { // Optional [:BAZ 87] or rest [:GARPLY]
            List *paramList = currentParam.listValue();

            if (paramList->isEmpty())
                Error::doesntLike(cmd, currentParam);

            if (paramList->count() == 1) { // rest input [:GARPLY]
                if (isRestDefined)
                    Error::doesntLike(cmd, currentParam);
                DatumPtr param = paramList->head;
                if (param.isWord()) {
                    QString restName = param.wordValue()->keyValue();
                    if (restName.startsWith(':') || restName.startsWith('"'))
                        restName.remove(0, 1);
                    if (restName.size() < 1)
                        Error::doesntLike(cmd, param);
                    body->restInput = restName;
                    isRestDefined = true;
                    body->countOfMaxParams = -1;
                } else {
                    Error::doesntLike(cmd, param);
                }
            } else { // Optional [:BAZ 87]
                if (isRestDefined || isDefaultDefined)
                    Error::doesntLike(cmd, currentParam);
                DatumPtr param = paramList->head;
                if (param.isWord()) {
                    QString name = param.wordValue()->keyValue();
                    if (name.startsWith(':') || name.startsWith('"'))
                        name.remove(0, 1);
                    if (name.size() < 1)
                        Error::doesntLike(cmd, param);
                    body->optionalInputs.append(name);
                    body->optionalDefaults.append(paramList);
                    isOptionalDefined = true;
                    body->countOfMaxParams += 1;
                } else {
                    Error::doesntLike(cmd, param);
                }
            } // endif optional or rest input
        } else {
            Error::doesntLike(cmd, currentParam);
        }
    } // /for each parameter

    body->instructionList = text.listValue()->tail;
    if (body->instructionList.isNothing())
        body->instructionList = DatumPtr(new List);

    ListIterator lineIter = body->instructionList.listValue()->newIterator();
    while (lineIter.elementExists()) {
        DatumPtr lineP = lineIter.element();
        ListIterator wordIter = lineP.listValue()->newIterator();
        while (wordIter.elementExists()) {
            DatumPtr d = wordIter.element();
            if (d.isWord() && (d.wordValue()->keyValue() == QObject::tr("TAG")) &&
                wordIter.elementExists()) {
                DatumPtr d = wordIter.element();
                if (d.isWord()) {
                    QString param = d.wordValue()->keyValue();
                    if ((param.size() > 1) && (param)[0] == '"') {
                        QString tag = param.right(param.size() - 1);
                        body->tagToLine[tag] = lineP;
                    }
                }
            }
        }
    }
    return bodyP;
}

void Procedures::copyProcedure(DatumPtr newnameP, DatumPtr oldnameP) {
    lastProcedureCreatedTimestamp = QDateTime::currentMSecsSinceEpoch();
    QString newname = newnameP.wordValue()->keyValue();
    QString oldname = oldnameP.wordValue()->keyValue();

    if (stringToCmd.contains(newname))
        Error::isPrimative(newnameP);

    if (stringToCmd.contains(oldname)) {
        Error::isPrimative(oldnameP);
    }
    if (isNamedProcedure(oldname)) {
        procedures[newname] = procedures[oldname];
        return;
    }
    Error::noHow(oldnameP);
}

void Procedures::eraseProcedure(DatumPtr procnameP) {
    lastProcedureCreatedTimestamp = QDateTime::currentMSecsSinceEpoch();

    QString procname = procnameP.wordValue()->keyValue();
    if (stringToCmd.contains(procname))
        Error::isPrimative(procnameP);
    procedures.remove(procname);
}

DatumPtr Procedures::procedureText(DatumPtr procnameP) {
    QString procname = procnameP.wordValue()->keyValue();

    if (stringToCmd.contains(procname))
        Error::isPrimative(procnameP);
    if ( ! isNamedProcedure(procname))
        Error::noHow(procnameP);
    Procedure *body = procedureForName(procname).procedureValue();

    List *retval = new List();

    List *inputs = new List();

    for (auto &i : body->requiredInputs) {
        inputs->append(DatumPtr(i));
    }

    QList<DatumPtr>::iterator d = body->optionalDefaults.begin();
    for (auto &i : body->optionalInputs) {
        List *optInput = d->listValue()->tail.listValue();
        ++d;
        inputs->append(new List(DatumPtr(i),optInput));
    }

    if (body->restInput != "") {
        List *restInput = new List();
        restInput->append(DatumPtr(body->restInput));
        inputs->append(DatumPtr(restInput));
    }

    if (body->countOfDefaultParams != body->requiredInputs.size()) {
        inputs->append(DatumPtr(body->countOfDefaultParams));
    }

    retval->append(DatumPtr(inputs));

    ListIterator b = body->instructionList.listValue()->newIterator();

    while (b.elementExists()) {
        retval->append(b.element());
    }

    return DatumPtr(retval);
}

DatumPtr Procedures::procedureFulltext(DatumPtr procnameP, bool shouldValidate) {
    const QString procname = procnameP.wordValue()->keyValue();
    if (stringToCmd.contains(procname))
        Error::isPrimative(procnameP);

    if (isNamedProcedure(procname)) {
        Procedure *body = procedureForName(procname).procedureValue();

        if (body->sourceText == nothing) {
            List *retval = new List();
            retval->append(DatumPtr(procedureTitle(procnameP)));

            ListIterator b = body->instructionList.listValue()->newIterator();

            while (b.elementExists()) {
                retval->append(DatumPtr(unreadList(b.element().listValue(), false)));
            }

            DatumPtr end(QObject::tr("END"));
            retval->append(end);
            return DatumPtr(retval);
        } else {
            return body->sourceText;
        }
    } else if (shouldValidate) {
        Error::noHow(procnameP);
    }
    List *retval = new List();
    retval->append(
        DatumPtr(QObject::tr("to ") + procnameP.wordValue()->printValue()));
    retval->append(DatumPtr(QObject::tr("END")));
    return DatumPtr(retval);
}

QString Procedures::procedureTitle(DatumPtr procnameP) {
    QString procname = procnameP.wordValue()->keyValue();

    if (stringToCmd.contains(procname))
        Error::isPrimative(procnameP);
    if ( ! isNamedProcedure(procname))
        Error::noHow(procnameP);

    Procedure *body = procedureForName(procname).procedureValue();

    DatumPtr firstlineP = DatumPtr(new List());

    List *firstLine = firstlineP.listValue();

    if (body->isMacro)
        firstLine->append(DatumPtr(QObject::tr(".macro")));
    else
        firstLine->append(DatumPtr(QObject::tr("to")));
    firstLine->append(procnameP);

    QString paramName;

    for (auto &i : body->requiredInputs) {
        paramName = i;
        paramName.prepend(':');
        firstLine->append(DatumPtr(paramName));
    }

    for (auto &i : body->optionalDefaults) {
        firstLine->append(i);
    }

    paramName = body->restInput;
    if (paramName != "") {
        paramName.push_front(':');
        List *restInput = new List();
        restInput->append(DatumPtr(paramName));
        firstLine->append(DatumPtr(restInput));
    }

    if (body->countOfDefaultParams != body->requiredInputs.size()) {
        firstLine->append(DatumPtr(body->countOfDefaultParams));
    }

    QString retval = unreadList(firstLine, false);
    return retval;
}

DatumPtr Procedures::procedureForName(QString aName)
{
    if ( ! procedures.contains(aName)) {
        if ( ! stdLib.allProcedureNames().contains(aName)) {
            return nothing;
        }
        QString libraryText = stdLib.procedureText(aName);
        Q_ASSERT( ! libraryText.isEmpty());
        Config::get().mainKernel()->executeText(libraryText);
    }
    Q_ASSERT(procedures.contains(aName));
    return procedures[aName];
}


bool Procedures::isNamedProcedure(QString aName)
{
    return procedures.contains(aName)
        || stdLib.allProcedureNames().contains(aName);
}


DatumPtr Procedures::astnodeFromCommand(DatumPtr cmdP, int &minParams,
                                    int &defaultParams, int &maxParams) {
    QString cmdString = cmdP.wordValue()->keyValue();

    Cmd_t command;
    DatumPtr node = DatumPtr(new ASTNode(cmdP));
    if (stringToCmd.contains(cmdString)) {
        command = stringToCmd[cmdString];
        defaultParams = command.countOfDefaultParams;
        minParams = command.countOfMinParams;
        maxParams = command.countOfMaxParams;
        node.astnodeValue()->kernel = command.method;
    } else if (isNamedProcedure(cmdString)) {
        DatumPtr procBody = procedureForName(cmdString);
        if (procBody.procedureValue()->isMacro)
            node.astnodeValue()->kernel = &Kernel::executeMacro;
        else
            node.astnodeValue()->kernel = &Kernel::executeProcedure;
        node.astnodeValue()->addChild(procBody);
        defaultParams = procBody.procedureValue()->countOfDefaultParams;
        minParams = procBody.procedureValue()->countOfMinParams;
        maxParams = procBody.procedureValue()->countOfMaxParams;
    } else if (cmdString.startsWith(QObject::tr("SET")) && (cmdString.size() > 3) &&
               Config::get().mainKernel()->varALLOWGETSET()) {
        node.astnodeValue()->kernel = &Kernel::excSetfoo;
        defaultParams = 1;
        minParams = 1;
        maxParams = 1;
    } else if (Config::get().mainKernel()->varALLOWGETSET()) {
        node.astnodeValue()->kernel = &Kernel::excFoo;
        defaultParams = 0;
        minParams = 0;
        maxParams = 0;
    } else {
        Error::noHow(cmdP);
    }
    return node;
}


DatumPtr Procedures::astnodeWithLiterals(DatumPtr cmd, DatumPtr params) {
    int minParams = 0, maxParams = 0, defaultParams = 0;
    DatumPtr node = astnodeFromCommand(cmd, minParams, defaultParams, maxParams);

    int countOfChildren = params.listValue()->count();
    if (countOfChildren < minParams)
        Error::notEnough(cmd);
    if ((countOfChildren > maxParams) && (maxParams != -1))
        Error::tooMany(cmd);

    ListIterator iter = params.listValue()->newIterator();
    while (iter.elementExists()) {
        DatumPtr p = iter.element();
        DatumPtr a = DatumPtr(new ASTNode(QObject::tr("literal")));
        a.astnodeValue()->kernel = &Kernel::executeLiteral;
        a.astnodeValue()->addChild(p);
        node.astnodeValue()->addChild(a);
    }
    return node;
}

bool Procedures::isProcedure(QString procname) {
    if (stringToCmd.contains(procname)
        || procedures.contains(procname))
        return true;
    return false;
}

bool Procedures::isMacro(QString procname) {
    if (procedures.contains(procname)) {
        DatumPtr procedure = procedures[procname];
        return procedure.procedureValue()->isMacro;
    }
    return false;
}

bool Procedures::isPrimitive(QString procname) {
    return (stringToCmd.contains(procname));
}

bool Procedures::isDefined(QString procname) {
    return (procedures.contains(procname));
}

DatumPtr Procedures::allProcedureNames(showContents_t showWhat) {
    List *retval = new List();

    for (const auto &iter : procedures.asKeyValueRange()) {

        if (shouldInclude(showWhat, iter.first))
            retval->append(DatumPtr(iter.first));
    }
    return DatumPtr(retval);
}

void Procedures::eraseAllProcedures() {
    QStringList names = procedures.keys();
    for (auto &iter : names) {
        if (!isBuried(iter)) {
            procedures.remove(iter);
        }
    }
}

DatumPtr Procedures::allPrimitiveProcedureNames() {
    List *retval = new List();

    for (const auto &iter : stringToCmd.asKeyValueRange()) {
        retval->append(DatumPtr(iter.first));
    }
    return DatumPtr(retval);
}

DatumPtr Procedures::arity(DatumPtr nameP) {
    int minParams, defParams, maxParams;
    QString procname = nameP.wordValue()->keyValue();

    if (procedures.contains(procname)) {
        DatumPtr command = procedures[procname];
        minParams = command.procedureValue()->countOfMinParams;
        defParams = command.procedureValue()->countOfDefaultParams;
        maxParams = command.procedureValue()->countOfMaxParams;
    } else if (stringToCmd.contains(procname)) {
        Cmd_t command = stringToCmd[procname];
        minParams = command.countOfMinParams;
        defParams = command.countOfDefaultParams;
        maxParams = command.countOfMaxParams;
    } else {
        Error::noHow(nameP);
        return nothing;
    }

    List *retval = new List();
    retval->append(DatumPtr(minParams));
    retval->append(DatumPtr(defParams));
    retval->append(DatumPtr(maxParams));
    return DatumPtr(retval);
}

QString Procedures::unreadDatum(DatumPtr aDatum, bool isInList) {
    switch (aDatum.isa()) {
    case Datum::wordType:
        return unreadWord(aDatum.wordValue(), isInList);
        break;
    case Datum::listType:
        return unreadList(aDatum.listValue(), isInList);
    case Datum::arrayType:
        return unreadArray(aDatum.arrayValue());
    default:
        Q_ASSERT(false);
    }
    return "";
}

QString Procedures::unreadList(List *aList, bool isInList) {
    QString retval("");
    if (isInList)
        retval = "[";
    ListIterator i = aList->newIterator();
    while (i.elementExists()) {
        DatumPtr e = i.element();
        if ((retval != "[") && (retval != ""))
            retval.append(' ');
        retval.append(unreadDatum(e, true));
    }
    if (isInList)
        retval.append(']');
    return retval;
}

QString Procedures::unreadArray(Array *anArray) {
    QString retval("{");
    for (auto &i : anArray->array) {
        if (retval.size() > 1)
            retval.append(' ');
        retval.append(unreadDatum(i, true));
    }
    retval.append('}');
    return retval;
}

QString Procedures::unreadWord(Word *aWord, bool isInList) {
    if ( ! std::isnan(aWord->numberValue()))
        return aWord->showValue();

    QString retval("");
    if (!isInList)
        retval = "\"";

    const QString src = aWord->showValue();
    if (src.size() == 0)
        return retval + "||";

    if (aWord->isForeverSpecial) {
        retval.append('|');
        for (auto iter = src.begin(); iter != src.end(); ++iter) {
            QChar letter = *iter;
            if ((iter == src.begin()) && (letter == '"')) {
                retval = "\"|";
            } else {
                if (letter == '|') {
                    retval.append('\\');
                }
                retval.append(letter);
            }
        }
        retval.append('|');
    } else {
        for (auto letter : src) {
            if ((letter == ' ') || (letter == '[') || (letter == ']') ||
                (letter == '{') || (letter == '}') || (letter == '|') ||
                (letter == '\n')) {
                retval.append('\\');
            }
            retval.append(letter);
        }
    }
    return retval;
}

QString Procedures::printoutDatum(DatumPtr aDatum) {
    switch (aDatum.isa()) {
    case Datum::wordType:
        return unreadWord(aDatum.wordValue());
        break;
    case Datum::listType:
        return unreadList(aDatum.listValue(), true);
    case Datum::arrayType:
        return unreadArray(aDatum.arrayValue());
    default:
        Q_ASSERT(false);
    }
    return "";
}

