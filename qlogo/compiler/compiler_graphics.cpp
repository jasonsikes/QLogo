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

#include "compiler_private.h"
#include "turtle.h"
#include "astnode.h"
#include "compiler.h"
#include "workspace/callframe.h"
#include "kernel.h"
#include <QFile>
using namespace llvm;
using namespace llvm::orc;


List* listFromColor(const QColor &c)
{
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(round(c.redF() * 100)));
    retvalBuilder.append(DatumPtr(round(c.greenF() * 100)));
    retvalBuilder.append(DatumPtr(round(c.blueF() * 100)));
    return retvalBuilder.finishedList().listValue();
}

// TURTLE MOTION

/***DOC FORWARD FD
FORWARD dist
FD dist

    moves the turtle forward, in the direction that it's facing, by
    the specified distance (measured in turtle steps).

COD***/
// CMD FORWARD 1 1 1 n
// CMD FD 1 1 1 n
Value *Compiler::genForward(DatumPtr node, RequestReturnType returnType)
{
    Value *distance = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    generateCallExtern(TyVoid, "moveTurtleForward", {PaAddr(evaluator), PaDouble(distance)});
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
Value *Compiler::genBack(DatumPtr node, RequestReturnType returnType)
{
    Value *reverseDistance = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *distance = scaff->builder.CreateFNeg(reverseDistance, "negativeDistance");
    generateCallExtern(TyVoid, "moveTurtleForward", {PaAddr(evaluator), PaDouble(distance)});
    return generateVoidRetval(node);
}

EXPORTC void moveTurtleForward(addr_t eAddr, double distance)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainTurtle()->forward(distance);
}


/***DOC LEFT LT
LEFT degrees
LT degrees

    turns the turtle counterclockwise by the specified angle, measured
    in degrees (1/360 of a circle).

COD***/
// CMD LEFT 1 1 1 n
// CMD LT 1 1 1 n
Value *Compiler::genLeft(DatumPtr node, RequestReturnType returnType)
{
    Value *angle = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *negativeAngle = scaff->builder.CreateFNeg(angle, "negativeAngle");
    generateCallExtern(TyVoid, "moveTurtleRotate", {PaAddr(evaluator), PaDouble(negativeAngle)});
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
Value *Compiler::genRight(DatumPtr node, RequestReturnType returnType)
{
    Value *angle = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    generateCallExtern(TyVoid, "moveTurtleRotate", {PaAddr(evaluator), PaDouble(angle)});
    return generateVoidRetval(node);
}


EXPORTC void moveTurtleRotate(addr_t eAddr, double angle)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainTurtle()->rotate(angle);
}


/***DOC SETXY
SETXY xcor ycor

    moves the turtle to an absolute position in the graphics window.  The
    two inputs are numbers, the X and Y coordinates.

COD***/
// CMD SETXY 2 2 2 n
Value *Compiler::genSetxy(DatumPtr node, RequestReturnType returnType)
{
    Value *x = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *y = generateChild(node.astnodeValue(), 1, RequestReturnReal);
    generateCallExtern(TyVoid, "setTurtleXY", {PaAddr(evaluator), PaDouble(x), PaDouble(y)});
    return generateVoidRetval(node);
}

EXPORTC void setTurtleXY(addr_t eAddr, double x, double y)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainTurtle()->setxy(x, y);
}

/***DOC SETX
SETX xcor

    moves the turtle horizontally from its old position to a new
    absolute horizontal coordinate.  The input is the new X
    coordinate.

COD***/
// CMD SETX 1 1 1 n
Value *Compiler::genSetx(DatumPtr node, RequestReturnType returnType)
{
    Value *x = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    generateCallExtern(TyVoid, "setTurtleX", {PaAddr(evaluator), PaDouble(x)});
    return generateVoidRetval(node);
}

EXPORTC void setTurtleX(addr_t eAddr, double x)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainTurtle()->setx(x);
}

/***DOC SETY
SETY ycor

    moves the turtle vertically from its old position to a new
    absolute vertical coordinate.  The input is the new Y
    coordinate.

COD***/
// CMD SETY 1 1 1 n
Value *Compiler::genSety(DatumPtr node, RequestReturnType returnType)
{
    Value *y = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    generateCallExtern(TyVoid, "setTurtleY", {PaAddr(evaluator), PaDouble(y)});
    return generateVoidRetval(node);
}

EXPORTC void setTurtleY(addr_t eAddr, double y)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainTurtle()->sety(y);
}


/***DOC SETPOS
SETPOS pos

    moves the turtle to an absolute position in the graphics window.  The
    input is a list of two numbers, the X and Y coordinates.

COD***/
// CMD SETPOS 1 1 1 n
Value *Compiler::genSetpos(DatumPtr node, RequestReturnType returnType)
{
    AllocaInst *posAry = generateNumberAryFromDatum(node.astnodeValue(), node.astnodeValue()->childAtIndex(0), 2);
    generateCallExtern(TyVoid, "setTurtlePos", {PaAddr(evaluator), PaAddr(posAry)});
    return generateVoidRetval(node);
}

EXPORTC void setTurtlePos(addr_t eAddr, addr_t posAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    double *pos = reinterpret_cast<double *>(posAddr);
    double x = pos[0];
    double y = pos[1];
    Config::get().mainTurtle()->setxy(x, y);
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
Value *Compiler::genSetheading(DatumPtr node, RequestReturnType returnType)
{
    Value *angle = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    generateCallExtern(TyVoid, "setTurtleHeading", {PaAddr(evaluator), PaDouble(angle)});
    return generateVoidRetval(node);
}

EXPORTC void setTurtleHeading(addr_t eAddr, double newHeading)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    double oldHeading = Config::get().mainTurtle()->getHeading();

    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    newHeading = 360 - newHeading;

    double adjustment = oldHeading - newHeading;
    Config::get().mainTurtle()->rotate(adjustment);
}



/***DOC HOME
HOME

    moves the turtle to the center of the screen.  Equivalent to
    SETPOS [0 0] SETHEADING 0.

COD***/
// CMD HOME 0 0 0 n
Value *Compiler::genHome(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setTurtleMoveToHome", {PaAddr(evaluator)});
    return generateVoidRetval(node);
}

EXPORTC void setTurtleMoveToHome(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainTurtle()->moveToHome();
}



/***DOC ARC
ARC angle radius

    draws an arc of a circle, with the turtle at the center, with the
    specified radius, starting at the turtle's heading and extending
    clockwise through the specified angle.  The turtle does not move.

COD***/
// CMD ARC 2 2 2 n
Value *Compiler::genArc(DatumPtr node, RequestReturnType returnType)
{
    Value *angle = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *radius = generateChild(node.astnodeValue(), 1, RequestReturnReal);
    generateCallExtern(TyVoid, "drawTurtleArc", {PaAddr(evaluator), PaDouble(angle), PaDouble(radius)});
    return generateVoidRetval(node);
}

EXPORTC void drawTurtleArc(addr_t eAddr, double angle, double radius)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    angle = 0 - angle;

    if ((angle < -360) || (angle > 360))
        angle = 360;

    if ((angle != 0) && (radius != 0))
        Config::get().mainTurtle()->drawArc(angle, radius);
}


// TURTLE MOTION QUERIES

/***DOC POS
POS

    outputs the turtle's current position, as a list of two
    numbers, the X and Y coordinates.

COD***/
// CMD POS 0 0 0 d
Value *Compiler::genPos(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, "getTurtlePos", {PaAddr(evaluator)});
}

EXPORTC addr_t getTurtlePos(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    double x = 0, y = 0;
    Config::get().mainTurtle()->getxy(x, y);
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(x));
    retvalBuilder.append(DatumPtr(y));
    Datum* retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t >(retval);
}



/***DOC HEADING
HEADING

    outputs a number, the turtle's heading in degrees.

COD***/
// CMD HEADING 0 0 0 r
Value *Compiler::genHeading(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyDouble, "getTurtleHeading", {PaAddr(evaluator)});
}

EXPORTC double getTurtleHeading(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    double retval = Config::get().mainTurtle()->getHeading();

    // Heading should only show two decimal places.
    retval = round(retval * 100.0) / 100.0;

    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    if (retval > 0)
        retval = 360 - retval;

    return retval;
}



/***DOC TOWARDS
TOWARDS pos

    outputs a number, the heading at which the turtle should be
    facing so that it would point from its current position to
    the position given as the input.

COD***/
// CMD TOWARDS 1 1 1 r
Value *Compiler::genTowards(DatumPtr node, RequestReturnType returnType)
{
    AllocaInst *posAry = generateNumberAryFromDatum(node.astnodeValue(), node.astnodeValue()->childAtIndex(0), 2);
    return generateCallExtern(TyDouble, "getTurtleTowards", {PaAddr(evaluator), PaAddr(posAry)});
}

EXPORTC double getTurtleTowards(addr_t eAddr, addr_t posAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    double x = 0, y = 0;
    Config::get().mainTurtle()->getxy(x, y);
    double *pos = reinterpret_cast<double *>(posAddr);
    double vx = pos[0];
    double vy = pos[1];
    double retval = atan2(x - vx, vy - y) * (180 / M_PI);

    // Heading should only show two decimal places.
    retval = round(retval * 100.0) / 100.0;

    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    retval = 0 - retval;
    if (retval < 0)
        retval = 360 + retval;

    return retval;
}


/***DOC SCRUNCH
SCRUNCH

    outputs a list containing two numbers, both '1'.  This primitive is
    maintained for backward compatibility. QLogo does not use SCRUNCH.
    SCRUNCH was used by UCBLogo because older monitors had pixels with
    varying width/height proportions.


COD***/
// CMD SCRUNCH 0 0 0 d
Value *Compiler::genScrunch(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, "getScrunch", {PaAddr(evaluator)});
}

EXPORTC addr_t getScrunch(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(1));
    retvalBuilder.append(DatumPtr(1));
    Datum* retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t >(retval);
}

// TURTLE AND WINDOW CONTROL

/***DOC SHOWTURTLE ST
SHOWTURTLE
ST

    makes the turtle visible.

COD***/
// CMD SHOWTURTLE 0 0 0 n
// CMD ST 0 0 0 n
Value *Compiler::genShowTurtle(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setTurtleVisible", {PaAddr(evaluator), PaInt32(CoInt32(1))});
    return generateVoidRetval(node);
}

EXPORTC void setTurtleVisible(addr_t eAddr, int visible)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainTurtle()->setIsTurtleVisible(visible);
}

/***DOC HIDETURTLE HT
HIDETURTLE
HT

    makes the turtle invisible.

COD***/
// CMD HIDETURTLE 0 0 0 n
// CMD HT 0 0 0 n
Value *Compiler::genHideTurtle(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setTurtleVisible", {PaAddr(evaluator), PaInt32(CoInt32(0))});
    return generateVoidRetval(node);
}



/***DOC CLEAN
CLEAN

    erases all lines that the turtle has drawn on the graphics window.
    The turtle's state (position, heading, pen mode, etc.) is not
    changed.

COD***/
// CMD CLEAN 0 0 0 n
Value *Compiler::genClean(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "clean", {PaAddr(evaluator)});
    return generateVoidRetval(node);
}

EXPORTC void clean(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainController()->clearCanvas();
}



/***DOC CLEARSCREEN CS
CLEARSCREEN
CS

    erases the graphics window and sends the turtle to its initial
    position and heading.  Like HOME and CLEAN together.

COD***/
// CMD CLEARSCREEN 0 0 0 n
// CMD CS 0 0 0 n
Value *Compiler::genClearscreen(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setTurtleMoveToHome", {PaAddr(evaluator)});
    generateCallExtern(TyVoid, "clean", {PaAddr(evaluator)});
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
Value *Compiler::genWrap(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setTurtleMode", {PaAddr(evaluator), PaInt32(CoInt32(turtleWrap))});
    return generateVoidRetval(node);
}

EXPORTC void setTurtleMode(addr_t eAddr, int mode)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    TurtleModeEnum newMode = static_cast<TurtleModeEnum>(mode);
    if (Config::get().mainTurtle()->getMode() != newMode)
    {
        bool isCanvasBounded = (newMode == turtleWindow);
        Config::get().mainTurtle()->setMode(newMode);
        Config::get().mainController()->setIsCanvasBounded(isCanvasBounded);
    }
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
Value *Compiler::genWindow(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setTurtleMode", {PaAddr(evaluator), PaInt32(CoInt32(turtleWindow))});
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
Value *Compiler::genFence(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setTurtleMode", {PaAddr(evaluator), PaInt32(CoInt32(turtleFence))});
    return generateVoidRetval(node);
}



/***DOC BOUNDS
BOUNDS

    outputs a list of two positive numbers [X,Y] giving the maximum bounds
    of the canvas. See SETBOUNDS.

COD***/
// CMD BOUNDS 0 0 0 d
Value *Compiler::genBounds(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, "getBounds", {PaAddr(evaluator)});
}

EXPORTC addr_t getBounds(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    double x = Config::get().mainController()->boundX();
    double y = Config::get().mainController()->boundY();

    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(x));
    retvalBuilder.append(DatumPtr(y));
    Datum* retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t >(retval);
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
Value *Compiler::genSetbounds(DatumPtr node, RequestReturnType returnType)
{
    Value *x = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    Value *y = generateChild(node.astnodeValue(), 1, RequestReturnReal);
    generateCallExtern(TyVoid, "setBounds", {PaAddr(evaluator), PaDouble(x), PaDouble(y)});
    return generateVoidRetval(node);
}

EXPORTC void setBounds(addr_t eAddr, double x, double y)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainController()->setBounds(x, y);
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
Value *Compiler::genFilled(DatumPtr node, RequestReturnType returnType)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    BasicBlock *colorNotGoodBB = BasicBlock::Create(*scaff->theContext, "colorNotGood", theFunction);
    BasicBlock *colorGoodBB = BasicBlock::Create(*scaff->theContext, "colorGood", theFunction);
    Value *color = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *instructions = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    Value *isGood = generateCallExtern(TyInt32, "beginFilledWithColor", {PaAddr(evaluator), PaAddr(color)});
    Value *isGoodCmp = scaff->builder.CreateICmpEQ(isGood, CoInt32(1), "isGood");
    scaff->builder.CreateCondBr(isGoodCmp, colorGoodBB, colorNotGoodBB);

    // Color is not good.
    scaff->builder.SetInsertPoint(colorNotGoodBB);
    Value *errVal = generateCallExtern(TyAddr, "getErrorNoLike", {PaAddr(evaluator), PaAddr(CoAddr(node.astnodeValue())), PaAddr(color)});
    scaff->builder.CreateRet(errVal);

    // Color is good.
    scaff->builder.SetInsertPoint(colorGoodBB);
    Value *result = generateCallList(instructions, RequestReturnDatum);
    generateCallExtern(TyVoid, "endFilled", {PaAddr(evaluator)});
    return result;
}

// Returns false if the color is not valid.
EXPORTC int32_t beginFilledWithColor(addr_t eAddr, addr_t colorAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Config::get().mainKernel()->colorFromDatumPtr(color, DatumPtr(d)))
        return 0;
    Config::get().mainTurtle()->beginFillWithColor(color);
    return 1;
}

EXPORTC void endFilled(addr_t eAddr)
{
    Config::get().mainTurtle()->endFill();
}


/***DOC LABEL
LABEL text

    takes a word, array, or list as input, and prints the input on the
    graphics window, starting at the turtle's position.

COD***/
// CMD LABEL 1 1 1 n
Value *Compiler::genLabel(DatumPtr node, RequestReturnType returnType)
{
    Value *text = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    generateCallExtern(TyVoid, "addLabel", {PaAddr(evaluator), PaAddr(text)});
    return generateVoidRetval(node);
}

EXPORTC void addLabel(addr_t eAddr, addr_t textAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *d = reinterpret_cast<Datum *>(textAddr);
    Config::get().mainController()->drawLabel(d->printValue());
}


/***DOC SETLABELHEIGHT
SETLABELHEIGHT height

    command. Takes a positive number argument and sets the label font size.

COD***/
// CMD SETLABELHEIGHT 1 1 1 n
Value *Compiler::genSetlabelheight(DatumPtr node, RequestReturnType returnType)
{
    Value *height = generateChild(node.astnodeValue(), 0, RequestReturnReal);
    generateCallExtern(TyVoid, "setLabelHeight", {PaAddr(evaluator), PaDouble(height)});
    return generateVoidRetval(node);
}

EXPORTC void setLabelHeight(addr_t eAddr, double height)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainController()->setLabelFontSize(height);
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
Value *Compiler::genTextscreen(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setScreenMode", {PaAddr(evaluator), PaInt32(CoInt32(textScreenMode))});
    return generateVoidRetval(node);
}

EXPORTC void setScreenMode(addr_t eAddr, int mode)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainController()->setScreenMode(static_cast<ScreenModeEnum>(mode));
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
Value *Compiler::genFullscreen(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setScreenMode", {PaAddr(evaluator), PaInt32(CoInt32(fullScreenMode))});
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
Value *Compiler::genSplitscreen(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setScreenMode", {PaAddr(evaluator), PaInt32(CoInt32(splitScreenMode))});
    return generateVoidRetval(node);
}



/***DOC SETSCRUNCH
SETSCRUNCH xscale yscale

    In QLogo this does nothing. See SCRUNCH.

COD***/
// CMD SETSCRUNCH 2 2 2 n
Value *Compiler::genSetscrunch(DatumPtr node, RequestReturnType returnType)
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
Value *Compiler::genShownp(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyBool, "isTurtleVisible", {PaAddr(evaluator)});
}

EXPORTC bool isTurtleVisible(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    return Config::get().mainTurtle()->isTurtleVisible();
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
Value *Compiler::genScreenmode(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, "getScreenMode", {PaAddr(evaluator)});
}

EXPORTC addr_t getScreenMode(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    ScreenModeEnum mode = Config::get().mainController()->getScreenMode();
    QString modeStr;
    switch (mode) {
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
    Word *retval = new Word(modeStr);
    e->watch(retval);
    return reinterpret_cast<addr_t >(retval);
}




/***DOC TURTLEMODE
TURTLEMODE

    outputs the word WRAP, FENCE, or WINDOW depending on the current
    turtle mode.

COD***/
// CMD TURTLEMODE 0 0 0 d
Value *Compiler::genTurtlemode(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, "getTurtleMode", {PaAddr(evaluator)});
}

EXPORTC addr_t getTurtleMode(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    TurtleModeEnum mode = Config::get().mainTurtle()->getMode();
    QString modeStr;
    switch (mode) {
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
    Word *retval = new Word(modeStr);
    e->watch(retval);
    return reinterpret_cast<addr_t >(retval);
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
Value *Compiler::genLabelsize(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, "getLabelSize", {PaAddr(evaluator)});
}

EXPORTC addr_t getLabelSize(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    double height = Config::get().mainController()->getLabelFontSize();
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(height));
    retvalBuilder.append(DatumPtr(height));
    Datum* retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t >(retval);
}



// PEN AND BACKGROUND CONTROL

/***DOC PENDOWN PD
PENDOWN
PD

    sets the pen's position to DOWN, without changing its mode.

COD***/
// CMD PENDOWN 0 0 0 n
// CMD PD 0 0 0 n
Value *Compiler::genPendown(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setPenIsDown", {PaAddr(evaluator), PaBool(CoBool(true))});
    return generateVoidRetval(node);
}

EXPORTC void setPenIsDown(addr_t eAddr, bool isDown)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainTurtle()->setPenIsDown(isDown);
}


/***DOC PENUP PU
PENUP
PU

    sets the pen's position to UP, without changing its mode.

COD***/
// CMD PENUP 0 0 0 n
// CMD PU 0 0 0 n
Value *Compiler::genPenup(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setPenIsDown", {PaAddr(evaluator), PaBool(CoBool(false))});
    return generateVoidRetval(node);
}


/***DOC PENPAINT PPT
PENPAINT
PPT

    sets the pen's position to DOWN and mode to PAINT.

COD***/
// CMD PENPAINT 0 0 0 n
// CMD PPT 0 0 0 n
Value *Compiler::genPenpaint(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setPenIsDown", {PaAddr(evaluator), PaBool(CoBool(true))});
    generateCallExtern(TyVoid, "setPenMode", {PaAddr(evaluator), PaInt32(CoInt32(static_cast<int32_t>(penModePaint)))});
    return generateVoidRetval(node);
}

EXPORTC void setPenMode(addr_t eAddr, int32_t mode)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainTurtle()->setPenMode(static_cast<PenModeEnum>(mode));
}


/***DOC PENERASE PE
PENERASE
PE

    sets the pen's position to DOWN and mode to ERASE.

COD***/
// CMD PENERASE 0 0 0 n
// CMD PE 0 0 0 n
Value *Compiler::genPenerase(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setPenIsDown", {PaAddr(evaluator), PaBool(CoBool(true))});
    generateCallExtern(TyVoid, "setPenMode", {PaAddr(evaluator), PaInt32(CoInt32(static_cast<int32_t>(penModeErase)))});
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
Value *Compiler::genPenreverse(DatumPtr node, RequestReturnType returnType)
{
    generateCallExtern(TyVoid, "setPenIsDown", {PaAddr(evaluator), PaBool(CoBool(true))});
    generateCallExtern(TyVoid, "setPenMode", {PaAddr(evaluator), PaInt32(CoInt32(static_cast<int32_t>(penModeReverse)))});
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
Value *Compiler::genSetpencolor(DatumPtr node, RequestReturnType returnType)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    BasicBlock *colorNotGoodBB = BasicBlock::Create(*scaff->theContext, "colorNotGood", theFunction);
    BasicBlock *colorGoodBB = BasicBlock::Create(*scaff->theContext, "colorGood", theFunction);
    Value *color = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *isGood = generateCallExtern(TyBool, "setPenColor", {PaAddr(evaluator), PaAddr(color)});
    Value *isGoodCmp = scaff->builder.CreateICmpEQ(isGood, CoBool(true), "isGood");
    scaff->builder.CreateCondBr(isGoodCmp, colorGoodBB, colorNotGoodBB);

    // Color is not good.
    scaff->builder.SetInsertPoint(colorNotGoodBB);
    Value *errVal = generateCallExtern(TyAddr, "getErrorNoLike", {PaAddr(evaluator), PaAddr(CoAddr(node.astnodeValue())), PaAddr(color)});
    scaff->builder.CreateRet(errVal);

    // Color is good.
    scaff->builder.SetInsertPoint(colorGoodBB);
    return generateVoidRetval(node);
}

EXPORTC bool setPenColor(addr_t eAddr, addr_t colorAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Config::get().mainKernel()->colorFromDatumPtr(color, DatumPtr(d)))
        return false;
    Config::get().mainTurtle()->setPenColor(color);
    return true;
}


/***DOC ALLCOLORS
ALLCOLORS

    returns a list of all of the color names that QLogo knows about.

COD***/
// CMD ALLCOLORS 0 0 0 d
Value *Compiler::genAllcolors(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, "getAllColors", {PaAddr(evaluator)});
}


EXPORTC addr_t getAllColors(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
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


/***DOC SETPALETTE
SETPALETTE colornumber color

    sets the actual color corresponding to a given number, if allowed by
    the hardware and operating system.  Colornumber must be an integer
    greater than or equal to 8.  (Logo tries to keep the first 8 colors
    constant.)  The second input is a color. See SETPENCOLOR for different
    methods of specifying a color.

COD***/
// CMD SETPALETTE 2 2 2 n
Value *Compiler::genSetpalette(DatumPtr node, RequestReturnType returnType)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    Value *colorIndex = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *isColorIndexGood = generateCallExtern(TyBool, "isColorIndexGood", {PaAddr(evaluator), PaAddr(colorIndex), PaDouble(CoDouble(8.0))});
    Value *isColorIndexGoodCmp = scaff->builder.CreateICmpEQ(isColorIndexGood, CoBool(true), "isColorIndexGood");
    BasicBlock *colorIndexNotGoodBB = BasicBlock::Create(*scaff->theContext, "colorIndexNotGood", theFunction);
    BasicBlock *colorIndexGoodBB = BasicBlock::Create(*scaff->theContext, "colorIndexGood", theFunction);
    BasicBlock *colorNotGoodBB = BasicBlock::Create(*scaff->theContext, "colorNotGood", theFunction);
    BasicBlock *colorGoodBB = BasicBlock::Create(*scaff->theContext, "colorGood", theFunction);
    scaff->builder.CreateCondBr(isColorIndexGoodCmp, colorIndexGoodBB, colorIndexNotGoodBB);

    // Color index is not good.
    scaff->builder.SetInsertPoint(colorIndexNotGoodBB);
    Value *errVal = generateCallExtern(TyAddr, "getErrorNoLike", {PaAddr(evaluator), PaAddr(CoAddr(node.astnodeValue())), PaAddr(colorIndex)});
    scaff->builder.CreateRet(errVal);

    // Color index is good.
    scaff->builder.SetInsertPoint(colorIndexGoodBB);
    Value *color = generateChild(node.astnodeValue(), 1, RequestReturnDatum);
    Value *colorIsGood = generateCallExtern(TyBool, "setPalette", {PaAddr(evaluator), PaAddr(colorIndex), PaAddr(color)});
    Value *colorIsGoodCmp = scaff->builder.CreateICmpEQ(colorIsGood, CoBool(true), "colorIsGood");
    scaff->builder.CreateCondBr(colorIsGoodCmp, colorGoodBB, colorNotGoodBB);

    // Color is not good.
    scaff->builder.SetInsertPoint(colorNotGoodBB);
    errVal = generateCallExtern(TyAddr, "getErrorNoLike", {PaAddr(evaluator), PaAddr(CoAddr(node.astnodeValue())), PaAddr(color)});
    scaff->builder.CreateRet(errVal);

    // Color is good.
    scaff->builder.SetInsertPoint(colorGoodBB);
    return generateVoidRetval(node);
}

EXPORTC bool isColorIndexGood(addr_t eAddr, addr_t colorIndexAddr, double lowerLimit)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Word *w = reinterpret_cast<Word *>(colorIndexAddr);
    double colorIndex = w->numberValue();

    return (w->numberIsValid)
      && (colorIndex == floor(colorIndex))
      && (colorIndex >= lowerLimit)
      && (colorIndex < Config::get().mainKernel()->palette.size());
}


EXPORTC bool setPalette(addr_t eAddr, addr_t colorIndexAddr, addr_t colorAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    int colorIndex = static_cast<int>((reinterpret_cast<Word *>(colorIndexAddr))->numberValue());
    Datum *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Config::get().mainKernel()->colorFromDatumPtr(color, DatumPtr(d)))
        return false;
    Config::get().mainKernel()->palette[colorIndex] = color;
    return true;
}



/***DOC SETPENSIZE
SETPENSIZE size

    sets the thickness of the pen.  The input is a single positive
    integer.

COD***/
// CMD SETPENSIZE 1 1 1 n
Value *Compiler::genSetpensize(DatumPtr node, RequestReturnType returnType)
{
    Value *size = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    size = generateGTZeroFromDouble(node.astnodeValue(), size);
    generateCallExtern(TyVoid, "setPenSize", {PaAddr(evaluator), PaAddr(size)});
    return generateVoidRetval(node);
}

EXPORTC void setPenSize(addr_t eAddr, double size)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Config::get().mainTurtle()->setPenSize(size);
}


/***DOC SETBACKGROUND SETBG
SETBACKGROUND color
SETBG color

    set the screen background color. See SETPENCOLOR for color details.


COD***/
// CMD SETBACKGROUND 1 1 1 n
// CMD SETBG 1 1 1 n
Value *Compiler::genSetbackground(DatumPtr node, RequestReturnType returnType)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    Value *color = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *isGood = generateCallExtern(TyBool, "setBackground", {PaAddr(evaluator), PaAddr(color)});
    Value *isGoodCmp = scaff->builder.CreateICmpEQ(isGood, CoBool(true), "isGood");
    BasicBlock *colorNotGoodBB = BasicBlock::Create(*scaff->theContext, "colorNotGood", theFunction);
    BasicBlock *colorGoodBB = BasicBlock::Create(*scaff->theContext, "colorGood", theFunction);
    scaff->builder.CreateCondBr(isGoodCmp, colorGoodBB, colorNotGoodBB);

    // Color is not good.
    scaff->builder.SetInsertPoint(colorNotGoodBB);
    Value *errVal = generateCallExtern(TyAddr, "getErrorNoLike", {PaAddr(evaluator), PaAddr(CoAddr(node.astnodeValue())), PaAddr(color)});
    scaff->builder.CreateRet(errVal);

    // Color is good.
    scaff->builder.SetInsertPoint(colorGoodBB);
    return generateVoidRetval(node);
}

EXPORTC bool setBackground(addr_t eAddr, addr_t colorAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Config::get().mainKernel()->colorFromDatumPtr(color, DatumPtr(d)))
        return false;
    Config::get().mainController()->setCanvasBackgroundColor(color);
    return true;
}


// PEN QUERIES

/***DOC PENDOWNP PENDOWN?
PENDOWNP
PENDOWN?

    outputs TRUE if the pen is down, FALSE if it's up.

COD***/
// CMD PENDOWNP 0 0 0 b
// CMD PENDOWN? 0 0 0 b
Value *Compiler::genPendownp(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyBool, "isPenDown", {PaAddr(evaluator)});
}

EXPORTC bool isPenDown(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    return Config::get().mainTurtle()->isPenDown();
}


/***DOC PENMODE
PENMODE

    outputs one of the words PAINT, ERASE, or REVERSE according to
    the current pen mode.

COD***/
// CMD PENMODE 0 0 0 d
Value *Compiler::genPenmode(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, "getPenMode", {PaAddr(evaluator)});
}

EXPORTC addr_t getPenMode(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
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
    Word *w = new Word(retval);
    e->watch(w);
    return reinterpret_cast<addr_t>(w);
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
Value *Compiler::genPencolor(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, "getPenColor", {PaAddr(evaluator)});
}

EXPORTC addr_t getPenColor(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    const QColor &color = Config::get().mainTurtle()->getPenColor();
    List *retval = listFromColor(color);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}



/***DOC PALETTE
PALETTE colornumber

    outputs a list of three nonnegative numbers less than 100 specifying
    the percent saturation of red, green, and blue in the color associated
    with the given number.

COD***/
// CMD PALETTE 1 1 1 d
Value *Compiler::genPalette(DatumPtr node, RequestReturnType returnType)
{
    Function *theFunction = scaff->builder.GetInsertBlock()->getParent();
    Value *colorIndex = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    Value *isColorIndexGood = generateCallExtern(TyBool, "isColorIndexGood", {PaAddr(evaluator), PaAddr(colorIndex), PaDouble(CoDouble(0.0))});
    Value *isColorIndexGoodCmp = scaff->builder.CreateICmpEQ(isColorIndexGood, CoBool(true), "isColorIndexGood");
    BasicBlock *colorIndexNotGoodBB = BasicBlock::Create(*scaff->theContext, "colorIndexNotGood", theFunction);
    BasicBlock *colorIndexGoodBB = BasicBlock::Create(*scaff->theContext, "colorIndexGood", theFunction);
    scaff->builder.CreateCondBr(isColorIndexGoodCmp, colorIndexGoodBB, colorIndexNotGoodBB);

    // Color index is not good.
    scaff->builder.SetInsertPoint(colorIndexNotGoodBB);
    Value *errVal = generateCallExtern(TyAddr, "getErrorNoLike", {PaAddr(evaluator), PaAddr(CoAddr(node.astnodeValue())), PaAddr(colorIndex)});
    scaff->builder.CreateRet(errVal);

    // Color index is good.
    scaff->builder.SetInsertPoint(colorIndexGoodBB);
    Value *color = generateCallExtern(TyAddr, "getPaletteColor", {PaAddr(evaluator), PaAddr(colorIndex)});
    return color;
}

EXPORTC addr_t getPaletteColor(addr_t eAddr, addr_t colorIndexAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    int colorIndex = static_cast<int>((reinterpret_cast<Word *>(colorIndexAddr))->numberValue());
    const QColor &color = Config::get().mainKernel()->palette[colorIndex];
    List *retval = listFromColor(color);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}


/***DOC PENSIZE
PENSIZE


    outputs a positive integer, specifying the thickness of the turtle pen.

COD***/
// CMD PENSIZE 0 0 0 r
Value *Compiler::genPensize(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyDouble, "getPenSize", {PaAddr(evaluator)});
}

EXPORTC double getPenSize(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    return Config::get().mainTurtle()->getPenSize();
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
Value *Compiler::genBackground(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, "getBackground", {PaAddr(evaluator)});
}

EXPORTC addr_t getBackground(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    const QColor &color = Config::get().mainController()->getCanvasBackgroundColor();
    List *retval = listFromColor(color);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
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
Value *Compiler::genSavepict(DatumPtr node, RequestReturnType returnType)
{
    Value *filename = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    return generateCallExtern(TyAddr, "savePict", {PaAddr(evaluator), PaAddr(filename), PaAddr(CoAddr(node.astnodeValue()))});
}

EXPORTC addr_t savePict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    QString filename = reinterpret_cast<Word *>(filenameAddr)->printValue();
    QString filepath = Config::get().mainKernel()->filepathForFilename(DatumPtr(filename));
    QImage image = Config::get().mainController()->getCanvasImage();
    bool isSuccessful = image.save(filepath);
    Datum *retval = reinterpret_cast<Datum *>(nodeAddr);
    if (!isSuccessful)
    {
        retval = FCError::fileSystem();
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}



/***DOC SVGPICT
SVGPICT filename

    command.  Writes a file with the specified name containing the
    contents of the graphics window in SVG format. The dimensions of the image
    are determined by the canvas bounds.

COD***/
// CMD SVGPICT 1 1 1 n
Value *Compiler::genSvgpict(DatumPtr node, RequestReturnType returnType)
{
    Value *filename = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    return generateCallExtern(TyAddr, "saveSvgpict", {PaAddr(evaluator), PaAddr(filename), PaAddr(CoAddr(node.astnodeValue()))});
}

EXPORTC addr_t saveSvgpict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    QString filename = reinterpret_cast<Word *>(filenameAddr)->printValue();
    QString filepath = Config::get().mainKernel()->filepathForFilename(DatumPtr(filename));
    QByteArray svgImage = Config::get().mainController()->getSvgImage();

    Datum *retval = reinterpret_cast<Datum *>(nodeAddr);
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


/***DOC LOADPICT
LOADPICT filename

    command.  Reads the image file with the specified filename and sets the image
    as the canvas background. The image will be stretched, if necessary, to fit
    the bounds of the canvas.

    The filename may also be an empty list, in which case any image previously
    set as the background will be cleared.

COD***/
// CMD LOADPICT 1 1 1 n
Value *Compiler::genLoadpict(DatumPtr node, RequestReturnType returnType)
{
    Value *filename = generateChild(node.astnodeValue(), 0, RequestReturnDatum);
    return generateCallExtern(TyAddr, "loadPict", {PaAddr(evaluator), PaAddr(filename), PaAddr(CoAddr(node.astnodeValue()))});
}

EXPORTC addr_t loadPict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    Datum *dFilename = reinterpret_cast<Datum *>(filenameAddr);
    Datum *retval = reinterpret_cast<Datum *>(nodeAddr);
    if (dFilename->isa == Datum::typeWord) {
        QString filename = reinterpret_cast<Word *>(filenameAddr)->printValue();
        QString filepath = Config::get().mainKernel()->filepathForFilename(DatumPtr(filename));
        QImage image = QImage(filepath);
        if (image.isNull())
        {
            retval = FCError::fileSystem();
        }
        Config::get().mainController()->setCanvasBackgroundImage(image);
        goto done;
    }
    if (dFilename->isa == Datum::typeList) {
        if (reinterpret_cast<List *>(dFilename)->isEmpty()) {
            Config::get().mainController()->setCanvasBackgroundImage(QImage());
            goto done;
        }
    }
    retval = FCError::doesntLike(DatumPtr(reinterpret_cast<ASTNode *>(nodeAddr)->nodeName), DatumPtr(dFilename));
done:
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
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
Value *Compiler::genMousepos(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, "getMousePos", {PaAddr(evaluator)});
}

EXPORTC addr_t getMousePos(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    QVector2D position = Config::get().mainController()->mousePosition();
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(position.x()));
    retvalBuilder.append(DatumPtr(position.y()));
    Datum* retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}


/***DOC CLICKPOS
CLICKPOS

    outputs the coordinates that the mouse was at when a mouse button
    was most recently pushed, provided that that position was within the
    graphics window, in turtle coordinates.

COD***/
// CMD CLICKPOS 0 0 0 d
Value *Compiler::genClickpos(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyAddr, "getClickPos", {PaAddr(evaluator)});
}

EXPORTC addr_t getClickPos(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    QVector2D position = Config::get().mainController()->lastMouseclickPosition();
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(position.x()));
    retvalBuilder.append(DatumPtr(position.y()));
    Datum* retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
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
Value *Compiler::genButtonp(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyBool, "isMouseButtonDown", {PaAddr(evaluator)});
}

EXPORTC bool isMouseButtonDown(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    return Config::get().mainController()->getIsMouseButtonDown();
}



/***DOC BUTTON
BUTTON

    outputs 0 if no mouse button has been pushed inside the Logo window
    since the last call to BUTTON.  Otherwise, it outputs an integer
    indicating which button was most recently pressed.
    1 means left, 2 means right.



COD***/
// CMD BUTTON 0 0 0 r
Value *Compiler::genButton(DatumPtr node, RequestReturnType returnType)
{
    return generateCallExtern(TyDouble, "getMouseButton", {PaAddr(evaluator)});
}

EXPORTC double getMouseButton(addr_t eAddr)
{
    Evaluator *e = reinterpret_cast<Evaluator *>(eAddr);
    return static_cast<double>(Config::get().mainController()->getAndResetButtonID());
}






