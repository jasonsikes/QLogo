#ifndef STRINGCONSTANTS_H
#define STRINGCONSTANTS_H

//===-- qlogo/stringconstants.h - StringContants class definition -------*- C++
//-*-===//
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
/// This file contains the declaration of the StringConstants class, which
/// provides translatable string contants for QLogo.
///
//===----------------------------------------------------------------------===//

#include <qobject.h>

// Here, the s is a dummy so that we can copy and paste the block of definitions
#define ck(n,s) const QString n();


class StringConstants : public QObject {
    Q_OBJECT

public:
    StringConstants(QObject *parent = 0);

    // ===================== Begin copied block ===================

    ck(nothing,"nothing")
        ck(word,"Word")
        ck(array,"Array")
        ck(list,"List")
        ck(astnode,"ASTNode")
        ck(quotedname,"QuotedName")
        ck(valueof,"ValueOf")
        ck(number,"number")
        ck(literal,"literal")
        ck(set,"SET")
        ck(stop, "STOP")
        ck(ktrue,"true")
        ck(kctrue,"TRUE")
        ck(kfalse,"false")
        ck(kcfalse,"FALSE")
        ck(toplevel,"TOPLEVEL")
        ck(system,"SYSTEM")
        ck(erract,"ERRACT")
        ck(_in_," in ")
        ck(errNoSay,"You don't say what to do with %1")
        ck(logoPlatform,"LOGOPLATFORM")
        ck(logoVersion,"LOGOVERSION")
        ck(allowGetSet,"ALLOWGETSET")
        ck(loadnoisily,"LOADNOISILY")
        ck(buttonact,"BUTTONACT")
        ck(fullprintp,"FULLPRINTP")
        ck(printdepthlimit,"PRINTDEPTHLIMIT")
        ck(printwidthlimit,"PRINTWIDTHLIMIT")
        ck(unburyonedit,"UNBURYONEDIT")
        ck(caseignoredp,"CASEIGNOREDP")
        ck(startup,"STARTUP")
        ck(keyact,"KEYACT")
        ck(shadowed_by_local," shadowed by local in procedure call")
        ck(dMacro, ".macro")
        ck(dcMacro, ".MACRO")
        ck(dDefmacro, ".DEFMACRO")
        ck(pause,"PAUSE")
        ck(tag,"TAG")
        ck(already_pausing, "Already Pausing")
        ck(overwrite,"OVERWRITE")
        ck(insert,"INSERT")
        ck(error, "ERROR")
        ck(kgoto, "GOTO")
        ck(textscreen, "textscreen")
        ck(fullscreen, "fullscreen")
        ck(splitscreen, "splitscreen")
        ck(wrap,"wrap")
        ck(fence,"fence")
        ck(window,"window")
        ck(paint,"paint")
        ck(reverse,"reverse")
        ck(erase,"erase")
        ck(make12,"Make \"%1 %2\n")
        ck(pprop123,"Pprop %1 %2 %3\n")
        ck(plist12,"Plist %1 = %2\n")
        ck(end,"END")
        ck(_defined," defined\n")
        ck(_stops," stops\n")
        ck(_outputs_," outputs ")
        ck(pausing,"Pausing...\n")
        ck(to, "to")
        ck(to_, "to ")

        // Error Messages

        ck(eOutBounds, "Turtle out of bounds")
        ck(eDontlike ,"%1 doesn't like %2 as input")
        ck(eDidntOutput ,"%1 didn't output to %2")
        ck(eNotEnoughInputs ,"not enough inputs to %1")
        ck(eTooManyInputs ,"too many inputs to %1")
        ck(eNoSay ,"You don't say what to do with %1")
        ck(eCParenNotFound ,"')' not found")
        ck(eNoValue ,"%1 has no value")
        ck(eNoHow ,"I don't know how to %1")
        ck(eAlreadyDefined ,"%1 is already defined")
        ck(eIsPrimitive ,"%1 is a primitive")
        ck(eCantInProcedure ,"can't use %1 inside a procedure")
        ck(eCantInPause ,"Can't use %1 within PAUSE")
        ck(eUnexpectedBracket ,"unexpected ']'")
        ck(eUnexpectedBrace ,"unexpected '}'")
        ck(eUnexpectedParen ,"unexpected ')'")
        ck(eAlreadyDribbling ,"already dribbling")
        ck(eFileError ,"File system error")
        ck(eRunlistMultExpressions ,"Runlist %1 has more than one expression")
        ck(eAlreadyOpen ,"File %1 already open")
        ck(eCantOpen ,"I can't open file %1")
        ck(eNotOpen ,"File %1 not open")
        ck(eAlreadyFilling ,"Already filling")
        ck(eNoGraphics ,"Graphics not initialized")
        ck(eNoTest ,"%1 without TEST")
        ck(eNotInProcedure ,"Can only use %1 inside a procedure")
        ck(eNotList ,"Macro returned %1 instead of a list")
        ck(eCantInsideRunresult ,"Can't use %1 inside RUNRESULT")
        ck(eCantNoApply ,"Can't use %1 without APPLY")
        ck(eStackOverflow ,"Stack overflow")
        ck(eNoCatch, "Can't find catch tag for %1")
        ck(throwError, "Throw \"Error")

        // Commands

        ck(cword, "WORD")
        ck(clist, "LIST")
        ck(sentence, "SENTENCE")
        ck(se, "SE")
        ck(fput, "FPUT")
        ck(lput, "LPUT")
        ck(carray, "ARRAY")
        ck(listtoarray, "LISTTOARRAY")
        ck(arraytolist, "ARRAYTOLIST")
        ck(readlist, "READLIST")
        ck(rl, "RL")
        ck(readword, "READWORD")
        ck(rw, "RW")
        ck(readrawline, "READRAWLINE")
        ck(readchar, "READCHAR")
        ck(rc, "RC")
        ck(readchars, "READCHARS")
        ck(rcs, "RCS")
        ck(shell, "SHELL")
        ck(setprefix, "SETPREFIX")
        ck(prefix, "PREFIX")
        ck(openread, "OPENREAD")
        ck(openwrite, "OPENWRITE")
        ck(openappend, "OPENAPPEND")
        ck(openupdate, "OPENUPDATE")
        ck(allopen, "ALLOPEN")
        ck(setread, "SETREAD")
        ck(setwrite, "SETWRITE")
        ck(reader, "READER")
        ck(writer, "WRITER")
        ck(readpos, "READPOS")
        ck(writepos, "WRITEPOS")
        ck(setreadpos, "SETREADPOS")
        ck(setwritepos, "SETWRITEPOS")
        ck(eofp, "EOFP")
        ck(eofq, "EOF?")
        ck(keyp, "KEYP")
        ck(keyq, "KEY?")
        ck(dribble, "DRIBBLE")
        ck(nodribble, "NODRIBBLE")
        ck(cleartext, "CLEARTEXT")
        ck(ct, "CT")
        ck(cursorinsert, "CURSORINSERT")
        ck(cursoroverwrite, "CURSOROVERWRITE")
        ck(cursormode, "CURSORMODE")
        ck(close, "CLOSE")
        ck(closeall, "CLOSEALL")
        ck(erasefile, "ERASEFILE")
        ck(erf, "ERF")
        ck(first, "FIRST")
        ck(last, "LAST")
        ck(butfirst, "BUTFIRST")
        ck(bf, "BF")
        ck(firsts, "FIRSTS")
        ck(butfirsts, "BUTFIRSTS")
        ck(bfs, "BFS")
        ck(butlast, "BUTLAST")
        ck(bl, "BL")
        ck(item, "ITEM")
        ck(setitem, "SETITEM")
        ck(dsetfirst, ".SETFIRST")
        ck(dsetbf, ".SETBF")
        ck(dsetitem, ".SETITEM")
        ck(wordp, "WORDP")
        ck(wordq, "WORD?")
        ck(listp, "LISTP")
        ck(listq, "LIST?")
        ck(arrayp, "ARRAYP")
        ck(arrayq, "ARRAY?")
        ck(emptyp, "EMPTYP")
        ck(emptyq, "EMPTY?")
        ck(equalp, "EQUALP")
        ck(equalq, "EQUAL?")
        ck(notequalp, "NOTEQUALP")
        ck(notequalq, "NOTEQUAL?")
        ck(beforep, "BEFOREP")
        ck(beforeq, "BEFORE?")
        ck(deq, ".EQ")
        ck(memberp, "MEMBERP")
        ck(memberq, "MEMBER?")
        ck(substringp, "SUBSTRINGP")
        ck(substringq, "SUBSTRING?")
        ck(numberp, "NUMBERP")
        ck(numberq, "NUMBER?")
        ck(vbarredp, "VBARREDP")
        ck(vbarredq, "VBARRED?")
        ck(count, "COUNT")
        ck(ascii, "ASCII")
        ck(rawascii, "RAWASCII")
        ck(kchar, "CHAR")
        ck(member, "MEMBER")
        ck(lowercase, "LOWERCASE")
        ck(uppercase, "UPPERCASE")
        ck(standout, "STANDOUT")
        ck(parse, "PARSE")
        ck(runparse, "RUNPARSE")
        ck(minus, "MINUS")
        ck(print, "PRINT")
        ck(pr, "PR")
        ck(type, "TYPE")
        ck(show, "SHOW")
        ck(make, "MAKE")
        ck(repeat, "REPEAT")
        ck(sqrt, "SQRT")
        ck(random, "RANDOM")
        ck(rerandom, "RERANDOM")
        ck(thing, "THING")
        ck(wait, "WAIT")
        ck(setcursor, "SETCURSOR")
        ck(cursor, "CURSOR")
        ck(settextcolor, "SETTEXTCOLOR")
        ck(settc, "SETTC")
        ck(increasefont, "INCREASEFONT")
        ck(decreasefont, "DECREASEFONT")
        ck(settextsize, "SETTEXTSIZE")
        ck(textsize, "TEXTSIZE")
        ck(setfont, "SETFONT")
        ck(font, "FONT")
        ck(allfonts, "ALLFONTS")
        ck(forward, "FORWARD")
        ck(fd, "FD")
        ck(back, "BACK")
        ck(bk, "BK")
        ck(right, "RIGHT")
        ck(rt, "RT")
        ck(left, "LEFT")
        ck(lt, "LT")
        ck(clearscreen, "CLEARSCREEN")
        ck(cs, "CS")
        ck(clean, "CLEAN")
        ck(penup, "PENUP")
        ck(pu, "PU")
        ck(pendown, "PENDOWN")
        ck(pd, "PD")
        ck(pendownp, "PENDOWNP")
        ck(pendownq, "PENDOWN?")
        ck(hideturtle, "HIDETURTLE")
        ck(ht, "HT")
        ck(showturtle, "SHOWTURTLE")
        ck(st, "ST")
        ck(setxy, "SETXY")
        ck(setx, "SETX")
        ck(sety, "SETY")
        ck(setpos, "SETPOS")
        ck(pos, "POS")
        ck(home, "HOME")
        ck(heading, "HEADING")
        ck(setheading, "SETHEADING")
        ck(seth, "SETH")
        ck(arc, "ARC")
        ck(towards, "TOWARDS")
        ck(scrunch, "SCRUNCH")
        ck(setscrunch, "SETSCRUNCH")
        ck(label, "LABEL")
        ck(labelheight, "LABELHEIGHT")
        ck(setlabelheight, "SETLABELHEIGHT")
        ck(shownp, "SHOWNP")
        ck(shownq, "SHOWN?")
        ck(setpencolor, "SETPENCOLOR")
        ck(setpc, "SETPC")
        ck(pencolor, "PENCOLOR")
        ck(pc, "PC")
        ck(setpalette, "SETPALETTE")
        ck(palette, "PALETTE")
        ck(background, "BACKGROUND")
        ck(bg, "BG")
        ck(setbackground, "SETBACKGROUND")
        ck(setbg, "SETBG")
        ck(savepict, "SAVEPICT")
        ck(loadpict, "LOADPICT")
        ck(svgpict, "SVGPICT")
        ck(penpaint, "PENPAINT")
        ck(ppt, "PPT")
        ck(penerase, "PENERASE")
        ck(pe, "PE")
        ck(penreverse, "PENREVERSE")
        ck(px, "PX")
        ck(penmode, "PENMODE")
        ck(setpensize, "SETPENSIZE")
        ck(pensize, "PENSIZE")
        ck(filled, "FILLED")
        ck(cwrap, "WRAP")
        ck(cfence, "FENCE")
        ck(cwindow, "WINDOW")
        ck(turtlemode, "TURTLEMODE")
        ck(mousepos, "MOUSEPOS")
        ck(clickpos, "CLICKPOS")
        ck(bounds, "BOUNDS")
        ck(setbounds, "SETBOUNDS")
        ck(ctextscreen, "TEXTSCREEN")
        ck(ts, "TS")
        ck(cfullscreen, "FULLSCREEN")
        ck(fs, "FS")
        ck(csplitscreen, "SPLITSCREEN")
        ck(ss, "SS")
        ck(screenmode, "SCREENMODE")
        ck(buttonp, "BUTTONP")
        ck(buttonq, "BUTTON?")
        ck(button, "BUTTON")
        ck(matrix, "MATRIX")
        ck(sum, "SUM")
        ck(difference, "DIFFERENCE")
        ck(product, "PRODUCT")
        ck(quotient, "QUOTIENT")
        ck(remainder, "REMAINDER")
        ck(modulo, "MODULO")
        ck(kint, "INT")
        ck(exp, "EXP")
        ck(log10, "LOG10")
        ck(ln, "LN")
        ck(sin, "SIN")
        ck(radsin, "RADSIN")
        ck(cos, "COS")
        ck(radcos, "RADCOS")
        ck(arctan, "ARCTAN")
        ck(radarctan, "RADARCTAN")
        ck(round, "ROUND")
        ck(power, "POWER")
        ck(kbitand, "BITAND")
        ck(kbitor, "BITOR")
        ck(bitxor, "BITXOR")
        ck(bitnot, "BITNOT")
        ck(ashift, "ASHIFT")
        ck(lshift, "LSHIFT")
        ck(kand, "AND")
        ck(kor, "OR")
        ck(knot, "NOT")
        ck(form, "FORM")
        ck(lessp, "LESSP")
        ck(lessq, "LESS?")
        ck(greaterp, "GREATERP")
        ck(greaterq, "GREATER?")
        ck(lessequalp, "LESSEQUALP")
        ck(lessequalq, "LESSEQUAL?")
        ck(greaterequalp, "GREATEREQUALP")
        ck(greaterequalq, "GREATEREQUAL?")
        ck(define, "DEFINE")
        ck(text, "TEXT")
        ck(fulltext, "FULLTEXT")
        ck(copydef, "COPYDEF")
        ck(local, "LOCAL")
        ck(global, "GLOBAL")
        ck(pprop, "PPROP")
        ck(gprop, "GPROP")
        ck(remprop, "REMPROP")
        ck(plist, "PLIST")
        ck(procedurep, "PROCEDUREP")
        ck(procedureq, "PROCEDURE?")
        ck(primitivep, "PRIMITIVEP")
        ck(primitiveq, "PRIMITIVE?")
        ck(definedp, "DEFINEDP")
        ck(definedq, "DEFINED?")
        ck(namep, "NAMEP")
        ck(nameq, "NAME?")
        ck(plistp, "PLISTP")
        ck(plistq, "PLIST?")
        ck(contents, "CONTENTS")
        ck(buried, "BURIED")
        ck(traced, "TRACED")
        ck(stepped, "STEPPED")
        ck(procedures, "PROCEDURES")
        ck(primitives, "PRIMITIVES")
        ck(names, "NAMES")
        ck(plists, "PLISTS")
        ck(arity, "ARITY")
        ck(nodes, "NODES")
        ck(printout, "PRINTOUT")
        ck(po, "PO")
        ck(pot, "POT")
        ck(cerase, "ERASE")
        ck(er, "ER")
        ck(erall, "ERALL")
        ck(erps, "ERPS")
        ck(erns, "ERNS")
        ck(erpls, "ERPLS")
        ck(bury, "BURY")
        ck(unbury, "UNBURY")
        ck(buriedp, "BURIEDP")
        ck(buriedq, "BURIED?")
        ck(trace, "TRACE")
        ck(untrace, "UNTRACE")
        ck(tracedp, "TRACEDP")
        ck(tracedq, "TRACED?")
        ck(step, "STEP")
        ck(unstep, "UNSTEP")
        ck(steppedp, "STEPPEDP")
        ck(steppedq, "STEPPED?")
        ck(edit, "EDIT")
        ck(ed, "ED")
        ck(editfile, "EDITFILE")
        ck(save, "SAVE")
        ck(load, "LOAD")
        ck(help, "HELP")
        ck(run, "RUN")
        ck(time, "TIME")
        ck(runresult, "RUNRESULT")
        ck(kforever, "FOREVER")
        ck(repcount, "REPCOUNT")
        ck(kif, "IF")
        ck(ifelse, "IFELSE")
        ck(test, "TEST")
        ck(iftrue, "IFTRUE")
        ck(ift, "IFT")
        ck(iffalse, "IFFALSE")
        ck(iff, "IFF")
        ck(output, "OUTPUT")
        ck(op, "OP")
        ck(kcatch, "CATCH")
        ck(kthrow, "THROW")
        ck(kcontinue, "CONTINUE")
        ck(co, "CO")
        ck(bye, "BYE")
        ck(dmaybeoutput, ".MAYBEOUTPUT")
        ck(apply, "APPLY")
        ck(cto, "TO")
        ck(macrop, "MACROP")
        ck(macroq, "MACRO?")
        ck(gc, "GC")
        ck(dsetsegmentsize, ".SETSEGMENTSIZE")
        ck(setpenpattern, "SETPENPATTERN")
        ck(penpattern, "PENPATTERN")
        ck(refresh, "REFRESH")
        ck(norefresh, "NOREFRESH")



    // =====================  End copied block  ===================

};

extern StringConstants k;

#endif // STRINGCONSTANTS_H
