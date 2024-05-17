
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
#include "kernel.h"
#include "stringconstants.h"
#include "datum/word.h"
#include "datum/array.h"
#include "error.h"
#include "datum/astnode.h"
#include <QDateTime>

static DatumPool<Procedure> pool(20);

// return the method pointer if a GUI is available,
// else return a pointer to the excErrorNoGUI method
KernelMethod ifGUI(KernelMethod method) {
    extern bool hasGUI;
    if (hasGUI) {
        return method;
    }
    return &Kernel::excErrorNoGui;
}

Procedures *theMainProcedures = NULL;

Procedures* mainProcedures()
{
  Q_ASSERT(theMainProcedures != NULL);
  return theMainProcedures;
}


Procedures::Procedures() {
    Q_ASSERT(theMainProcedures == NULL);
    theMainProcedures = this;

    // DATA STRUCTURE PRIMITIVES (MIN, default, MAX)
    // (MIN = -1)     = All parameters are read as Words, e.g. "TO PROC :p1"
    // becomes ["TO", "PROC", ":p1"]
    // (default = -1) = All parameters are consumed
    // until end of line
    // (MAX = -1)     = All parameters are consumed within
    // parens
    stringToCmd[k.cword()] = {&Kernel::excWord, 0, 2, -1};
    stringToCmd[k.clist()] = {&Kernel::excList, 0, 2, -1};
    stringToCmd[k.sentence()] = {&Kernel::excSentence, 0, 2, -1};
    stringToCmd[k.se()] = {&Kernel::excSentence, 0, 2, -1};
    stringToCmd[k.fput()] = {&Kernel::excFput, 2, 2, 2};
    stringToCmd[k.lput()] = {&Kernel::excLput, 2, 2, 2};
    stringToCmd[k.carray()] = {&Kernel::excArray, 1, 1, 2};
    stringToCmd[k.listtoarray()] = {&Kernel::excListtoarray, 1, 1, 2};
    stringToCmd[k.arraytolist()] = {&Kernel::excArraytolist, 1, 1, 1};
    stringToCmd[k.readlist()] = {&Kernel::excReadlist, 0, 0, 0};
    stringToCmd[k.rl()] = {&Kernel::excReadlist, 0, 0, 0};
    stringToCmd[k.readword()] = {&Kernel::excReadword, 0, 0, 0};
    stringToCmd[k.rw()] = {&Kernel::excReadword, 0, 0, 0};
    stringToCmd[k.readrawline()] = {&Kernel::excReadrawline, 0, 0, 0};
    stringToCmd[k.readchar()] = {&Kernel::excReadchar, 0, 0, 0};
    stringToCmd[k.rc()] = {&Kernel::excReadchar, 0, 0, 0};
    stringToCmd[k.readchars()] = {&Kernel::excReadchars, 1, 1, 1};
    stringToCmd[k.rcs()] = {&Kernel::excReadchars, 1, 1, 1};
    stringToCmd[k.shell()] = {&Kernel::excShell, 1, 1, 2};

    stringToCmd[k.setprefix()] = {&Kernel::excSetprefix, 1, 1, 1};
    stringToCmd[k.prefix()] = {&Kernel::excPrefix, 0, 0, 0};
    stringToCmd[k.openread()] = {&Kernel::excOpenread, 1, 1, 1};
    stringToCmd[k.openwrite()] = {&Kernel::excOpenwrite, 1, 1, 1};
    stringToCmd[k.openappend()] = {&Kernel::excOpenappend, 1, 1, 1};
    stringToCmd[k.openupdate()] = {&Kernel::excOpenupdate, 1, 1, 1};
    stringToCmd[k.allopen()] = {&Kernel::excAllopen, 0, 0, 0};
    stringToCmd[k.setread()] = {&Kernel::excSetread, 1, 1, 1};
    stringToCmd[k.setwrite()] = {&Kernel::excSetwrite, 1, 1, 1};
    stringToCmd[k.reader()] = {&Kernel::excReader, 0, 0, 0};
    stringToCmd[k.writer()] = {&Kernel::excWriter, 0, 0, 0};
    stringToCmd[k.readpos()] = {&Kernel::excReadpos, 0, 0, 0};
    stringToCmd[k.writepos()] = {&Kernel::excWritepos, 0, 0, 0};
    stringToCmd[k.setreadpos()] = {&Kernel::excSetreadpos, 1, 1, 1};
    stringToCmd[k.setwritepos()] = {&Kernel::excSetwritepos, 1, 1, 1};
    stringToCmd[k.eofp()] = {&Kernel::excEofp, 0, 0, 0};
    stringToCmd[k.eofq()] = {&Kernel::excEofp, 0, 0, 0};
    stringToCmd[k.keyp()] = {&Kernel::excKeyp, 0, 0, 0};
    stringToCmd[k.keyq()] = {&Kernel::excKeyp, 0, 0, 0};
    stringToCmd[k.dribble()] = {&Kernel::excDribble, 1, 1, 1};
    stringToCmd[k.nodribble()] = {&Kernel::excNodribble, 0, 0, 0};

    stringToCmd[k.cleartext()] = {&Kernel::excCleartext, 0, 0, 0};
    stringToCmd[k.ct()] = {&Kernel::excCleartext, 0, 0, 0};
    stringToCmd[k.cursorinsert()] = {ifGUI(&Kernel::excCursorInsert), 0, 0, 0};
    stringToCmd[k.cursoroverwrite()] = {ifGUI(&Kernel::excCursorOverwrite), 0, 0, 0};
    stringToCmd[k.cursormode()] = {ifGUI(&Kernel::excCursorMode), 0, 0, 0};

    stringToCmd[k.close()] = {&Kernel::excClose, 1, 1, 1};
    stringToCmd[k.closeall()] = {&Kernel::excCloseall, 0, 0, 0};
    stringToCmd[k.erasefile()] = {&Kernel::excErasefile, 1, 1, 1};
    stringToCmd[k.erf()] = {&Kernel::excErasefile, 1, 1, 1};

    stringToCmd[k.first()] = {&Kernel::excFirst, 1, 1, 1};
    stringToCmd[k.last()] = {&Kernel::excLast, 1, 1, 1};
    stringToCmd[k.butfirst()] = {&Kernel::excButfirst, 1, 1, 1};
    stringToCmd[k.bf()] = {&Kernel::excButfirst, 1, 1, 1};
    stringToCmd[k.firsts()] = {&Kernel::excFirsts, 1, 1, 1};
    stringToCmd[k.butfirsts()] = {&Kernel::excButfirsts, 1, 1, 1};
    stringToCmd[k.bfs()] = {&Kernel::excButfirsts, 1, 1, 1};
    stringToCmd[k.butlast()] = {&Kernel::excButlast, 1, 1, 1};
    stringToCmd[k.bl()] = {&Kernel::excButlast, 1, 1, 1};
    stringToCmd[k.item()] = {&Kernel::excItem, 2, 2, 2};

    stringToCmd[k.setitem()] = {&Kernel::excSetitem, 3, 3, 3};
    stringToCmd[k.dsetfirst()] = {&Kernel::excDotSetfirst, 2, 2, 2};
    stringToCmd[k.dsetbf()] = {&Kernel::excDotSetbf, 2, 2, 2};
    stringToCmd[k.dsetitem()] = {&Kernel::excDotSetitem, 3, 3, 3};

    stringToCmd[k.wordp()] = {&Kernel::excWordp, 1, 1, 1};
    stringToCmd[k.wordq()] = {&Kernel::excWordp, 1, 1, 1};
    stringToCmd[k.listp()] = {&Kernel::excListp, 1, 1, 1};
    stringToCmd[k.listq()] = {&Kernel::excListp, 1, 1, 1};
    stringToCmd[k.arrayp()] = {&Kernel::excArrayp, 1, 1, 1};
    stringToCmd[k.arrayq()] = {&Kernel::excArrayp, 1, 1, 1};
    stringToCmd[k.emptyp()] = {&Kernel::excEmptyp, 1, 1, 1};
    stringToCmd[k.emptyq()] = {&Kernel::excEmptyp, 1, 1, 1};
    stringToCmd[k.equalp()] = {&Kernel::excEqualp, 2, 2, 2};
    stringToCmd[k.equalq()] = {&Kernel::excEqualp, 2, 2, 2};
    stringToCmd[k.notequalp()] = {&Kernel::excNotequal, 2, 2, 2};
    stringToCmd[k.notequalq()] = {&Kernel::excNotequal, 2, 2, 2};
    stringToCmd[k.beforep()] = {&Kernel::excBeforep, 2, 2, 2};
    stringToCmd[k.beforeq()] = {&Kernel::excBeforep, 2, 2, 2};
    stringToCmd[k.deq()] = {&Kernel::excDotEq, 2, 2, 2};
    stringToCmd[k.memberp()] = {&Kernel::excMemberp, 2, 2, 2};
    stringToCmd[k.memberq()] = {&Kernel::excMemberp, 2, 2, 2};
    stringToCmd[k.substringp()] = {&Kernel::excSubstringp, 2, 2, 2};
    stringToCmd[k.substringq()] = {&Kernel::excSubstringp, 2, 2, 2};
    stringToCmd[k.numberp()] = {&Kernel::excNumberp, 1, 1, 1};
    stringToCmd[k.numberq()] = {&Kernel::excNumberp, 1, 1, 1};
    stringToCmd[k.vbarredp()] = {&Kernel::excVbarredp, 1, 1, 1};
    stringToCmd[k.vbarredq()] = {&Kernel::excVbarredp, 1, 1, 1};

    stringToCmd[k.count()] = {&Kernel::excCount, 1, 1, 1};
    stringToCmd[k.ascii()] = {&Kernel::excAscii, 1, 1, 1};
    stringToCmd[k.rawascii()] = {&Kernel::excRawascii, 1, 1, 1};
    stringToCmd[k.kchar()] = {&Kernel::excChar, 1, 1, 1};
    stringToCmd[k.member()] = {&Kernel::excMember, 2, 2, 2};
    stringToCmd[k.lowercase()] = {&Kernel::excLowercase, 1, 1, 1};
    stringToCmd[k.uppercase()] = {&Kernel::excUppercase, 1, 1, 1};
    stringToCmd[k.standout()] = {ifGUI(&Kernel::excStandout), 1, 1, 1};
    stringToCmd[k.parse()] = {&Kernel::excParse, 1, 1, 1};
    stringToCmd[k.runparse()] = {&Kernel::excRunparse, 1, 1, 1};

    stringToCmd[k.minus()] = {&Kernel::excMinus, 1, 1, 1};
    stringToCmd["-"] = {&Kernel::excMinus, 1, 1, 1};
    stringToCmd["--"] = {&Kernel::excMinus, 1, 1, 1};

    stringToCmd[k.print()] = {&Kernel::excPrint, 0, 1, -1};
    stringToCmd[k.pr()] = {&Kernel::excPrint, 0, 1, -1};
    stringToCmd[k.type()] = {&Kernel::excType, 0, 1, -1};
    stringToCmd[k.show()] = {&Kernel::excShow, 0, 1, -1};
    stringToCmd[k.make()] = {&Kernel::excMake, 2, 2, 2};
    stringToCmd[k.repeat()] = {&Kernel::excRepeat, 2, 2, 2};
    stringToCmd[k.sqrt()] = {&Kernel::excSqrt, 1, 1, 1};
    stringToCmd[k.random()] = {&Kernel::excRandom, 1, 1, 2};
    stringToCmd[k.rerandom()] = {&Kernel::excRerandom, 0, 0, 1};
    stringToCmd[k.thing()] = {&Kernel::excThing, 1, 1, 1};
    stringToCmd[k.wait()] = {&Kernel::excWait, 1, 1, 1};
    stringToCmd[k.setcursor()] = {ifGUI(&Kernel::excSetcursor), 1, 1, 1};
    stringToCmd[k.cursor()] = {ifGUI(&Kernel::excCursor), 0, 0, 0};
    stringToCmd[k.settextcolor()] = {ifGUI(&Kernel::excSettextcolor), 1, 2, 2};
    stringToCmd[k.settc()] = {ifGUI(&Kernel::excSettextcolor), 1, 2, 2};
    stringToCmd[k.increasefont()] = {ifGUI(&Kernel::excIncreasefont), 0, 0, 0};
    stringToCmd[k.decreasefont()] = {ifGUI(&Kernel::excDecreasefont), 0, 0, 0};
    stringToCmd[k.settextsize()] = {ifGUI(&Kernel::excSettextsize), 1, 1, 1};
    stringToCmd[k.textsize()] = {ifGUI(&Kernel::excTextsize), 0, 0, 0};
    stringToCmd[k.setfont()] = {ifGUI(&Kernel::excSetfont), 1, 1, 1};
    stringToCmd[k.font()] = {ifGUI(&Kernel::excFont), 0, 0, 0};
    stringToCmd[k.allfonts()] = {ifGUI(&Kernel::excAllfonts), 0, 0, 0};

    stringToCmd[k.forward()] = {ifGUI(&Kernel::excForward), 1, 1, 1};
    stringToCmd[k.fd()] = {ifGUI(&Kernel::excForward), 1, 1, 1};
    stringToCmd[k.back()] = {ifGUI(&Kernel::excBack), 1, 1, 1};
    stringToCmd[k.bk()] = {ifGUI(&Kernel::excBack), 1, 1, 1};
    stringToCmd[k.right()] = {ifGUI(&Kernel::excRight), 1, 1, 1};
    stringToCmd[k.rt()] = {ifGUI(&Kernel::excRight), 1, 1, 1};
    stringToCmd[k.left()] = {ifGUI(&Kernel::excLeft), 1, 1, 1};
    stringToCmd[k.lt()] = {ifGUI(&Kernel::excLeft), 1, 1, 1};
    stringToCmd[k.clearscreen()] = {ifGUI(&Kernel::excClearscreen), 0, 0, 0};
    stringToCmd[k.cs()] = {ifGUI(&Kernel::excClearscreen), 0, 0, 0};
    stringToCmd[k.clean()] = {ifGUI(&Kernel::excClean), 0, 0, 0};
    stringToCmd[k.penup()] = {ifGUI(&Kernel::excPenup), 0, 0, 0};
    stringToCmd[k.pu()] = {ifGUI(&Kernel::excPenup), 0, 0, 0};
    stringToCmd[k.pendown()] = {ifGUI(&Kernel::excPendown), 0, 0, 0};
    stringToCmd[k.pd()] = {ifGUI(&Kernel::excPendown), 0, 0, 0};
    stringToCmd[k.pendownp()] = {ifGUI(&Kernel::excPendownp), 0, 0, 0};
    stringToCmd[k.pendownq()] = {ifGUI(&Kernel::excPendownp), 0, 0, 0};
    stringToCmd[k.hideturtle()] = {ifGUI(&Kernel::excHideturtle), 0, 0, 0};
    stringToCmd[k.ht()] = {ifGUI(&Kernel::excHideturtle), 0, 0, 0};
    stringToCmd[k.showturtle()] = {ifGUI(&Kernel::excShowturtle), 0, 0, 0};
    stringToCmd[k.st()] = {ifGUI(&Kernel::excShowturtle), 0, 0, 0};
    stringToCmd[k.setxy()] = {ifGUI(&Kernel::excSetXY), 2, 2, 2};
    stringToCmd[k.setx()] = {ifGUI(&Kernel::excSetX), 1, 1, 1};
    stringToCmd[k.sety()] = {ifGUI(&Kernel::excSetY), 1, 1, 1};
    stringToCmd[k.setpos()] = {ifGUI(&Kernel::excSetpos), 1, 1, 1};
    stringToCmd[k.pos()] = {ifGUI(&Kernel::excPos), 0, 0, 1};
    stringToCmd[k.home()] = {ifGUI(&Kernel::excHome), 0, 0, 0};
    stringToCmd[k.heading()] = {ifGUI(&Kernel::excHeading), 0, 0, 1};
    stringToCmd[k.setheading()] = {ifGUI(&Kernel::excSetheading), 1, 1, 1};
    stringToCmd[k.seth()] = {ifGUI(&Kernel::excSetheading), 1, 1, 1};
    stringToCmd[k.arc()] = {ifGUI(&Kernel::excArc), 2, 2, 2};
    stringToCmd[k.towards()] = {ifGUI(&Kernel::excTowards), 1, 1, 1};
    stringToCmd[k.scrunch()] = {ifGUI(&Kernel::excScrunch), 0, 0, 0};
    stringToCmd[k.setscrunch()] = {ifGUI(&Kernel::excSetscrunch), 2, 2, 2};
    stringToCmd[k.label()] = {ifGUI(&Kernel::excLabel), 1, 1, 1};
    stringToCmd[k.labelheight()] = {ifGUI(&Kernel::excLabelheight), 0, 0, 0};
    stringToCmd[k.setlabelheight()] = {ifGUI(&Kernel::excSetlabelheight), 1, 1, 1};
    stringToCmd[k.shownp()] = {ifGUI(&Kernel::excShownp), 0, 0, 0};
    stringToCmd[k.shownq()] = {ifGUI(&Kernel::excShownp), 0, 0, 0};
    stringToCmd[k.setpencolor()] = {ifGUI(&Kernel::excSetpencolor), 1, 1, 1};
    stringToCmd[k.setpc()] = {ifGUI(&Kernel::excSetpencolor), 1, 1, 1};
    stringToCmd[k.pencolor()] = {ifGUI(&Kernel::excPencolor), 0, 0, 0};
    stringToCmd[k.pc()] = {ifGUI(&Kernel::excPencolor), 0, 0, 0};
    stringToCmd[k.setpalette()] = {ifGUI(&Kernel::excSetpalette), 2, 2, 2};
    stringToCmd[k.palette()] = {ifGUI(&Kernel::excPalette), 1, 1, 1};
    stringToCmd[k.background()] = {ifGUI(&Kernel::excBackground), 0, 0, 0};
    stringToCmd[k.bg()] = {ifGUI(&Kernel::excBackground), 0, 0, 0};
    stringToCmd[k.setbackground()] = {ifGUI(&Kernel::excSetbackground), 1, 1, 1};
    stringToCmd[k.setbg()] = {ifGUI(&Kernel::excSetbackground), 1, 1, 1};
    stringToCmd[k.savepict()] = {ifGUI(&Kernel::excSavepict), 1, 1, 1};
    stringToCmd[k.loadpict()] = {ifGUI(&Kernel::excLoadpict), 1, 1, 1};
    stringToCmd[k.svgpict()] = {ifGUI(&Kernel::excSvgpict), 1, 1, 1};

    stringToCmd[k.penpaint()] = {ifGUI(&Kernel::excPenpaint), 0, 0, 0};
    stringToCmd[k.ppt()] = {ifGUI(&Kernel::excPenpaint), 0, 0, 0};
    stringToCmd[k.penerase()] = {ifGUI(&Kernel::excPenerase), 0, 0, 0};
    stringToCmd[k.pe()] = {ifGUI(&Kernel::excPenerase), 0, 0, 0};
    stringToCmd[k.penreverse()] = {ifGUI(&Kernel::excPenreverse), 0, 0, 0};
    stringToCmd[k.px()] = {ifGUI(&Kernel::excPenreverse), 0, 0, 0};
    stringToCmd[k.penmode()] = {ifGUI(&Kernel::excPenmode), 0, 0, 0};
    stringToCmd[k.setpensize()] = {ifGUI(&Kernel::excSetpensize), 1, 1, 1};
    stringToCmd[k.pensize()] = {ifGUI(&Kernel::excPensize), 0, 0, 0};
    stringToCmd[k.filled()] = {ifGUI(&Kernel::excFilled), 2, 2, 2};

    stringToCmd[k.cwrap()] = {ifGUI(&Kernel::excWrap), 0, 0, 0};
    stringToCmd[k.cfence()] = {ifGUI(&Kernel::excFence), 0, 0, 0};
    stringToCmd[k.cwindow()] = {ifGUI(&Kernel::excWindow), 0, 0, 0};
    stringToCmd[k.turtlemode()] = {ifGUI(&Kernel::excTurtlemode), 0, 0, 0};

    stringToCmd[k.mousepos()] = {ifGUI(&Kernel::excMousepos), 0, 0, 0};
    stringToCmd[k.clickpos()] = {ifGUI(&Kernel::excClickpos), 0, 0, 0};
    stringToCmd[k.bounds()] = {ifGUI(&Kernel::excBounds), 0, 0, 0};
    stringToCmd[k.setbounds()] = {ifGUI(&Kernel::excSetbounds), 2, 2, 2};

    stringToCmd[k.ctextscreen()] = {ifGUI(&Kernel::excTextscreen), 0, 0, 0};
    stringToCmd[k.ts()] = {ifGUI(&Kernel::excTextscreen), 0, 0, 0};
    stringToCmd[k.cfullscreen()] = {ifGUI(&Kernel::excFullscreen), 0, 0, 0};
    stringToCmd[k.fs()] = {ifGUI(&Kernel::excFullscreen), 0, 0, 0};
    stringToCmd[k.csplitscreen()] = {ifGUI(&Kernel::excSplitscreen), 0, 0, 0};
    stringToCmd[k.ss()] = {ifGUI(&Kernel::excSplitscreen), 0, 0, 0};
    stringToCmd[k.screenmode()] = {ifGUI(&Kernel::excScreenmode), 0, 0, 0};

    stringToCmd[k.buttonp()] = {ifGUI(&Kernel::excButtonp), 0, 0, 0};
    stringToCmd[k.buttonq()] = {ifGUI(&Kernel::excButtonp), 0, 0, 0};
    stringToCmd[k.button()] = {ifGUI(&Kernel::excButton), 0, 0, 0};

    stringToCmd[k.matrix()] = {ifGUI(&Kernel::excMatrix), 0, 0, 0}; // for debugging

    stringToCmd[k.sum()] = {&Kernel::excSum, 0, 2, -1};
    stringToCmd[k.difference()] = {&Kernel::excDifference, 2, 2, 2};
    stringToCmd[k.product()] = {&Kernel::excProduct, 0, 2, -1};
    stringToCmd[k.quotient()] = {&Kernel::excQuotient, 1, 2, 2};
    stringToCmd[k.remainder()] = {&Kernel::excRemainder, 2, 2, 2};
    stringToCmd[k.modulo()] = {&Kernel::excModulo, 2, 2, 2};
    stringToCmd[k.kint()] = {&Kernel::excInt, 1, 1, 1};
    stringToCmd[k.exp()] = {&Kernel::excExp, 1, 1, 1};
    stringToCmd[k.log10()] = {&Kernel::excLog10, 1, 1, 1};
    stringToCmd[k.ln()] = {&Kernel::excLn, 1, 1, 1};
    stringToCmd[k.sin()] = {&Kernel::excSin, 1, 1, 1};
    stringToCmd[k.radsin()] = {&Kernel::excRadsin, 1, 1, 1};
    stringToCmd[k.cos()] = {&Kernel::excCos, 1, 1, 1};
    stringToCmd[k.radcos()] = {&Kernel::excRadcos, 1, 1, 1};
    stringToCmd[k.arctan()] = {&Kernel::excArctan, 1, 1, 2};
    stringToCmd[k.radarctan()] = {&Kernel::excRadarctan, 1, 1, 2};
    stringToCmd[k.round()] = {&Kernel::excRound, 1, 1, 1};
    stringToCmd[k.power()] = {&Kernel::excPower, 2, 2, 2};
    stringToCmd[k.kbitand()] = {&Kernel::excBitand, 0, 2, -1};
    stringToCmd[k.kbitor()] = {&Kernel::excBitor, 0, 2, -1};
    stringToCmd[k.bitxor()] = {&Kernel::excBitxor, 0, 2, -1};
    stringToCmd[k.bitnot()] = {&Kernel::excBitnot, 1, 1, 1};
    stringToCmd[k.ashift()] = {&Kernel::excAshift, 2, 2, 2};
    stringToCmd[k.lshift()] = {&Kernel::excLshift, 2, 2, 2};
    stringToCmd[k.kand()] = {&Kernel::excAnd, 0, 2, -1};
    stringToCmd[k.kor()] = {&Kernel::excOr, 0, 2, -1};
    stringToCmd[k.knot()] = {&Kernel::excNot, 1, 1, 1};

    stringToCmd[k.form()] = {&Kernel::excForm, 3, 3, 3};

    stringToCmd[k.lessp()] = {&Kernel::excLessp, 2, 2, 2};
    stringToCmd[k.lessq()] = {&Kernel::excLessp, 2, 2, 2};
    stringToCmd[k.greaterp()] = {&Kernel::excGreaterp, 2, 2, 2};
    stringToCmd[k.greaterq()] = {&Kernel::excGreaterp, 2, 2, 2};
    stringToCmd[k.lessequalp()] = {&Kernel::excLessequalp, 2, 2, 2};
    stringToCmd[k.lessequalq()] = {&Kernel::excLessequalp, 2, 2, 2};
    stringToCmd[k.greaterequalp()] = {&Kernel::excGreaterequalp, 2, 2, 2};
    stringToCmd[k.greaterequalq()] = {&Kernel::excGreaterequalp, 2, 2, 2};

    stringToCmd[k.define()] = {&Kernel::excDefine, 2, 2, 2};
    stringToCmd[k.text()] = {&Kernel::excText, 1, 1, 1};
    stringToCmd[k.fulltext()] = {&Kernel::excFulltext, 1, 1, 1};
    stringToCmd[k.copydef()] = {&Kernel::excCopydef, 2, 2, 2};
    stringToCmd[k.local()] = {&Kernel::excLocal, 1, 1, -1};
    stringToCmd[k.global()] = {&Kernel::excGlobal, 1, 1, -1};

    stringToCmd[k.pprop()] = {&Kernel::excPprop, 3, 3, 3};
    stringToCmd[k.gprop()] = {&Kernel::excGprop, 2, 2, 2};
    stringToCmd[k.remprop()] = {&Kernel::excRemprop, 2, 2, 2};
    stringToCmd[k.plist()] = {&Kernel::excPlist, 1, 1, 1};

    stringToCmd[k.procedurep()] = {&Kernel::excProcedurep, 1, 1, 1};
    stringToCmd[k.procedureq()] = {&Kernel::excProcedurep, 1, 1, 1};
    stringToCmd[k.primitivep()] = {&Kernel::excPrimitivep, 1, 1, 1};
    stringToCmd[k.primitiveq()] = {&Kernel::excPrimitivep, 1, 1, 1};
    stringToCmd[k.definedp()] = {&Kernel::excDefinedp, 1, 1, 1};
    stringToCmd[k.definedq()] = {&Kernel::excDefinedp, 1, 1, 1};
    stringToCmd[k.namep()] = {&Kernel::excNamep, 1, 1, 1};
    stringToCmd[k.nameq()] = {&Kernel::excNamep, 1, 1, 1};
    stringToCmd[k.plistp()] = {&Kernel::excPlistp, 1, 1, 1};
    stringToCmd[k.plistq()] = {&Kernel::excPlistp, 1, 1, 1};

    stringToCmd[k.contents()] = {&Kernel::excContents, 0, 0, 0};
    stringToCmd[k.buried()] = {&Kernel::excBuried, 0, 0, 0};
    stringToCmd[k.traced()] = {&Kernel::excTraced, 0, 0, 0};
    stringToCmd[k.stepped()] = {&Kernel::excStepped, 0, 0, 0};
    stringToCmd[k.procedures()] = {&Kernel::excProcedures, 0, 0, 0};
    stringToCmd[k.primitives()] = {&Kernel::excPrimitives, 0, 0, 0};
    stringToCmd[k.names()] = {&Kernel::excNames, 0, 0, 0};
    stringToCmd[k.plists()] = {&Kernel::excPlists, 0, 0, 0};
    stringToCmd[k.arity()] = {&Kernel::excArity, 1, 1, 1};
    stringToCmd[k.nodes()] = {&Kernel::excNodes, 0, 0, 0};

    stringToCmd[k.printout()] = {&Kernel::excPrintout, 1, 1, 1};
    stringToCmd[k.po()] = {&Kernel::excPrintout, 1, 1, 1};
    stringToCmd[k.pot()] = {&Kernel::excPot, 1, 1, 1};

    stringToCmd[k.cerase()] = {&Kernel::excErase, 1, 1, 1};
    stringToCmd[k.er()] = {&Kernel::excErase, 1, 1, 1};
    stringToCmd[k.erall()] = {&Kernel::excErall, 0, 0, 0};
    stringToCmd[k.erps()] = {&Kernel::excErps, 0, 0, 0};
    stringToCmd[k.erns()] = {&Kernel::excErns, 0, 0, 0};
    stringToCmd[k.erpls()] = {&Kernel::excErpls, 0, 0, 0};
    stringToCmd[k.bury()] = {&Kernel::excBury, 1, 1, 1};
    stringToCmd[k.unbury()] = {&Kernel::excUnbury, 1, 1, 1};
    stringToCmd[k.buriedp()] = {&Kernel::excBuriedp, 1, 1, 1};
    stringToCmd[k.buriedq()] = {&Kernel::excBuriedp, 1, 1, 1};
    stringToCmd[k.trace()] = {&Kernel::excTrace, 1, 1, 1};
    stringToCmd[k.untrace()] = {&Kernel::excUntrace, 1, 1, 1};
    stringToCmd[k.tracedp()] = {&Kernel::excTracedp, 1, 1, 1};
    stringToCmd[k.tracedq()] = {&Kernel::excTracedp, 1, 1, 1};
    stringToCmd[k.step()] = {&Kernel::excStep, 1, 1, 1};
    stringToCmd[k.unstep()] = {&Kernel::excUnstep, 1, 1, 1};
    stringToCmd[k.steppedp()] = {&Kernel::excSteppedp, 1, 1, 1};
    stringToCmd[k.steppedq()] = {&Kernel::excSteppedp, 1, 1, 1};
    stringToCmd[k.edit()] = {&Kernel::excEdit, 0, -1, 1};
    stringToCmd[k.ed()] = {&Kernel::excEdit, 0, -1, 1};
    stringToCmd[k.editfile()] = {&Kernel::excEditfile, 1, 1, 1};
    stringToCmd[k.save()] = {&Kernel::excSave, 0, -1, 1};
    stringToCmd[k.load()] = {&Kernel::excLoad, 1, 1, 1};
    stringToCmd[k.help()] = {&Kernel::excHelp, 0, -1, -1};

    // CONTROL STRUCTURES

    stringToCmd[k.run()] = {&Kernel::excRun, 1, 1, 1};
    stringToCmd[k.time()] = {&Kernel::excTime, 1, 1, 1};
    stringToCmd[k.runresult()] = {&Kernel::excRunresult, 1, 1, 1};
    stringToCmd[k.kforever()] = {&Kernel::excForever, 1, 1, 1};
    stringToCmd[k.repcount()] = {&Kernel::excRepcount, 0, 0, 0};
    stringToCmd[k.kif()] = {&Kernel::excIf, 2, 2, 2};
    stringToCmd[k.ifelse()] = {&Kernel::excIfelse, 3, 3, 3};
    stringToCmd[k.test()] = {&Kernel::excTest, 1, 1, 1};
    stringToCmd[k.iftrue()] = {&Kernel::excIftrue, 1, 1, 1};
    stringToCmd[k.ift()] = {&Kernel::excIftrue, 1, 1, 1};
    stringToCmd[k.iffalse()] = {&Kernel::excIffalse, 1, 1, 1};
    stringToCmd[k.iff()] = {&Kernel::excIffalse, 1, 1, 1};
    stringToCmd[k.stop()] = {&Kernel::excStop, 0, 0, 1};
    stringToCmd[k.output()] = {&Kernel::excOutput, 1, 1, 1};
    stringToCmd[k.op()] = {&Kernel::excOutput, 1, 1, 1};
    stringToCmd[k.kcatch()] = {&Kernel::excCatch, 2, 2, 2};
    stringToCmd[k.kthrow()] = {&Kernel::excThrow, 1, 1, 2};
    stringToCmd[k.error()] = {&Kernel::excError, 0, 0, 0};
    stringToCmd[k.pause()] = {&Kernel::excPause, 0, 0, 0};
    stringToCmd[k.kcontinue()] = {&Kernel::excContinue, 0, -1, 1};
    stringToCmd[k.co()] = {&Kernel::excContinue, 0, -1, 1};
    stringToCmd[k.bye()] = {&Kernel::excBye, 0, 0, 0};
    stringToCmd[k.dmaybeoutput()] = {&Kernel::excDotMaybeoutput, 1, 1, 1};
    stringToCmd[k.tag()] = {&Kernel::excTag, 1, 1, 1};
    stringToCmd[k.kgoto()] = {&Kernel::excGoto, 1, 1, 1};

    stringToCmd[k.apply()] = {&Kernel::excApply, 2, 2, 2};
    stringToCmd["?"] = {&Kernel::excNamedSlot, 0, 0, 1};

    stringToCmd[k.cto()] = {&Kernel::excTo, -1, -1, -1};
    stringToCmd[k.dcMacro()] = {&Kernel::excTo, -1, -1, -1};
    stringToCmd[k.dDefmacro()] = {&Kernel::excDefine, 2, 2, 2};
    stringToCmd[k.macrop()] = {&Kernel::excMacrop, 1, 1, 1};
    stringToCmd[k.macroq()] = {&Kernel::excMacrop, 1, 1, 1};

    stringToCmd[k.gc()] = {&Kernel::excNoop, 0, 0, -1};
    stringToCmd[k.dsetsegmentsize()] = {&Kernel::excNoop, 1, 1, 1};
    stringToCmd[k.setpenpattern()] = {&Kernel::excNoop, 1, 1, 1};
    stringToCmd[k.penpattern()] = {&Kernel::excNoop, 1, 1, 1};
    stringToCmd[k.refresh()] = {&Kernel::excNoop, 0, 0, 0};
    stringToCmd[k.norefresh()] = {&Kernel::excNoop, 0, 0, 0};

    stringToCmd["+"] = {&Kernel::excSum, 0, 2, -1};
    stringToCmd["*"] = {&Kernel::excProduct, 0, 2, -1};
    stringToCmd["/"] = {&Kernel::excQuotient, 1, 2, 2};
    stringToCmd[">"] = {&Kernel::excGreaterp, 2, 2, 2};
    stringToCmd["<"] = {&Kernel::excLessp, 2, 2, 2};
    stringToCmd["="] = {&Kernel::excEqualp, 2, 2, 2};
    stringToCmd[">="] = {&Kernel::excGreaterequalp, 2, 2, 2};
    stringToCmd["<="] = {&Kernel::excLessequalp, 2, 2, 2};
    stringToCmd["<>"] = {&Kernel::excNotequal, 2, 2, 2};

}


void Procedure::addToPool()
{
    instructionList = nothing;
    requiredInputs.clear();
    optionalInputs.clear();
    optionalDefaults.clear();
    tagToLine.clear();
    sourceText = nothing;
    pool.dealloc(this);

}

void Procedures::defineProcedure(DatumPtr cmd, DatumPtr procnameP, DatumPtr text,
                             DatumPtr sourceText) {
    procnameP.wordValue()->numberValue();
    if (procnameP.wordValue()->didNumberConversionSucceed())
        Error::doesntLike(cmd, procnameP);

    QString procname = procnameP.wordValue()->keyValue();

    QChar firstChar = (procname)[0];
    if ((firstChar == '"') || (firstChar == ':'))
        Error::doesntLike(cmd, procnameP);

    if (stringToCmd.contains(procname))
        Error::isPrimative(procnameP);

    DatumPtr procBody = createProcedure(cmd, text, sourceText);

    procedures[procname] = procBody;

    if (mainKernel()->isInputRedirected() && mainKernel()->varUNBURYONEDIT()) {
        unbury(procname);
    }
}

DatumPtr Procedures::createProcedure(DatumPtr cmd, DatumPtr text, DatumPtr sourceText) {
    Procedure *body = (Procedure *) pool.alloc();
    body->init();
    DatumPtr bodyP(body);

    lastProcedureCreatedTimestamp = QDateTime::currentMSecsSinceEpoch();

    QString cmdString = cmd.wordValue()->keyValue();
    bool isMacro = ((cmdString == k.dcMacro()) || (cmdString == k.dDefmacro()));

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

    ListIterator paramIter = text.listValue()->first().listValue()->newIterator();

    while (paramIter.elementExists()) {
        DatumPtr currentParam = paramIter.element();

        if (currentParam.isWord()) { // default 5 OR required :FOO
            double paramAsNumber = currentParam.wordValue()->numberValue();
            if (currentParam.wordValue()->didNumberConversionSucceed()) { // default 5
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

            if (paramList->size() == 0)
                Error::doesntLike(cmd, currentParam);

            if (paramList->size() == 1) { // rest input [:GARPLY]
                if (isRestDefined)
                    Error::doesntLike(cmd, currentParam);
                DatumPtr param = paramList->first();
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
                DatumPtr param = paramList->first();
                DatumPtr defaultValue = paramList->butfirst();
                if (param.isWord()) {
                    QString name = param.wordValue()->keyValue();
                    if (name.startsWith(':') || name.startsWith('"'))
                        name.remove(0, 1);
                    if (name.size() < 1)
                        Error::doesntLike(cmd, param);
                    body->optionalInputs.append(name);
                    body->optionalDefaults.append(defaultValue);
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

    body->instructionList = text.listValue()->butfirst();

    ListIterator lineIter = body->instructionList.listValue()->newIterator();
    while (lineIter.elementExists()) {
        DatumPtr lineP = lineIter.element();
        ListIterator wordIter = lineP.listValue()->newIterator();
        while (wordIter.elementExists()) {
            DatumPtr d = wordIter.element();
            if (d.isWord() && (d.wordValue()->keyValue() == k.tag()) &&
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
    if (procedures.contains(oldname)) {
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
    if (!procedures.contains(procname))
        Error::noHow(procnameP);
    Procedure *body = procedures[procname].procedureValue();

    List *retval = List::alloc();

    List *inputs = List::alloc();

    for (auto &i : body->requiredInputs) {
        inputs->append(DatumPtr(i));
    }

    QList<DatumPtr>::iterator d = body->optionalDefaults.begin();
    for (auto &i : body->optionalInputs) {
        List *optInput = List::alloc(d->listValue());
        optInput->prepend(DatumPtr(i));
        ++d;
        inputs->append(DatumPtr(optInput));
    }

    if (body->restInput != "") {
        List *restInput = List::alloc();
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

    if (procedures.contains(procname)) {
        Procedure *body = procedures[procname].procedureValue();

        if (body->sourceText == nothing) {
            List *retval = List::alloc();
            retval->append(DatumPtr(procedureTitle(procnameP)));

            ListIterator b = body->instructionList.listValue()->newIterator();

            while (b.elementExists()) {
                retval->append(DatumPtr(unreadList(b.element().listValue(), false)));
            }

            DatumPtr end(k.end());
            retval->append(end);
            return DatumPtr(retval);
        } else {
            return body->sourceText;
        }
    } else if (shouldValidate) {
        Error::noHow(procnameP);
    }
    List *retval = List::alloc();
    retval->append(
        DatumPtr(k.to_() + procnameP.wordValue()->printValue()));
    retval->append(DatumPtr(k.end()));
    return DatumPtr(retval);
}

QString Procedures::procedureTitle(DatumPtr procnameP) {
    QString procname = procnameP.wordValue()->keyValue();

    if (stringToCmd.contains(procname))
        Error::isPrimative(procnameP);
    if (!procedures.contains(procname))
        Error::noHow(procnameP);

    Procedure *body = procedures[procname].procedureValue();

    DatumPtr firstlineP = DatumPtr(List::alloc());

    List *firstLine = firstlineP.listValue();

    if (body->isMacro)
        firstLine->append(DatumPtr(k.dMacro()));
    else
        firstLine->append(DatumPtr(k.to()));
    firstLine->append(procnameP);

    QString paramName;

    for (auto &i : body->requiredInputs) {
        paramName = i;
        paramName.prepend(':');
        firstLine->append(DatumPtr(paramName));
    }

    QList<DatumPtr>::iterator d = body->optionalDefaults.begin();
    for (auto &i : body->optionalInputs) {
        paramName = i;
        paramName.push_front(':');
        List *optInput = List::alloc(d->listValue());
        optInput->prepend(DatumPtr(paramName));
        firstLine->append(DatumPtr(optInput));
        ++d;
    }

    paramName = body->restInput;
    if (paramName != "") {
        paramName.push_front(':');
        List *restInput = List::alloc();
        restInput->append(DatumPtr(paramName));
        firstLine->append(DatumPtr(restInput));
    }

    if (body->countOfDefaultParams != body->requiredInputs.size()) {
        firstLine->append(DatumPtr(body->countOfDefaultParams));
    }

    QString retval = unreadList(firstLine, false);
    return retval;
}

DatumPtr Procedures::astnodeFromCommand(DatumPtr cmdP, int &minParams,
                                    int &defaultParams, int &maxParams) {
    QString cmdString = cmdP.wordValue()->keyValue();

    Cmd_t command;
    DatumPtr node = DatumPtr(ASTNode::alloc(cmdP));
    if (procedures.contains(cmdString)) {
        DatumPtr procBody = procedures[cmdString];
        if (procBody.procedureValue()->isMacro)
            node.astnodeValue()->kernel = &Kernel::executeMacro;
        else
            node.astnodeValue()->kernel = &Kernel::executeProcedure;
        node.astnodeValue()->addChild(procBody);
        defaultParams = procBody.procedureValue()->countOfDefaultParams;
        minParams = procBody.procedureValue()->countOfMinParams;
        maxParams = procBody.procedureValue()->countOfMaxParams;
    } else if (stringToCmd.contains(cmdString)) {
        command = stringToCmd[cmdString];
        defaultParams = command.countOfDefaultParams;
        minParams = command.countOfMinParams;
        maxParams = command.countOfMaxParams;
        node.astnodeValue()->kernel = command.method;
    } else if (cmdString.startsWith(k.set()) && (cmdString.size() > 3) &&
               mainKernel()->varALLOWGETSET()) {
        node.astnodeValue()->kernel = &Kernel::excSetfoo;
        defaultParams = 1;
        minParams = 1;
        maxParams = 1;
    } else if (mainKernel()->varALLOWGETSET()) {
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

    int countOfChildren = params.listValue()->size();
    if (countOfChildren < minParams)
        Error::notEnough(cmd);
    if ((countOfChildren > maxParams) && (maxParams != -1))
        Error::tooMany(cmd);

    ListIterator iter = params.listValue()->newIterator();
    while (iter.elementExists()) {
        DatumPtr p = iter.element();
        DatumPtr a = DatumPtr(ASTNode::alloc(k.literal()));
        a.astnodeValue()->kernel = &Kernel::executeLiteral;
        a.astnodeValue()->addChild(p);
        node.astnodeValue()->addChild(a);
    }
    return node;
}

bool Procedures::isProcedure(QString procname) {
    return (stringToCmd.contains(procname) || procedures.contains(procname));
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
    List *retval = List::alloc();

    for (auto &name : procedures.keys()) {

        if (shouldInclude(showWhat, name))
            retval->append(DatumPtr(name));
    }
    return DatumPtr(retval);
}

void Procedures::eraseAllProcedures() {
    for (auto &iter : procedures.keys()) {
        if (!isBuried(iter)) {
            procedures.remove(iter);
        }
    }
}

DatumPtr Procedures::allPrimitiveProcedureNames() {
    List *retval = List::alloc();

    for (auto name : stringToCmd.keys()) {
        retval->append(DatumPtr(name));
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

    List *retval = List::alloc();
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
    ArrayIterator i = anArray->newIterator();
    while (i.elementExists()) {
        DatumPtr e = i.element();
        if (retval != "{")
            retval.append(' ');
        retval.append(unreadDatum(e, true));
    }
    retval.append('}');
    return retval;
}

QString Procedures::unreadWord(Word *aWord, bool isInList) {
    aWord->numberValue();
    if (aWord->didNumberConversionSucceed())
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

