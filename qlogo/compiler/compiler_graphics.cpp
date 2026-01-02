//===-- qlogo/graphics.cpp - Turtle implementations -------*- C++ -*-===//
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
/// This file contains the implementation of the graphics methods of the
/// Compiler class, which mostly involve the turtle.
///
//===----------------------------------------------------------------------===//

#include "astnode.h"
#include "compiler.h"
#include "compiler_private.h"
#include "datum_types.h"
#include "exports.h"
#include "kernel.h"
#include "sharedconstants.h"
#include "turtle.h"
#include "workspace/callframe.h"
using namespace llvm;
using namespace llvm::orc;

// TURTLE MOTION

/***DOC FORWARD FD
FORWARD dist
FD dist

    moves the turtle forward, in the direction that it's facing, by
    the specified distance (measured in turtle steps).

COD***/
// CMD FORWARD 1 1 1 n
// CMD FD 1 1 1 n
Value *Compiler::genForward(const DatumPtr &node, RequestReturnType returnType)
{
    Value *distance = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    generateCallExtern(TyVoid, moveTurtleForward, PaDouble(distance));
    return generateVoidRetval(node);
}

/***DOC BACK BK
BACK dist
BK dist

    moves the turtle backward, i.e., exactly opposite to the direction
    that it's facing, by the specified distance.  (The heading of the
    turtle does not change.)

COD***/
// CMD BACK 1 1 1 n
// CMD BK 1 1 1 n
Value *Compiler::genBack(const DatumPtr &node, RequestReturnType returnType)
{
    Value *reverseDistance = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *distance = scaff->builder.CreateFNeg(reverseDistance, "negativeDistance");
    generateCallExtern(TyVoid, moveTurtleForward, PaDouble(distance));
    return generateVoidRetval(node);
}
/***DOC LEFT LT
LEFT degrees
LT degrees

    turns the turtle counterclockwise by the specified angle, measured
    in degrees (1/360 of a circle).

COD***/
// CMD LEFT 1 1 1 n
// CMD LT 1 1 1 n
Value *Compiler::genLeft(const DatumPtr &node, RequestReturnType returnType)
{
    Value *angle = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *negativeAngle = scaff->builder.CreateFNeg(angle, "negativeAngle");
    generateCallExtern(TyVoid, moveTurtleRotate, PaDouble(negativeAngle));
    return generateVoidRetval(node);
}

/***DOC RIGHT RT
RIGHT degrees
RT degrees

    turns the turtle clockwise by the specified angle, measured in
    degrees (1/360 of a circle).

COD***/
// CMD RIGHT 1 1 1 n
// CMD RT 1 1 1 n
Value *Compiler::genRight(const DatumPtr &node, RequestReturnType returnType)
{
    Value *angle = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    generateCallExtern(TyVoid, moveTurtleRotate, PaDouble(angle));
    return generateVoidRetval(node);
}
/***DOC SETXY
SETXY xcor ycor

    moves the turtle to an absolute position in the graphics window.  The
    two inputs are numbers, the X and Y coordinates.

COD***/
// CMD SETXY 2 2 2 n
Value *Compiler::genSetxy(const DatumPtr &node, RequestReturnType returnType)
{
    Value *x = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *y = generateChild(node.astnodeValue(), 1, RequestReturnReal);
    generateCallExtern(TyVoid, setTurtleXY, PaDouble(x), PaDouble(y));
    return generateVoidRetval(node);
}
/***DOC SETX
SETX xcor

    moves the turtle horizontally from its old position to a new
    absolute horizontal coordinate.  The input is the new X
    coordinate.

COD***/
// CMD SETX 1 1 1 n
Value *Compiler::genSetx(const DatumPtr &node, RequestReturnType returnType)
{
    Value *x = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    generateCallExtern(TyVoid, setTurtleX, PaDouble(x));
    return generateVoidRetval(node);
}
/***DOC SETY
SETY ycor

    moves the turtle vertically from its old position to a new
    absolute vertical coordinate.  The input is the new Y
    coordinate.

COD***/
// CMD SETY 1 1 1 n
Value *Compiler::genSety(const DatumPtr &node, RequestReturnType returnType)
{
    Value *y = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    generateCallExtern(TyVoid, setTurtleY, PaDouble(y));
    return generateVoidRetval(node);
}
/***DOC SETPOS
SETPOS pos

    moves the turtle to an absolute position in the graphics window.  The
    input is a list of two numbers, the X and Y coordinates.

COD***/
// CMD SETPOS 1 1 1 n
Value *Compiler::genSetpos(const DatumPtr &node, RequestReturnType returnType)
{
    AllocaInst *posAry = generateNumberAryFromDatum(node.astnodeValue(), node.astnodeValue()->childAtIndex(0), 2);
    generateCallExtern(TyVoid, setTurtlePos, PaAddr(posAry));
    return generateVoidRetval(node);
}
/***DOC SETHEADING SETH
SETHEADING degrees
SETH degrees

    turns the turtle to a new absolute heading.  The input is
    a number, the heading in degrees clockwise from the positive
    Y axis.

COD***/
// CMD SETHEADING 1 1 1 n
// CMD SETH 1 1 1 n
Value *Compiler::genSetheading(const DatumPtr &node, RequestReturnType returnType)
{
    Value *angle = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    generateCallExtern(TyVoid, setTurtleHeading, PaDouble(angle));
    return generateVoidRetval(node);
}
/***DOC HOME
HOME

    moves the turtle to the center of the screen.  Equivalent to
    SETPOS [0 0] SETHEADING 0.

COD***/
// CMD HOME 0 0 0 n
Value *Compiler::genHome(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setTurtleMoveToHome);
    return generateVoidRetval(node);
}
/***DOC ARC
ARC angle radius

    draws an arc of a circle, with the turtle at the center, with the
    specified radius, starting at the turtle's heading and extending
    clockwise through the specified angle.  The turtle does not move.

COD***/
// CMD ARC 2 2 2 n
Value *Compiler::genArc(const DatumPtr &node, RequestReturnType returnType)
{
    Value *angle = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *radius = generateChild(node.astnodeValue(), 1, RequestReturnReal);
    generateCallExtern(TyVoid, drawTurtleArc, PaDouble(angle), PaDouble(radius));
    return generateVoidRetval(node);
}
// TURTLE MOTION QUERIES

/***DOC POS
POS

    outputs the turtle's current position, as a list of two
    numbers, the X and Y coordinates.

COD***/
// CMD POS 0 0 0 d
Value *Compiler::genPos(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, getTurtlePos, PaAddr(evaluator));
}
/***DOC HEADING
HEADING

    outputs a number, the turtle's heading in degrees.

COD***/
// CMD HEADING 0 0 0 r
Value *Compiler::genHeading(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyDouble, getTurtleHeading);
}
/***DOC TOWARDS
TOWARDS pos

    outputs a number, the heading at which the turtle should be
    facing so that it would point from its current position to
    the position given as the input.

COD***/
// CMD TOWARDS 1 1 1 r
Value *Compiler::genTowards(const DatumPtr &node, RequestReturnType returnType)
{
    AllocaInst *posAry = generateNumberAryFromDatum(node.astnodeValue(), node.astnodeValue()->childAtIndex(0), 2);
    return generateCallExtern(TyDouble, getTurtleTowards, PaAddr(posAry));
}
/***DOC SCRUNCH
SCRUNCH

    outputs a list containing two numbers, both '1'.  This primitive is
    maintained for backward compatibility. QLogo does not use SCRUNCH.
    SCRUNCH was used by UCBLogo because older monitors had pixels with
    varying width/height proportions.


COD***/
// CMD SCRUNCH 0 0 0 d
Value *Compiler::genScrunch(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, getScrunch, PaAddr(evaluator));
}
// TURTLE AND WINDOW CONTROL

/***DOC SHOWTURTLE ST
SHOWTURTLE
ST

    makes the turtle visible.

COD***/
// CMD SHOWTURTLE 0 0 0 n
// CMD ST 0 0 0 n
Value *Compiler::genShowTurtle(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setTurtleVisible, PaInt32(CoInt32(1)));
    return generateVoidRetval(node);
}
/***DOC HIDETURTLE HT
HIDETURTLE
HT

    makes the turtle invisible.

COD***/
// CMD HIDETURTLE 0 0 0 n
// CMD HT 0 0 0 n
Value *Compiler::genHideTurtle(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setTurtleVisible, PaInt32(CoInt32(0)));
    return generateVoidRetval(node);
}

/***DOC CLEAN
CLEAN

    erases all lines that the turtle has drawn on the graphics window.
    The turtle's state (position, heading, pen mode, etc.) is not
    changed.

COD***/
// CMD CLEAN 0 0 0 n
Value *Compiler::genClean(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, clean, PaAddr(evaluator));
    return generateVoidRetval(node);
}
/***DOC CLEARSCREEN CS
CLEARSCREEN
CS

    erases the graphics window and sends the turtle to its initial
    position and heading.  Like HOME and CLEAN together.

COD***/
// CMD CLEARSCREEN 0 0 0 n
// CMD CS 0 0 0 n
Value *Compiler::genClearscreen(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setTurtleMoveToHome);
    generateCallExtern(TyVoid, clean, PaAddr(evaluator));
    return generateVoidRetval(node);
}

/***DOC WRAP
WRAP

    tells the turtle to enter wrap mode:  From now on, if the turtle
    is asked to move past the boundary of the graphics window, it
    will "wrap around" and reappear at the opposite edge of the
    window.  The top edge wraps to the bottom edge, while the left
    edge wraps to the right edge.  (So the window is topologically
    equivalent to a torus.)  This is the turtle's initial mode.
    Compare WINDOW and FENCE.

COD***/
// CMD WRAP 0 0 0 n
Value *Compiler::genWrap(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setTurtleMode, PaAddr(evaluator), PaInt32(CoInt32(turtleWrap)));
    return generateVoidRetval(node);
}
/***DOC WINDOW
WINDOW

    tells the turtle to enter adaptive mode:  From now on, if the turtle
    is asked to move past the boundary of the graphics window, the
    boundary will grow to accomodate the turtle's new position. Note
    that the lower and left boundaries are the negatives of the upper
    and right boundaries and that the origin is always in the center.
    Compare WRAP and FENCE.

COD***/
// CMD WINDOW 0 0 0 n
Value *Compiler::genWindow(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setTurtleMode, PaAddr(evaluator), PaInt32(CoInt32(turtleWindow)));
    return generateVoidRetval(node);
}

/***DOC FENCE
FENCE

    tells the turtle to enter fence mode:  From now on, if the turtle
    is asked to move past the boundary of the graphics window, it
    will move as far as it can and then stop at the edge with an
    "out of bounds" error message.  Compare WRAP and WINDOW.

COD***/
// CMD FENCE 0 0 0 n
Value *Compiler::genFence(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setTurtleMode, PaAddr(evaluator), PaInt32(CoInt32(turtleFence)));
    return generateVoidRetval(node);
}

/***DOC BOUNDS
BOUNDS

    outputs a list of two positive numbers [X,Y] giving the maximum bounds
    of the canvas. See SETBOUNDS.

COD***/
// CMD BOUNDS 0 0 0 d
Value *Compiler::genBounds(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, getBounds, PaAddr(evaluator));
}
/***DOC SETBOUNDS
SETBOUNDS x y

    sets the bounds for the canvas:  The input should be two positive
    numbers, the X-maximum, and Y-maximum. The canvas will reshape itself
    to those proportions. The drawing area is a Cartesian coordinate system
    where the origin (position 0 0) will always be in the center. The
    horizontal range will be [-x, x] while the horizontal range will be
    [-y, y].

COD***/
// CMD SETBOUNDS 2 2 2 n
Value *Compiler::genSetbounds(const DatumPtr &node, RequestReturnType returnType)
{
    Value *x = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *y = generateChild(node.astnodeValue(), 1, RequestReturnReal);
    generateCallExtern(TyVoid, setBounds, PaAddr(evaluator), PaDouble(x), PaDouble(y));
    return generateVoidRetval(node);
}
/***DOC FILLED
FILLED color instructions

    runs the instructions, remembering all points visited by turtle
    motion commands, starting *and ending* with the turtle's initial
    position.  Then draws (ignoring penmode) the resulting polygon,
    in the current pen color, filling the polygon with the given color,
    which can be a color number or an RGB list.  The instruction list
    cannot include another FILLED invocation.

COD***/
// CMD FILLED 2 2 2 n
Value *Compiler::genFilled(const DatumPtr &node, RequestReturnType returnType)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    BasicBlock *colorNotGoodBB = BasicBlock::Create(*scaff->theContext, "colorNotGood", theFunction);
    BasicBlock *colorGoodBB = BasicBlock::Create(*scaff->theContext, "colorGood", theFunction);
    Value *color = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *instructions = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    Value *isGood = generateCallExtern(TyInt32, beginFilledWithColor, PaAddr(evaluator), PaAddr(color));
    Value *isGoodCmp = scaff->builder.CreateICmpEQ(isGood, CoInt32(1), "isGood");
    scaff->builder.CreateCondBr(isGoodCmp, colorGoodBB, colorNotGoodBB);

    // Color is not good.
    scaff->builder.SetInsertPoint(colorNotGoodBB);
    Value *errVal = generateCallExtern(
        TyAddr, getErrorNoLike, PaAddr(evaluator), PaAddr(CoAddr(node.astnodeValue())), PaAddr(color));
    scaff->builder.CreateRet(errVal);

    // Color is good.
    scaff->builder.SetInsertPoint(colorGoodBB);
    Value *result = generateCallList(instructions, RequestReturnDatum);
    generateCallExtern(TyVoid, endFilled, PaAddr(evaluator));
    return result;
}

// Returns false if the color is not valid.
/***DOC LABEL
LABEL text

    takes a word, array, or list as input, and prints the input on the
    graphics window, starting at the turtle's position.

COD***/
// CMD LABEL 1 1 1 n
Value *Compiler::genLabel(const DatumPtr &node, RequestReturnType returnType)
{
    Value *text = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    generateCallExtern(TyVoid, addLabel, PaAddr(evaluator), PaAddr(text));
    return generateVoidRetval(node);
}
/***DOC SETLABELHEIGHT
SETLABELHEIGHT height

    command. Takes a positive number argument and sets the label font size.

COD***/
// CMD SETLABELHEIGHT 1 1 1 n
Value *Compiler::genSetlabelheight(const DatumPtr &node, RequestReturnType returnType)
{
    Value *height = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    generateCallExtern(TyVoid, setLabelHeight, PaAddr(evaluator), PaDouble(height));
    return generateVoidRetval(node);
}
/***DOC TEXTSCREEN TS
TEXTSCREEN
TS

    rearranges the size and position of windows to maximize the
    space available in the text window (the window used for
    interaction with Logo).  Compare SPLITSCREEN and FULLSCREEN.

COD***/
// CMD TEXTSCREEN 0 0 0 n
// CMD TS 0 0 0 n
Value *Compiler::genTextscreen(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setScreenMode, PaAddr(evaluator), PaInt32(CoInt32(textScreenMode)));
    return generateVoidRetval(node);
}
/***DOC FULLSCREEN FS
FULLSCREEN
FS

    rearranges the size and position of windows to maximize the space
    available in the graphics window.  The details differ among machines.
    Compare SPLITSCREEN and TEXTSCREEN.

    Since there must be a text window to allow printing (including the
    printing of the Logo prompt), the proportions are 75% turtle canvas and
    25% text console. This is identical to SPLITSCREEN.

COD***/
// CMD FULLSCREEN 0 0 0 n
// CMD FS 0 0 0 n
Value *Compiler::genFullscreen(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setScreenMode, PaAddr(evaluator), PaInt32(CoInt32(fullScreenMode)));
    return generateVoidRetval(node);
}

/***DOC SPLITSCREEN SS
SPLITSCREEN
SS

    rearranges the size and position of windows to allow some room for
    text interaction while also keeping most of the graphics window
    visible.  The proportions are 75% turtle canvas and 25% text console.
    Compare TEXTSCREEN and FULLSCREEN.

COD***/
// CMD SPLITSCREEN 0 0 0 n
// CMD SS 0 0 0 n
Value *Compiler::genSplitscreen(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setScreenMode, PaAddr(evaluator), PaInt32(CoInt32(splitScreenMode)));
    return generateVoidRetval(node);
}

/***DOC SETSCRUNCH
SETSCRUNCH xscale yscale

    In QLogo this does nothing. See SCRUNCH.

COD***/
// CMD SETSCRUNCH 2 2 2 n
Value *Compiler::genSetscrunch(const DatumPtr &node, RequestReturnType returnType)
{
    return generateVoidRetval(node);
}

// TURTLE AND WINDOW QUERIES

/***DOC SHOWNP SHOWN?
SHOWNP
SHOWN?

    outputs TRUE if the turtle is shown (visible), FALSE if the
    turtle is hidden.  See SHOWTURTLE and HIDETURTLE.

COD***/
// CMD SHOWNP 0 0 0 b
// CMD SHOWN? 0 0 0 b
Value *Compiler::genShownp(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyBool, isTurtleVisible, PaAddr(evaluator));
}
/***DOC SCREENMODE
SCREENMODE

    outputs the word TEXTSCREEN, SPLITSCREEN, or FULLSCREEN depending
    on the last requested screen mode.

    In QLogo, since the user is freely able to adjust the split between
    the canvas and console, this will only return the mode set by the
    last used mode command.

COD***/
// CMD SCREENMODE 0 0 0 d
Value *Compiler::genScreenmode(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, getScreenMode, PaAddr(evaluator));
}
/***DOC TURTLEMODE
TURTLEMODE

    outputs the word WRAP, FENCE, or WINDOW depending on the current
    turtle mode.

COD***/
// CMD TURTLEMODE 0 0 0 d
Value *Compiler::genTurtlemode(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, getTurtleMode, PaAddr(evaluator));
}
/***DOC LABELSIZE
LABELSIZE

    outputs the height of the label font as a list of two numbers.
    The first number is the font height, and the second is the same as the first.
    Note that UCBLogo returned a list of two numbers, representing the font
    height and width. However, the width of most fonts are variable, and so
    they are difficult to calculate. Therefore, QLogo only returns the height,
    but in the form of a list of two numbers for compatibility with UCBLogo.

COD***/
// CMD LABELSIZE 0 0 0 d
Value *Compiler::genLabelsize(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, getLabelSize, PaAddr(evaluator));
}
// PEN AND BACKGROUND CONTROL

/***DOC PENDOWN PD
PENDOWN
PD

    sets the pen's position to DOWN, without changing its mode.

COD***/
// CMD PENDOWN 0 0 0 n
// CMD PD 0 0 0 n
Value *Compiler::genPendown(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setPenIsDown, PaAddr(evaluator), PaBool(CoBool(true)));
    return generateVoidRetval(node);
}
/***DOC PENUP PU
PENUP
PU

    sets the pen's position to UP, without changing its mode.

COD***/
// CMD PENUP 0 0 0 n
// CMD PU 0 0 0 n
Value *Compiler::genPenup(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setPenIsDown, PaAddr(evaluator), PaBool(CoBool(false)));
    return generateVoidRetval(node);
}

/***DOC PENPAINT PPT
PENPAINT
PPT

    sets the pen's position to DOWN and mode to PAINT.

COD***/
// CMD PENPAINT 0 0 0 n
// CMD PPT 0 0 0 n
Value *Compiler::genPenpaint(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setPenIsDown, PaAddr(evaluator), PaBool(CoBool(true)));
    generateCallExtern(TyVoid, setPenMode, PaAddr(evaluator), PaInt32(CoInt32(static_cast<int32_t>(penModePaint))));
    return generateVoidRetval(node);
}
/***DOC PENERASE PE
PENERASE
PE

    sets the pen's position to DOWN and mode to ERASE.

COD***/
// CMD PENERASE 0 0 0 n
// CMD PE 0 0 0 n
Value *Compiler::genPenerase(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setPenIsDown, PaAddr(evaluator), PaBool(CoBool(true)));
    generateCallExtern(TyVoid, setPenMode, PaAddr(evaluator), PaInt32(CoInt32(static_cast<int32_t>(penModeErase))));
    return generateVoidRetval(node);
}

/***DOC PENREVERSE PX
PENREVERSE
PX

    sets the pen's position to DOWN and mode to REVERSE.
    The pen color value is ignored while in penreverse mode.

COD***/
// CMD PENREVERSE 0 0 0 n
// CMD PX 0 0 0 n
Value *Compiler::genPenreverse(const DatumPtr &node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, setPenIsDown, PaAddr(evaluator), PaBool(CoBool(true)));
    generateCallExtern(TyVoid, setPenMode, PaAddr(evaluator), PaInt32(CoInt32(static_cast<int32_t>(penModeReverse))));
    return generateVoidRetval(node);
}

/***DOC SETPENCOLOR SETPC
SETPENCOLOR color
SETPC color

    sets the pen color to the given color, which must be one of the following:

    Option 1: a nonnegative integer.  There are initial assignments for the
    first 16 colors:

     0  black    1  blue         2  green        3  cyan
     4  red      5  magenta      6  yellow       7 white
     8  brown    9  tan         10  forest      11  aqua
    12  salmon  13  purple      14  orange      15  grey

    but other colors can be assigned to numbers by the SETPALETTE command.

    Option 2: RGB values (a list of three numbers between 0 and 100
    specifying the percent saturation of red, green, and blue in the desired
    color).

    Option 3: RGBA values (a list of four numbers between 0 and 100
    specifying the percent saturation of red, green, blue, and alpha).
    This is the only way to specify the alpha component.

    Option 4: a named color from the X Color Database, e.g. "white or
    "lemonchiffon. The list of color names can be retrieved using the
    ALLCOLORS command or from the X Color database found here:
    https://en.wikipedia.org/wiki/X11_color_names

    Option 5: a hex triplet preceded by a '#'. Each component may contain
    one to four hex digits. Each of the following produces the color red:
    "#f00 "#ff0000 "#fff000000 and "#ffff00000000

COD***/
// CMD SETPENCOLOR 1 1 1 n
// CMD SETPC 1 1 1 n
Value *Compiler::genSetpencolor(const DatumPtr &node, RequestReturnType returnType)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    BasicBlock *colorNotGoodBB = BasicBlock::Create(*scaff->theContext, "colorNotGood", theFunction);
    BasicBlock *colorGoodBB = BasicBlock::Create(*scaff->theContext, "colorGood", theFunction);
    Value *color = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *isGood = generateCallExtern(TyBool, setPenColor, PaAddr(evaluator), PaAddr(color));
    Value *isGoodCmp = scaff->builder.CreateICmpEQ(isGood, CoBool(true), "isGood");
    scaff->builder.CreateCondBr(isGoodCmp, colorGoodBB, colorNotGoodBB);

    // Color is not good.
    scaff->builder.SetInsertPoint(colorNotGoodBB);
    Value *errVal = generateCallExtern(
        TyAddr, getErrorNoLike, PaAddr(evaluator), PaAddr(CoAddr(node.astnodeValue())), PaAddr(color));
    scaff->builder.CreateRet(errVal);

    // Color is good.
    scaff->builder.SetInsertPoint(colorGoodBB);
    return generateVoidRetval(node);
}
/***DOC ALLCOLORS
ALLCOLORS

    returns a list of all of the color names that QLogo knows about.

COD***/
// CMD ALLCOLORS 0 0 0 d
Value *Compiler::genAllcolors(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, getAllColors, PaAddr(evaluator));
}
/***DOC SETPALETTE
SETPALETTE colornumber color

    sets the actual color corresponding to a given number, if allowed by
    the hardware and operating system.  Colornumber must be an integer
    greater than or equal to 8.  (Logo tries to keep the first 8 colors
    constant.)  The second input is a color. See SETPENCOLOR for different
    methods of specifying a color.

COD***/
// CMD SETPALETTE 2 2 2 n
Value *Compiler::genSetpalette(const DatumPtr &node, RequestReturnType returnType)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    Value *colorIndex = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *isColorIndexGoodResult =
        generateCallExtern(TyBool, isColorIndexGood, PaAddr(evaluator), PaAddr(colorIndex), PaDouble(CoDouble(8.0)));
    Value *isColorIndexGoodCmp = scaff->builder.CreateICmpEQ(isColorIndexGoodResult, CoBool(true), "isColorIndexGood");
    BasicBlock *colorIndexNotGoodBB = BasicBlock::Create(*scaff->theContext, "colorIndexNotGood", theFunction);
    BasicBlock *colorIndexGoodBB = BasicBlock::Create(*scaff->theContext, "colorIndexGood", theFunction);
    BasicBlock *colorNotGoodBB = BasicBlock::Create(*scaff->theContext, "colorNotGood", theFunction);
    BasicBlock *colorGoodBB = BasicBlock::Create(*scaff->theContext, "colorGood", theFunction);
    scaff->builder.CreateCondBr(isColorIndexGoodCmp, colorIndexGoodBB, colorIndexNotGoodBB);

    // Color index is not good.
    scaff->builder.SetInsertPoint(colorIndexNotGoodBB);
    Value *errVal = generateCallExtern(
        TyAddr, getErrorNoLike, PaAddr(evaluator), PaAddr(CoAddr(node.astnodeValue())), PaAddr(colorIndex));
    scaff->builder.CreateRet(errVal);

    // Color index is good.
    scaff->builder.SetInsertPoint(colorIndexGoodBB);
    Value *color = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    Value *colorIsGood = generateCallExtern(TyBool, setPalette, PaAddr(evaluator), PaAddr(colorIndex), PaAddr(color));
    Value *colorIsGoodCmp = scaff->builder.CreateICmpEQ(colorIsGood, CoBool(true), "colorIsGood");
    scaff->builder.CreateCondBr(colorIsGoodCmp, colorGoodBB, colorNotGoodBB);

    // Color is not good.
    scaff->builder.SetInsertPoint(colorNotGoodBB);
    errVal = generateCallExtern(
        TyAddr, getErrorNoLike, PaAddr(evaluator), PaAddr(CoAddr(node.astnodeValue())), PaAddr(color));
    scaff->builder.CreateRet(errVal);

    // Color is good.
    scaff->builder.SetInsertPoint(colorGoodBB);
    return generateVoidRetval(node);
}
/***DOC SETPENSIZE
SETPENSIZE size

    sets the thickness of the pen.  The input is a single positive
    integer.

COD***/
// CMD SETPENSIZE 1 1 1 n
Value *Compiler::genSetpensize(const DatumPtr &node, RequestReturnType returnType)
{
    Value *size = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    size = generateGTZeroFromDouble(node.astnodeValue(), size);
    generateCallExtern(TyVoid, setPenSize, PaAddr(evaluator), PaAddr(size));
    return generateVoidRetval(node);
}
/***DOC SETBACKGROUND SETBG
SETBACKGROUND color
SETBG color

    set the screen background color. See SETPENCOLOR for color details.


COD***/
// CMD SETBACKGROUND 1 1 1 n
// CMD SETBG 1 1 1 n
Value *Compiler::genSetbackground(const DatumPtr &node, RequestReturnType returnType)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    Value *color = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *isGood = generateCallExtern(TyBool, setBackground, PaAddr(evaluator), PaAddr(color));
    Value *isGoodCmp = scaff->builder.CreateICmpEQ(isGood, CoBool(true), "isGood");
    BasicBlock *colorNotGoodBB = BasicBlock::Create(*scaff->theContext, "colorNotGood", theFunction);
    BasicBlock *colorGoodBB = BasicBlock::Create(*scaff->theContext, "colorGood", theFunction);
    scaff->builder.CreateCondBr(isGoodCmp, colorGoodBB, colorNotGoodBB);

    // Color is not good.
    scaff->builder.SetInsertPoint(colorNotGoodBB);
    Value *errVal = generateCallExtern(
        TyAddr, getErrorNoLike, PaAddr(evaluator), PaAddr(CoAddr(node.astnodeValue())), PaAddr(color));
    scaff->builder.CreateRet(errVal);

    // Color is good.
    scaff->builder.SetInsertPoint(colorGoodBB);
    return generateVoidRetval(node);
}
// PEN QUERIES

/***DOC PENDOWNP PENDOWN?
PENDOWNP
PENDOWN?

    outputs TRUE if the pen is down, FALSE if it's up.

COD***/
// CMD PENDOWNP 0 0 0 b
// CMD PENDOWN? 0 0 0 b
Value *Compiler::genPendownp(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyBool, isPenDown, PaAddr(evaluator));
}
/***DOC PENMODE
PENMODE

    outputs one of the words PAINT, ERASE, or REVERSE according to
    the current pen mode.

COD***/
// CMD PENMODE 0 0 0 d
Value *Compiler::genPenmode(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, getPenMode, PaAddr(evaluator));
}
/***DOC PENCOLOR PC
PENCOLOR
PC

    outputs a list of three nonnegative numbers less than 100 specifying
    the percent saturation of red, green, and blue in the color associated
    with the current pen color.

COD***/
// CMD PENCOLOR 0 0 0 d
// CMD PC 0 0 0 d
Value *Compiler::genPencolor(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, getPenColor, PaAddr(evaluator));
}
/***DOC PALETTE
PALETTE colornumber

    outputs a list of three nonnegative numbers less than 100 specifying
    the percent saturation of red, green, and blue in the color associated
    with the given number.

COD***/
// CMD PALETTE 1 1 1 d
Value *Compiler::genPalette(const DatumPtr &node, RequestReturnType returnType)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    Value *colorIndex = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *isColorIndexGood =
        generateCallExtern(TyBool, isColorIndexGood, PaAddr(evaluator), PaAddr(colorIndex), PaDouble(CoDouble(0.0)));
    Value *isColorIndexGoodCmp = scaff->builder.CreateICmpEQ(isColorIndexGood, CoBool(true), "isColorIndexGood");
    BasicBlock *colorIndexNotGoodBB = BasicBlock::Create(*scaff->theContext, "colorIndexNotGood", theFunction);
    BasicBlock *colorIndexGoodBB = BasicBlock::Create(*scaff->theContext, "colorIndexGood", theFunction);
    scaff->builder.CreateCondBr(isColorIndexGoodCmp, colorIndexGoodBB, colorIndexNotGoodBB);

    // Color index is not good.
    scaff->builder.SetInsertPoint(colorIndexNotGoodBB);
    Value *errVal = generateCallExtern(
        TyAddr, getErrorNoLike, PaAddr(evaluator), PaAddr(CoAddr(node.astnodeValue())), PaAddr(colorIndex));
    scaff->builder.CreateRet(errVal);

    // Color index is good.
    scaff->builder.SetInsertPoint(colorIndexGoodBB);
    Value *color = generateCallExtern(TyAddr, getPaletteColor, PaAddr(evaluator), PaAddr(colorIndex));
    return color;
}
/***DOC PENSIZE
PENSIZE


    outputs a positive integer, specifying the thickness of the turtle pen.

COD***/
// CMD PENSIZE 0 0 0 r
Value *Compiler::genPensize(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyDouble, getPenSize, PaAddr(evaluator));
}
/***DOC BACKGROUND BG
BACKGROUND
BG

    outputs a list of three nonnegative numbers less than 100 specifying
    the percent saturation of red, green, and blue in the color associated
    with the current background color.


COD***/
// CMD BACKGROUND 0 0 0 d
// CMD BG 0 0 0 d
Value *Compiler::genBackground(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, getBackground, PaAddr(evaluator));
}
// SAVING AND LOADING PICTURES

/***DOC SAVEPICT
SAVEPICT filename

    command.  Writes a file with the specified name containing the
    contents of the graphics window, in the format determined by the filename's
    extension. The dimensions of the image are determined by the canvas bounds.
    See SVGPICT to export Logo graphics as SVG.

COD***/
// CMD SAVEPICT 1 1 1 n
Value *Compiler::genSavepict(const DatumPtr &node, RequestReturnType returnType)
{
    Value *filename = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    return generateCallExtern(
        TyAddr, savePict, PaAddr(evaluator), PaAddr(filename), PaAddr(CoAddr(node.astnodeValue())));
}
/***DOC SVGPICT
SVGPICT filename

    command.  Writes a file with the specified name containing the
    contents of the graphics window in SVG format. The dimensions of the image
    are determined by the canvas bounds.

COD***/
// CMD SVGPICT 1 1 1 n
Value *Compiler::genSvgpict(const DatumPtr &node, RequestReturnType returnType)
{
    Value *filename = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    return generateCallExtern(
        TyAddr, saveSvgpict, PaAddr(evaluator), PaAddr(filename), PaAddr(CoAddr(node.astnodeValue())));
}
/***DOC LOADPICT
LOADPICT filename

    command.  Reads the image file with the specified filename and sets the image
    as the canvas background. The image will be stretched, if necessary, to fit
    the bounds of the canvas.

    The filename may also be an empty list, in which case any image previously
    set as the background will be cleared.

COD***/
// CMD LOADPICT 1 1 1 n
Value *Compiler::genLoadpict(const DatumPtr &node, RequestReturnType returnType)
{
    Value *filename = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    return generateCallExtern(
        TyAddr, loadPict, PaAddr(evaluator), PaAddr(filename), PaAddr(CoAddr(node.astnodeValue())));
}
// MOUSE QUERIES

/***DOC MOUSEPOS
MOUSEPOS

    outputs the coordinates of the mouse, provided that it's within the
    graphics window, in turtle coordinates.  If the mouse is outside the
    graphics window, then the last position within the window is returned.
    Exception:  If a mouse button is pressed within the graphics window
    and held while the mouse is dragged outside the window, the mouse's
    position is returned as if the window were big enough to include it.

COD***/
// CMD MOUSEPOS 0 0 0 d
Value *Compiler::genMousepos(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, getMousePos, PaAddr(evaluator));
}
/***DOC CLICKPOS
CLICKPOS

    outputs the coordinates that the mouse was at when a mouse button
    was most recently pushed, provided that that position was within the
    graphics window, in turtle coordinates.

COD***/
// CMD CLICKPOS 0 0 0 d
Value *Compiler::genClickpos(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, getClickPos, PaAddr(evaluator));
}
/***DOC BUTTONP BUTTON?
BUTTONP
BUTTON?

    outputs TRUE if a mouse button is down and the mouse is over the
    graphics window.  Once the button is down, BUTTONP remains true until
    the button is released, even if the mouse is dragged out of the
    graphics window.

COD***/
// CMD BUTTONP 0 0 0 b
// CMD BUTTON? 0 0 0 b
Value *Compiler::genButtonp(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyBool, isMouseButtonDown, PaAddr(evaluator));
}
/***DOC BUTTON
BUTTON

    outputs 0 if no mouse button has been pushed inside the Logo window
    since the last call to BUTTON.  Otherwise, it outputs an integer
    indicating which button was most recently pressed.
    1 means left, 2 means right.



COD***/
// CMD BUTTON 0 0 0 r
Value *Compiler::genButton(const DatumPtr &node, RequestReturnType returnType)
{
    return generateCallExtern(TyDouble, getMouseButton, PaAddr(evaluator));
}
