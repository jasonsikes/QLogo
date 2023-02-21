//===-- qlogo/datum_object.cpp - Object class implementation -------*-
// C++ -*-===//
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
/// This file contains the implementation of the Object class.
/// An object contains methods, instance variables, parents, etc.
///
//===----------------------------------------------------------------------===//

#include "datum.h"
#include <qdebug.h>

/// Object::counter is a generator for 'licenseplate' ids
int Object::counter = 0;


Object::Object()
{
  parents.reserve(0);
  amILogoObject = true;

  init();
}

Object::Object(DatumP aParent)
{
  Q_ASSERT(aParent.isObject());

  parents.reserve(1);
  parents.push_back(aParent);

  init();
}


Object::Object(List *aParents)
{
  parents.reserve(aParents->size());

  ListIterator iter = aParents->newIterator();
  while (iter.elementExists()) {
      DatumP element = iter.element();
      Q_ASSERT(element.isObject());
      parents.push_back(element);
    }

  init();
}


void Object::init()
{
  havemake("LICENSEPLATE", DatumP(new Word(QString("G%1").arg(++counter))));

  // Create the flat list of parents to search,
  // starting with myself.
  ancestors.append(DatumP(this));

  // Then add my parents and grandparents
  addParentsToAncestors(&ancestors);
}


void Object::addParentsToAncestors(QList<DatumP> *aAncestorAry)
{
  for (auto &obj : parents) {
      if ( ! obj.objectValue()->isLogoObject()) {
          aAncestorAry->push_back(obj);
          obj.objectValue()->addParentsToAncestors(aAncestorAry);
        }
    }
}

Datum::DatumType Object::isa()
{
  return objectType;
}


QString Object::name()
{
  return licenseplate();
}


const QString Object::licenseplate()
{
  return valueForName("LICENSEPLATE").wordValue()->printValue();
}


QString Object::printValue(bool fullPrintp, int printDepthLimit,
                   int printWidthLimit)
{
  if (variables.contains("NAME")
      && ( ! variables["NAME"].isNothing()))
    {
      return QString("${Object %1: %2}").arg(licenseplate(), variables["NAME"].printValue());
    }

  return QString("${Object %1}").arg(licenseplate());
}


QString Object::showValue(bool fullPrintp, int printDepthLimit,
                  int printWidthLimit)
{
  return printValue(fullPrintp, printDepthLimit, printWidthLimit);
}


bool Object::isEqual(DatumP other, bool ignoreCase)
{
  return (other.datumValue() == this);
}


void Object::havemake(const QString name, DatumP value)
{
  // TODO: some values must be words for certain names (name, licenseplate).
  variables[name] = value;
}


Object* Object::hasVar(const QString varname, bool shouldSearchParents)
{
  if (variables.contains(varname)) return this;

  if (shouldSearchParents) {
      for (auto &item : ancestors) {
          Object *candidate = item.objectValue()->hasVar(varname);
          if (candidate != NULL) return candidate;
        }
    }

  return NULL;
}


DatumP Object::valueForName(const QString varname)
{
  Q_ASSERT(variables.contains(varname));

  return variables[varname];
}


List* Object::getParents()
{
  List *retval = new List;

  for (auto &item : parents) {
      Q_ASSERT(item.isObject());
      retval->append(item);
    }

  return retval;
}


void Object::setProc(const QString name, DatumP body)
{
  procedures[name] = body;
}


DatumP Object::procForName(const QString procname)
{
  Q_ASSERT(procedures.contains(procname));

  return procedures[procname];
}


List *Object::getProcNames()
{
  List* retval = new List;

  for (auto iter = procedures.keyBegin(); iter != procedures.keyEnd(); ++iter) {
      retval->prepend(DatumP(new Word(*iter)));
    }

  return retval;
}


Object* Object::hasProc(const QString procname, bool shouldSearchParents)
{
  if (procedures.contains(procname)) return this;

  if (shouldSearchParents) {
      for (auto &item : ancestors) {
          Object *candidate = item.objectValue()->hasProc(procname);
          if (candidate != NULL) return candidate;
        }
    }

  return NULL;
}


Object* Object::nextUsualProc(const QString procname, Object *startObject)
{
  QList<DatumP>::iterator iter = ancestors.begin();

  // Find the first occurrence of startObject.
  while (iter->objectValue() != startObject) {
      ++iter;
      Q_ASSERT(iter != ancestors.end());
    }

  // Now find the next object that has procname.
  while ( ++iter != ancestors.end()) {
      Object *candidate = iter->objectValue();
      if (candidate->hasProc(procname))
        return candidate;
    }

  // Not found.
  return NULL;
}



List *Object::getVarnames()
{
  List* retval = new List;

  for (auto iter = variables.keyBegin(); iter != variables.keyEnd(); ++iter) {
      retval->prepend(DatumP(new Word(*iter)));
    }

  return retval;
}
