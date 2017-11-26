
//===-- qlogo/help.cpp - Help class implementation -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
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
/// This file contains the implementation of the Help class, which contains
/// the QLogo help text.
///
//===----------------------------------------------------------------------===//

#include "help.h"
#include <QHash>
#include <QStringList>

QHash<QString, DatumP> rsrc;

Help::Help() {
  if (rsrc.size() == 0)
    initRsrc();
}

void Help::set(const QString &name, const QString &text) {
  DatumP textP(new Word(text));
  rsrc[name] = textP;
}

void Help::alt(const QString &newName, const QString &oldName) {
  rsrc[newName] = rsrc[oldName];
}

DatumP Help::helpForKeyword(const QString &keyWord) {
  if (rsrc.contains(keyWord)) {
    return rsrc[keyWord];
  }
  return nothing;
}

DatumP Help::allKeywords() {
  List *retval = new List;
  QStringList keys = rsrc.keys();
  keys.sort();
  for (auto key : keys) {
    retval->append(DatumP(new Word(key)));
  }
  return DatumP(retval);
}

void Help::initRsrc() {
  setDataStructurePrimitives();
  setCommunication();
  setArithmetic();
  setGraphics();
  setWorkspaceManagement();
  setControlStructures();
}

void Help::setDataStructurePrimitives() {
  //    CONSTRUCTORS
  //    ------------

  set("WORD", "WORD word1 word2\n"
              "(WORD word1 word2 word3 ...)\n"
              "\n"
              "        outputs a word formed by concatenating its inputs.\n"
              "\n");

  set("LIST",
      "LIST thing1 thing2\n"
      "(LIST thing1 thing2 thing3 ...)\n"
      "\n"
      "        outputs a list whose members are its inputs, which can be any\n"
      "        Logo datum (word, list, or array).\n"
      "\n");

  set("SENTENCE", "SENTENCE thing1 thing2\n"
                  "SE thing1 thing2\n"
                  "(SENTENCE thing1 thing2 thing3 ...)\n"
                  "(SE thing1 thing2 thing3 ...)\n"
                  "\n"
                  "        outputs a list whose members are its inputs, if "
                  "those inputs are\n"
                  "        not lists, or the members of its inputs, if those "
                  "inputs are lists.\n"
                  "\n");

  alt("SE", "SENTENCE");

  set("FPUT", "FPUT thing list\n"
              "\n"
              "        outputs a list equal to its second input with one extra "
              "member,\n"
              "        the first input, at the beginning.  If the second input "
              "is a word,\n"
              "        then FPUT is equivalent to WORD.\n"
              "\n");

  set("LPUT",
      "LPUT thing list\n"
      "\n"
      "        outputs a list equal to its second input with one extra "
      "member,\n"
      "        the first input, at the end.  If the second input is a word,\n"
      "        then LPUT is equivalent to WORD with its inputs in the other "
      "order.\n"
      "\n");

  set("ARRAY", "ARRAY size\n"
               "(ARRAY size origin)\n"
               "\n"
               "        outputs an array of \"size\" members (must be a "
               "positive integer),\n"
               "        each of which initially is an empty list.  Array "
               "members can be\n"
               "        selected with ITEM and changed with SETITEM.  The "
               "first member of\n"
               "        the array is member number 1 unless an \"origin\" "
               "input (must be an\n"
               "        integer) is given, in which case the first member of "
               "the array has\n"
               "        that number as its index.  (Typically 0 is used as the "
               "origin if\n"
               "        anything.)  Arrays are printed by PRINT and friends, "
               "and can be\n"
               "        typed in, inside curly braces; indicate an origin with "
               "{a b c}@0.\n"
               "\n");

  set("MDARRAY", "MDARRAY sizelist                                        "
                 "(library procedure)\n"
                 "(MDARRAY sizelist origin)\n"
                 "\n"
                 "        outputs a multi-dimensional array.  The first input "
                 "must be a list\n"
                 "        of one or more positive integers.  The second input, "
                 "if present,\n"
                 "        must be a single integer that applies to every "
                 "dimension of the array.\n"
                 "        Ex: (MDARRAY [3 5] 0) outputs a two-dimensional "
                 "array whose members\n"
                 "        range from [0 0] to [2 4].\n"
                 "\n");

  set("LISTTOARRAY", "LISTTOARRAY list\n"
                     "(LISTTOARRAY list origin)\n"
                     "\n"
                     "        outputs an array of the same size as the input "
                     "list, whose members\n"
                     "        are the members of the input list.\n"
                     "\n");

  set("ARRAYTOLIST", "ARRAYTOLIST array\n"
                     "\n"
                     "        outputs a list whose members are the members of "
                     "the input array.\n"
                     "        The first member of the output is the first "
                     "member of the array,\n"
                     "        regardless of the array's origin.\n"
                     "\n");

  set("COMBINE", "COMBINE thing1 thing2                                   "
                 "(library procedure)\n"
                 "\n"
                 "        if thing2 is a word, outputs WORD thing1 thing2.  If "
                 "thing2 is a list,\n"
                 "        outputs FPUT thing1 thing2.\n"
                 "\n");

  set("REVERSE", "REVERSE list                                            "
                 "(library procedure)\n"
                 "\n"
                 "        outputs a list whose members are the members of the "
                 "input list, in\n"
                 "        reverse order.\n"
                 "\n");

  set("GENSYM", "GENSYM                                                  "
                "(library procedure)\n"
                "\n"
                "        outputs a unique word each time it's invoked.  The "
                "words are of the\n"
                "        form G1, G2, etc.\n"
                "\n");

  //    SELECTORS
  //    ---------

  set("FIRST",
      "FIRST thing\n"
      "\n"
      "        if the input is a word, outputs the first character of the "
      "word.\n"
      "        If the input is a list, outputs the first member of the list.\n"
      "        If the input is an array, outputs the origin of the array "
      "(that\n"
      "        is, the INDEX OF the first member of the array).\n"
      "\n");

  set("FIRSTS", "FIRSTS list\n"
                "\n"
                "        outputs a list containing the FIRST of each member of "
                "the input\n"
                "        list.  It is an error if any member of the input list "
                "is empty.\n"
                "        (The input itself may be empty, in which case the "
                "output is also\n"
                "        empty.)  This could be written as\n"
                "\n"
                "                to firsts :list\n"
                "                output map \"first :list\n"
                "                end\n"
                "\n"
                "        but is provided as a primitive in order to speed up "
                "the iteration\n"
                "        tools MAP, MAP.SE, and FOREACH.\n"
                " \n"
                "                to transpose :matrix\n"
                "                if emptyp first :matrix [op []]\n"
                "                op fput firsts :matrix transpose bfs :matrix\n"
                "                end\n"
                "\n");

  set("LAST",
      "LAST wordorlist\n"
      "\n"
      "        if the input is a word, outputs the last character of the "
      "word.\n"
      "        If the input is a list, outputs the last member of the list.\n"
      "\n");

  set("BUTFIRST", "BUTFIRST wordorlist\n"
                  "BF wordorlist\n"
                  "\n"
                  "        if the input is a word, outputs a word containing "
                  "all but the first\n"
                  "        character of the input.  If the input is a list, "
                  "outputs a list\n"
                  "        containing all but the first member of the input.\n"
                  "\n");

  alt("BF", "BUTFIRST");

  set("BUTFIRSTS", "BUTFIRSTS list\n"
                   "BFS list\n"
                   "\n"
                   "        outputs a list containing the BUTFIRST of each "
                   "member of the input\n"
                   "        list.  It is an error if any member of the input "
                   "list is empty or an\n"
                   "        array.  (The input itself may be empty, in which "
                   "case the output is\n"
                   "        also empty.)  This could be written as\n"
                   "\n"
                   "                to butfirsts :list\n"
                   "                output map \"butfirst :list\n"
                   "                end\n"
                   "\n"
                   "        but is provided as a primitive in order to speed "
                   "up the iteration\n"
                   "        tools MAP, MAP.SE, and FOREACH.\n"
                   "\n");

  alt("BFS", "BUTFIRSTS");

  set("BUTLAST", "BUTLAST wordorlist\n"
                 "BL wordorlist\n"
                 "\n"
                 "        if the input is a word, outputs a word containing "
                 "all but the last\n"
                 "        character of the input.  If the input is a list, "
                 "outputs a list\n"
                 "        containing all but the last member of the input.\n"
                 "\n");

  alt("BL", "BUTLAST");

  set("ITEM",
      "ITEM index thing\n"
      "\n"
      "        if the \"thing\" is a word, outputs the \"index\"th character "
      "of the\n"
      "        word.  If the \"thing\" is a list, outputs the \"index\"th "
      "member of\n"
      "        the list.  If the \"thing\" is an array, outputs the "
      "\"index\"th\n"
      "        member of the array.  \"Index\" starts at 1 for words and "
      "lists;\n"
      "        the starting index of an array is specified when the array is\n"
      "        created.\n"
      "\n");

  set("MDITEM", "MDITEM indexlist array                                  "
                "(library procedure)\n"
                "\n"
                "        outputs the member of the multidimensional \"array\" "
                "selected by\n"
                "        the list of numbers \"indexlist\".\n"
                "\n");

  set("PICK", "PICK list                                               "
              "(library procedure)\n"
              "\n"
              "        outputs a randomly chosen member of the input list.\n"
              "\n");

  set("REMOVE", "REMOVE thing list                                       "
                "(library procedure)\n"
                "\n"
                "        outputs a copy of \"list\" with every member equal to "
                "\"thing\" removed.\n"
                "\n");

  set("REMDUP", "REMDUP list                                             "
                "(library procedure)\n"
                "\n"
                "        outputs a copy of \"list\" with duplicate members "
                "removed.  If two or\n"
                "        more members of the input are equal, the rightmost of "
                "those members\n"
                "        is the one that remains in the output.\n"
                "\n");

  set("QUOTED", "QUOTED thing                                            "
                "(library procedure)\n"
                "\n"
                "        outputs its input, if a list; outputs its input with "
                "a quotation\n"
                "        mark prepended, if a word.\n"
                "\n");

  //    MUTATORS
  //    --------

  set("SETITEM",
      "SETITEM index array value\n"
      "\n"
      "        command.  Replaces the \"index\"th member of \"array\" with the "
      "new\n"
      "        \"value\".  Ensures that the resulting array is not circular, "
      "i.e.,\n"
      "        \"value\" may not be a list or array that contains \"array\".\n"
      "\n");

  set("MDSETITEM", "MDSETITEM indexlist array value                         "
                   "(library procedure)\n"
                   "\n"
                   "        command.  Replaces the member of \"array\" chosen "
                   "by \"indexlist\"\n"
                   "        with the new \"value\".\n"
                   "\n");

  set(".SETFIRST",
      ".SETFIRST list value\n"
      "\n"
      "        command.  Changes the first member of \"list\" to be "
      "\"value\".\n"
      "\n"
      "        WARNING:  Primitives whose names start with a period are "
      "DANGEROUS.\n"
      "        Their use by non-experts is not recommended.  The use of "
      ".SETFIRST can\n"
      "        lead to circular list structures, which will get some Logo "
      "primitives\n"
      "        into infinite loops, and to unexpected changes to other data\n"
      "        structures that share storage with the list being modified.\n"
      "\n");

  set(".SETBF",
      ".SETBF list value\n"
      "\n"
      "        command.  Changes the butfirst of \"list\" to be \"value\".\n"
      "\n"
      "        WARNING: Primitives whose names start with a period are "
      "DANGEROUS.\n"
      "        Their use by non-experts is not recommended.  The use of .SETBF "
      "can\n"
      "        lead to circular list structures, which will get some Logo "
      "primitives\n"
      "        into infinite loops; unexpected changes to other data "
      "structures that\n"
      "        share storage with the list being modified; or to Logo crashes "
      "and\n"
      "        coredumps if the butfirst of a list is not itself a list.\n"
      "\n");

  set(".SETITEM",
      ".SETITEM index array value\n"
      "\n"
      "        command.  Changes the \"index\"th member of \"array\" to be "
      "\"value\",\n"
      "        like SETITEM, but without checking for circularity.\n"
      "\n"
      "        WARNING: Primitives whose names start with a period are "
      "DANGEROUS.\n"
      "        Their use by non-experts is not recommended.  The use of "
      ".SETITEM\n"
      "        can lead to circular arrays, which will get some Logo "
      "primitives into\n"
      "        infinite loops.\n"
      "\n");

  set("PUSH",
      "PUSH stackname thing                                    (library "
      "procedure)\n"
      "\n"
      "        command.  Adds the \"thing\" to the stack that is the value of "
      "the\n"
      "        variable whose name is \"stackname\".  This variable must have "
      "a list\n"
      "        as its value; the initial value should be the empty list.  New\n"
      "        members are added at the front of the list.\n"
      "\n");

  set("POP", "POP stackname                                           (library "
             "procedure)\n"
             "\n"
             "        outputs the most recently PUSHed member of the stack "
             "that is the\n"
             "        value of the variable whose name is \"stackname\" and "
             "removes that\n"
             "        member from the stack.\n"
             "\n");

  set("QUEUE",
      "QUEUE queuename thing                                   (library "
      "procedure)\n"
      "\n"
      "        command.  Adds the \"thing\" to the queue that is the value of "
      "the\n"
      "        variable whose name is \"queuename\".  This variable must have "
      "a list\n"
      "        as its value; the initial value should be the empty list.  New\n"
      "        members are added at the back of the list.\n"
      "\n");

  set("DEQUEUE", "DEQUEUE queuename                                       "
                 "(library procedure)\n"
                 "\n"
                 "        outputs the least recently QUEUEd member of the "
                 "queue that is the\n"
                 "        value of the variable whose name is \"queuename\" "
                 "and removes that\n"
                 "        member from the queue.\n"
                 "\n");

  //    PREDICATES
  //    ----------

  set("WORDP", "WORDP thing\n"
               "WORD? thing\n"
               "\n"
               "        outputs TRUE if the input is a word, FALSE otherwise.\n"
               "\n");

  alt("WORD?", "WORDP");

  set("LISTP", "LISTP thing\n"
               "LIST? thing\n"
               "\n"
               "        outputs TRUE if the input is a list, FALSE otherwise.\n"
               "\n");

  alt("LIST?", "LISTP");

  set("ARRAYP",
      "ARRAYP thing\n"
      "ARRAY? thing\n"
      "\n"
      "        outputs TRUE if the input is an array, FALSE otherwise.\n"
      "\n");

  alt("ARRAY?", "ARRAYP");

  set("EMPTYP",
      "EMPTYP thing\n"
      "EMPTY? thing\n"
      "\n"
      "        outputs TRUE if the input is the empty word or the empty list,\n"
      "        FALSE otherwise.\n"
      "\n");

  alt("EMPTY?", "EMPTYP");

  set("EQUALP",
      "EQUALP thing1 thing2\n"
      "EQUAL? thing1 thing2\n"
      "thing1 = thing2\n"
      "\n"
      "        outputs TRUE if the inputs are equal, FALSE otherwise.  Two "
      "numbers\n"
      "        are equal if they have the same numeric value.  Two non-numeric "
      "words\n"
      "        are equal if they contain the same characters in the same "
      "order.  If\n"
      "        there is a variable named CASEIGNOREDP whose value is TRUE, "
      "then an\n"
      "        upper case letter is considered the same as the corresponding "
      "lower\n"
      "        case letter.  (This is the case by default.)  Two lists are "
      "equal if\n"
      "        their members are equal.  An array is only equal to itself; "
      "two\n"
      "        separately created arrays are never equal even if their members "
      "are\n"
      "        equal.  (It is important to be able to know if two expressions "
      "have\n"
      "        the same array as their value because arrays are mutable; if, "
      "for\n"
      "        example, two variables have the same array as their values "
      "then\n"
      "        performing SETITEM on one of them will also change the other.)\n"
      "\n");

  alt("EQUAL?", "EQUALP");
  alt("=", "EQUALP");

  set("NOTEQUALP",
      "NOTEQUALP thing1 thing2\n"
      "NOTEQUAL? thing1 thing2\n"
      "thing1 <> thing2\n"
      "\n"
      "        outputs FALSE if the inputs are equal, TRUE otherwise.  See "
      "EQUALP\n"
      "        for the meaning of equality for different data types.\n"
      "\n");

  alt("NOTEQUAL?", "NOTEQUALP");
  alt("<>", "NOTEQUALP");

  set("BEFOREP", "BEFOREP word1 word2\n"
                 "BEFORE? word1 word2\n"
                 "\n"
                 "        outputs TRUE if word1 comes before word2 in ASCII "
                 "collating sequence\n"
                 "        (for words of letters, in alphabetical order).  "
                 "Case-sensitivity is\n"
                 "        determined by the value of CASEIGNOREDP.  Note that "
                 "if the inputs are\n"
                 "        numbers, the result may not be the same as with "
                 "LESSP; for example,\n"
                 "        BEFOREP 3 12 is false because 3 collates after 1.\n"
                 "\n");

  alt("BEFORE?", "BEFOREP");

  set(".EQ", ".EQ thing1 thing2\n"
             "\n"
             "        outputs TRUE if its two inputs are the same datum, so "
             "that applying a\n"
             "        mutator to one will change the other as well.  Outputs "
             "FALSE otherwise,\n"
             "        even if the inputs are equal in value.\n"
             "        WARNING: Primitives whose names start with a period are "
             "DANGEROUS.\n"
             "        Their use by non-experts is not recommended.  The use of "
             "mutators\n"
             "        can lead to circular data structures, infinite loops, or "
             "Logo crashes.\n"
             "\n");

  set("MEMBERP",
      "MEMBERP thing1 thing2\n"
      "MEMBER? thing1 thing2\n"
      "\n"
      "        if \"thing2\" is a list or an array, outputs TRUE if \"thing1\" "
      "is EQUALP\n"
      "        to a member of \"thing2\", FALSE otherwise.  If \"thing2\" is\n"
      "        a word, outputs TRUE if \"thing1\" is a one-character word "
      "EQUALP to a\n"
      "        character of \"thing2\", FALSE otherwise.\n"
      "\n");

  alt("MEMBER?", "MEMBERP");

  set("SUBSTRINGP", "SUBSTRINGP thing1 thing2\n"
                    "SUBSTRING? thing1 thing2\n"
                    "\n"
                    "        if \"thing1\" or \"thing2\" is a list or an "
                    "array, outputs FALSE.  If\n"
                    "        \"thing2\" is a word, outputs TRUE if \"thing1\" "
                    "is EQUALP to a\n"
                    "        substring of \"thing2\", FALSE otherwise.\n"
                    "\n");

  alt("SUBSTRING?", "SUBSTRINGP");

  set("NUMBERP",
      "NUMBERP thing\n"
      "NUMBER? thing\n"
      "\n"
      "        outputs TRUE if the input is a number, FALSE otherwise.\n"
      "\n");

  alt("NUMBER?", "NUMBERP");

  set("VBARREDP",
      "VBARREDP char\n"
      "VBARRED? char\n"
      "BACKSLASHEDP char                               (library procedure)\n"
      "BACKSLASHED? char                               (library procedure)\n"
      "\n"
      "        outputs TRUE if the input character was originally entered into "
      "Logo\n"
      "        within vertical bars (|) to prevent its usual special "
      "syntactic\n"
      "        meaning, FALSE otherwise.  (Outputs TRUE only if the character "
      "is a\n"
      "        backslashed space, tab, newline, or one of ()[]+-*/=<>\":;\\~?| "
      ")\n"
      "\n"
      "        The names BACKSLASHEDP and BACKSLASHED? are included in the "
      "Logo\n"
      "        library for backward compatibility with the former names of "
      "this\n"
      "        primitive, although it does *not* output TRUE for characters\n"
      "        originally entered with backslashes.\n"
      "\n");

  alt("VBARRED?", "VBARREDP");
  alt("BACKSLASHEDP", "VBARREDP");
  alt("BACKSLASHED?", "VBARREDP");

  //    QUERIES
  //    -------

  set("COUNT",
      "COUNT thing\n"
      "\n"
      "        outputs the number of characters in the input, if the input is "
      "a word;\n"
      "        outputs the number of members in the input, if it is a list\n"
      "        or an array.  (For an array, this may or may not be the index "
      "of the\n"
      "        last member, depending on the array's origin.)\n"
      "\n");

  set("ASCII",
      "ASCII char\n"
      "\n"
      "        outputs the integer (between 0 and 255) that represents the "
      "input\n"
      "        character in the ASCII code.  Interprets control characters as\n"
      "        representing vbarred punctuation, and returns the character "
      "code\n"
      "        for the corresponding punctuation character without vertical "
      "bars.\n"
      "        (Compare RAWASCII.)\n"
      "\n");

  set("RAWASCII",
      "RAWASCII char\n"
      "\n"
      "        outputs the integer (between 0 and 255) that represents the "
      "input\n"
      "        character in the ASCII code.  Interprets control characters as\n"
      "        representing themselves.  To find out the ASCII code of an "
      "arbitrary\n"
      "        keystroke, use RAWASCII RC.\n"
      "\n");

  set("CHAR", "CHAR int\n"
              "\n"
              "        outputs the character represented in the ASCII code by "
              "the input,\n"
              "        which must be an integer between 0 and 255.\n"
              "\n");

  set("MEMBER", "MEMBER thing1 thing2\n"
                "\n"
                "        if \"thing2\" is a word or list and if MEMBERP with "
                "these inputs would\n"
                "        output TRUE, outputs the portion of \"thing2\" from "
                "the first instance\n"
                "        of \"thing1\" to the end.  If MEMBERP would output "
                "FALSE, outputs the\n"
                "        empty word or list according to the type of "
                "\"thing2\".  It is an error\n"
                "        for \"thing2\" to be an array.\n"
                "\n");

  set("LOWERCASE", "LOWERCASE word\n"
                   "\n"
                   "        outputs a copy of the input word, but with all "
                   "uppercase letters\n"
                   "        changed to the corresponding lowercase letter.\n"
                   "\n");

  set("UPPERCASE", "UPPERCASE word\n"
                   "\n"
                   "        outputs a copy of the input word, but with all "
                   "lowercase letters\n"
                   "        changed to the corresponding uppercase letter.\n"
                   "\n");

  set("STANDOUT",
      "STANDOUT thing\n"
      "\n"
      "        outputs a word that, when printed, will appear like the input "
      "but\n"
      "        displayed in standout mode (boldface, reverse video, or "
      "whatever your\n"
      "        version does for standout).  The word contains magic "
      "characters\n"
      "        at the beginning and end; in between is the printed form (as if "
      "\n"
      "        displayed using TYPE) of the input.  The output is always a "
      "word,\n"
      "        even if the input is of some other type, but it may include\n"
      "        spaces and other formatting characters.\n"
      "\n");

  set("PARSE",
      "PARSE word\n"
      "\n"
      "        outputs the list that would result if the input word were "
      "entered\n"
      "        in response to a READLIST operation.  That is, PARSE READWORD "
      "has\n"
      "        the same value as READLIST for the same characters read.\n"
      "\n");

  set("RUNPARSE",
      "RUNPARSE wordorlist\n"
      "\n"
      "        outputs the list that would result if the input word or list "
      "were\n"
      "        entered as an instruction line; characters such as infix "
      "operators\n"
      "        and parentheses are separate members of the output.  Note that\n"
      "        sublists of a runparsed list are not themselves runparsed.\n"
      "\n");
}

void Help::setCommunication() {
  //    TRANSMITTERS
  //    ------------

  set("PRINT", "PRINT thing\n"
               "PR thing\n"
               "(PRINT thing1 thing2 ...)\n"
               "(PR thing1 thing2 ...)\n"
               "\n"
               "        command.  Prints the input or inputs to the current "
               "write stream\n"
               "        (initially the screen).  All the inputs are printed on "
               "a single\n"
               "        line, separated by spaces, ending with a newline.  If "
               "an input is a\n"
               "        list, square brackets are not printed around it, but "
               "brackets are\n"
               "        printed around sublists.  Braces are always printed "
               "around arrays.\n"
               "\n");

  alt("PR", "PRINT");

  set("TYPE",
      "TYPE thing\n"
      "(TYPE thing1 thing2 ...)\n"
      "\n"
      "        command.  Prints the input or inputs like PRINT, except that "
      "no\n"
      "        newline character is printed at the end and multiple inputs are "
      "not\n"
      "        separated by spaces.  Note: printing to the terminal is "
      "ordinarily\n"
      "        \"line buffered\"; that is, the characters you print using TYPE "
      "will\n"
      "        not actually appear on the screen until either a newline "
      "character\n"
      "        is printed (for example, by PRINT or SHOW) or Logo tries to "
      "read\n"
      "        from the keyboard (either at the request of your program or "
      "after an\n"
      "        instruction prompt).  This buffering makes the program much "
      "faster\n"
      "        than it would be if each character appeared immediately, and in "
      "most\n"
      "        cases the effect is not disconcerting.  To accommodate programs "
      "that\n"
      "        do a lot of positioned text display using TYPE, Logo will "
      "force\n"
      "        printing whenever SETCURSOR is invoked.  This solves most "
      "buffering\n"
      "        problems.  Still, on occasion you may find it necessary to "
      "force the\n"
      "        buffered characters to be printed explicitly; this can be done "
      "using\n"
      "        the WAIT command.  WAIT 0 will force printing without actually\n"
      "        waiting.\n"
      "\n");

  set("SHOW",
      "SHOW thing\n"
      "(SHOW thing1 thing2 ...)\n"
      "\n"
      "        command.  Prints the input or inputs like PRINT, except that\n"
      "        if an input is a list it is printed inside square brackets.\n"
      "\n");

  //    RECEIVERS
  //    ---------

  set("READLIST",
      "READLIST\n"
      "RL\n"
      "\n"
      "        reads a line from the read stream (initially the keyboard) and\n"
      "        outputs that line as a list.  The line is separated into "
      "members as\n"
      "        though it were typed in square brackets in an instruction.  If "
      "the\n"
      "        read stream is a file, and the end of file is reached, "
      "READLIST\n"
      "        outputs the empty word (not the empty list).  READLIST "
      "processes\n"
      "        backslash, vertical bar, and tilde characters in the read "
      "stream;\n"
      "        the output list will not contain these characters but they will "
      "have\n"
      "        had their usual effect.  READLIST does not, however, treat "
      "semicolon\n"
      "        as a comment character.\n"
      "\n");

  alt("RL", "READLIST");

  set("READWORD",
      "READWORD\n"
      "RW\n"
      "\n"
      "        reads a line from the read stream and outputs that line as a "
      "word.\n"
      "        The output is a single word even if the line contains spaces,\n"
      "        brackets, etc.  If the read stream is a file, and the end of "
      "file is\n"
      "        reached, READWORD outputs the empty list (not the empty word).\n"
      "        READWORD processes backslash, vertical bar, and tilde "
      "characters in\n"
      "        the read stream.  In the case of a tilde used for line "
      "continuation,\n"
      "        the output word DOES include the tilde and the newline "
      "characters, so\n"
      "        that the user program can tell exactly what the user entered.\n"
      "        Vertical bars in the line are also preserved in the output.\n"
      "        Backslash characters are not preserved in the output.\n"
      "\n");

  alt("RW", "READWORD");

  set("READRAWLINE",
      "READRAWLINE\n"
      "\n"
      "        reads a line from the read stream and outputs that line as a "
      "word.\n"
      "        The output is a single word even if the line contains spaces,\n"
      "        brackets, etc.  If the read stream is a file, and the end of "
      "file is\n"
      "        reached, READRAWLINE outputs the empty list (not the empty "
      "word).\n"
      "        READRAWLINE outputs the exact string of characters as they "
      "appear\n"
      "        in the line, with no special meaning for backslash, vertical "
      "bar,\n"
      "        tilde, or any other formatting characters.\n"
      "\n");

  set("READCHAR",
      "READCHAR\n"
      "RC\n"
      "\n"
      "        reads a single character from the read stream and outputs that\n"
      "        character as a word.  If the read stream is a file, and the end "
      "of\n"
      "        file is reached, READCHAR outputs the empty list (not the "
      "empty\n"
      "        word).  If the read stream is the keyboard, echoing is turned "
      "off\n"
      "        when READCHAR is invoked, and remains off until READLIST or "
      "READWORD\n"
      "        is invoked or a Logo prompt is printed.  Backslash, vertical "
      "bar,\n"
      "        and tilde characters have no special meaning in this context.\n"
      "\n");

  alt("RC", "READCHAR");

  set("READCHARS",
      "READCHARS num\n"
      "RCS num\n"
      "\n"
      "        reads \"num\" characters from the read stream and outputs "
      "those\n"
      "        characters as a word.  If the read stream is a file, and the "
      "end of\n"
      "        file is reached, READCHARS outputs the empty list (not the "
      "empty\n"
      "        word).  If the read stream is a terminal, echoing is turned "
      "off\n"
      "        when READCHARS is invoked, and remains off until READLIST or "
      "READWORD\n"
      "        is invoked or a Logo prompt is printed.  Backslash, vertical "
      "bar,\n"
      "        and tilde characters have no special meaning in this context.\n"
      "\n");

  alt("RCS", "READCHARS");

  set("SHELL",
      "SHELL command\n"
      "(SHELL command wordflag)\n"
      "\n"
      "        Under Unix, outputs the result of running \"command\" as a "
      "shell\n"
      "        command.  (The command is sent to /bin/sh, not csh or other\n"
      "        alternatives.)  If the command is a literal list in the "
      "instruction\n"
      "        line, and if you want a backslash character sent to the shell, "
      "you\n"
      "        must use \\\\ to get the backslash through Logo's reader "
      "intact.  The\n"
      "        output is a list containing one member for each line generated "
      "by\n"
      "        the shell command.  Ordinarily each such line is represented by "
      "a\n"
      "        list in the output, as though the line were read using "
      "READLIST.  If\n"
      "        a second input is given, regardless of the value of the input, "
      "each\n"
      "        line is represented by a word in the output as though it were "
      "read\n"
      "        with READWORD.  Example:\n"
      "\n"
      "                        to dayofweek\n"
      "                        output first first shell [date]\n"
      "                        end\n"
      "\n"
      "        This is \"first first\" to extract the first word of the first "
      "(and\n"
      "        only) line of the shell output.\n"
      "\n"
      "        Under MacOS X, SHELL works as under Unix.  SHELL is not "
      "available\n"
      "        under Mac Classic.\n"
      "\n"
      "        Under DOS, SHELL is a command, not an operation; it sends its\n"
      "        input to a DOS command processor but does not collect the "
      "result\n"
      "        of the command.\n"
      "\n"
      "        Under Windows, the wxWidgets version of Logo behaves as under "
      "Unix (except\n"
      "        that DOS-style commands are understood; use \"dir\" rather than "
      "\"ls\").\n"
      "        The non-wxWidgets version behaves like the DOS version.\n"
      "\n");

  //    FILE ACCESS
  //    -----------

  set("SETPREFIX",
      "SETPREFIX string\n"
      "\n"
      "        command.  Sets a prefix that will be used as the implicit "
      "beginning\n"
      "        of filenames in OPENREAD, OPENWRITE, OPENAPPEND, OPENUPDATE, "
      "LOAD,\n"
      "        and SAVE commands.  Logo will put the appropriate separator\n"
      "        character (slash for Unix, backslash for DOS/Windows, colon "
      "for\n"
      "        MacOS Classic) between the prefix and the filename entered by "
      "the user.\n"
      "        The input to SETPREFIX must be a word, unless it is the empty "
      "list,\n"
      "        to indicate that there should be no prefix.\n"
      "\n");

  set("PREFIX",
      "PREFIX\n"
      "\n"
      "        outputs the current file prefix, or [] if there is no prefix.\n"
      "        See SETPREFIX.\n"
      "\n");

  set("OPENREAD", "OPENREAD filename\n"
                  "\n"
                  "        command.  Opens the named file for reading.  The "
                  "read position is\n"
                  "        initially at the beginning of the file.\n"
                  "\n");

  set("OPENWRITE",
      "OPENWRITE filename\n"
      "\n"
      "        command.  Opens the named file for writing.  If the file "
      "already\n"
      "        existed, the old version is deleted and a new, empty file "
      "created.\n"
      "\n"
      "        OPENWRITE, but not the other OPEN variants, will accept as "
      "input\n"
      "        a two-element list, in which the first element must be a "
      "variable\n"
      "        name, and the second must be a positive integer.  A character\n"
      "        buffer of the specified size will be created.  When a SETWRITE "
      "is\n"
      "        done with this same list (in the sense of .EQ, not a copy, so\n"
      "        you must do something like\n"
      "                ? make \"buf [foo 100]\n"
      "                ? openwrite :buf\n"
      "                ? setwrite :buf\n"
      "                    [...]\n"
      "                ? close :buf\n"
      "        and not just\n"
      "                ? openwrite [foo 100]\n"
      "                ? setwrite [foo 100]\n"
      "        and so on), the printed characters are stored in the buffer;\n"
      "        when a CLOSE is done with the same list as input, the "
      "characters\n"
      "        from the buffer (treated as one long word, even if spaces and\n"
      "        newlines are included) become the value of the specified "
      "variable.\n"
      "\n");

  set("OPENAPPEND",
      "OPENAPPEND filename\n"
      "\n"
      "        command.  Opens the named file for writing.  If the file "
      "already\n"
      "        exists, the write position is initially set to the end of the "
      "old\n"
      "        file, so that newly written data will be appended to it.\n"
      "\n");

  set("OPENUPDATE", "OPENUPDATE filename\n"
                    "\n"
                    "        command.  Opens the named file for reading and "
                    "writing.  The read and\n"
                    "        write position is initially set to the end of the "
                    "old file, if any.\n"
                    "        Note: each open file has only one position, for "
                    "both reading and\n"
                    "        writing.  If a file opened for update is both "
                    "READER and WRITER at\n"
                    "        the same time, then SETREADPOS will also affect "
                    "WRITEPOS and vice\n"
                    "        versa.  Also, if you alternate reading and "
                    "writing the same file,\n"
                    "        you must SETREADPOS between a write and a read, "
                    "and SETWRITEPOS\n"
                    "        between a read and a write.\n"
                    "\n");

  set("CLOSE",
      "CLOSE filename\n"
      "\n"
      "        command.  Closes the named file.  If the file was currently "
      "the\n"
      "        reader or writer, then the reader or writer is changed to the\n"
      "        keyboard or screen, as if SETREAD [] or SETWRITE [] had been "
      "done.\n"
      "\n");

  set("ALLOPEN",
      "ALLOPEN\n"
      "\n"
      "        outputs a list whose members are the names of all files "
      "currently open.\n"
      "        This list does not include the dribble file, if any.\n"
      "\n");

  set("CLOSEALL", "CLOSEALL                                                "
                  "(library procedure)\n"
                  "\n"
                  "        command.  Closes all open files.  Abbreviates\n"
                  "        FOREACH ALLOPEN [CLOSE ?]\n"
                  "\n");

  set("ERASEFILE", "ERASEFILE filename\n"
                   "ERF filename\n"
                   "\n"
                   "        command.  Erases (deletes, removes) the named "
                   "file, which should not\n"
                   "        currently be open.\n"
                   "\n");

  alt("ERF", "ERASEFILE");

  set("DRIBBLE",
      "DRIBBLE filename\n"
      "\n"
      "        command.  Creates a new file whose name is the input, like "
      "OPENWRITE,\n"
      "        and begins recording in that file everything that is read from "
      "the\n"
      "        keyboard or written to the terminal.  That is, this writing is "
      "in\n"
      "        addition to the writing to WRITER.  The intent is to create a\n"
      "        transcript of a Logo session, including things like prompt\n"
      "        characters and interactions.\n"
      "\n");

  set("NODRIBBLE",
      "NODRIBBLE\n"
      "\n"
      "        command.  Stops copying information into the dribble file, and\n"
      "        closes the file.\n"
      "\n");

  set("SETREAD",
      "SETREAD filename\n"
      "\n"
      "        command.  Makes the named file the read stream, used for "
      "READLIST,\n"
      "        etc.  The file must already be open with OPENREAD or "
      "OPENUPDATE.  If\n"
      "        the input is the empty list, then the read stream becomes the\n"
      "        keyboard, as usual.  Changing the read stream does not close "
      "the\n"
      "        file that was previously the read stream, so it is possible to\n"
      "        alternate between files.\n"
      "\n");

  set("SETWRITE",
      "SETWRITE filename\n"
      "\n"
      "        command.  Makes the named file the write stream, used for "
      "PRINT,\n"
      "        etc.  The file must already be open with OPENWRITE, OPENAPPEND, "
      "or\n"
      "        OPENUPDATE.  If the input is the empty list, then the write "
      "stream\n"
      "        becomes the screen, as usual.  Changing the write stream does\n"
      "        not close the file that was previously the write stream, so it "
      "is\n"
      "        possible to alternate between files.\n"
      "\n"
      "        If the input is a list, then its first element must be a "
      "variable\n"
      "        name, and its second and last element must be a positive "
      "integer; a\n"
      "        buffer of that many characters will be allocated, and will "
      "become the\n"
      "        writestream.  If the same list (same in the .EQ sense, not a "
      "copy)\n"
      "        has been used as input to OPENWRITE, then the "
      "already-allocated\n"
      "        buffer will be used, and the writer can be changed to and from "
      "this\n"
      "        buffer, with all the characters accumulated as in a file.  When "
      "the\n"
      "        same list is used as input to CLOSE, the contents of the "
      "buffer\n"
      "        (as an unparsed word, which may contain newline characters) "
      "will\n"
      "        become the value of the named variable.  For compatibility "
      "with\n"
      "        earlier versions, if the list has not been opened when the "
      "SETWRITE\n"
      "        is done, it will be opened implicitly, but the first SETWRITE "
      "after\n"
      "        this one will implicitly close it, setting the variable and "
      "freeing\n"
      "        the allocated buffer.\n"
      "\n");

  set("READER", "READER\n"
                "\n"
                "        outputs the name of the current read stream file, or "
                "the empty list\n"
                "        if the read stream is the terminal.\n"
                "\n");

  set("WRITER", "WRITER\n"
                "\n"
                "        outputs the name of the current write stream file, or "
                "the empty list\n"
                "        if the write stream is the screen.\n"
                "\n");

  set("SETREADPOS", "SETREADPOS charpos\n"
                    "\n"
                    "        command.  Sets the file pointer of the read "
                    "stream file so that the\n"
                    "        next READLIST, etc., will begin reading at the "
                    "\"charpos\"th character\n"
                    "        in the file, counting from 0.  (That is, "
                    "SETREADPOS 0 will start\n"
                    "        reading from the beginning of the file.)  "
                    "Meaningless if the read\n"
                    "        stream is the screen.\n"
                    "\n");

  set("SETWRITEPOS", "SETWRITEPOS charpos\n"
                     "\n"
                     "        command.  Sets the file pointer of the write "
                     "stream file so that the\n"
                     "        next PRINT, etc., will begin writing at the "
                     "\"charpos\"th character\n"
                     "        in the file, counting from 0.  (That is, "
                     "SETWRITEPOS 0 will start\n"
                     "        writing from the beginning of the file.)  "
                     "Meaningless if the write\n"
                     "        stream is the screen.\n"
                     "\n");

  set("READPOS",
      "READPOS\n"
      "\n"
      "        outputs the file position of the current read stream file.\n"
      "\n");

  set("WRITEPOS",
      "WRITEPOS\n"
      "\n"
      "        outputs the file position of the current write stream file.\n"
      "\n");

  set("EOFP",
      "EOFP\n"
      "EOF?\n"
      "\n"
      "        predicate, outputs TRUE if there are no more characters to be\n"
      "        read in the read stream file, FALSE otherwise.\n"
      "\n");

  alt("EOF?", "EOFP");

  set("FILEP",
      "FILEP filename\n"
      "FILE? filename                                          (library "
      "procedure)\n"
      "\n"
      "        predicate, outputs TRUE if a file of the specified name exists\n"
      "        and can be read, FALSE otherwise.\n"
      "\n");

  alt("FILE?", "FILEP");

  //    TERMINAL ACCESS
  //    ---------------

  set("KEYP",
      "KEYP\n"
      "KEY?\n"
      "\n"
      "        predicate, outputs TRUE if there are characters waiting to be\n"
      "        read from the read stream.  If the read stream is a file, this\n"
      "        is equivalent to NOT EOFP.  If the read stream is the "
      "terminal,\n"
      "        then echoing is turned off and the terminal is set to CBREAK\n"
      "        (character at a time instead of line at a time) mode.  It\n"
      "        remains in this mode until some line-mode reading is requested\n"
      "        (e.g., READLIST).  The Unix operating system forgets about any\n"
      "        pending characters when it switches modes, so the first KEYP\n"
      "        invocation will always output FALSE.\n"
      "\n");

  alt("KEY?", "KEYP");

  set("CLEARTEXT", "CLEARTEXT\n"
                   "CT\n"
                   "\n"
                   "        command.  Clears the text window.\n"
                   "\n");

  alt("CT", "CLEARTEXT");

  set("SETCURSOR",
      "SETCURSOR vector\n"
      "\n"
      "        command.  The input is a list of two numbers, the x and y\n"
      "        coordinates of a text window position (origin in the upper "
      "left\n"
      "        corner, positive direction is southeast).  The text cursor\n"
      "        is moved to the requested position.  This command also forces\n"
      "        the immediate printing of any buffered characters.\n"
      "\n");

  set("CURSOR",
      "CURSOR\n"
      "\n"
      "        outputs a list containing the current x and y coordinates of\n"
      "        the text cursor.  Logo may get confused about the current\n"
      "        cursor position if, e.g., you type in a long line that wraps\n"
      "        around or your program prints escape codes that affect the\n"
      "        screen strangely.\n"
      "\n");

  set("SETMARGINS",
      "SETMARGINS vector\n"
      "\n"
      "        command.  The input must be a list of two numbers, as for\n"
      "        SETCURSOR.  The effect is to clear the screen and then arrange "
      "for\n"
      "        all further printing to be shifted down and to the right "
      "according\n"
      "        to the indicated margins.  Specifically, every time a newline\n"
      "        character is printed (explicitly or implicitly) Logo will type\n"
      "        x_margin spaces, and on every invocation of SETCURSOR the "
      "margins\n"
      "        will be added to the input x and y coordinates.  (CURSOR will "
      "report\n"
      "        the cursor position relative to the margins, so that this shift "
      "will\n"
      "        be invisible to Logo programs.)  The purpose of this command is "
      "to\n"
      "        accommodate the display of terminal screens in lecture halls "
      "with\n"
      "        inadequate TV monitors that miss the top and left edges of the\n"
      "        screen.\n"
      "\n");

  set("SETTEXTCOLOR", "SETTEXTCOLOR foreground background\n"
                      "SETTC foreground background\n"
                      "\n"
                      "        command (wxWidgets only).  The inputs are color "
                      "numbers, or RGB color\n"
                      "        lists, as for turtle graphics.  The foreground "
                      "and background colors\n"
                      "        for the textscreen/splitscreen text window are "
                      "changed to the given\n"
                      "        values.  The change affects text already "
                      "printed as well as future\n"
                      "        text printing; there is only one text color for "
                      "the entire window.\n"
                      "\n"
                      "        command (non-wxWidgets Windows and DOS extended "
                      "only).  The inputs are\n"
                      "        color numbers, as for turtle graphics.  Future "
                      "printing to the text\n"
                      "        window will use the specified colors for "
                      "foreground (the characters\n"
                      "        printed) and background (the space under those "
                      "characters).  Using\n"
                      "        STANDOUT will revert to the default text window "
                      "colors.  In the DOS\n"
                      "        extended (ucblogo.exe) version, colors in "
                      "textscreen mode are limited\n"
                      "        to numbers 0-7, and the coloring applies only "
                      "to text printed by the\n"
                      "        program, not to the echoing of text typed by "
                      "the user.  Neither\n"
                      "        limitation applies to the text portion of "
                      "splitscreen mode, which is\n"
                      "        actually drawn as graphics internally.\n"
                      "\n");

  alt("SETTC", "SETTEXTCOLOR");

  set("INCREASEFONT", "INCREASEFONT\n"
                      "DECREASEFONT\n"
                      "\n"
                      "        command (wxWidgets only).  Increase or decrease "
                      "the size of the font\n"
                      "        used in the text and edit windows to the next "
                      "larger or smaller\n"
                      "        available size.\n"
                      "\n");

  alt("DECREASEFONT", "INCREASEFONT");

  set("SETTEXTSIZE", "SETTEXTSIZE height\n"
                     "\n"
                     "        command (wxWidgets only).  Set the \"point "
                     "size\" of the font used in\n"
                     "        the text and edit windows to the given integer "
                     "input.  The desired\n"
                     "        size may not be available, in which case the "
                     "nearest available size\n"
                     "        will be used.  Note: There is only a slight "
                     "correlation between these\n"
                     "        integers and pixel sizes.  Our rough estimate is "
                     "that the number of\n"
                     "        pixels of height is about 1.5 times the point "
                     "size, but it varies for\n"
                     "        different fonts.  See SETLABELHEIGHT for a "
                     "different approach used for\n"
                     "        the graphics window.\n"
                     "\n");

  set("TEXTSIZE", "TEXTSIZE\n"
                  "\n"
                  "        (wxWidgets only) outputs the \"point size\" of the "
                  "font used in the text\n"
                  "        and edit windows.  See SETTEXTSIZE for a discussion "
                  "of font sizing.\n"
                  "        See LABELSIZE for a different approach used for the "
                  "graphics window.\n"
                  "\n");

  set("SETFONT",
      "SETFONT fontname\n"
      "\n"
      "        command (wxWidgets only).  Set the font family used in all "
      "windows\n"
      "        to the one named by the input.  Try 'Courier' or 'Monospace' as "
      "likely\n"
      "        possibilities.  Not all computers have the same fonts "
      "installed.  It's\n"
      "        a good idea to stick with monospace fonts (ones in which all\n"
      "        characters have the same width).\n"
      "\n");

  set("FONT", "FONT\n"
              "\n"
              "        (wxWidgets only) outputs the name of the font family "
              "used in all\n"
              "        windows.\n"
              "\n");
}

void Help::setArithmetic() {
  //    NUMERIC OPERATIONS
  //    ------------------

  set("SUM", "SUM num1 num2\n"
             "(SUM num1 num2 num3 ...)\n"
             "num1 + num2\n"
             "\n"
             "        outputs the sum of its inputs.\n"
             "\n");

  alt("+", "SUM");

  set("DIFFERENCE",
      "DIFFERENCE num1 num2\n"
      "num1 - num2\n"
      "\n"
      "        outputs the difference of its inputs.  Minus sign means infix\n"
      "        difference in ambiguous contexts (when preceded by a complete\n"
      "        expression), unless it is preceded by a space and followed\n"
      "        by a nonspace.  (See also MINUS.)\n"
      "\n");

  alt("-", "DIFFERENCE");

  set("MINUS", "MINUS num\n"
               "- num\n"
               "\n"
               "        outputs the negative of its input.  Minus sign means "
               "unary minus if\n"
               "        the previous token is an infix operator or open "
               "parenthesis, or it is\n"
               "        preceded by a space and followed by a nonspace.  There "
               "is a difference\n"
               "        in binding strength between the two forms:\n"
               "\n"
               "                MINUS 3 + 4     means   -(3+4)\n"
               "                - 3 + 4         means   (-3)+4\n"
               "\n");

  set("PRODUCT", "PRODUCT num1 num2\n"
                 "(PRODUCT num1 num2 num3 ...)\n"
                 "num1 * num2\n"
                 "\n"
                 "        outputs the product of its inputs.\n"
                 "\n");

  alt("*", "PRODUCT");

  set("QUOTIENT",
      "QUOTIENT num1 num2\n"
      "(QUOTIENT num)\n"
      "num1 / num2\n"
      "\n"
      "        outputs the quotient of its inputs.  The quotient of two "
      "integers\n"
      "        is an integer if and only if the dividend is a multiple of the "
      "divisor.\n"
      "        (In other words, QUOTIENT 5 2 is 2.5, not 2, but QUOTIENT 4 2 "
      "is\n"
      "        2, not 2.0 -- it does the right thing.)  With a single input,\n"
      "        QUOTIENT outputs the reciprocal of the input.\n"
      "\n");

  alt("/", "QUOTIENT");

  set("REMAINDER", "REMAINDER num1 num2\n"
                   "\n"
                   "        outputs the remainder on dividing \"num1\" by "
                   "\"num2\"; both must be\n"
                   "        integers and the result is an integer with the "
                   "same sign as num1.\n"
                   "\n");

  set("MODULO", "MODULO num1 num2\n"
                "\n"
                "        outputs the remainder on dividing \"num1\" by "
                "\"num2\"; both must be\n"
                "        integers and the result is an integer with the same "
                "sign as num2.\n"
                "\n");

  set("INT",
      "INT num\n"
      "\n"
      "        outputs its input with fractional part removed, i.e., an "
      "integer\n"
      "        with the same sign as the input, whose absolute value is the\n"
      "        largest integer less than or equal to the absolute value of\n"
      "        the input.\n"
      "\n");

  set("ROUND", "ROUND num\n"
               "\n"
               "        outputs the nearest integer to the input.\n"
               "\n");

  set("SQRT", "SQRT num\n"
              "\n"
              "        outputs the square root of the input, which must be "
              "nonnegative.\n"
              "\n");

  set("POWER", "POWER num1 num2\n"
               "\n"
               "        outputs \"num1\" to the \"num2\" power.  If num1 is "
               "negative, then\n"
               "        num2 must be an integer.\n"
               "\n");

  set("EXP", "EXP num\n"
             "\n"
             "        outputs e (2.718281828+) to the input power.\n"
             "\n");

  set("LOG10", "LOG10 num\n"
               "\n"
               "        outputs the common logarithm of the input.\n"
               "\n");

  set("LN", "LN num\n"
            "\n"
            "        outputs the natural logarithm of the input.\n"
            "\n");

  set("SIN",
      "SIN degrees\n"
      "\n"
      "        outputs the sine of its input, which is taken in degrees.\n"
      "\n");

  set("RADSIN",
      "RADSIN radians\n"
      "\n"
      "        outputs the sine of its input, which is taken in radians.\n"
      "\n");

  set("COS",
      "COS degrees\n"
      "\n"
      "        outputs the cosine of its input, which is taken in degrees.\n"
      "\n");

  set("RADCOS",
      "RADCOS radians\n"
      "\n"
      "        outputs the cosine of its input, which is taken in radians.\n"
      "\n");

  set("ARCTAN",
      "ARCTAN num\n"
      "(ARCTAN x y)\n"
      "\n"
      "        outputs the arctangent, in degrees, of its input.  With two\n"
      "        inputs, outputs the arctangent of y/x, if x is nonzero, or\n"
      "        90 or -90 depending on the sign of y, if x is zero.\n"
      "\n");

  set("RADARCTAN",
      "RADARCTAN num\n"
      "(RADARCTAN x y)\n"
      "\n"
      "        outputs the arctangent, in radians, of its input.  With two\n"
      "        inputs, outputs the arctangent of y/x, if x is nonzero, or\n"
      "        pi/2 or -pi/2 depending on the sign of y, if x is zero.\n"
      "\n"
      "        The expression 2*(RADARCTAN 0 1) can be used to get the\n"
      "        value of pi.\n"
      "\n");

  set("ISEQ",
      "ISEQ from to                                            (library "
      "procedure)\n"
      "\n"
      "        outputs a list of the integers from FROM to TO, inclusive.\n"
      "\n"
      "                ? show iseq 3 7\n"
      "                [3 4 5 6 7]\n"
      "                ? show iseq 7 3\n"
      "                [7 6 5 4 3]\n"
      "\n");

  set("RSEQ",
      "RSEQ from to count                                      (library "
      "procedure)\n"
      "\n"
      "        outputs a list of COUNT equally spaced rational numbers\n"
      "        between FROM and TO, inclusive.\n"
      "\n"
      "                ? show rseq 3 5 9\n"
      "                [3 3.25 3.5 3.75 4 4.25 4.5 4.75 5]\n"
      "                ? show rseq 3 5 5\n"
      "                [3 3.5 4 4.5 5]\n"
      "\n");

  //    "PREDICATES
  //    ----------

  set("LESSP", "LESSP num1 num2\n"
               "LESS? num1 num2\n"
               "num1 < num2\n"
               "\n"
               "        outputs TRUE if its first input is strictly less than "
               "its second.\n"
               "\n");

  alt("LESS?", "LESSP");
  alt("<", "LESSP");

  set("GREATERP", "GREATERP num1 num2\n"
                  "GREATER? num1 num2\n"
                  "num1 > num2\n"
                  "\n"
                  "        outputs TRUE if its first input is strictly greater "
                  "than its second.\n"
                  "\n");

  alt("GREATER?", "GREATERP");
  alt(">", "GREATERP");

  set("LESSEQUALP", "LESSEQUALP num1 num2\n"
                    "LESSEQUAL? num1 num2\n"
                    "num1 <= num2\n"
                    "\n"
                    "        outputs TRUE if its first input is less than or "
                    "equal to its second.\n"
                    "\n");

  alt("LESSEQUAL?", "LESSEQUALP");
  alt("<=", "LESSEQUALP");

  set("GREATEREQUALP", "GREATEREQUALP num1 num2\n"
                       "GREATEREQUAL? num1 num2\n"
                       "num1 >= num2\n"
                       "\n"
                       "        outputs TRUE if its first input is greater "
                       "than or equal to its second.\n"
                       "\n"
                       "\n");

  alt("GREATEREQUAL?", "GREATEREQUALP");
  alt(">=", "GREATEREQUALP");

  //    RANDOM NUMBERS
  //    --------------

  set("RANDOM",
      "RANDOM num\n"
      "(RANDOM start end)\n"
      "\n"
      "        with one input, outputs a random nonnegative integer less than "
      "its\n"
      "        input, which must be a positive integer.\n"
      "\n"
      "        With two inputs, RANDOM outputs a random integer greater than "
      "or\n"
      "        equal to the first input, and less than or equal to the second\n"
      "        input.  Both inputs must be integers, and the first must be "
      "less\n"
      "        than the second.  (RANDOM 0 9) is equivalent to RANDOM 10;\n"
      "        (RANDOM 3 8) is equivalent to (RANDOM 6)+3.\n"
      "\n");

  set("RERANDOM",
      "RERANDOM\n"
      "(RERANDOM seed)\n"
      "\n"
      "        command.  Makes the results of RANDOM reproducible.  "
      "Ordinarily\n"
      "        the sequence of random numbers is different each time Logo is\n"
      "        used.  If you need the same sequence of pseudo-random numbers\n"
      "        repeatedly, e.g. to debug a program, say RERANDOM before the\n"
      "        first invocation of RANDOM.  If you need more than one "
      "repeatable\n"
      "        sequence, you can give RERANDOM an integer input; each "
      "possible\n"
      "        input selects a unique sequence of numbers.\n"
      "\n");

  //    PRINT FORMATTING
  //    ----------------

  set("FORM",
      "FORM num width precision\n"
      "\n"
      "        outputs a word containing a printable representation of "
      "\"num\",\n"
      "        possibly preceded by spaces (and therefore not a number for\n"
      "        purposes of performing arithmetic operations), with at least\n"
      "        \"width\" characters, including exactly \"precision\" digits "
      "after\n"
      "        the decimal point.  (If \"precision\" is 0 then there will be "
      "no\n"
      "        decimal point in the output.)\n"
      "\n");

  //    BITWISE OPERATIONS
  //    ------------------

  set("BITAND",
      "BITAND num1 num2\n"
      "(BITAND num1 num2 num3 ...)\n"
      "\n"
      "        outputs the bitwise AND of its inputs, which must be integers.\n"
      "\n");

  set("BITOR",
      "BITOR num1 num2\n"
      "(BITOR num1 num2 num3 ...)\n"
      "\n"
      "        outputs the bitwise OR of its inputs, which must be integers.\n"
      "\n");

  set("BITXOR",
      "BITXOR num1 num2\n"
      "(BITXOR num1 num2 num3 ...)\n"
      "\n"
      "        outputs the bitwise EXCLUSIVE OR of its inputs, which must be\n"
      "        integers.\n"
      "\n");

  set("BITNOT", "BITNOT num\n"
                "\n"
                "        outputs the bitwise NOT of its input, which must be "
                "an integer.\n"
                "\n");

  set("ASHIFT",
      "ASHIFT num1 num2\n"
      "\n"
      "        outputs \"num1\" arithmetic-shifted to the left by \"num2\" "
      "bits.\n"
      "        If num2 is negative, the shift is to the right with sign\n"
      "        extension.  The inputs must be integers.\n"
      "\n");

  set("LSHIFT",
      "LSHIFT num1 num2\n"
      "\n"
      "        outputs \"num1\" logical-shifted to the left by \"num2\" bits.\n"
      "        If num2 is negative, the shift is to the right with zero fill.\n"
      "        The inputs must be integers.\n"
      "\n");

  //    LOGICAL OPERATIONS
  //    ==================

  set("AND",
      "AND tf1 tf2\n"
      "(AND tf1 tf2 tf3 ...)\n"
      "\n"
      "        outputs TRUE if all inputs are TRUE, otherwise FALSE.  All "
      "inputs\n"
      "        must be TRUE or FALSE.  (Comparison is case-insensitive "
      "regardless\n"
      "        of the value of CASEIGNOREDP.  That is, \"true\" or \"True\" or "
      "\"TRUE\"\n"
      "        are all the same.)  An input can be a list, in which case it "
      "is\n"
      "        taken as an expression to run; that expression must produce a "
      "TRUE\n"
      "        or FALSE value.  List expressions are evaluated from left to "
      "right;\n"
      "        as soon as a FALSE value is found, the remaining inputs are "
      "not\n"
      "        examined.  Example:\n"
      "                MAKE \"RESULT AND [NOT (:X = 0)] [(1 / :X) > .5]\n"
      "        to avoid the division by zero if the first part is false.\n"
      "\n");

  set("OR",
      "OR tf1 tf2\n"
      "(OR tf1 tf2 tf3 ...)\n"
      "\n"
      "        outputs TRUE if any input is TRUE, otherwise FALSE.  All "
      "inputs\n"
      "        must be TRUE or FALSE.  (Comparison is case-insensitive "
      "regardless\n"
      "        of the value of CASEIGNOREDP.  That is, \"true\" or \"True\" or "
      "\"TRUE\"\n"
      "        are all the same.)  An input can be a list, in which case it "
      "is\n"
      "        taken as an expression to run; that expression must produce a "
      "TRUE\n"
      "        or FALSE value.  List expressions are evaluated from left to "
      "right;\n"
      "        as soon as a TRUE value is found, the remaining inputs are not\n"
      "        examined.  Example:\n"
      "                IF OR :X=0 [some.long.computation] [...]\n"
      "        to avoid the long computation if the first condition is met.\n"
      "\n");

  set("NOT", "NOT tf\n"
             "\n"
             "        outputs TRUE if the input is FALSE, and vice versa.  The "
             "input can be\n"
             "        a list, in which case it is taken as an expression to "
             "run; that\n"
             "        expression must produce a TRUE or FALSE value.\n"
             "\n");
}

void Help::setGraphics() {

  //    TURTLE MOTION
  //    -------------

  set("FORWARD", "FORWARD dist\n"
                 "FD dist\n"
                 "\n"
                 "        moves the turtle forward, in the direction that it's "
                 "facing, by\n"
                 "        the specified distance (measured in turtle steps).\n"
                 "\n");

  alt("FD", "FORWARD");

  set("BACK", "BACK dist\n"
              "BK dist\n"
              "\n"
              "        moves the turtle backward, i.e., exactly opposite to "
              "the direction\n"
              "        that it's facing, by the specified distance.  (The "
              "heading of the\n"
              "        turtle does not change.)\n"
              "\n");

  alt("BK", "BACK");

  set("LEFT", "LEFT degrees\n"
              "LT degrees\n"
              "\n"
              "        turns the turtle counterclockwise by the specified "
              "angle, measured\n"
              "        in degrees (1/360 of a circle).\n"
              "\n");

  alt("LT", "LEFT");

  set("RIGHT",
      "RIGHT degrees\n"
      "RT degrees\n"
      "\n"
      "        turns the turtle clockwise by the specified angle, measured in\n"
      "        degrees (1/360 of a circle).\n"
      "\n");

  alt("RT", "RIGHT");

  set("SETPOS",
      "SETPOS pos\n"
      "\n"
      "        moves the turtle to an absolute position in the graphics "
      "window.  The\n"
      "        input is a list of two numbers, the X and Y coordinates.\n"
      "\n");

  set("SETXY", "SETXY xcor ycor\n"
               "\n"
               "        moves the turtle to an absolute position in the "
               "graphics window.  The\n"
               "        two inputs are numbers, the X and Y coordinates.\n"
               "\n");

  set("SETX",
      "SETX xcor\n"
      "\n"
      "        moves the turtle horizontally from its old position to a new\n"
      "        absolute horizontal coordinate.  The input is the new X\n"
      "        coordinate.\n"
      "\n");

  set("SETY",
      "SETY ycor\n"
      "\n"
      "        moves the turtle vertically from its old position to a new\n"
      "        absolute vertical coordinate.  The input is the new Y\n"
      "        coordinate.\n"
      "\n");

  set("SETHEADING",
      "SETHEADING degrees\n"
      "SETH degrees\n"
      "\n"
      "        turns the turtle to a new absolute heading.  The input is\n"
      "        a number, the heading in degrees counter-clockwise from the "
      "positive\n"
      "        Y axis.\n"
      "\n");

  alt("SETH", "SETHEADING");

  set("HOME",
      "HOME\n"
      "\n"
      "        moves the turtle to the center of the screen.  Equivalent to\n"
      "        SETPOS [0 0] SETHEADING 0.\n"
      "\n");

  set("ARC", "ARC angle radius\n"
             "\n"
             "        draws an arc of a circle, with the turtle at the center, "
             "with the\n"
             "        specified radius, starting at the turtle's heading and "
             "extending\n"
             "        counter-clockwise through the specified angle.  The "
             "turtle does not move.\n"
             "\n");

  //    TURTLE MOTION QUERIES
  //    ---------------------

  set("POS", "POS\n"
             "\n"
             "        outputs the turtle's current position, as a list of two\n"
             "        numbers, the X and Y coordinates.\n"
             "\n");

  set("XCOR", "XCOR                                                    "
              "(library procedure)\n"
              "\n"
              "        outputs a number, the turtle's X coordinate.\n"
              "\n");

  set("YCOR", "YCOR                                                    "
              "(library procedure)\n"
              "\n"
              "        outputs a number, the turtle's Y coordinate.\n"
              "\n");

  set("HEADING", "HEADING\n"
                 "\n"
                 "        outputs a number, the turtle's heading in degrees.\n"
                 "\n");

  set("TOWARDS",
      "TOWARDS pos\n"
      "\n"
      "        outputs a number, the heading at which the turtle should be\n"
      "        facing so that it would point from its current position to\n"
      "        the position given as the input.\n"
      "\n");

  set("SCRUNCH",
      "SCRUNCH\n"
      "\n"
      "        outputs a list containing two numbers, the X and Y scrunch\n"
      "        factors, as used by SETSCRUNCH.  (But note that SETSCRUNCH\n"
      "        takes two numbers as inputs, not one list of numbers.)\n"
      "\n");

  //    TURTLE AND WINDOW CONTROL
  //    -------------------------

  set("SHOWTURTLE", "SHOWTURTLE\n"
                    "ST\n"
                    "\n"
                    "        makes the turtle visible.\n"
                    "\n");

  alt("ST", "SHOWTURTLE");

  set("HIDETURTLE",
      "HIDETURTLE\n"
      "HT\n"
      "\n"
      "        makes the turtle invisible.  It's a good idea to do this while\n"
      "        you're in the middle of a complicated drawing, because hiding\n"
      "        the turtle speeds up the drawing substantially.\n"
      "\n");

  alt("HT", "HIDETURTLE");

  set("CLEAN",
      "CLEAN\n"
      "\n"
      "        erases all lines that the turtle has drawn on the graphics "
      "window.\n"
      "        The turtle's state (position, heading, pen mode, etc.) is not\n"
      "        changed.\n"
      "\n");

  set("CLEARSCREEN",
      "CLEARSCREEN\n"
      "CS\n"
      "\n"
      "        erases the graphics window and sends the turtle to its initial\n"
      "        position and heading.  Like HOME and CLEAN together.\n"
      "\n");

  alt("CS", "CLEARSCREEN");

  set("WRAP",
      "WRAP\n"
      "\n"
      "        tells the turtle to enter wrap mode:  From now on, if the "
      "turtle\n"
      "        is asked to move past the boundary of the graphics window, it\n"
      "        will \"wrap around\" and reappear at the opposite edge of the\n"
      "        window.  The top edge wraps to the bottom edge, while the left\n"
      "        edge wraps to the right edge.  (So the window is topologically\n"
      "        equivalent to a torus.)  This is the turtle's initial mode.\n"
      "        Compare WINDOW and FENCE.\n"
      "\n");

  set("WINDOW",
      "WINDOW\n"
      "\n"
      "        tells the turtle to enter window mode:  From now on, if the "
      "turtle\n"
      "        is asked to move past the boundary of the graphics window, it\n"
      "        will move offscreen.  The visible graphics window is "
      "considered\n"
      "        as just part of an infinite graphics plane; the turtle can be\n"
      "        anywhere on the plane.  (If you lose the turtle, HOME will "
      "bring\n"
      "        it back to the center of the window.)  Compare WRAP and FENCE.\n"
      "\n");

  set("FENCE",
      "FENCE\n"
      "\n"
      "        tells the turtle to enter fence mode:  From now on, if the "
      "turtle\n"
      "        is asked to move past the boundary of the graphics window, it\n"
      "        will move as far as it can and then stop at the edge with an\n"
      "        \"out of bounds\" error message.  Compare WRAP and WINDOW.\n"
      "\n");

  set("FILL",
      "FILL\n"
      "\n"
      "        fills in a region of the graphics window containing the turtle\n"
      "        and bounded by lines that have been drawn earlier.  This is "
      "not\n"
      "        portable; it doesn't work for all machines, and may not work\n"
      "        exactly the same way on different machines.\n"
      "\n");

  set("FILLED", "FILLED color instructions\n"
                "\n"
                "        runs the instructions, remembering all points visited "
                "by turtle\n"
                "        motion commands, starting *and ending* with the "
                "turtle's initial\n"
                "        position.  Then draws (ignoring penmode) the "
                "resulting polygon,\n"
                "        in the current pen color, filling the polygon with "
                "the given color,\n"
                "        which can be a color number or an RGB list.  The "
                "instruction list\n"
                "        cannot include another FILLED invocation.\n"
                "\n");

  set("LABEL",
      "LABEL text\n"
      "\n"
      "        takes a word or list as input, and prints the input on the\n"
      "        graphics window, starting at the turtle's position.\n"
      "\n");

  set("SETLABELHEIGHT",
      "SETLABELHEIGHT height\n"
      "\n"
      "        command (wxWidgets only).  Takes a positive integer argument "
      "and tries\n"
      "        to set the font size so that the character height (including\n"
      "        descenders) is that many turtle steps.  This will be different "
      "from\n"
      "        the number of screen pixels if SETSCRUNCH has been used.  Also, "
      "note\n"
      "        that SETSCRUNCH changes the font size to try to preserve this "
      "height\n"
      "        in turtle steps.  Note that the query operation corresponding "
      "to this\n"
      "        command is LABELSIZE, not LABELHEIGHT, because it tells you the "
      "width\n"
      "        as well as the height of characters in the current font.\n"
      "\n");

  set("TEXTSCREEN",
      "TEXTSCREEN\n"
      "TS\n"
      "\n"
      "        rearranges the size and position of windows to maximize the\n"
      "        space available in the text window (the window used for\n"
      "        interaction with Logo).  The details differ among machines.\n"
      "        Compare SPLITSCREEN and FULLSCREEN.\n"
      "\n");

  alt("TS", "TEXTSCREEN");

  set("FULLSCREEN",
      "FULLSCREEN\n"
      "FS\n"
      "\n"
      "        rearranges the size and position of windows to maximize the "
      "space\n"
      "        available in the graphics window.  The details differ among "
      "machines.\n"
      "        Compare SPLITSCREEN and TEXTSCREEN.\n"
      "\n"
      "        Since there must be a text window to allow printing (including "
      "the\n"
      "        printing of the Logo prompt), Logo automatically switches from\n"
      "        fullscreen to splitscreen whenever anything is printed.\n"
      "\n"
      "        In the DOS version, switching from fullscreen to splitscreen "
      "loses the\n"
      "        part of the picture that's hidden by the text window.  [This "
      "design\n"
      "        decision follows from the scarcity of memory, so that the extra "
      "memory\n"
      "        to remember an invisible part of a drawing seems too "
      "expensive.]\n"
      "\n");
  ;
  alt("FS", "FULLSCREEN");

  set("SPLITSCREEN", "SPLITSCREEN\n"
                     "SS\n"
                     "\n"
                     "        rearranges the size and position of windows to "
                     "allow some room for\n"
                     "        text interaction while also keeping most of the "
                     "graphics window\n"
                     "        visible.  The details differ among machines.  "
                     "Compare TEXTSCREEN\n"
                     "        and FULLSCREEN.\n"
                     "\n");

  alt("SS", "SPLITSCREEN");

  set("SETSCRUNCH",
      "SETSCRUNCH xscale yscale\n"
      "\n"
      "        adjusts the aspect ratio and scaling of the graphics display.\n"
      "        After this command is used, all further turtle motion will be\n"
      "        adjusted by multiplying the horizontal and vertical extent of\n"
      "        the motion by the two numbers given as inputs.  For example,\n"
      "        after the instruction \"SETSCRUNCH 2 1\" motion at a heading "
      "of\n"
      "        45 degrees will move twice as far horizontally as vertically.\n"
      "        If your squares don't come out square, try this.  "
      "(Alternatively,\n"
      "        you can deliberately misadjust the aspect ratio to draw an "
      "ellipse.)\n"
      "\n"
      "        In wxWidgets only, SETSCRUNCH also changes the size of the text "
      "font\n"
      "        used for the LABEL command to try to keep the height of "
      "characters\n"
      "        scaled with the vertical turtle step size.\n"
      "\n"
      "        For all modern computers For DOS machines, the scale factors "
      "are\n"
      "        initially set according to what the hardware claims the aspect "
      "ratio\n"
      "        is, but the hardware sometimes lies.  For DOS, the values set "
      "by\n"
      "        SETSCRUNCH are remembered in a file (called SCRUNCH.DAT) and "
      "are\n"
      "        automatically put into effect when a Logo session begins.\n"
      "\n");

  //    TURTLE AND WINDOW QUERIES
  //    -------------------------

  set("SHOWNP",
      "SHOWNP\n"
      "SHOWN?\n"
      "\n"
      "        outputs TRUE if the turtle is shown (visible), FALSE if the\n"
      "        turtle is hidden.  See SHOWTURTLE and HIDETURTLE.\n"
      "\n");

  alt("SHOWN?", "SHOWNP");

  set("SCREENMODE", "SCREENMODE\n"
                    "\n"
                    "        outputs the word TEXTSCREEN, SPLITSCREEN, or "
                    "FULLSCREEN depending\n"
                    "        on the current screen mode.\n"
                    "\n");

  set("TURTLEMODE", "TURTLEMODE\n"
                    "\n"
                    "        outputs the word WRAP, FENCE, or WINDOW depending "
                    "on the current\n"
                    "        turtle mode.\n"
                    "\n");

  set("LABELSIZE", "LABELSIZE\n"
                   "\n"
                   "        (wxWidgets only) outputs a list of two positive "
                   "integers, the width\n"
                   "        and height of characters displayed by LABEL "
                   "measured in turtle steps\n"
                   "        (which will be different from screen pixels if "
                   "SETSCRUNCH has been\n"
                   "        used).  There is no SETLABELSIZE because the width "
                   "and height of a\n"
                   "        font are not separately controllable, so the "
                   "inverse of this operation\n"
                   "        is SETLABELHEIGHT, which takes just one number for "
                   "the desired height.\n"
                   "\n");

  //    PEN AND BACKGROUND CONTROL
  //    --------------------------

  set("PENDOWN",
      "PENDOWN\n"
      "PD\n"
      "\n"
      "        sets the pen's position to DOWN, without changing its mode.\n"
      "\n");

  alt("PD", "PENDOWN");

  set("PENUP",
      "PENUP\n"
      "PU\n"
      "\n"
      "        sets the pen's position to UP, without changing its mode.\n"
      "\n");

  alt("PU", "PENUP");

  set("PENPAINT", "PENPAINT\n"
                  "PPT\n"
                  "\n"
                  "        sets the pen's position to DOWN and mode to PAINT.\n"
                  "\n");

  alt("PPT", "PENPAINT");

  set("PENERASE", "PENERASE\n"
                  "PE\n"
                  "\n"
                  "        sets the pen's position to DOWN and mode to ERASE.\n"
                  "\n");

  alt("PE", "PENERASE");

  set("PENREVERSE",
      "PENREVERSE\n"
      "PX\n"
      "\n"
      "        sets the pen's position to DOWN and mode to REVERSE.\n"
      "        (This may interact in system-dependent ways with use of "
      "color.)\n"
      "\n");

  alt("PX", "PENREVERSE");

  set("SETPENCOLOR",
      "SETPENCOLOR colornumber.or.rgblist\n"
      "SETPC colornumber.or.rgblist\n"
      "\n"
      "        sets the pen color to the given number, which must be a "
      "nonnegative\n"
      "        integer.  There are initial assignments for the first 16 "
      "colors:\n"
      "\n"
      "         0  black        1  blue         2  green        3  cyan\n"
      "         4  red          5  magenta      6  yellow       7 white\n"
      "         8  brown        9  tan         10  forest      11  aqua\n"
      "        12  salmon      13  purple      14  orange      15  grey\n"
      "\n"
      "        but other colors can be assigned to numbers by the PALETTE "
      "command.\n"
      "        Alternatively, sets the pen color to the given RGB values (a "
      "list of\n"
      "        three nonnegative numbers less than 100 specifying the percent\n"
      "        saturation of red, green, and blue in the desired color).\n"
      "\n");

  alt("SETPC", "SETPENCOLOR");

  set("SETPALETTE", "SETPALETTE colornumber rgblist\n"
                    "\n"
                    "        sets the actual color corresponding to a given "
                    "number, if allowed by\n"
                    "        the hardware and operating system.  Colornumber "
                    "must be an integer\n"
                    "        greater than or equal to 8.  (Logo tries to keep "
                    "the first 8 colors\n"
                    "        constant.)  The second input is a list of three "
                    "nonnegative numbers\n"
                    "        less than 100 specifying the percent saturation "
                    "of red, green, and\n"
                    "        blue in the desired color.\n"
                    "\n");

  set("SETPENSIZE",
      "SETPENSIZE size\n"
      "\n"
      "        sets the thickness of the pen.  The input is either a single "
      "positive\n"
      "        integer or a list of two positive integers (for horizontal and\n"
      "        vertical thickness).  Some versions pay no attention to the "
      "second\n"
      "        number, but always have a square pen.\n"
      "\n");

  set("SETPENPATTERN",
      "SETPENPATTERN pattern\n"
      "\n"
      "        sets hardware-dependent pen characteristics.  This command is\n"
      "        not guaranteed compatible between implementations on different\n"
      "        machines.\n"
      "\n");

  set("SETPEN", "SETPEN list                                             "
                "(library procedure)\n"
                "\n"
                "        sets the pen's position, mode, thickness, and "
                "hardware-dependent\n"
                "        characteristics according to the information in the "
                "input list, which\n"
                "        should be taken from an earlier invocation of PEN.\n"
                "\n");

  set("SETBACKGROUND",
      "SETBACKGROUND colornumber.or.rgblist\n"
      "SETBG colornumber.or.rgblist\n"
      "\n"
      "        set the screen background color by slot number or RGB values.\n"
      "        See SETPENCOLOR for details.\n"
      "\n");

  alt("SETBG", "SETBACKGROUND");

  //    PEN QUERIES
  //    -----------

  set("PENDOWNP", "PENDOWNP\n"
                  "PENDOWN?\n"
                  "\n"
                  "        outputs TRUE if the pen is down, FALSE if it's up.\n"
                  "\n");

  alt("PENDOWN?", "PENDOWNP");

  set("PENMODE",
      "PENMODE\n"
      "\n"
      "        outputs one of the words PAINT, ERASE, or REVERSE according to\n"
      "        the current pen mode.\n"
      "\n");

  set("PENCOLOR",
      "PENCOLOR\n"
      "PC\n"
      "\n"
      "        outputs a color number, a nonnegative integer that is "
      "associated with\n"
      "        a particular color, or a list of RGB values if such a list was "
      "used as\n"
      "        the most recent input to SETPENCOLOR.  There are initial "
      "assignments\n"
      "        for the first 16 colors:\n"
      "\n"
      "         0  black        1  blue         2  green        3  cyan\n"
      "         4  red          5  magenta      6  yellow       7 white\n"
      "         8  brown        9  tan         10  forest      11  aqua\n"
      "        12  salmon      13  purple      14  orange      15  grey\n"
      "\n"
      "        but other colors can be assigned to numbers by the PALETTE "
      "command.\n"
      "\n");

  alt("PC", "PENCOLOR");

  set("PALETTE", "PALETTE colornumber\n"
                 "\n"
                 "        outputs a list of three nonnegative numbers less "
                 "than 100 specifying\n"
                 "        the percent saturation of red, green, and blue in "
                 "the color associated\n"
                 "        with the given number.\n"
                 "\n");

  set("PENSIZE",
      "PENSIZE\n"
      "\n"
      "\n"
      "        outputs a list of two positive integers, specifying the "
      "horizontal\n"
      "        and vertical thickness of the turtle pen.  (In some "
      "implementations,\n"
      "        including wxWidgets, the two numbers are always equal.)\n"
      "\n");

  set("PENPATTERN", "PENPATTERN\n"
                    "\n"
                    "        outputs system-specific pen information.\n"
                    "\n");

  set("PEN", "PEN                                                     (library "
             "procedure)\n"
             "\n"
             "        outputs a list containing the pen's position, mode, "
             "thickness, and\n"
             "        hardware-specific characteristics, for use by SETPEN.\n"
             "\n");

  set("BACKGROUND",
      "BACKGROUND\n"
      "BG\n"
      "\n"
      "        outputs the graphics background color, either as a slot number "
      "or\n"
      "        as an RGB list, whichever way it was set.  (See PENCOLOR.)\n"
      "\n");

  alt("BG", "BACKGROUND");

  //    SAVING AND LOADING PICTURES
  //    ---------------------------

  set("SAVEPICT",
      "SAVEPICT filename\n"
      "\n"
      "        command.  Writes a file with the specified name containing the\n"
      "        state of the graphics window, including any nonstandard color\n"
      "        palette settings, in Logo's internal format.  This picture can\n"
      "        be restored to the screen using LOADPICT.  The format is not\n"
      "        portable between platforms, nor is it readable by other "
      "programs.\n"
      "        See EPSPICT to export Logo graphics for other programs.\n"
      "\n");

  set("LOADPICT",
      "LOADPICT filename\n"
      "\n"
      "        command.  Reads the specified file, which must have been\n"
      "        written by a SAVEPICT command, and restores the graphics\n"
      "        window and color palette settings to the values stored in\n"
      "        the file.  Any drawing previously on the screen is cleared.\n"
      "\n");

  //    MOUSE QUERIES
  //    -------------

  set("MOUSEPOS", "MOUSEPOS\n"
                  "\n"
                  "        outputs the coordinates of the mouse, provided that "
                  "it's within the\n"
                  "        graphics window, in turtle coordinates.  If the "
                  "mouse is outside the\n"
                  "        graphics window, then the last position within the "
                  "window is returned.\n"
                  "        Exception:  If a mouse button is pressed within the "
                  "graphics window\n"
                  "        and held while the mouse is dragged outside the "
                  "window, the mouse's\n"
                  "        position is returned as if the window were big "
                  "enough to include it.\n"
                  "\n");

  set("CLICKPOS",
      "CLICKPOS\n"
      "\n"
      "        outputs the coordinates that the mouse was at when a mouse "
      "button\n"
      "        was most recently pushed, provided that that position was "
      "within the\n"
      "        graphics window, in turtle coordinates.  (wxWidgets only)\n"
      "\n");

  set("BUTTONP", "BUTTONP\n"
                 "BUTTON?\n"
                 "\n"
                 "        outputs TRUE if a mouse button is down and the mouse "
                 "is over the\n"
                 "        graphics window.  Once the button is down, BUTTONP "
                 "remains true until\n"
                 "        the button is released, even if the mouse is dragged "
                 "out of the\n"
                 "        graphics window.\n"
                 "\n");

  set("BUTTON?", "BUTTONP");

  set("BUTTON", "BUTTON\n"
                "\n"
                "        outputs 0 if no mouse button has been pushed inside "
                "the Logo window\n"
                "        since the last call to BUTTON.  Otherwise, it outputs "
                "an integer\n"
                "        between 1 and 3 indicating which button was most "
                "recently pressed.\n"
                "        Ordinarily 1 means left, 2 means right, and 3 means "
                "center, but\n"
                "        operating systems may reconfigure these.\n"
                "\n");
}

void Help::setWorkspaceManagement() {

  set("TO",
      "TO procname :input1 :input2 ...                         (special form)\n"
      "\n"
      "        command.  Prepares Logo to accept a procedure definition.  The\n"
      "        procedure will be named \"procname\" and there must not "
      "already\n"
      "        be a procedure by that name.  The inputs will be called "
      "\"input1\"\n"
      "        etc.  Any number of inputs are allowed, including none.  Names\n"
      "        of procedures and inputs are case-insensitive.\n"
      "\n"
      "        Unlike every other Logo procedure, TO takes as its inputs the\n"
      "        actual words typed in the instruction line, as if they were\n"
      "        all quoted, rather than the results of evaluating expressions\n"
      "        to provide the inputs.  (That's what \"special form\" means.)\n"
      "\n"
      "        This version of Logo allows variable numbers of inputs to a\n"
      "        procedure.  After the procedure name come four kinds of\n"
      "        things, *in this order*:\n"
      "\n"
      "            1.   0 or more REQUIRED inputs    :FOO :FROBOZZ\n"
      "            2.   0 or more OPTIONAL inputs    [:BAZ 87] [:THINGO 5+9]\n"
      "            3.   0 or 1 REST input            [:GARPLY]\n"
      "            4.   0 or 1 DEFAULT number        5\n"
      "\n"
      "        Every procedure has a MINIMUM, DEFAULT, and MAXIMUM\n"
      "        number of inputs.  (The latter can be infinite.)\n"
      "\n"
      "        The MINIMUM number of inputs is the number of required inputs,\n"
      "        which must come first.  A required input is indicated by the\n"
      "\n"
      "                        :inputname\n"
      "\n"
      "        notation.\n"
      "\n"
      "        After all the required inputs can be zero or more optional "
      "inputs,\n"
      "        each of which is represented by the following notation:\n"
      "\n"
      "                        [:inputname default.value.expression]\n"
      "\n"
      "        When the procedure is invoked, if actual inputs are not "
      "supplied\n"
      "        for these optional inputs, the default value expressions are\n"
      "        evaluated to set values for the corresponding input names.  "
      "The\n"
      "        inputs are processed from left to right, so a default value\n"
      "        expression can be based on earlier inputs.  Example:\n"
      "\n"
      "                        to proc :inlist [:startvalue first :inlist]\n"
      "\n"
      "        If the procedure is invoked by saying\n"
      "\n"
      "                        proc [a b c]\n"
      "\n"
      "        then the variable INLIST will have the value [A B C] and the\n"
      "        variable STARTVALUE will have the value A.  If the procedure\n"
      "        is invoked by saying\n"
      "\n"
      "                        (proc [a b c] \"x)\n"
      "\n"
      "        then INLIST will have the value [A B C] and STARTVALUE will\n"
      "        have the value X.\n"
      "\n"
      "        After all the required and optional input can come a single "
      "\"rest\"\n"
      "        input, represented by the following notation:\n"
      "\n"
      "                        [:inputname]\n"
      "\n"
      "        This is a rest input rather than an optional input because "
      "there\n"
      "        is no default value expression.  There can be at most one rest\n"
      "        input.  When the procedure is invoked, the value of this "
      "inputname\n"
      "        will be a list containing all of the actual inputs provided "
      "that\n"
      "        were not used for required or optional inputs.  Example:\n"
      "\n"
      "                        to proc :in1 [:in2 \"foo] [:in3 \"baz] [:in4]\n"
      "\n"
      "        If this procedure is invoked by saying\n"
      "\n"
      "                        proc \"x\n"
      "\n"
      "        then IN1 has the value X, IN2 has the value FOO, IN3 has the "
      "value\n"
      "        BAZ, and IN4 has the value [] (the empty list).  If it's "
      "invoked\n"
      "        by saying\n"
      "\n"
      "                        (proc \"a \"b \"c \"d \"e)\n"
      "\n"
      "        then IN1 has the value A, IN2 has the value B, IN3 has the "
      "value C,\n"
      "        and IN4 has the value [D E].\n"
      "\n"
      "        The MAXIMUM number of inputs for a procedure is infinite if a\n"
      "        rest input is given; otherwise, it is the number of required\n"
      "        inputs plus the number of optional inputs.\n"
      "\n"
      "        The DEFAULT number of inputs for a procedure, which is the "
      "number\n"
      "        of inputs that it will accept if its invocation is not "
      "enclosed\n"
      "        in parentheses, is ordinarily equal to the minimum number.  If\n"
      "        you want a different default number you can indicate that by\n"
      "        putting the desired default number as the last thing on the\n"
      "        TO line.  example:\n"
      "\n"
      "                        to proc :in1 [:in2 \"foo] [:in3] 3\n"
      "\n"
      "        This procedure has a minimum of one input, a default of three\n"
      "        inputs, and an infinite maximum.\n"
      "\n"
      "        Logo responds to the TO command by entering procedure "
      "definition\n"
      "        mode.  The prompt character changes from \"?\" to \">\" and "
      "whatever\n"
      "        instructions you type become part of the definition until you\n"
      "        type a line containing only the word END.\n"
      "\n");

  set("DEFINE",
      "DEFINE procname text\n"
      "\n"
      "        command.  Defines a procedure with name \"procname\" and text "
      "\"text\".\n"
      "        If there is already a procedure with the same name, the new\n"
      "        definition replaces the old one.  The text input must be a "
      "list\n"
      "        whose members are lists.  The first member is a list of "
      "inputs;\n"
      "        it looks like a TO line but without the word TO, without the\n"
      "        procedure name, and without the colons before input names.  In\n"
      "        other words, the members of this first sublist are words for\n"
      "        the names of required inputs and lists for the names of "
      "optional\n"
      "        or rest inputs.  The remaining sublists of the text input make\n"
      "        up the body of the procedure, with one sublist for each "
      "instruction\n"
      "        line of the body.  (There is no END line in the text input.)\n"
      "        It is an error to redefine a primitive procedure unless the "
      "variable\n"
      "        REDEFP has the value TRUE.\n"
      "\n");

  set("TEXT",
      "TEXT procname\n"
      "\n"
      "        outputs the text of the procedure named \"procname\" in the "
      "form\n"
      "        expected by DEFINE: a list of lists, the first of which "
      "describes\n"
      "        the inputs to the procedure and the rest of which are the lines "
      "of\n"
      "        its body.  The text does not reflect formatting information "
      "used\n"
      "        when the procedure was defined, such as continuation lines and\n"
      "        extra spaces.\n"
      "\n");

  set("FULLTEXT",
      "FULLTEXT procname\n"
      "\n"
      "        outputs a representation of the procedure \"procname\" in "
      "which\n"
      "        formatting information is preserved.  If the procedure was "
      "defined\n"
      "        with TO, EDIT, or LOAD, then the output is a list of words.  "
      "Each\n"
      "        word represents one entire line of the definition in the form\n"
      "        output by READWORD, including extra spaces and continuation "
      "lines.\n"
      "        The last member of the output represents the END line.  If the\n"
      "        procedure was defined with DEFINE, then the output is a list "
      "of\n"
      "        lists.  If these lists are printed, one per line, the result "
      "will\n"
      "        look like a definition using TO.  Note: the output from "
      "FULLTEXT\n"
      "        is not suitable for use as input to DEFINE!\n"
      "\n");

  set("COPYDEF",
      "COPYDEF newname oldname\n"
      "\n"
      "        command.  Makes \"newname\" a procedure identical to "
      "\"oldname\".\n"
      "        The latter may be a primitive.  If \"newname\" was already "
      "defined,\n"
      "        its previous definition is lost.  If \"newname\" was already a\n"
      "        primitive, the redefinition is not permitted unless the "
      "variable\n"
      "        REDEFP has the value TRUE.\n"
      "\n"
      "        Note: dialects of Logo differ as to the order of inputs to "
      "COPYDEF.\n"
      "        This dialect uses \"MAKE order,\" not \"NAME order.\"\n"
      "\n");

  //    VARIABLE DEFINITION
  //    -------------------

  set("MAKE",
      "MAKE varname value\n"
      "\n"
      "        command.  Assigns the value \"value\" to the variable named "
      "\"varname\",\n"
      "        which must be a word.  Variable names are case-insensitive.  If "
      "a\n"
      "        variable with the same name already exists, the value of that\n"
      "        variable is changed.  If not, a new global variable is "
      "created.\n"
      "\n");

  set("NAME",
      "NAME value varname                                      (library "
      "procedure)\n"
      "\n"
      "        command.  Same as MAKE but with the inputs in reverse order.\n"
      "\n");

  set("LOCAL",
      "LOCAL varname\n"
      "LOCAL varnamelist\n"
      "(LOCAL varname1 varname2 ...)\n"
      "\n"
      "        command.  Accepts as inputs one or more words, or a list of\n"
      "        words.  A variable is created for each of these words, with\n"
      "        that word as its name.  The variables are local to the\n"
      "        currently running procedure.  Logo variables follow dynamic\n"
      "        scope rules; a variable that is local to a procedure is\n"
      "        available to any subprocedure invoked by that procedure.\n"
      "        The variables created by LOCAL have no initial value; they\n"
      "        must be assigned a value (e.g., with MAKE) before the "
      "procedure\n"
      "        attempts to read their value.\n"
      "\n");

  set("LOCALMAKE",
      "LOCALMAKE varname value                         (library procedure)\n"
      "\n"
      "        command.  Makes the named variable local, like LOCAL, and\n"
      "        assigns it the given value, like MAKE.\n"
      "\n");

  set("THING",
      "THING varname\n"
      ":quoted.varname\n"
      "\n"
      "        outputs the value of the variable whose name is the input.\n"
      "        If there is more than one such variable, the innermost local\n"
      "        variable of that name is chosen.  The colon notation is an\n"
      "        abbreviation not for THING but for the combination\n"
      "\n"
      "                                thing \"\n"
      "\n"
      "        so that :FOO means THING \"FOO.\n"
      "\n");

  set("GLOBAL",
      "GLOBAL varname\n"
      "GLOBAL varnamelist\n"
      "(GLOBAL varname1 varname2 ...)\n"
      "\n"
      "        command.  Accepts as inputs one or more words, or a list of\n"
      "        words.  A global variable is created for each of these words, "
      "with\n"
      "        that word as its name.  The only reason this is necessary is "
      "that\n"
      "        you might want to use the \"setter\" notation SETXYZ for a "
      "variable\n"
      "        XYZ that does not already have a value; GLOBAL \"XYZ makes that "
      "legal.\n"
      "        Note: If there is currently a local variable of the same name, "
      "this\n"
      "        command does *not* make Logo use the global value instead of "
      "the\n"
      "        local one.\n"
      "\n");

  //    PROPERTY LISTS
  //    --------------

  set("PPROP",
      "PPROP plistname propname value\n"
      "\n"
      "        command.  Adds a property to the \"plistname\" property list\n"
      "        with name \"propname\" and value \"value\".\n"
      "\n");

  set("GPROP",
      "GPROP plistname propname\n"
      "\n"
      "        outputs the value of the \"propname\" property in the "
      "\"plistname\"\n"
      "        property list, or the empty list if there is no such property.\n"
      "\n");

  set("REMPROP",
      "REMPROP plistname propname\n"
      "\n"
      "        command.  Removes the property named \"propname\" from the\n"
      "        property list named \"plistname\".\n"
      "\n");

  set("PLIST",
      "PLIST plistname\n"
      "\n"
      "        outputs a list whose odd-numbered members are the names, and\n"
      "        whose even-numbered members are the values, of the properties\n"
      "        in the property list named \"plistname\".  The output is a "
      "copy\n"
      "        of the actual property list; changing properties later will "
      "not\n"
      "        magically change a list output earlier by PLIST.\n"
      "\n");

  //    PREDICATES
  //    ----------

  set("PROCEDUREP",
      "PROCEDUREP name\n"
      "PROCEDURE? name\n"
      "\n"
      "        outputs TRUE if the input is the name of a procedure.\n"
      "\n");

  alt("PROCEDURE?", "PROCEDUREP");

  set("PRIMITIVEP",
      "PRIMITIVEP name\n"
      "PRIMITIVE? name\n"
      "\n"
      "        outputs TRUE if the input is the name of a primitive procedure\n"
      "        (one built into Logo).  Note that some of the procedures\n"
      "        described in this document are library procedures, not "
      "primitives.\n"
      "\n");

  alt("PRIMITIVE?", "PRIMITIVEP");

  set("DEFINEDP", "DEFINEDP name\n"
                  "DEFINED? name\n"
                  "\n"
                  "        outputs TRUE if the input is the name of a "
                  "user-defined procedure,\n"
                  "        including a library procedure.\n"
                  "\n");

  alt("DEFINED?", "DEFINEDP");

  set("NAMEP", "NAMEP name\n"
               "NAME? name\n"
               "\n"
               "        outputs TRUE if the input is the name of a variable.\n"
               "\n");

  alt("NAME?", "NAMEP");

  set("PLISTP", "PLISTP name\n"
                "PLIST? name\n"
                "\n"
                "        outputs TRUE if the input is the name of a *nonempty* "
                "property list.\n"
                "        (In principle every word is the name of a property "
                "list; if you haven't\n"
                "        put any properties in it, PLIST of that name outputs "
                "an empty list,\n"
                "        rather than giving an error message.)\n"
                "\n");

  alt("PLIST?", "PLISTP");

  //    QUERIES
  //    -------

  set("CONTENTS",
      "CONTENTS\n"
      "\n"
      "        outputs a \"contents list,\" i.e., a list of three lists "
      "containing\n"
      "        names of defined procedures, variables, and property lists\n"
      "        respectively.  This list includes all unburied named items in\n"
      "        the workspace.\n"
      "\n");

  set("BURIED",
      "BURIED\n"
      "\n"
      "        outputs a contents list including all buried named items in\n"
      "        the workspace.\n"
      "\n");

  set("TRACED",
      "TRACED\n"
      "\n"
      "        outputs a contents list including all traced named items in\n"
      "        the workspace.\n"
      "\n");

  set("STEPPED",
      "STEPPED\n"
      "\n"
      "        outputs a contents list including all stepped named items in\n"
      "        the workspace.\n"
      "\n");

  set("PROCEDURES",
      "PROCEDURES\n"
      "\n"
      "        outputs a list of the names of all unburied user-defined "
      "procedures\n"
      "        in the workspace.  Note that this is a list of names, not a\n"
      "        contents list.  (However, procedures that require a contents "
      "list\n"
      "        as input will accept this list.)\n"
      "\n");

  set("PRIMITIVES",
      "PRIMITIVES\n"
      "\n"
      "        outputs a list of the names of all primitive procedures\n"
      "        in the workspace.  Note that this is a list of names, not a\n"
      "        contents list.  (However, procedures that require a contents "
      "list\n"
      "        as input will accept this list.)\n"
      "\n");

  set("NAMES", "NAMES\n"
               "\n"
               "        outputs a contents list consisting of an empty list "
               "(indicating\n"
               "        no procedure names) followed by a list of all unburied "
               "variable\n"
               "        names in the workspace.\n"
               "\n");

  set("PLISTS",
      "PLISTS\n"
      "\n"
      "        outputs a contents list consisting of two empty lists "
      "(indicating\n"
      "        no procedures or variables) followed by a list of all unburied\n"
      "        nonempty property lists in the workspace.\n"
      "\n");

  set("NAMELIST",
      "NAMELIST varname                                        (library "
      "procedure)\n"
      "NAMELIST varnamelist\n"
      "\n"
      "        outputs a contents list consisting of an empty list followed "
      "by\n"
      "        a list of the name or names given as input.  This is useful in\n"
      "        conjunction with workspace control procedures that require a "
      "contents\n"
      "        list as input.\n"
      "\n");

  set("PLLIST",
      "PLLIST plname                                           (library "
      "procedure)\n"
      "PLLIST plnamelist\n"
      "\n"
      "        outputs a contents list consisting of two empty lists followed "
      "by\n"
      "        a list of the name or names given as input.  This is useful in\n"
      "        conjunction with workspace control procedures that require a "
      "contents\n"
      "        list as input.\n"
      "\n");

  set("ARITY", "ARITY procedurename\n"
               "\n"
               "        outputs a list of three numbers: the minimum, default, "
               "and maximum\n"
               "        number of inputs for the procedure whose name is the "
               "input.  It is an\n"
               "        error if there is no such procedure.  A maximum of -1 "
               "means that the\n"
               "        number of inputs is unlimited.\n"
               "\n");

  set("NODES",
      "NODES\n"
      "\n"
      "        outputs a list of two numbers.  The first represents the number "
      "of\n"
      "        nodes of memory currently in use.  The second shows the "
      "maximum\n"
      "        number of nodes that have been in use at any time since the "
      "last\n"
      "        invocation of NODES.  (A node is a small block of computer "
      "memory\n"
      "        as used by Logo.  Each number uses one node.  Each non-numeric\n"
      "        word uses one node, plus some non-node memory for the "
      "characters\n"
      "        in the word.  Each array takes one node, plus some non-node\n"
      "        memory, as well as the memory required by its elements.  Each\n"
      "        list requires one node per element, as well as the memory "
      "within\n"
      "        the elements.)  If you want to track the memory use of an\n"
      "        algorithm, it is best if you invoke GC at the beginning of "
      "each\n"
      "        iteration, since otherwise the maximum will include storage "
      "that\n"
      "        is unused but not yet collected.\n"
      "\n");

  //    INSPECTION
  //    ----------

  set("PRINTOUT",
      "PRINTOUT contentslist\n"
      "PO contentslist\n"
      "\n"
      "        command.  Prints to the write stream the definitions of all\n"
      "        procedures, variables, and property lists named in the input\n"
      "        contents list.\n"
      "\n");

  alt("PO", "PRINTOUT");

  set("POALL",
      "POALL                                                   (library "
      "procedure)\n"
      "\n"
      "        command.  Prints all unburied definitions in the workspace.\n"
      "        Abbreviates PO CONTENTS.\n"
      "\n");

  set("POPS",
      "POPS                                                    (library "
      "procedure)\n"
      "\n"
      "        command.  Prints the definitions of all unburied procedures in\n"
      "        the workspace.  Abbreviates PO PROCEDURES.\n"
      "\n");

  set("PONS",
      "PONS                                                    (library "
      "procedure)\n"
      "\n"
      "        command.  Prints the definitions of all unburied variables in\n"
      "        the workspace.  Abbreviates PO NAMES.\n"
      "\n");

  set("POPLS", "POPLS                                                   "
               "(library procedure)\n"
               "\n"
               "        command.  Prints the contents of all unburied nonempty "
               "property\n"
               "        lists in the workspace.  Abbreviates PO PLISTS.\n"
               "\n");

  set("PON",
      "PON varname                                             (library "
      "procedure)\n"
      "PON varnamelist\n"
      "\n"
      "        command.  Prints the definitions of the named variable(s).\n"
      "        Abbreviates PO NAMELIST varname(list).\n"
      "\n");

  set("POPL", "POPL plname                                             "
              "(library procedure)\n"
              "POPL plnamelist\n"
              "\n"
              "        command.  Prints the definitions of the named property "
              "list(s).\n"
              "        Abbreviates PO PLLIST plname(list).\n"
              "\n");

  set("POT",
      "POT contentslist\n"
      "\n"
      "        command.  Prints the title lines of the named procedures and\n"
      "        the definitions of the named variables and property lists.\n"
      "        For property lists, the entire list is shown on one line\n"
      "        instead of as a series of PPROP instructions as in PO.\n"
      "\n");

  set("POTS",
      "POTS                                                    (library "
      "procedure)\n"
      "\n"
      "        command.  Prints the title lines of all unburied procedures\n"
      "        in the workspace.  Abbreviates POT PROCEDURES.\n"
      "\n");

  //    WORKSPACE CONTROL
  //    -----------------

  set("ERASE",
      "ERASE contentslist\n"
      "ER contentslist\n"
      "\n"
      "        command.  Erases from the workspace the procedures, variables,\n"
      "        and property lists named in the input.  Primitive procedures "
      "may\n"
      "        not be erased unless the variable REDEFP has the value TRUE.\n"
      "\n");

  alt("ER", "ERASE");

  set("ERALL",
      "ERALL\n"
      "\n"
      "        command.  Erases all unburied procedures, variables, and "
      "property\n"
      "        lists from the workspace.  Abbreviates ERASE CONTENTS.\n"
      "\n");

  set("ERPS",
      "ERPS\n"
      "\n"
      "        command.  Erases all unburied procedures from the workspace.\n"
      "        Abbreviates ERASE PROCEDURES.\n"
      "\n");

  set("ERNS",
      "ERNS\n"
      "\n"
      "        command.  Erases all unburied variables from the workspace.\n"
      "        Abbreviates ERASE NAMES.\n"
      "\n");

  set("ERPLS", "ERPLS\n"
               "\n"
               "        command.  Erases all unburied property lists from the "
               "workspace.\n"
               "        Abbreviates ERASE PLISTS.\n"
               "\n");

  set("ERN", "ERN varname                                             (library "
             "procedure)\n"
             "ERN varnamelist\n"
             "\n"
             "        command.  Erases from the workspace the variable(s) "
             "named in the\n"
             "        input.  Abbreviates ERASE NAMELIST varname(list).\n"
             "\n");

  set("ERPL", "ERPL plname                                             "
              "(library procedure)\n"
              "ERPL plnamelist\n"
              "\n"
              "        command.  Erases from the workspace the property "
              "list(s) named in the\n"
              "        input.  Abbreviates ERASE PLLIST plname(list).\n"
              "\n");

  set("BURY",
      "BURY contentslist\n"
      "\n"
      "        command.  Buries the procedures, variables, and property lists\n"
      "        named in the input.  A buried item is not included in the "
      "lists\n"
      "        output by CONTENTS, PROCEDURES, VARIABLES, and PLISTS, but is\n"
      "        included in the list output by BURIED.  By implication, buried\n"
      "        things are not printed by POALL or saved by SAVE.\n"
      "\n");

  set("BURYALL", "BURYALL                                                 "
                 "(library procedure)\n"
                 "\n"
                 "        command.  Abbreviates BURY CONTENTS.\n"
                 "\n");

  set("BURYNAME", "BURYNAME varname                                        "
                  "(library procedure)\n"
                  "BURYNAME varnamelist\n"
                  "\n"
                  "        command.  Abbreviates BURY NAMELIST varname(list).\n"
                  "\n");

  set("UNBURY", "UNBURY contentslist\n"
                "\n"
                "        command.  Unburies the procedures, variables, and "
                "property lists\n"
                "        named in the input.  That is, the named items will be "
                "returned to\n"
                "        view in CONTENTS, etc.\n"
                "\n");

  set("UNBURYALL", "UNBURYALL                                               "
                   "(library procedure)\n"
                   "\n"
                   "        command.  Abbreviates UNBURY BURIED.\n"
                   "\n");

  set("UNBURYNAME",
      "UNBURYNAME varname                                      (library "
      "procedure)\n"
      "UNBURYNAME varnamelist\n"
      "\n"
      "        command.  Abbreviates UNBURY NAMELIST varname(list).\n"
      "\n");

  set("BURIEDP",
      "BURIEDP contentslist\n"
      "BURIED? contentslist\n"
      "\n"
      "        outputs TRUE if the first procedure, variable, or property list "
      "named\n"
      "        in the contents list is buried, FALSE if not.  Only the first "
      "thing in\n"
      "        the list is tested; the most common use will be with a word as "
      "input,\n"
      "        naming a procedure, but a contents list is allowed so that you "
      "can\n"
      "        BURIEDP [[] [VARIABLE]] or BURIEDP [[] [] [PROPLIST]].\n"
      "\n");

  alt("BURIED?", "BURIEDP");

  set("TRACE",
      "TRACE contentslist\n"
      "\n"
      "        command.  Marks the named items for tracing.  A message is "
      "printed\n"
      "        whenever a traced procedure is invoked, giving the actual "
      "input\n"
      "        values, and whenever a traced procedure STOPs or OUTPUTs.  A\n"
      "        message is printed whenever a new value is assigned to a "
      "traced\n"
      "        variable using MAKE.  A message is printed whenever a new "
      "property\n"
      "        is given to a traced property list using PPROP.\n"
      "\n");

  set("UNTRACE", "UNTRACE contentslist\n"
                 "\n"
                 "        command.  Turns off tracing for the named items.\n"
                 "\n");

  set("TRACEDP",
      "TRACEDP contentslist\n"
      "TRACED? contentslist\n"
      "\n"
      "        outputs TRUE if the first procedure, variable, or property list "
      "named\n"
      "        in the contents list is traced, FALSE if not.  Only the first "
      "thing in\n"
      "        the list is tested; the most common use will be with a word as "
      "input,\n"
      "        naming a procedure, but a contents list is allowed so that you "
      "can\n"
      "        TRACEDP [[] [VARIABLE]] or TRACEDP [[] [] [PROPLIST]].\n"
      "\n");
  ;
  alt("TRACED?", "TRACEDP");

  set("STEP",
      "STEP contentslist\n"
      "\n"
      "        command.  Marks the named items for stepping.  Whenever a "
      "stepped\n"
      "        procedure is invoked, each instruction line in the procedure "
      "body\n"
      "        is printed before being executed, and Logo waits for the user "
      "to\n"
      "        type a newline at the terminal.  A message is printed whenever "
      "a\n"
      "        stepped variable name is \"shadowed\" because a local variable "
      "of\n"
      "        the same name is created either as a procedure input or by the\n"
      "        LOCAL command.\n"
      "\n");

  set("UNSTEP", "UNSTEP contentslist\n"
                "\n"
                "        command.  Turns off stepping for the named items.\n"
                "\n");

  set("STEPPEDP",
      "STEPPEDP contentslist\n"
      "STEPPED? contentslist\n"
      "\n"
      "        outputs TRUE if the first procedure, variable, or property list "
      "named\n"
      "        in the contents list is stepped, FALSE if not.  Only the first "
      "thing\n"
      "        in the list is tested; the most common use will be with a word "
      "as\n"
      "        input, naming a procedure, but a contents list is allowed so "
      "that you\n"
      "        can STEPPEDP [[] [VARIABLE]] or STEPPEDP [[] [] [PROPLIST]].\n"
      "\n");

  alt("STEPPED?", "STEPPEDP");

  set("EDIT",
      "EDIT contentslist\n"
      "ED contentslist\n"
      "(EDIT)\n"
      "(ED)\n"
      "\n"
      "        command.  If invoked with an input, EDIT writes the "
      "definitions\n"
      "        of the named items into a temporary file and edits that file, "
      "using\n"
      "        an editor that depends on the platform you're using.  In "
      "wxWidgets,\n"
      "        and in the MacOS Classic version, there is an editor built into "
      "Logo.\n"
      "        In the non-wxWidgets versions for Unix, MacOS X, Windows, and "
      "DOS,\n"
      "        Logo uses your favorite editor as determined by the EDITOR "
      "environment\n"
      "        variable.  If you don't have an EDITOR variable, edits the\n"
      "        definitions using jove.  If invoked without an input, EDIT "
      "edits\n"
      "        the same file left over from a previous EDIT or EDITFILE "
      "instruction.\n"
      "        When you leave the editor, Logo reads the revised definitions "
      "and\n"
      "        modifies the workspace accordingly.  It is not an error if the\n"
      "        input includes names for which there is no previous "
      "definition.\n"
      "\n"
      "        If there is a variable LOADNOISILY whose value is TRUE, then, "
      "after\n"
      "        leaving the editor, TO commands in the temporary file print "
      "\"PROCNAME\n"
      "        defined\" (where PROCNAME is the name of the procedure being "
      "defined);\n"
      "        if LOADNOISILY is FALSE or undefined, TO commands in the file "
      "are\n"
      "        carried out silently.\n"
      "\n"
      "        If there is an environment variable called TEMP, then Logo "
      "uses\n"
      "        its value as the directory in which to write the temporary "
      "file\n"
      "        used for editing.\n"
      "\n"
      "        Exceptionally, the EDIT command can be used without its "
      "default\n"
      "        input and without parentheses provided that nothing follows it "
      "on\n"
      "        the instruction line.\n"
      "\n");

  alt("ED", "EDIT");

  set("EDITFILE",
      "EDITFILE filename\n"
      "\n"
      "        command.  Starts the Logo editor, like EDIT, but instead of "
      "editing\n"
      "        a temporary file it edits the file specified by the input.  "
      "When you\n"
      "        leave the editor, Logo reads the revised file, as for EDIT.\n"
      "        EDITFILE also remembers the filename, so that a subsequent "
      "EDIT\n"
      "        command with no input will re-edit the same file.\n"
      "\n"
      "        EDITFILE is intended as an alternative to LOAD and SAVE.  You "
      "can\n"
      "        maintain a workspace file yourself, controlling the order in "
      "which\n"
      "        definitions appear, maintaining comments in the file, and so "
      "on.\n"
      "\n");

  set("EDALL", "EDALL                                                   "
               "(library procedure)\n"
               "\n"
               "        command.  Abbreviates EDIT CONTENTS.\n"
               "\n");

  set("EDPS", "EDPS                                                    "
              "(library procedure)\n"
              "\n"
              "        command.  Abbreviates EDIT PROCEDURES.\n"
              "\n");

  set("EDNS", "EDNS                                                    "
              "(library procedure)\n"
              "\n"
              "        command.  Abbreviates EDIT NAMES.\n"
              "\n");

  set("EDPLS", "EDPLS                                                   "
               "(library procedure)\n"
               "\n"
               "        command.  Abbreviates EDIT PLISTS.\n"
               "\n");

  set("EDN", "EDN varname                                             (library "
             "procedure)\n"
             "EDN varnamelist\n"
             "\n"
             "        command.  Abbreviates EDIT NAMELIST varname(list).\n"
             "\n");

  set("EDPL", "EDPL plname                                             "
              "(library procedure)\n"
              "EDPL plnamelist\n"
              "\n"
              "        command.  Abbreviates EDIT PLLIST plname(list).\n"
              "\n");

  set("SAVE",
      "SAVE filename\n"
      "\n"
      "        command.  Saves the definitions of all unburied procedures,\n"
      "        variables, and nonempty property lists in the named file.\n"
      "        Equivalent to\n"
      "\n"
      "                        to save :filename\n"
      "                        local \"oldwriter\n"
      "                        make \"oldwriter writer\n"
      "                        openwrite :filename\n"
      "                        setwrite :filename\n"
      "                        poall\n"
      "                        setwrite :oldwriter\n"
      "                        close :filename\n"
      "                        end\n"
      "\n"
      "        Exceptionally, SAVE can be used with no input and without "
      "parentheses\n"
      "        if it is the last thing on the command line.  In this case, "
      "the\n"
      "        filename from the most recent LOAD or SAVE command will be "
      "used.  (It\n"
      "        is an error if there has been no previous LOAD or SAVE.)\n"
      "\n");

  set("SAVEL",
      "SAVEL contentslist filename                             (library "
      "procedure)\n"
      "\n"
      "        command.  Saves the definitions of the procedures, variables, "
      "and\n"
      "        property lists specified by \"contentslist\" to the file named\n"
      "        \"filename\".\n"
      "\n");

  set("LOAD",
      "LOAD filename\n"
      "\n"
      "        command.  Reads instructions from the named file and executes\n"
      "        them.  The file can include procedure definitions with TO, and\n"
      "        these are accepted even if a procedure by the same name "
      "already\n"
      "        exists.  If the file assigns a list value to a variable named\n"
      "        STARTUP, then that list is run as an instructionlist after the\n"
      "        file is loaded.  If there is a variable LOADNOISILY whose "
      "value\n"
      "        is TRUE, then TO commands in the file print \"PROCNAME "
      "defined\"\n"
      "        (where PROCNAME is the name of the procedure being defined); "
      "if\n"
      "        LOADNOISILY is FALSE or undefined, TO commands in the file are\n"
      "        carried out silently.\n"
      "\n");

  set("CSLSLOAD", "CSLSLOAD name\n"
                  "\n"
                  "        command.  Loads the named file, like LOAD, but from "
                  "the directory\n"
                  "        containing the Computer Science Logo Style programs "
                  "instead of the\n"
                  "        current user's directory.\n"
                  "\n");

  set("HELP",
      "HELP name\n"
      "(HELP)\n"
      "\n"
      "        command.  Prints information from the reference manual about\n"
      "        the primitive procedure named by the input.  With no input,\n"
      "        lists all the primitives about which help is available.\n"
      "        If there is an environment variable LOGOHELP, then its value\n"
      "        is taken as the directory in which to look for help files,\n"
      "        instead of the default help directory.\n"
      "\n"
      "        If HELP is called with the name of a defined procedure for "
      "which there\n"
      "        is no help file, it will print the title line of the procedure\n"
      "        followed by lines from the procedure body that start with "
      "semicolon,\n"
      "        stopping when a non-semicolon line is seen.\n"
      "\n"
      "        Exceptionally, the HELP command can be used without its "
      "default\n"
      "        input and without parentheses provided that nothing follows it "
      "on\n"
      "        the instruction line.\n"
      "\n");

  set("SETCSLSLOC", "SETCSLSLOC path\n"
                    "\n"
                    "        command.  Tells Logo to use the specified "
                    "directory for the CSLSLOAD\n"
                    "        command, instead of the default directory.  The "
                    "format of a path\n"
                    "        depends on your operating system.\n"
                    "\n");
}

void Help::setControlStructures() {
  set("RUN",
      "RUN instructionlist\n"
      "\n"
      "        command or operation.  Runs the Logo instructions in the input\n"
      "        list; outputs if the list contains an expression that outputs.\n"
      "\n");

  set("RUNRESULT",
      "RUNRESULT instructionlist\n"
      "\n"
      "        runs the instructions in the input; outputs an empty list if\n"
      "        those instructions produce no output, or a list whose only\n"
      "        member is the output from running the input instructionlist.\n"
      "        Useful for inventing command-or-operation control structures:\n"
      "\n"
      "                local \"result\n"
      "                make \"result runresult [something]\n"
      "                if emptyp :result [stop]\n"
      "                output first :result\n"
      "\n");

  set("REPEAT", "REPEAT num instructionlist\n"
                "\n"
                "        command.  Runs the \"instructionlist\" repeatedly, "
                "\"num\" times.\n"
                "\n");

  set("FOREVER", "FOREVER instructionlist\n"
                 "\n"
                 "        command.  Runs the \"instructionlist\" repeatedly, "
                 "until something\n"
                 "        inside the instructionlist (such as STOP or THROW) "
                 "makes it stop.\n"
                 "\n");

  set("REPCOUNT",
      "REPCOUNT\n"
      "\n"
      "        outputs the repetition count of the innermost current REPEAT "
      "or\n"
      "        FOREVER, starting from 1.  If no REPEAT or FOREVER is active,\n"
      "        outputs -1.\n"
      "\n"
      "        The abbreviation # can be used for REPCOUNT unless the REPEAT "
      "is\n"
      "        inside the template input to a higher order procedure such as\n"
      "        FOREACH, in which case # has a different meaning.\n"
      "\n");

  set("IF",
      "IF tf instructionlist\n"
      "(IF tf instructionlist1 instructionlist2)\n"
      "\n"
      "        command.  If the first input has the value TRUE, then IF runs\n"
      "        the second input.  If the first input has the value FALSE, "
      "then\n"
      "        IF does nothing.  (If given a third input, IF acts like "
      "IFELSE,\n"
      "        as described below.)  It is an error if the first input is not\n"
      "        either TRUE or FALSE.\n"
      "\n");

  set("IFELSE",
      "IFELSE tf instructionlist1 instructionlist2\n"
      "\n"
      "        command or operation.  If the first input has the value TRUE, "
      "then\n"
      "        IFELSE runs the second input.  If the first input has the value "
      "FALSE,\n"
      "        then IFELSE runs the third input.  IFELSE outputs a value if "
      "the\n"
      "        instructionlist contains an expression that outputs a value.\n"
      "\n");

  set("TEST",
      "TEST tf\n"
      "\n"
      "        command.  Remembers its input, which must be TRUE or FALSE, for "
      "use\n"
      "        by later IFTRUE or IFFALSE instructions.  The effect of TEST is "
      "local\n"
      "        to the procedure in which it is used; any corresponding IFTRUE "
      "or\n"
      "        IFFALSE must be in the same procedure or a subprocedure.\n"
      "\n");

  set("IFTRUE", "IFTRUE instructionlist\n"
                "IFT instructionlist\n"
                "\n"
                "        command.  Runs its input if the most recent TEST "
                "instruction had\n"
                "        a TRUE input.  The TEST must have been in the same "
                "procedure or a\n"
                "        superprocedure.\n"
                "\n");

  alt("IFT", "IFTRUE");

  set("IFFALSE", "IFFALSE instructionlist\n"
                 "IFF instructionlist\n"
                 "\n"
                 "        command.  Runs its input if the most recent TEST "
                 "instruction had\n"
                 "        a FALSE input.  The TEST must have been in the same "
                 "procedure or a\n"
                 "        superprocedure.\n"
                 "\n");

  alt("IFF", "IFFALSE");

  set("STOP",
      "STOP\n"
      "\n"
      "        command.  Ends the running of the procedure in which it "
      "appears.\n"
      "        Control is returned to the context in which that procedure was\n"
      "        invoked.  The stopped procedure does not output a value.\n"
      "\n");

  set("OUTPUT",
      "OUTPUT value\n"
      "OP value\n"
      "\n"
      "        command.  Ends the running of the procedure in which it "
      "appears.\n"
      "        That procedure outputs the value \"value\" to the context in "
      "which\n"
      "        it was invoked.  Don't be confused: OUTPUT itself is a "
      "command,\n"
      "        but the procedure that invokes OUTPUT is an operation.\n"
      "\n");

  alt("OP", "OUTPUT");

  set("CATCH",
      "CATCH tag instructionlist\n"
      "\n"
      "        command or operation.  Runs its second input.  Outputs if that\n"
      "        instructionlist outputs.  If, while running the "
      "instructionlist,\n"
      "        a THROW instruction is executed with a tag equal to the first\n"
      "        input (case-insensitive comparison), then the running of the\n"
      "        instructionlist is terminated immediately.  In this case the "
      "CATCH\n"
      "        outputs if a value input is given to THROW.  The tag must be a "
      "word.\n"
      "\n"
      "        If the tag is the word ERROR, then any error condition that "
      "arises\n"
      "        during the running of the instructionlist has the effect of "
      "THROW\n"
      "        \"ERROR instead of printing an error message and returning to\n"
      "        toplevel.  The CATCH does not output if an error is caught.  "
      "Also,\n"
      "        during the running of the instructionlist, the variable ERRACT "
      "is\n"
      "        temporarily unbound.  (If there is an error while ERRACT has a\n"
      "        value, that value is taken as an instructionlist to be run "
      "after\n"
      "        printing the error message.  Typically the value of ERRACT, if "
      "any,\n"
      "        is the list [PAUSE].)\n"
      "\n");

  set("THROW",
      "THROW tag\n"
      "(THROW tag value)\n"
      "\n"
      "        command.  Must be used within the scope of a CATCH with an "
      "equal\n"
      "        tag.  Ends the running of the instructionlist of the CATCH.  "
      "If\n"
      "        THROW is used with only one input, the corresponding CATCH "
      "does\n"
      "        not output a value.  If THROW is used with two inputs, the "
      "second\n"
      "        provides an output for the CATCH.\n"
      "\n"
      "        THROW \"TOPLEVEL can be used to terminate all running "
      "procedures and\n"
      "        interactive pauses, and return to the toplevel instruction "
      "prompt.\n"
      "        Typing the system interrupt character (alt-S for wxWidgets; "
      "otherwise\n"
      "        normally control-C for Unix, control-Q for DOS, or "
      "command-period for\n"
      "        Mac) has the same effect.\n"
      "\n"
      "        THROW \"ERROR can be used to generate an error condition.  If "
      "the\n"
      "        error is not caught, it prints a message (THROW \"ERROR) with "
      "the\n"
      "        usual indication of where the error (in this case the THROW)\n"
      "        occurred.  If a second input is used along with a tag of "
      "ERROR,\n"
      "        that second input is used as the text of the error message\n"
      "        instead of the standard message.  Also, in this case, the "
      "location\n"
      "        indicated for the error will be, not the location of the "
      "THROW,\n"
      "        but the location where the procedure containing the THROW was\n"
      "        invoked.  This allows user-defined procedures to generate "
      "error\n"
      "        messages as if they were primitives.  Note: in this case the\n"
      "        corresponding CATCH \"ERROR, if any, does not output, since the "
      "second\n"
      "        input to THROW is not considered a return value.\n"
      "\n"
      "        THROW \"SYSTEM immediately leaves Logo, returning to the "
      "operating\n"
      "        system, without printing the usual parting message and without\n"
      "        deleting any editor temporary file written by EDIT.\n"
      "\n");

  set("ERROR", "ERROR\n"
               "\n"
               "        outputs a list describing the error just caught, if "
               "any.  If there was\n"
               "        not an error caught since the last use of ERROR, the "
               "empty list will\n"
               "        be output.  The error list contains four members: an "
               "integer code\n"
               "        corresponding to the type of error, the text of the "
               "error message (as\n"
               "        a single word including spaces), the name of the "
               "procedure in which\n"
               "        the error occurred, and the instruction line on which "
               "the error\n"
               "        occurred.\n"
               "\n");

  set("PAUSE",
      "PAUSE\n"
      "\n"
      "        command or operation.  Enters an interactive pause.  The user "
      "is\n"
      "        prompted for instructions, as at toplevel, but with a prompt "
      "that\n"
      "        includes the name of the procedure in which PAUSE was invoked.\n"
      "        Local variables of that procedure are available during the "
      "pause.\n"
      "        PAUSE outputs if the pause is ended by a CONTINUE with an "
      "input.\n"
      "\n"
      "        If the variable ERRACT exists, and an error condition occurs, "
      "the\n"
      "        contents of that variable are run as an instructionlist.  "
      "Typically\n"
      "        ERRACT is given the value [PAUSE] so that an interactive pause "
      "will\n"
      "        be entered in the event of an error.  This allows the user to "
      "check\n"
      "        values of local variables at the time of the error.\n"
      "\n"
      "        Typing the system quit character (alt-S for wxWidgets; "
      "otherwise\n"
      "        normally control-\\ for Unix, control-W for DOS, or "
      "command-comma for\n"
      "        Mac) will also enter a pause.\n"
      "\n");

  set("CONTINUE",
      "CONTINUE value\n"
      "CO value\n"
      "(CONTINUE)\n"
      "(CO)\n"
      "\n"
      "        command.  Ends the current interactive pause, returning to the\n"
      "        context of the PAUSE invocation that began it.  If CONTINUE is\n"
      "        given an input, that value is used as the output from the "
      "PAUSE.\n"
      "        If not, the PAUSE does not output.\n"
      "\n"
      "        Exceptionally, the CONTINUE command can be used without its "
      "default\n"
      "        input and without parentheses provided that nothing follows it "
      "on\n"
      "        the instruction line.\n"
      "\n");

  alt("CO", "CONTINUE");

  set("WAIT",
      "WAIT time\n"
      "\n"
      "        command.  Delays further execution for \"time\" 60ths of a "
      "second.\n"
      "        Also causes any buffered characters destined for the terminal "
      "to\n"
      "        be printed immediately.  WAIT 0 can be used to achieve this\n"
      "        buffer flushing without actually waiting.\n"
      "\n");

  set("BYE",
      "BYE\n"
      "\n"
      "        command.  Exits from Logo; returns to the operating system.\n"
      "\n");

  set(".MAYBEOUTPUT",
      ".MAYBEOUTPUT value                                      (special form)\n"
      "\n"
      "        works like OUTPUT except that the expression that provides the\n"
      "        input value might not, in fact, output a value, in which case\n"
      "        the effect is like STOP.  This is intended for use in control\n"
      "        structure definitions, for cases in which you don't know "
      "whether\n"
      "        or not some expression produces a value.  Example:\n"
      "\n"
      "                to invoke :function [:inputs] 2\n"
      "                .maybeoutput apply :function :inputs\n"
      "                end\n"
      "\n"
      "                ? (invoke \"print \"a \"b \"c)\n"
      "                a b c\n"
      "                ? print (invoke \"word \"a \"b \"c)\n"
      "                abc\n"
      "\n"
      "        This is an alternative to RUNRESULT.  It's fast and easy to "
      "use,\n"
      "        at the cost of being an exception to Logo's evaluation rules.\n"
      "        (Ordinarily, it should be an error if the expression that's\n"
      "        supposed to provide an input to something doesn't have a "
      "value.)\n"
      "\n");

  set("GOTO", "GOTO word\n"
              "\n"
              "        command.  Looks for a TAG command with the same input "
              "in the same\n"
              "        procedure, and continues running the procedure from the "
              "location of\n"
              "        that TAG.  It is meaningless to use GOTO outside of a "
              "procedure.\n"
              "\n");

  set("TAG", "TAG quoted.word\n"
             "\n"
             "        command.  Does nothing.  The input must be a literal "
             "word following\n"
             "        a quotation mark (\"), not the result of a computation.  "
             "Tags are\n"
             "        used by the GOTO command.\n"
             "\n");

  set("IGNORE", "IGNORE value                                            "
                "(library procedure)\n"
                "\n"
                "        command.  Does nothing.  Used when an expression is "
                "evaluated for\n"
                "        a side effect and its actual value is unimportant.\n"
                "\n");

  set("`",
      "` list                                                  (library "
      "procedure)\n"
      "\n"
      "        outputs a list equal to its input but with certain "
      "substitutions.\n"
      "        If a member of the input list is the word \",\" (comma) then "
      "the\n"
      "        following member should be an instructionlist that produces an\n"
      "        output when run.  That output value replaces the comma and the\n"
      "        instructionlist.  If a member of the input list is the word "
      "\",@\"\n"
      "        (comma atsign) then the following member should be an "
      "instructionlist\n"
      "        that outputs a list when run.  The members of that list replace "
      "the\n"
      "        ,@ and the instructionlist.  Example:\n"
      "\n"
      "                show `[foo baz ,[bf [a b c]] garply ,@[bf [a b c]]]\n"
      "\n"
      "        will print\n"
      "\n"
      "                [foo baz [b c] garply b c]\n"
      "\n"
      "        A word starting with , or ,@ is treated as if the rest of the "
      "word\n"
      "        were a one-word list, e.g., ,:FOO is equivalent to ,[:FOO].\n"
      "\n"
      "        A word starting with \", (quote comma) or :, (colon comma) "
      "becomes a\n"
      "        word starting with \" or : but with the result of running the\n"
      "        substitution (or its first word, if the result is a list) "
      "replacing\n"
      "        what comes after the comma.\n"
      "\n"
      "        Backquotes can be nested.  Substitution is done only for commas "
      "at\n"
      "        the same depth as the backquote in which they are found:\n"
      "\n"
      "                ? show `[a `[b ,[1+2] ,[foo ,[1+3] d] e] f]\n"
      "                [a ` [b , [1+2] , [foo 4 d] e] f]\n"
      "\n"
      "                ?make \"name1 \"x\n"
      "                ?make \"name2 \"y\n"
      "                ? show `[a `[b ,:,:name1 ,\",:name2 d] e]\n"
      "                [a ` [b , [:x] , [\"y] d] e]\n"
      "\n");

  set("FOR",
      "FOR forcontrol instructionlist                          (library "
      "procedure)\n"
      "\n"
      "        command.  The first input must be a list containing three or "
      "four\n"
      "        members: (1) a word, which will be used as the name of a local\n"
      "        variable; (2) a word or list that will be evaluated as by RUN "
      "to\n"
      "        determine a number, the starting value of the variable; (3) a "
      "word\n"
      "        or list that will be evaluated to determine a number, the limit "
      "value\n"
      "        of the variable; (4) an optional word or list that will be "
      "evaluated\n"
      "        to determine the step size.  If the fourth member is missing, "
      "the\n"
      "        step size will be 1 or -1 depending on whether the limit value "
      "is\n"
      "        greater than or less than the starting value, respectively.\n"
      "\n"
      "        The second input is an instructionlist.  The effect of FOR is "
      "to run\n"
      "        that instructionlist repeatedly, assigning a new value to the "
      "control\n"
      "        variable (the one named by the first member of the forcontrol "
      "list)\n"
      "        each time.  First the starting value is assigned to the "
      "control\n"
      "        variable.  Then the value is compared to the limit value.  FOR "
      "is\n"
      "        complete when the sign of (current - limit) is the same as the "
      "sign\n"
      "        of the step size.  (If no explicit step size is provided, the\n"
      "        instructionlist is always run at least once.  An explicit step "
      "size\n"
      "        can lead to a zero-trip FOR, e.g., FOR [I 1 0 1] ...)  "
      "Otherwise, the\n"
      "        instructionlist is run, then the step is added to the current "
      "value\n"
      "        of the control variable and FOR returns to the comparison "
      "step.\n"
      "\n"
      "                ? for [i 2 7 1.5] [print :i]\n"
      "                2\n"
      "                3.5\n"
      "                5\n"
      "                6.5\n"
      "                ?\n"
      "\n");

  set("DO.WHILE", "DO.WHILE instructionlist tfexpression                   "
                  "(library procedure)\n"
                  "\n"
                  "        command.  Repeatedly evaluates the "
                  "\"instructionlist\" as long as the\n"
                  "        evaluated \"tfexpression\" remains TRUE.  Evaluates "
                  "the first input\n"
                  "        first, so the \"instructionlist\" is always run at "
                  "least once.  The\n"
                  "        \"tfexpression\" must be an expressionlist whose "
                  "value when evaluated\n"
                  "        is TRUE or FALSE.\n"
                  "\n");

  set("WHILE", "WHILE tfexpression instructionlist                      "
               "(library procedure)\n"
               "\n"
               "        command.  Repeatedly evaluates the \"instructionlist\" "
               "as long as the\n"
               "        evaluated \"tfexpression\" remains TRUE.  Evaluates "
               "the first input\n"
               "        first, so the \"instructionlist\" may never be run at "
               "all.  The\n"
               "        \"tfexpression\" must be an expressionlist whose value "
               "when evaluated\n"
               "        is TRUE or FALSE.\n"
               "\n");

  set("DO.UNTIL", "DO.UNTIL instructionlist tfexpression                   "
                  "(library procedure)\n"
                  "\n"
                  "        command.  Repeatedly evaluates the "
                  "\"instructionlist\" as long as the\n"
                  "        evaluated \"tfexpression\" remains FALSE.  "
                  "Evaluates the first input\n"
                  "        first, so the \"instructionlist\" is always run at "
                  "least once.  The\n"
                  "        \"tfexpression\" must be an expressionlist whose "
                  "value when evaluated\n"
                  "        is TRUE or FALSE.\n"
                  "\n");

  set("UNTIL", "UNTIL tfexpression instructionlist                      "
               "(library procedure)\n"
               "\n"
               "        command.  Repeatedly evaluates the \"instructionlist\" "
               "as long as the\n"
               "        evaluated \"tfexpression\" remains FALSE.  Evaluates "
               "the first input\n"
               "        first, so the \"instructionlist\" may never be run at "
               "all.  The\n"
               "        \"tfexpression\" must be an expressionlist whose value "
               "when evaluated\n"
               "        is TRUE or FALSE.\n"
               "\n");

  set("CASE",
      "CASE value clauses                                      (library "
      "procedure)\n"
      "\n"
      "        command or operation.  The second input is a list of lists "
      "(clauses);\n"
      "        each clause is a list whose first element is either a list of "
      "values\n"
      "        or the word ELSE and whose butfirst is a Logo expression or\n"
      "        instruction.  CASE examines the clauses in order.  If a clause "
      "begins\n"
      "        with the word ELSE (upper or lower case), then the butfirst of "
      "that\n"
      "        clause is evaluated and CASE outputs its value, if any.  If the "
      "first\n"
      "        input to CASE is a member of the first element of a clause, "
      "then the\n"
      "        butfirst of that clause is evaluated and CASE outputs its "
      "value, if\n"
      "        any.  If neither of these conditions is met, then CASE goes on "
      "to the\n"
      "        next clause.  If no clause is satisfied, CASE does nothing.  "
      "Example:\n"
      "\n"
      "                to vowelp :letter\n"
      "                output case :letter [ [[a e i o u] \"true] [else "
      "\"false] ]\n"
      "                end\n"
      "\n");

  set("COND",
      "COND clauses                                            (library "
      "procedure)\n"
      "\n"
      "        command or operation.  The input is a list of lists (clauses); "
      "each\n"
      "        clause is a list whose first element is either an expression "
      "whose\n"
      "        value is TRUE or FALSE, or the word ELSE, and whose butfirst is "
      "a Logo\n"
      "        expression or instruction.  COND examines the clauses in order. "
      " If a\n"
      "        clause begins with the word ELSE (upper or lower case), then "
      "the\n"
      "        butfirst of that clause is evaluated and CASE outputs its "
      "value, if\n"
      "        any.  Otherwise, the first element of the clause is evaluated; "
      "the\n"
      "        resulting value must be TRUE or FALSE.  If it's TRUE, then the\n"
      "        butfirst of that clause is evaluated and COND outputs its "
      "value, if\n"
      "        any.  If the value is FALSE, then COND goes on to the next "
      "clause.  If\n"
      "        no clause is satisfied, COND does nothing.  Example:\n"
      "\n"
      "                to evens :numbers       ; select even numbers from a "
      "list\n"
      "                op cond [ [[emptyp :numbers] []]\n"
      "                          [[evenp first :numbers]  ; assuming EVENP is "
      "defined\n"
      "                           fput first :numbers evens butfirst "
      ":numbers]\n"
      "                          [else evens butfirst :numbers] ]\n"
      "                end\n"
      "\n");

  //    TEMPLATE-BASED ITERATION
  //    ------------------------

  set("APPLY",
      "APPLY template inputlist\n"
      "\n"
      "        command or operation.  Runs the \"template,\" filling its slots "
      "with\n"
      "        the members of \"inputlist.\"  The number of members in "
      "\"inputlist\"\n"
      "        must be an acceptable number of slots for \"template.\"  It is\n"
      "        illegal to apply the primitive TO as a template, but anything "
      "else\n"
      "        is okay.  APPLY outputs what \"template\" outputs, if "
      "anything.\n"
      "\n");

  set("INVOKE",
      "INVOKE template input                                   (library "
      "procedure)\n"
      "(INVOKE template input1 input2 ...)\n"
      "\n"
      "        command or operation.  Exactly like APPLY except that the "
      "inputs\n"
      "        are provided as separate expressions rather than in a list.\n"
      "\n");

  set("FOREACH",
      "FOREACH data template                                   (library "
      "procedure)\n"
      "(FOREACH data1 data2 ... template)\n"
      "\n"
      "        command.  Evaluates the template list repeatedly, once for "
      "each\n"
      "        member of the data list.  If more than one data list are "
      "given,\n"
      "        each of them must be the same length.  (The data inputs can be\n"
      "        words, in which case the template is evaluated once for each\n"
      "        character.)\n"
      "\n"
      "        In a template, the symbol ?REST represents the portion of the\n"
      "        data input to the right of the member currently being used as\n"
      "        the ? slot-filler.  That is, if the data input is [A B C D E]\n"
      "        and the template is being evaluated with ? replaced by B, then\n"
      "        ?REST would be replaced by [C D E].  If multiple parallel "
      "slots\n"
      "        are used, then (?REST 1) goes with ?1, etc.\n"
      "\n"
      "        In a template, the symbol # represents the position in the "
      "data\n"
      "        input of the member currently being used as the ? slot-filler.\n"
      "        That is, if the data input is [A B C D E] and the template is\n"
      "        being evaluated with ? replaced by B, then # would be replaced\n"
      "        by 2.\n"
      "\n");

  set("MAP",
      "MAP template data                                       (library "
      "procedure)\n"
      "(MAP template data1 data2 ...)\n"
      "\n"
      "        outputs a word or list, depending on the type of the data "
      "input,\n"
      "        of the same length as that data input.  (If more than one data\n"
      "        input are given, the output is of the same type as data1.)  "
      "Each\n"
      "        member of the output is the result of evaluating the template\n"
      "        list, filling the slots with the corresponding member(s) of "
      "the\n"
      "        data input(s).  (All data inputs must be the same length.)  In "
      "the\n"
      "        case of a word output, the results of the template evaluation "
      "must\n"
      "        be words, and they are concatenated with WORD.\n"
      "\n"
      "        In a template, the symbol ?REST represents the portion of the\n"
      "        data input to the right of the member currently being used as\n"
      "        the ? slot-filler.  That is, if the data input is [A B C D E]\n"
      "        and the template is being evaluated with ? replaced by B, then\n"
      "        ?REST would be replaced by [C D E].  If multiple parallel "
      "slots\n"
      "        are used, then (?REST 1) goes with ?1, etc.\n"
      "\n"
      "        In a template, the symbol # represents the position in the "
      "data\n"
      "        input of the member currently being used as the ? slot-filler.\n"
      "        That is, if the data input is [A B C D E] and the template is\n"
      "        being evaluated with ? replaced by B, then # would be replaced\n"
      "        by 2.\n"
      "\n");

  set("MAP.SE",
      "MAP.SE template data                                    (library "
      "procedure)\n"
      "(MAP.SE template data1 data2 ...)\n"
      "\n"
      "        outputs a list formed by evaluating the template list "
      "repeatedly\n"
      "        and concatenating the results using SENTENCE.  That is, the\n"
      "        members of the output are the members of the results of the\n"
      "        evaluations.  The output list might, therefore, be of a "
      "different\n"
      "        length from that of the data input(s).  (If the result of an\n"
      "        evaluation is the empty list, it contributes nothing to the "
      "final\n"
      "        output.)  The data inputs may be words or lists.\n"
      "\n"
      "        In a template, the symbol ?REST represents the portion of the\n"
      "        data input to the right of the member currently being used as\n"
      "        the ? slot-filler.  That is, if the data input is [A B C D E]\n"
      "        and the template is being evaluated with ? replaced by B, then\n"
      "        ?REST would be replaced by [C D E].  If multiple parallel "
      "slots\n"
      "        are used, then (?REST 1) goes with ?1, etc.\n"
      "\n"
      "        In a template, the symbol # represents the position in the "
      "data\n"
      "        input of the member currently being used as the ? slot-filler.\n"
      "        That is, if the data input is [A B C D E] and the template is\n"
      "        being evaluated with ? replaced by B, then # would be replaced\n"
      "        by 2.\n"
      "\n");

  set("FILTER",
      "FILTER tftemplate data                                  (library "
      "procedure)\n"
      "\n"
      "        outputs a word or list, depending on the type of the data "
      "input,\n"
      "        containing a subset of the members (for a list) or characters "
      "(for\n"
      "        a word) of the input.  The template is evaluated once for each\n"
      "        member or character of the data, and it must produce a TRUE or\n"
      "        FALSE value.  If the value is TRUE, then the corresponding "
      "input\n"
      "        constituent is included in the output.\n"
      "\n"
      "                ? print filter \"vowelp \"elephant\n"
      "                eea\n"
      "                ?\n"
      "\n"
      "        In a template, the symbol ?REST represents the portion of the\n"
      "        data input to the right of the member currently being used as\n"
      "        the ? slot-filler.  That is, if the data input is [A B C D E]\n"
      "        and the template is being evaluated with ? replaced by B, then\n"
      "        ?REST would be replaced by [C D E].\n"
      "\n"
      "        In a template, the symbol # represents the position in the "
      "data\n"
      "        input of the member currently being used as the ? slot-filler.\n"
      "        That is, if the data input is [A B C D E] and the template is\n"
      "        being evaluated with ? replaced by B, then # would be replaced\n"
      "        by 2.\n"
      "\n");

  set("FIND",
      "FIND tftemplate data                                    (library "
      "procedure)\n"
      "\n"
      "        outputs the first constituent of the data input (the first "
      "member\n"
      "        of a list, or the first character of a word) for which the "
      "value\n"
      "        produced by evaluating the template with that consituent in "
      "its\n"
      "        slot is TRUE.  If there is no such constituent, the empty list\n"
      "        is output.\n"
      "\n"
      "        In a template, the symbol ?REST represents the portion of the\n"
      "        data input to the right of the member currently being used as\n"
      "        the ? slot-filler.  That is, if the data input is [A B C D E]\n"
      "        and the template is being evaluated with ? replaced by B, then\n"
      "        ?REST would be replaced by [C D E].\n"
      "\n"
      "        In a template, the symbol # represents the position in the "
      "data\n"
      "        input of the member currently being used as the ? slot-filler.\n"
      "        That is, if the data input is [A B C D E] and the template is\n"
      "        being evaluated with ? replaced by B, then # would be replaced\n"
      "        by 2.\n"
      "\n");

  set("REDUCE",
      "REDUCE template data                                    (library "
      "procedure)\n"
      "\n"
      "        outputs the result of applying the template to accumulate the\n"
      "        members of the data input.  The template must be a two-slot\n"
      "        function.  Typically it is an associative function name like "
      "SUM.\n"
      "        If the data input has only one constituent (member in a list "
      "or\n"
      "        character in a word), the output is that consituent.  "
      "Otherwise,\n"
      "        the template is first applied with ?1 filled with the "
      "next-to-last\n"
      "        consitient and ?2 with the last constituent.  Then, if there "
      "are\n"
      "        more constituents, the template is applied with ?1 filled with "
      "the\n"
      "        next constituent to the left and ?2 with the result from the\n"
      "        previous evaluation.  This process continues until all "
      "constituents\n"
      "        have been used.  The data input may not be empty.\n"
      "\n"
      "        Note: If the template is, like SUM, the name of a procedure "
      "that is\n"
      "        capable of accepting arbitrarily many inputs, it is more "
      "efficient\n"
      "        to use APPLY instead of REDUCE.  The latter is good for "
      "associative\n"
      "        procedures that have been written to accept exactly two "
      "inputs:\n"
      "\n"
      "                to max :a :b\n"
      "                output ifelse :a > :b [:a] [:b]\n"
      "                end\n"
      "\n"
      "                print reduce \"max [...]\n"
      "\n"
      "        Alternatively, REDUCE can be used to write MAX as a procedure\n"
      "        that accepts any number of inputs, as SUM does:\n"
      "\n"
      "                to max [:inputs] 2\n"
      "                if emptyp :inputs ~\n"
      "                   [(throw \"error [not enough inputs to max])]\n"
      "                output reduce [ifelse ?1 > ?2 [?1] [?2]] :inputs\n"
      "                end\n"
      "\n");

  set("CROSSMAP",
      "CROSSMAP template listlist                              (library "
      "procedure)\n"
      "(CROSSMAP template data1 data2 ...)\n"
      "\n"
      "        outputs a list containing the results of template evaluations.\n"
      "        Each data list contributes to a slot in the template; the "
      "number\n"
      "        of slots is equal to the number of data list inputs.  As a "
      "special\n"
      "        case, if only one data list input is given, that list is taken "
      "as\n"
      "        a list of data lists, and each of its members contributes "
      "values\n"
      "        to a slot.  CROSSMAP differs from MAP in that instead of "
      "taking\n"
      "        members from the data inputs in parallel, it takes all "
      "possible\n"
      "        combinations of members of data inputs, which need not be the "
      "same\n"
      "        length.\n"
      "\n"
      "                ? show (crossmap [word ?1 ?2] [a b c] [1 2 3 4])\n"
      "                [a1 a2 a3 a4 b1 b2 b3 b4 c1 c2 c3 c4]\n"
      "                ?\n"
      "\n"
      "        For compatibility with the version in the first edition of "
      "CSLS,\n"
      "        CROSSMAP templates may use the notation :1 instead of ?1 to "
      "indicate\n"
      "        slots.\n"
      "\n");

  set("CASCADE",
      "CASCADE endtest template startvalue                     (library "
      "procedure)\n"
      "(CASCADE endtest tmp1 sv1 tmp2 sv2 ...)\n"
      "(CASCADE endtest tmp1 sv1 tmp2 sv2 ... finaltemplate)\n"
      "\n"
      "        outputs the result of applying a template (or several "
      "templates,\n"
      "        as explained below) repeatedly, with a given value filling the\n"
      "        slot the first time, and the result of each application "
      "filling\n"
      "        the slot for the following application.\n"
      "\n"
      "        In the simplest case, CASCADE has three inputs.  The second "
      "input\n"
      "        is a one-slot expression template.  That template is evaluated\n"
      "        some number of times (perhaps zero).  On the first evaluation,\n"
      "        the slot is filled with the third input; on subsequent "
      "evaluations,\n"
      "        the slot is filled with the result of the previous evaluation.\n"
      "        The number of evaluations is determined by the first input.  "
      "This\n"
      "        can be either a nonnegative integer, in which case the template "
      "is\n"
      "        evaluated that many times, or a predicate expression template, "
      "in\n"
      "        which case it is evaluated (with the same slot filler that will "
      "be\n"
      "        used for the evaluation of the second input) repeatedly, and "
      "the\n"
      "        CASCADE evaluation continues as long as the predicate value is\n"
      "        FALSE.  (In other words, the predicate template indicates the\n"
      "        condition for stopping.)\n"
      "\n"
      "        If the template is evaluated zero times, the output from "
      "CASCADE\n"
      "        is the third (startvalue) input.  Otherwise, the output is the\n"
      "        value produced by the last template evaluation.\n"
      "\n"
      "        CASCADE templates may include the symbol # to represent the "
      "number\n"
      "        of times the template has been evaluated.  This slot is filled "
      "with\n"
      "        1 for the first evaluation, 2 for the second, and so on.\n"
      "\n"
      "                ? show cascade 5 [lput # ?] []\n"
      "                [1 2 3 4 5]\n"
      "                ? show cascade [vowelp first ?] [bf ?] \"spring\n"
      "                ing\n"
      "                ? show cascade 5 [# * ?] 1\n"
      "                120\n"
      "                ?\n"
      "\n"
      "        Several cascaded results can be computed in parallel by "
      "providing\n"
      "        additional template-startvalue pairs as inputs to CASCADE.  In "
      "this\n"
      "        case, all templates (including the endtest template, if used) "
      "are\n"
      "        multi-slot, with the number of slots equal to the number of "
      "pairs of\n"
      "        inputs.  In each round of evaluations, ?2, for example, "
      "represents the\n"
      "        result of evaluating the second template in the previous round. "
      " If\n"
      "        the total number of inputs (including the first endtest input) "
      "is odd,\n"
      "        then the output from CASCADE is the final value of the first "
      "template.\n"
      "        If the total number of inputs is even, then the last input is "
      "a\n"
      "        template that is evaluated once, after the end test is "
      "satisfied, to\n"
      "        determine the output from CASCADE.\n"
      "\n"
      "                to fibonacci :n\n"
      "                output (cascade :n [?1 + ?2] 1 [?1] 0)\n"
      "                end\n"
      "\n"
      "                to piglatin :word\n"
      "                output (cascade [vowelp first ?] ~\n"
      "                                [word bf ? first ?] ~\n"
      "                                :word ~\n"
      "                                [word ? \"ay])\n"
      "                end\n"
      "\n");

  set("CASCADE.2",
      "CASCADE.2 endtest temp1 startval1 temp2 startval2       (library "
      "procedure)\n"
      "\n"
      "        outputs the result of invoking CASCADE with the same inputs.\n"
      "        The only difference is that the default number of inputs is\n"
      "        five instead of three.\n"
      "\n");

  set("TRANSFER",
      "TRANSFER endtest template inbasket                      (library "
      "procedure)\n"
      "\n"
      "        outputs the result of repeated evaluation of the template.\n"
      "        The template is evaluated once for each member of the list\n"
      "        \"inbasket.\"  TRANSFER maintains an \"outbasket\" that is\n"
      "        initially the empty list.  After each evaluation of the\n"
      "        template, the resulting value becomes the new outbasket.\n"
      "\n"
      "        In the template, the symbol ?IN represents the current member\n"
      "        from the inbasket; the symbol ?OUT represents the entire\n"
      "        current outbasket.  Other slot symbols should not be used.\n"
      "\n"
      "        If the first (endtest) input is an empty list, evaluation\n"
      "        continues until all inbasket members have been used.  If not,\n"
      "        the first input must be a predicate expression template, and\n"
      "        evaluation continues until either that template's value is "
      "TRUE\n"
      "        or the inbasket is used up.\n"
      "\n");
}

void Help::setMacros() {
  set(".MACRO",
      ".MACRO procname :input1 :input2 ...                             "
      "(special form)\n"
      ".DEFMACRO procname text\n"
      "\n"
      "        A macro is a special kind of procedure whose output is "
      "evaluated\n"
      "        as Logo instructions in the context of the macro's caller.\n"
      "        .MACRO is exactly like TO except that the new procedure "
      "becomes\n"
      "        a macro; .DEFMACRO is exactly like DEFINE with the same "
      "exception.\n"
      "\n"
      "        Macros are useful for inventing new control structures "
      "comparable\n"
      "        to REPEAT, IF, and so on.  Such control structures can almost, "
      "but\n"
      "        not quite, be duplicated by ordinary Logo procedures.  For "
      "example,\n"
      "        here is an ordinary procedure version of REPEAT:\n"
      "\n"
      "                to my.repeat :num :instructions\n"
      "                if :num=0 [stop]\n"
      "                run :instructions\n"
      "                my.repeat :num-1 :instructions\n"
      "                end\n"
      "\n"
      "        This version works fine for most purposes, e.g.,\n"
      "\n"
      "                my.repeat 5 [print \"hello]\n"
      "\n"
      "        But it doesn't work if the instructions to be carried out "
      "include\n"
      "        OUTPUT, STOP, or LOCAL.  For example, consider this procedure:\n"
      "\n"
      "                to example\n"
      "                print [Guess my secret word.  You get three guesses.]\n"
      "                repeat 3 [type \"|?? | ~\n"
      "                          if readword = \"secret [pr \"Right! stop]]\n"
      "                print [Sorry, the word was \"secret\"!]\n"
      "                end\n"
      "\n"
      "        This procedure works as written, but if MY.REPEAT is used "
      "instead\n"
      "        of REPEAT, it won't work because the STOP will stop MY.REPEAT\n"
      "        instead of stopping EXAMPLE as desired.\n"
      "\n"
      "        The solution is to make MY.REPEAT a macro.  Instead of "
      "actually\n"
      "        carrying out the computation, a macro must return a list "
      "containing\n"
      "        Logo instructions.  The contents of that list are evaluated as "
      "if\n"
      "        they appeared in place of the call to the macro.  Here's a "
      "macro\n"
      "        version of REPEAT:\n"
      "\n"
      "                .macro my.repeat :num :instructions\n"
      "                if :num=0 [output []]\n"
      "                output sentence :instructions ~\n"
      "                                (list \"my.repeat :num-1 "
      ":instructions)\n"
      "                end\n"
      "\n"
      "        Every macro is an operation -- it must always output "
      "something.\n"
      "        Even in the base case, MY.REPEAT outputs an empty instruction\n"
      "        list.  To show how MY.REPEAT works, let's take the example\n"
      "\n"
      "                my.repeat 5 [print \"hello]\n"
      "\n"
      "        For this example, MY.REPEAT will output the instruction list\n"
      "\n"
      "                [print \"hello my.repeat 4 [print \"hello]]\n"
      "\n"
      "        Logo then executes these instructions in place of the original\n"
      "        invocation of MY.REPEAT; this prints \"hello\" once and "
      "invokes\n"
      "        another repetition.\n"
      "\n"
      "        The technique just shown, although fairly easy to understand,\n"
      "        has the defect of slowness because each repetition has to\n"
      "        construct an instruction list for evaluation.  Another "
      "approach\n"
      "        is to make MY.REPEAT a macro that works just like the "
      "non-macro\n"
      "        version unless the instructions to be repeated include OUTPUT\n"
      "        or STOP:\n"
      "\n"
      "                .macro my.repeat :num :instructions\n"
      "                catch \"repeat.catchtag ~\n"
      "                      [op repeat.done runresult [repeat1 :num "
      ":instructions]]\n"
      "                op []\n"
      "                end\n"
      "\n"
      "                to repeat1 :num :instructions\n"
      "                if :num=0 [throw \"repeat.catchtag]\n"
      "                run :instructions\n"
      "                .maybeoutput repeat1 :num-1 :instructions\n"
      "                end\n"
      "\n"
      "                to repeat.done :repeat.result\n"
      "                if emptyp :repeat.result [op [stop]]\n"
      "                op list \"output quoted first :repeat.result\n"
      "                end\n"
      "\n"
      "        If the instructions do not include STOP or OUTPUT, then REPEAT1 "
      "will\n"
      "        reach its base case and invoke THROW.  As a result, MY.REPEAT's "
      "last\n"
      "        instruction line will output an empty list, so the evaluation "
      "of the\n"
      "        macro result by the caller will do nothing.  But if a STOP or "
      "OUTPUT\n"
      "        happens, then REPEAT.DONE will output a STOP or OUTPUT "
      "instruction\n"
      "        that will be executed in the caller's context.\n"
      "\n"
      "        The macro-defining commands have names starting with a dot "
      "because\n"
      "        macros are an advanced feature of Logo; it's easy to get in "
      "trouble\n"
      "        by defining a macro that doesn't terminate, or by failing to\n"
      "        construct the instruction list properly.\n"
      "\n"
      "        Lisp users should note that Logo macros are NOT special forms.\n"
      "        That is, the inputs to the macro are evaluated normally, as "
      "they\n"
      "        would be for any other Logo procedure.  It's only the output "
      "from\n"
      "        the macro that's handled unusually.\n"
      "\n"
      "        Here's another example:\n"
      "\n"
      "                .macro localmake :name :value\n"
      "                output (list \"local             ~\n"
      "                             word \"\" :name      ~\n"
      "                             \"apply             ~\n"
      "                             \"\"make             ~\n"
      "                             (list :name :value))\n"
      "                end\n"
      "\n"
      "        It's used this way:\n"
      "\n"
      "                to try\n"
      "                localmake \"garply \"hello\n"
      "                print :garply\n"
      "                end\n"
      "\n"
      "        LOCALMAKE outputs the list\n"
      "\n"
      "                [local \"garply apply \"make [garply hello]]\n"
      "\n"
      "        The reason for the use of APPLY is to avoid having to decide\n"
      "        whether or not the second input to MAKE requires a quotation\n"
      "        mark before it.  (In this case it would -- MAKE \"GARPLY "
      "\"HELLO --\n"
      "        but the quotation mark would be wrong if the value were a "
      "list.)\n"
      "\n"
      "        It's often convenient to use the ` function to construct the\n"
      "        instruction list:\n"
      "\n"
      "                .macro localmake :name :value\n"
      "                op `[local ,[word \"\" :name] apply \"make [,[:name] "
      ",[:value]]]\n"
      "                end\n"
      "\n"
      "        On the other hand, ` is pretty slow, since it's tree recursive "
      "and\n"
      "        written in Logo.\n"
      "\n");

  alt(".DEFMACRO", ".MACRO");

  set("MACROP", "MACROP name\n"
                "MACRO? name\n"
                "\n"
                "        outputs TRUE if its input is the name of a macro.\n"
                "\n");

  alt("MACRO?", "MACROP");

  set("MACROEXPAND",
      "MACROEXPAND expr                                        (library "
      "procedure)\n"
      "\n"
      "        takes as its input a Logo expression that invokes a macro (that "
      "is,\n"
      "        one that begins with the name of a macro) and outputs the the "
      "Logo\n"
      "        expression into which the macro would translate the input "
      "expression.\n"
      "\n"
      "\n"
      "                .macro localmake :name :value\n"
      "                op `[local ,[word \"\" :name] apply \"make [,[:name] "
      ",[:value]]]\n"
      "                end\n"
      "\n"
      "                ? show macroexpand [localmake \"pi 3.14159]\n"
      "                [local \"pi apply \"make [pi 3.14159]]\n"
      "\n");
}
