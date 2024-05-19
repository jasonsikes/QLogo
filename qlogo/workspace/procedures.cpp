
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
#include "datum/iterator.h"
#include "kernel.h"
#include "datum/word.h"
#include "datum/array.h"
#include "error.h"
#include "datum/astnode.h"
#include <QDateTime>
#include "QApplication"

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
    stringToCmd[QObject::tr("WORD")] = {&Kernel::excWord, 0, 2, -1};
    stringToCmd[QObject::tr("LIST")] = {&Kernel::excList, 0, 2, -1};
    stringToCmd[QObject::tr("SENTENCE")] = {&Kernel::excSentence, 0, 2, -1};
    stringToCmd[QObject::tr("SE")] = {&Kernel::excSentence, 0, 2, -1};
    stringToCmd[QObject::tr("FPUT")] = {&Kernel::excFput, 2, 2, 2};
    stringToCmd[QObject::tr("LPUT")] = {&Kernel::excLput, 2, 2, 2};
    stringToCmd[QObject::tr("ARRAY")] = {&Kernel::excArray, 1, 1, 2};
    stringToCmd[QObject::tr("LISTTOARRAY")] = {&Kernel::excListtoarray, 1, 1, 2};
    stringToCmd[QObject::tr("ARRAYTOLIST")] = {&Kernel::excArraytolist, 1, 1, 1};
    stringToCmd[QObject::tr("READLIST")] = {&Kernel::excReadlist, 0, 0, 0};
    stringToCmd[QObject::tr("RL")] = {&Kernel::excReadlist, 0, 0, 0};
    stringToCmd[QObject::tr("READWORD")] = {&Kernel::excReadword, 0, 0, 0};
    stringToCmd[QObject::tr("RW")] = {&Kernel::excReadword, 0, 0, 0};
    stringToCmd[QObject::tr("READRAWLINE")] = {&Kernel::excReadrawline, 0, 0, 0};
    stringToCmd[QObject::tr("READCHAR")] = {&Kernel::excReadchar, 0, 0, 0};
    stringToCmd[QObject::tr("RC")] = {&Kernel::excReadchar, 0, 0, 0};
    stringToCmd[QObject::tr("READCHARS")] = {&Kernel::excReadchars, 1, 1, 1};
    stringToCmd[QObject::tr("RCS")] = {&Kernel::excReadchars, 1, 1, 1};
    stringToCmd[QObject::tr("SHELL")] = {&Kernel::excShell, 1, 1, 2};

    stringToCmd[QObject::tr("SETPREFIX")] = {&Kernel::excSetprefix, 1, 1, 1};
    stringToCmd[QObject::tr("PREFIX")] = {&Kernel::excPrefix, 0, 0, 0};
    stringToCmd[QObject::tr("OPENREAD")] = {&Kernel::excOpenread, 1, 1, 1};
    stringToCmd[QObject::tr("OPENWRITE")] = {&Kernel::excOpenwrite, 1, 1, 1};
    stringToCmd[QObject::tr("OPENAPPEND")] = {&Kernel::excOpenappend, 1, 1, 1};
    stringToCmd[QObject::tr("OPENUPDATE")] = {&Kernel::excOpenupdate, 1, 1, 1};
    stringToCmd[QObject::tr("ALLOPEN")] = {&Kernel::excAllopen, 0, 0, 0};
    stringToCmd[QObject::tr("SETREAD")] = {&Kernel::excSetread, 1, 1, 1};
    stringToCmd[QObject::tr("SETWRITE")] = {&Kernel::excSetwrite, 1, 1, 1};
    stringToCmd[QObject::tr("READER")] = {&Kernel::excReader, 0, 0, 0};
    stringToCmd[QObject::tr("WRITER")] = {&Kernel::excWriter, 0, 0, 0};
    stringToCmd[QObject::tr("READPOS")] = {&Kernel::excReadpos, 0, 0, 0};
    stringToCmd[QObject::tr("WRITEPOS")] = {&Kernel::excWritepos, 0, 0, 0};
    stringToCmd[QObject::tr("SETREADPOS")] = {&Kernel::excSetreadpos, 1, 1, 1};
    stringToCmd[QObject::tr("SETWRITEPOS")] = {&Kernel::excSetwritepos, 1, 1, 1};
    stringToCmd[QObject::tr("EOFP")] = {&Kernel::excEofp, 0, 0, 0};
    stringToCmd[QObject::tr("EOF?")] = {&Kernel::excEofp, 0, 0, 0};
    stringToCmd[QObject::tr("KEYP")] = {&Kernel::excKeyp, 0, 0, 0};
    stringToCmd[QObject::tr("KEY?")] = {&Kernel::excKeyp, 0, 0, 0};
    stringToCmd[QObject::tr("DRIBBLE")] = {&Kernel::excDribble, 1, 1, 1};
    stringToCmd[QObject::tr("NODRIBBLE")] = {&Kernel::excNodribble, 0, 0, 0};

    stringToCmd[QObject::tr("CLEARTEXT")] = {&Kernel::excCleartext, 0, 0, 0};
    stringToCmd[QObject::tr("CT")] = {&Kernel::excCleartext, 0, 0, 0};
    stringToCmd[QObject::tr("CURSORINSERT")] = {ifGUI(&Kernel::excCursorInsert), 0, 0, 0};
    stringToCmd[QObject::tr("CURSOROVERWRITE")] = {ifGUI(&Kernel::excCursorOverwrite), 0, 0, 0};
    stringToCmd[QObject::tr("CURSORMODE")] = {ifGUI(&Kernel::excCursorMode), 0, 0, 0};

    stringToCmd[QObject::tr("CLOSE")] = {&Kernel::excClose, 1, 1, 1};
    stringToCmd[QObject::tr("CLOSEALL")] = {&Kernel::excCloseall, 0, 0, 0};
    stringToCmd[QObject::tr("ERASEFILE")] = {&Kernel::excErasefile, 1, 1, 1};
    stringToCmd[QObject::tr("ERF")] = {&Kernel::excErasefile, 1, 1, 1};

    stringToCmd[QObject::tr("FIRST")] = {&Kernel::excFirst, 1, 1, 1};
    stringToCmd[QObject::tr("LAST")] = {&Kernel::excLast, 1, 1, 1};
    stringToCmd[QObject::tr("BUTFIRST")] = {&Kernel::excButfirst, 1, 1, 1};
    stringToCmd[QObject::tr("BF")] = {&Kernel::excButfirst, 1, 1, 1};
    stringToCmd[QObject::tr("FIRSTS")] = {&Kernel::excFirsts, 1, 1, 1};
    stringToCmd[QObject::tr("BUTFIRSTS")] = {&Kernel::excButfirsts, 1, 1, 1};
    stringToCmd[QObject::tr("BFS")] = {&Kernel::excButfirsts, 1, 1, 1};
    stringToCmd[QObject::tr("BUTLAST")] = {&Kernel::excButlast, 1, 1, 1};
    stringToCmd[QObject::tr("BL")] = {&Kernel::excButlast, 1, 1, 1};
    stringToCmd[QObject::tr("ITEM")] = {&Kernel::excItem, 2, 2, 2};

    stringToCmd[QObject::tr("SETITEM")] = {&Kernel::excSetitem, 3, 3, 3};
    stringToCmd[QObject::tr(".SETFIRST")] = {&Kernel::excDotSetfirst, 2, 2, 2};
    stringToCmd[QObject::tr(".SETBF")] = {&Kernel::excDotSetbf, 2, 2, 2};
    stringToCmd[QObject::tr(".SETITEM")] = {&Kernel::excDotSetitem, 3, 3, 3};

    stringToCmd[QObject::tr("WORDP")] = {&Kernel::excWordp, 1, 1, 1};
    stringToCmd[QObject::tr("WORD?")] = {&Kernel::excWordp, 1, 1, 1};
    stringToCmd[QObject::tr("LISTP")] = {&Kernel::excListp, 1, 1, 1};
    stringToCmd[QObject::tr("LIST?")] = {&Kernel::excListp, 1, 1, 1};
    stringToCmd[QObject::tr("ARRAYP")] = {&Kernel::excArrayp, 1, 1, 1};
    stringToCmd[QObject::tr("ARRAY?")] = {&Kernel::excArrayp, 1, 1, 1};
    stringToCmd[QObject::tr("EMPTYP")] = {&Kernel::excEmptyp, 1, 1, 1};
    stringToCmd[QObject::tr("EMPTY?")] = {&Kernel::excEmptyp, 1, 1, 1};
    stringToCmd[QObject::tr("EQUALP")] = {&Kernel::excEqualp, 2, 2, 2};
    stringToCmd[QObject::tr("EQUAL?")] = {&Kernel::excEqualp, 2, 2, 2};
    stringToCmd[QObject::tr("NOTEQUALP")] = {&Kernel::excNotequal, 2, 2, 2};
    stringToCmd[QObject::tr("NOTEQUAL?")] = {&Kernel::excNotequal, 2, 2, 2};
    stringToCmd[QObject::tr("BEFOREP")] = {&Kernel::excBeforep, 2, 2, 2};
    stringToCmd[QObject::tr("BEFORE?")] = {&Kernel::excBeforep, 2, 2, 2};
    stringToCmd[QObject::tr(".EQ")] = {&Kernel::excDotEq, 2, 2, 2};
    stringToCmd[QObject::tr("MEMBERP")] = {&Kernel::excMemberp, 2, 2, 2};
    stringToCmd[QObject::tr("MEMBER?")] = {&Kernel::excMemberp, 2, 2, 2};
    stringToCmd[QObject::tr("SUBSTRINGP")] = {&Kernel::excSubstringp, 2, 2, 2};
    stringToCmd[QObject::tr("SUBSTRING?")] = {&Kernel::excSubstringp, 2, 2, 2};
    stringToCmd[QObject::tr("NUMBERP")] = {&Kernel::excNumberp, 1, 1, 1};
    stringToCmd[QObject::tr("NUMBER?")] = {&Kernel::excNumberp, 1, 1, 1};
    stringToCmd[QObject::tr("VBARREDP")] = {&Kernel::excVbarredp, 1, 1, 1};
    stringToCmd[QObject::tr("VBARRED?")] = {&Kernel::excVbarredp, 1, 1, 1};

    stringToCmd[QObject::tr("COUNT")] = {&Kernel::excCount, 1, 1, 1};
    stringToCmd[QObject::tr("ASCII")] = {&Kernel::excAscii, 1, 1, 1};
    stringToCmd[QObject::tr("RAWASCII")] = {&Kernel::excRawascii, 1, 1, 1};
    stringToCmd[QObject::tr("CHAR")] = {&Kernel::excChar, 1, 1, 1};
    stringToCmd[QObject::tr("MEMBER")] = {&Kernel::excMember, 2, 2, 2};
    stringToCmd[QObject::tr("LOWERCASE")] = {&Kernel::excLowercase, 1, 1, 1};
    stringToCmd[QObject::tr("UPPERCASE")] = {&Kernel::excUppercase, 1, 1, 1};
    stringToCmd[QObject::tr("STANDOUT")] = {ifGUI(&Kernel::excStandout), 1, 1, 1};
    stringToCmd[QObject::tr("PARSE")] = {&Kernel::excParse, 1, 1, 1};
    stringToCmd[QObject::tr("RUNPARSE")] = {&Kernel::excRunparse, 1, 1, 1};

    stringToCmd[QObject::tr("MINUS")] = {&Kernel::excMinus, 1, 1, 1};
    stringToCmd["-"] = {&Kernel::excMinus, 1, 1, 1};
    stringToCmd["--"] = {&Kernel::excMinus, 1, 1, 1};

    stringToCmd[QObject::tr("PRINT")] = {&Kernel::excPrint, 0, 1, -1};
    stringToCmd[QObject::tr("PR")] = {&Kernel::excPrint, 0, 1, -1};
    stringToCmd[QObject::tr("TYPE")] = {&Kernel::excType, 0, 1, -1};
    stringToCmd[QObject::tr("SHOW")] = {&Kernel::excShow, 0, 1, -1};
    stringToCmd[QObject::tr("MAKE")] = {&Kernel::excMake, 2, 2, 2};
    stringToCmd[QObject::tr("REPEAT")] = {&Kernel::excRepeat, 2, 2, 2};
    stringToCmd[QObject::tr("SQRT")] = {&Kernel::excSqrt, 1, 1, 1};
    stringToCmd[QObject::tr("RANDOM")] = {&Kernel::excRandom, 1, 1, 2};
    stringToCmd[QObject::tr("RERANDOM")] = {&Kernel::excRerandom, 0, 0, 1};
    stringToCmd[QObject::tr("THING")] = {&Kernel::excThing, 1, 1, 1};
    stringToCmd[QObject::tr("WAIT")] = {&Kernel::excWait, 1, 1, 1};
    stringToCmd[QObject::tr("SETCURSOR")] = {ifGUI(&Kernel::excSetcursor), 1, 1, 1};
    stringToCmd[QObject::tr("CURSOR")] = {ifGUI(&Kernel::excCursor), 0, 0, 0};
    stringToCmd[QObject::tr("SETTEXTCOLOR")] = {ifGUI(&Kernel::excSettextcolor), 1, 2, 2};
    stringToCmd[QObject::tr("SETTC")] = {ifGUI(&Kernel::excSettextcolor), 1, 2, 2};
    stringToCmd[QObject::tr("INCREASEFONT")] = {ifGUI(&Kernel::excIncreasefont), 0, 0, 0};
    stringToCmd[QObject::tr("DECREASEFONT")] = {ifGUI(&Kernel::excDecreasefont), 0, 0, 0};
    stringToCmd[QObject::tr("SETTEXTSIZE")] = {ifGUI(&Kernel::excSettextsize), 1, 1, 1};
    stringToCmd[QObject::tr("TEXTSIZE")] = {ifGUI(&Kernel::excTextsize), 0, 0, 0};
    stringToCmd[QObject::tr("SETFONT")] = {ifGUI(&Kernel::excSetfont), 1, 1, 1};
    stringToCmd[QObject::tr("FONT")] = {ifGUI(&Kernel::excFont), 0, 0, 0};
    stringToCmd[QObject::tr("ALLFONTS")] = {ifGUI(&Kernel::excAllfonts), 0, 0, 0};

    stringToCmd[QObject::tr("FORWARD")] = {ifGUI(&Kernel::excForward), 1, 1, 1};
    stringToCmd[QObject::tr("FD")] = {ifGUI(&Kernel::excForward), 1, 1, 1};
    stringToCmd[QObject::tr("BACK")] = {ifGUI(&Kernel::excBack), 1, 1, 1};
    stringToCmd[QObject::tr("BK")] = {ifGUI(&Kernel::excBack), 1, 1, 1};
    stringToCmd[QObject::tr("RIGHT")] = {ifGUI(&Kernel::excRight), 1, 1, 1};
    stringToCmd[QObject::tr("RT")] = {ifGUI(&Kernel::excRight), 1, 1, 1};
    stringToCmd[QObject::tr("LEFT")] = {ifGUI(&Kernel::excLeft), 1, 1, 1};
    stringToCmd[QObject::tr("LT")] = {ifGUI(&Kernel::excLeft), 1, 1, 1};
    stringToCmd[QObject::tr("CLEARSCREEN")] = {ifGUI(&Kernel::excClearscreen), 0, 0, 0};
    stringToCmd[QObject::tr("CS")] = {ifGUI(&Kernel::excClearscreen), 0, 0, 0};
    stringToCmd[QObject::tr("CLEAN")] = {ifGUI(&Kernel::excClean), 0, 0, 0};
    stringToCmd[QObject::tr("PENUP")] = {ifGUI(&Kernel::excPenup), 0, 0, 0};
    stringToCmd[QObject::tr("PU")] = {ifGUI(&Kernel::excPenup), 0, 0, 0};
    stringToCmd[QObject::tr("PENDOWN")] = {ifGUI(&Kernel::excPendown), 0, 0, 0};
    stringToCmd[QObject::tr("PD")] = {ifGUI(&Kernel::excPendown), 0, 0, 0};
    stringToCmd[QObject::tr("PENDOWNP")] = {ifGUI(&Kernel::excPendownp), 0, 0, 0};
    stringToCmd[QObject::tr("PENDOWN?")] = {ifGUI(&Kernel::excPendownp), 0, 0, 0};
    stringToCmd[QObject::tr("HIDETURTLE")] = {ifGUI(&Kernel::excHideturtle), 0, 0, 0};
    stringToCmd[QObject::tr("HT")] = {ifGUI(&Kernel::excHideturtle), 0, 0, 0};
    stringToCmd[QObject::tr("SHOWTURTLE")] = {ifGUI(&Kernel::excShowturtle), 0, 0, 0};
    stringToCmd[QObject::tr("ST")] = {ifGUI(&Kernel::excShowturtle), 0, 0, 0};
    stringToCmd[QObject::tr("SETXY")] = {ifGUI(&Kernel::excSetXY), 2, 2, 2};
    stringToCmd[QObject::tr("SETX")] = {ifGUI(&Kernel::excSetX), 1, 1, 1};
    stringToCmd[QObject::tr("SETY")] = {ifGUI(&Kernel::excSetY), 1, 1, 1};
    stringToCmd[QObject::tr("SETPOS")] = {ifGUI(&Kernel::excSetpos), 1, 1, 1};
    stringToCmd[QObject::tr("POS")] = {ifGUI(&Kernel::excPos), 0, 0, 1};
    stringToCmd[QObject::tr("HOME")] = {ifGUI(&Kernel::excHome), 0, 0, 0};
    stringToCmd[QObject::tr("HEADING")] = {ifGUI(&Kernel::excHeading), 0, 0, 1};
    stringToCmd[QObject::tr("SETHEADING")] = {ifGUI(&Kernel::excSetheading), 1, 1, 1};
    stringToCmd[QObject::tr("SETH")] = {ifGUI(&Kernel::excSetheading), 1, 1, 1};
    stringToCmd[QObject::tr("ARC")] = {ifGUI(&Kernel::excArc), 2, 2, 2};
    stringToCmd[QObject::tr("TOWARDS")] = {ifGUI(&Kernel::excTowards), 1, 1, 1};
    stringToCmd[QObject::tr("SCRUNCH")] = {ifGUI(&Kernel::excScrunch), 0, 0, 0};
    stringToCmd[QObject::tr("SETSCRUNCH")] = {ifGUI(&Kernel::excSetscrunch), 2, 2, 2};
    stringToCmd[QObject::tr("LABEL")] = {ifGUI(&Kernel::excLabel), 1, 1, 1};
    stringToCmd[QObject::tr("LABELHEIGHT")] = {ifGUI(&Kernel::excLabelheight), 0, 0, 0};
    stringToCmd[QObject::tr("SETLABELHEIGHT")] = {ifGUI(&Kernel::excSetlabelheight), 1, 1, 1};
    stringToCmd[QObject::tr("SHOWNP")] = {ifGUI(&Kernel::excShownp), 0, 0, 0};
    stringToCmd[QObject::tr("SHOWN?")] = {ifGUI(&Kernel::excShownp), 0, 0, 0};
    stringToCmd[QObject::tr("SETPENCOLOR")] = {ifGUI(&Kernel::excSetpencolor), 1, 1, 1};
    stringToCmd[QObject::tr("SETPC")] = {ifGUI(&Kernel::excSetpencolor), 1, 1, 1};
    stringToCmd[QObject::tr("PENCOLOR")] = {ifGUI(&Kernel::excPencolor), 0, 0, 0};
    stringToCmd[QObject::tr("PC")] = {ifGUI(&Kernel::excPencolor), 0, 0, 0};
    stringToCmd[QObject::tr("SETPALETTE")] = {ifGUI(&Kernel::excSetpalette), 2, 2, 2};
    stringToCmd[QObject::tr("PALETTE")] = {ifGUI(&Kernel::excPalette), 1, 1, 1};
    stringToCmd[QObject::tr("BACKGROUND")] = {ifGUI(&Kernel::excBackground), 0, 0, 0};
    stringToCmd[QObject::tr("BG")] = {ifGUI(&Kernel::excBackground), 0, 0, 0};
    stringToCmd[QObject::tr("SETBACKGROUND")] = {ifGUI(&Kernel::excSetbackground), 1, 1, 1};
    stringToCmd[QObject::tr("SETBG")] = {ifGUI(&Kernel::excSetbackground), 1, 1, 1};
    stringToCmd[QObject::tr("SAVEPICT")] = {ifGUI(&Kernel::excSavepict), 1, 1, 1};
    stringToCmd[QObject::tr("LOADPICT")] = {ifGUI(&Kernel::excLoadpict), 1, 1, 1};
    stringToCmd[QObject::tr("SVGPICT")] = {ifGUI(&Kernel::excSvgpict), 1, 1, 1};

    stringToCmd[QObject::tr("PENPAINT")] = {ifGUI(&Kernel::excPenpaint), 0, 0, 0};
    stringToCmd[QObject::tr("PPT")] = {ifGUI(&Kernel::excPenpaint), 0, 0, 0};
    stringToCmd[QObject::tr("PENERASE")] = {ifGUI(&Kernel::excPenerase), 0, 0, 0};
    stringToCmd[QObject::tr("PE")] = {ifGUI(&Kernel::excPenerase), 0, 0, 0};
    stringToCmd[QObject::tr("PENREVERSE")] = {ifGUI(&Kernel::excPenreverse), 0, 0, 0};
    stringToCmd[QObject::tr("PX")] = {ifGUI(&Kernel::excPenreverse), 0, 0, 0};
    stringToCmd[QObject::tr("PENMODE")] = {ifGUI(&Kernel::excPenmode), 0, 0, 0};
    stringToCmd[QObject::tr("SETPENSIZE")] = {ifGUI(&Kernel::excSetpensize), 1, 1, 1};
    stringToCmd[QObject::tr("PENSIZE")] = {ifGUI(&Kernel::excPensize), 0, 0, 0};
    stringToCmd[QObject::tr("FILLED")] = {ifGUI(&Kernel::excFilled), 2, 2, 2};

    stringToCmd[QObject::tr("WRAP")] = {ifGUI(&Kernel::excWrap), 0, 0, 0};
    stringToCmd[QObject::tr("FENCE")] = {ifGUI(&Kernel::excFence), 0, 0, 0};
    stringToCmd[QObject::tr("WINDOW")] = {ifGUI(&Kernel::excWindow), 0, 0, 0};
    stringToCmd[QObject::tr("TURTLEMODE")] = {ifGUI(&Kernel::excTurtlemode), 0, 0, 0};

    stringToCmd[QObject::tr("MOUSEPOS")] = {ifGUI(&Kernel::excMousepos), 0, 0, 0};
    stringToCmd[QObject::tr("CLICKPOS")] = {ifGUI(&Kernel::excClickpos), 0, 0, 0};
    stringToCmd[QObject::tr("BOUNDS")] = {ifGUI(&Kernel::excBounds), 0, 0, 0};
    stringToCmd[QObject::tr("SETBOUNDS")] = {ifGUI(&Kernel::excSetbounds), 2, 2, 2};

    stringToCmd[QObject::tr("TEXTSCREEN")] = {ifGUI(&Kernel::excTextscreen), 0, 0, 0};
    stringToCmd[QObject::tr("TS")] = {ifGUI(&Kernel::excTextscreen), 0, 0, 0};
    stringToCmd[QObject::tr("FULLSCREEN")] = {ifGUI(&Kernel::excFullscreen), 0, 0, 0};
    stringToCmd[QObject::tr("FS")] = {ifGUI(&Kernel::excFullscreen), 0, 0, 0};
    stringToCmd[QObject::tr("SPLITSCREEN")] = {ifGUI(&Kernel::excSplitscreen), 0, 0, 0};
    stringToCmd[QObject::tr("SS")] = {ifGUI(&Kernel::excSplitscreen), 0, 0, 0};
    stringToCmd[QObject::tr("SCREENMODE")] = {ifGUI(&Kernel::excScreenmode), 0, 0, 0};

    stringToCmd[QObject::tr("BUTTONP")] = {ifGUI(&Kernel::excButtonp), 0, 0, 0};
    stringToCmd[QObject::tr("BUTTON?")] = {ifGUI(&Kernel::excButtonp), 0, 0, 0};
    stringToCmd[QObject::tr("BUTTON")] = {ifGUI(&Kernel::excButton), 0, 0, 0};

    stringToCmd[QObject::tr("MATRIX")] = {ifGUI(&Kernel::excMatrix), 0, 0, 0}; // for debugging

    stringToCmd[QObject::tr("SUM")] = {&Kernel::excSum, 0, 2, -1};
    stringToCmd[QObject::tr("DIFFERENCE")] = {&Kernel::excDifference, 2, 2, 2};
    stringToCmd[QObject::tr("PRODUCT")] = {&Kernel::excProduct, 0, 2, -1};
    stringToCmd[QObject::tr("QUOTIENT")] = {&Kernel::excQuotient, 1, 2, 2};
    stringToCmd[QObject::tr("REMAINDER")] = {&Kernel::excRemainder, 2, 2, 2};
    stringToCmd[QObject::tr("MODULO")] = {&Kernel::excModulo, 2, 2, 2};
    stringToCmd[QObject::tr("INT")] = {&Kernel::excInt, 1, 1, 1};
    stringToCmd[QObject::tr("EXP")] = {&Kernel::excExp, 1, 1, 1};
    stringToCmd[QObject::tr("LOG10")] = {&Kernel::excLog10, 1, 1, 1};
    stringToCmd[QObject::tr("LN")] = {&Kernel::excLn, 1, 1, 1};
    stringToCmd[QObject::tr("SIN")] = {&Kernel::excSin, 1, 1, 1};
    stringToCmd[QObject::tr("RADSIN")] = {&Kernel::excRadsin, 1, 1, 1};
    stringToCmd[QObject::tr("COS")] = {&Kernel::excCos, 1, 1, 1};
    stringToCmd[QObject::tr("RADCOS")] = {&Kernel::excRadcos, 1, 1, 1};
    stringToCmd[QObject::tr("ARCTAN")] = {&Kernel::excArctan, 1, 1, 2};
    stringToCmd[QObject::tr("RADARCTAN")] = {&Kernel::excRadarctan, 1, 1, 2};
    stringToCmd[QObject::tr("ROUND")] = {&Kernel::excRound, 1, 1, 1};
    stringToCmd[QObject::tr("POWER")] = {&Kernel::excPower, 2, 2, 2};
    stringToCmd[QObject::tr("BITAND")] = {&Kernel::excBitand, 0, 2, -1};
    stringToCmd[QObject::tr("BITOR")] = {&Kernel::excBitor, 0, 2, -1};
    stringToCmd[QObject::tr("BITXOR")] = {&Kernel::excBitxor, 0, 2, -1};
    stringToCmd[QObject::tr("BITNOT")] = {&Kernel::excBitnot, 1, 1, 1};
    stringToCmd[QObject::tr("ASHIFT")] = {&Kernel::excAshift, 2, 2, 2};
    stringToCmd[QObject::tr("LSHIFT")] = {&Kernel::excLshift, 2, 2, 2};
    stringToCmd[QObject::tr("AND")] = {&Kernel::excAnd, 0, 2, -1};
    stringToCmd[QObject::tr("OR")] = {&Kernel::excOr, 0, 2, -1};
    stringToCmd[QObject::tr("NOT")] = {&Kernel::excNot, 1, 1, 1};

    stringToCmd[QObject::tr("FORM")] = {&Kernel::excForm, 3, 3, 3};

    stringToCmd[QObject::tr("LESSP")] = {&Kernel::excLessp, 2, 2, 2};
    stringToCmd[QObject::tr("LESS?")] = {&Kernel::excLessp, 2, 2, 2};
    stringToCmd[QObject::tr("GREATERP")] = {&Kernel::excGreaterp, 2, 2, 2};
    stringToCmd[QObject::tr("GREATER?")] = {&Kernel::excGreaterp, 2, 2, 2};
    stringToCmd[QObject::tr("LESSEQUALP")] = {&Kernel::excLessequalp, 2, 2, 2};
    stringToCmd[QObject::tr("LESSEQUAL?")] = {&Kernel::excLessequalp, 2, 2, 2};
    stringToCmd[QObject::tr("GREATEREQUALP")] = {&Kernel::excGreaterequalp, 2, 2, 2};
    stringToCmd[QObject::tr("GREATEREQUAL?")] = {&Kernel::excGreaterequalp, 2, 2, 2};

    stringToCmd[QObject::tr("DEFINE")] = {&Kernel::excDefine, 2, 2, 2};
    stringToCmd[QObject::tr("TEXT")] = {&Kernel::excText, 1, 1, 1};
    stringToCmd[QObject::tr("FULLTEXT")] = {&Kernel::excFulltext, 1, 1, 1};
    stringToCmd[QObject::tr("COPYDEF")] = {&Kernel::excCopydef, 2, 2, 2};
    stringToCmd[QObject::tr("LOCAL")] = {&Kernel::excLocal, 1, 1, -1};
    stringToCmd[QObject::tr("GLOBAL")] = {&Kernel::excGlobal, 1, 1, -1};

    stringToCmd[QObject::tr("PPROP")] = {&Kernel::excPprop, 3, 3, 3};
    stringToCmd[QObject::tr("GPROP")] = {&Kernel::excGprop, 2, 2, 2};
    stringToCmd[QObject::tr("REMPROP")] = {&Kernel::excRemprop, 2, 2, 2};
    stringToCmd[QObject::tr("PLIST")] = {&Kernel::excPlist, 1, 1, 1};

    stringToCmd[QObject::tr("PROCEDUREP")] = {&Kernel::excProcedurep, 1, 1, 1};
    stringToCmd[QObject::tr("PROCEDURE?")] = {&Kernel::excProcedurep, 1, 1, 1};
    stringToCmd[QObject::tr("PRIMITIVEP")] = {&Kernel::excPrimitivep, 1, 1, 1};
    stringToCmd[QObject::tr("PRIMITIVE?")] = {&Kernel::excPrimitivep, 1, 1, 1};
    stringToCmd[QObject::tr("DEFINEDP")] = {&Kernel::excDefinedp, 1, 1, 1};
    stringToCmd[QObject::tr("DEFINED?")] = {&Kernel::excDefinedp, 1, 1, 1};
    stringToCmd[QObject::tr("NAMEP")] = {&Kernel::excNamep, 1, 1, 1};
    stringToCmd[QObject::tr("NAME?")] = {&Kernel::excNamep, 1, 1, 1};
    stringToCmd[QObject::tr("PLISTP")] = {&Kernel::excPlistp, 1, 1, 1};
    stringToCmd[QObject::tr("PLIST?")] = {&Kernel::excPlistp, 1, 1, 1};

    stringToCmd[QObject::tr("CONTENTS")] = {&Kernel::excContents, 0, 0, 0};
    stringToCmd[QObject::tr("BURIED")] = {&Kernel::excBuried, 0, 0, 0};
    stringToCmd[QObject::tr("TRACED")] = {&Kernel::excTraced, 0, 0, 0};
    stringToCmd[QObject::tr("STEPPED")] = {&Kernel::excStepped, 0, 0, 0};
    stringToCmd[QObject::tr("PROCEDURES")] = {&Kernel::excProcedures, 0, 0, 0};
    stringToCmd[QObject::tr("PRIMITIVES")] = {&Kernel::excPrimitives, 0, 0, 0};
    stringToCmd[QObject::tr("NAMES")] = {&Kernel::excNames, 0, 0, 0};
    stringToCmd[QObject::tr("PLISTS")] = {&Kernel::excPlists, 0, 0, 0};
    stringToCmd[QObject::tr("ARITY")] = {&Kernel::excArity, 1, 1, 1};
    stringToCmd[QObject::tr("NODES")] = {&Kernel::excNodes, 0, 0, 0};

    stringToCmd[QObject::tr("PRINTOUT")] = {&Kernel::excPrintout, 1, 1, 1};
    stringToCmd[QObject::tr("PO")] = {&Kernel::excPrintout, 1, 1, 1};
    stringToCmd[QObject::tr("POT")] = {&Kernel::excPot, 1, 1, 1};

    stringToCmd[QObject::tr("ERASE")] = {&Kernel::excErase, 1, 1, 1};
    stringToCmd[QObject::tr("ER")] = {&Kernel::excErase, 1, 1, 1};
    stringToCmd[QObject::tr("ERALL")] = {&Kernel::excErall, 0, 0, 0};
    stringToCmd[QObject::tr("ERPS")] = {&Kernel::excErps, 0, 0, 0};
    stringToCmd[QObject::tr("ERNS")] = {&Kernel::excErns, 0, 0, 0};
    stringToCmd[QObject::tr("ERPLS")] = {&Kernel::excErpls, 0, 0, 0};
    stringToCmd[QObject::tr("BURY")] = {&Kernel::excBury, 1, 1, 1};
    stringToCmd[QObject::tr("UNBURY")] = {&Kernel::excUnbury, 1, 1, 1};
    stringToCmd[QObject::tr("BURIEDP")] = {&Kernel::excBuriedp, 1, 1, 1};
    stringToCmd[QObject::tr("BURIED?")] = {&Kernel::excBuriedp, 1, 1, 1};
    stringToCmd[QObject::tr("TRACE")] = {&Kernel::excTrace, 1, 1, 1};
    stringToCmd[QObject::tr("UNTRACE")] = {&Kernel::excUntrace, 1, 1, 1};
    stringToCmd[QObject::tr("TRACEDP")] = {&Kernel::excTracedp, 1, 1, 1};
    stringToCmd[QObject::tr("TRACED?")] = {&Kernel::excTracedp, 1, 1, 1};
    stringToCmd[QObject::tr("STEP")] = {&Kernel::excStep, 1, 1, 1};
    stringToCmd[QObject::tr("UNSTEP")] = {&Kernel::excUnstep, 1, 1, 1};
    stringToCmd[QObject::tr("STEPPEDP")] = {&Kernel::excSteppedp, 1, 1, 1};
    stringToCmd[QObject::tr("STEPPED?")] = {&Kernel::excSteppedp, 1, 1, 1};
    stringToCmd[QObject::tr("EDIT")] = {&Kernel::excEdit, 0, -1, 1};
    stringToCmd[QObject::tr("ED")] = {&Kernel::excEdit, 0, -1, 1};
    stringToCmd[QObject::tr("EDITFILE")] = {&Kernel::excEditfile, 1, 1, 1};
    stringToCmd[QObject::tr("SAVE")] = {&Kernel::excSave, 0, -1, 1};
    stringToCmd[QObject::tr("LOAD")] = {&Kernel::excLoad, 1, 1, 1};
    stringToCmd[QObject::tr("HELP")] = {&Kernel::excHelp, 0, -1, -1};

    // CONTROL STRUCTURES

    stringToCmd[QObject::tr("RUN")] = {&Kernel::excRun, 1, 1, 1};
    stringToCmd[QObject::tr("TIME")] = {&Kernel::excTime, 1, 1, 1};
    stringToCmd[QObject::tr("RUNRESULT")] = {&Kernel::excRunresult, 1, 1, 1};
    stringToCmd[QObject::tr("FOREVER")] = {&Kernel::excForever, 1, 1, 1};
    stringToCmd[QObject::tr("REPCOUNT")] = {&Kernel::excRepcount, 0, 0, 0};
    stringToCmd[QObject::tr("IF")] = {&Kernel::excIf, 2, 2, 2};
    stringToCmd[QObject::tr("IFELSE")] = {&Kernel::excIfelse, 3, 3, 3};
    stringToCmd[QObject::tr("TEST")] = {&Kernel::excTest, 1, 1, 1};
    stringToCmd[QObject::tr("IFTRUE")] = {&Kernel::excIftrue, 1, 1, 1};
    stringToCmd[QObject::tr("IFT")] = {&Kernel::excIftrue, 1, 1, 1};
    stringToCmd[QObject::tr("IFFALSE")] = {&Kernel::excIffalse, 1, 1, 1};
    stringToCmd[QObject::tr("IFF")] = {&Kernel::excIffalse, 1, 1, 1};
    stringToCmd[QObject::tr("STOP")] = {&Kernel::excStop, 0, 0, 1};
    stringToCmd[QObject::tr("OUTPUT")] = {&Kernel::excOutput, 1, 1, 1};
    stringToCmd[QObject::tr("OP")] = {&Kernel::excOutput, 1, 1, 1};
    stringToCmd[QObject::tr("CATCH")] = {&Kernel::excCatch, 2, 2, 2};
    stringToCmd[QObject::tr("THROW")] = {&Kernel::excThrow, 1, 1, 2};
    stringToCmd[QObject::tr("ERROR")] = {&Kernel::excError, 0, 0, 0};
    stringToCmd[QObject::tr("PAUSE")] = {&Kernel::excPause, 0, 0, 0};
    stringToCmd[QObject::tr("CONTINUE")] = {&Kernel::excContinue, 0, -1, 1};
    stringToCmd[QObject::tr("CO")] = {&Kernel::excContinue, 0, -1, 1};
    stringToCmd[QObject::tr("BYE")] = {&Kernel::excBye, 0, 0, 0};
    stringToCmd[QObject::tr(".MAYBEOUTPUT")] = {&Kernel::excDotMaybeoutput, 1, 1, 1};
    stringToCmd[QObject::tr("TAG")] = {&Kernel::excTag, 1, 1, 1};
    stringToCmd[QObject::tr("GOTO")] = {&Kernel::excGoto, 1, 1, 1};

    stringToCmd[QObject::tr("APPLY")] = {&Kernel::excApply, 2, 2, 2};
    stringToCmd["?"] = {&Kernel::excNamedSlot, 0, 0, 1};

    stringToCmd[QObject::tr("TO")] = {&Kernel::excTo, -1, -1, -1};
    stringToCmd[QObject::tr(".MACRO")] = {&Kernel::excTo, -1, -1, -1};
    stringToCmd[QObject::tr(".DEFMACRO")] = {&Kernel::excDefine, 2, 2, 2};
    stringToCmd[QObject::tr("MACROP")] = {&Kernel::excMacrop, 1, 1, 1};
    stringToCmd[QObject::tr("MACRO?")] = {&Kernel::excMacrop, 1, 1, 1};

    stringToCmd[QObject::tr("GC")] = {&Kernel::excNoop, 0, 0, -1};
    stringToCmd[QObject::tr(".SETSEGMENTSIZE")] = {&Kernel::excNoop, 1, 1, 1};
    stringToCmd[QObject::tr("SETPENPATTERN")] = {&Kernel::excNoop, 1, 1, 1};
    stringToCmd[QObject::tr("PENPATTERN")] = {&Kernel::excNoop, 1, 1, 1};
    stringToCmd[QObject::tr("REFRESH")] = {&Kernel::excNoop, 0, 0, 0};
    stringToCmd[QObject::tr("NOREFRESH")] = {&Kernel::excNoop, 0, 0, 0};

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

    List *retval = new List();

    List *inputs = new List();

    for (auto &i : body->requiredInputs) {
        inputs->append(DatumPtr(i));
    }

    QList<DatumPtr>::iterator d = body->optionalDefaults.begin();
    for (auto &i : body->optionalInputs) {
        List *optInput = new List(d->listValue());
        optInput->prepend(DatumPtr(i));
        ++d;
        inputs->append(DatumPtr(optInput));
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

    if (procedures.contains(procname)) {
        Procedure *body = procedures[procname].procedureValue();

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
    if (!procedures.contains(procname))
        Error::noHow(procnameP);

    Procedure *body = procedures[procname].procedureValue();

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

    QList<DatumPtr>::iterator d = body->optionalDefaults.begin();
    for (auto &i : body->optionalInputs) {
        paramName = i;
        paramName.push_front(':');
        List *optInput = new List(d->listValue());
        optInput->prepend(DatumPtr(paramName));
        firstLine->append(DatumPtr(optInput));
        ++d;
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

DatumPtr Procedures::astnodeFromCommand(DatumPtr cmdP, int &minParams,
                                    int &defaultParams, int &maxParams) {
    QString cmdString = cmdP.wordValue()->keyValue();

    Cmd_t command;
    DatumPtr node = DatumPtr(new ASTNode(cmdP));
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
    } else if (cmdString.startsWith(QObject::tr("SET")) && (cmdString.size() > 3) &&
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
        DatumPtr a = DatumPtr(new ASTNode(QObject::tr("literal")));
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

