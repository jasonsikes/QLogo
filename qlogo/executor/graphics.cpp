
//===-- qlogo/kernel.cpp - Kernel class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Kernel class, which is the
/// executor proper of the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "error.h"
#include "kernel.h"
#include "turtle.h"
#include "datum/word.h"
#include "datum/list.h"
#include "datum/astnode.h"
#include "controller/logocontroller.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DatumPtr listFromColor(QColor c) {
  List *retval = new List();
  retval->append(DatumPtr(round(c.redF() * 100)));
  retval->append(DatumPtr(round(c.greenF() * 100)));
  retval->append(DatumPtr(round(c.blueF() * 100)));
  return DatumPtr(retval);
}

// TURTLE MOTION


/***DOC FORWARD FD
FORWARD dist
FD dist

    moves the turtle forward, in the direction that it's facing, by
    the specified distance (measured in turtle steps).

COD***/
//CMD FORWARD 1 1 1
//CMD FD 1 1 1
DatumPtr Kernel::excForward(DatumPtr node) {
  ProcedureHelper h(this, node);
  double value = h.numberAtIndex(0);

  mainTurtle()->forward(value);

  return nothing;
}


/***DOC BACK BK
BACK dist
BK dist

    moves the turtle backward, i.e., exactly opposite to the direction
    that it's facing, by the specified distance.  (The heading of the
    turtle does not change.)

COD***/
//CMD BACK 1 1 1
//CMD BK 1 1 1
DatumPtr Kernel::excBack(DatumPtr node) {
  ProcedureHelper h(this, node);
  double value = h.numberAtIndex(0);

  mainTurtle()->forward(-value);

  return nothing;
}


/***DOC LEFT LT
LEFT degrees
LT degrees

    turns the turtle counterclockwise by the specified angle, measured
    in degrees (1/360 of a circle).

COD***/
//CMD LEFT 1 1 1
//CMD LT 1 1 1
DatumPtr Kernel::excLeft(DatumPtr node) {
  ProcedureHelper h(this, node);
  double value = h.numberAtIndex(0);

  mainTurtle()->rotate(value);

  return nothing;
}


/***DOC RIGHT RT
RIGHT degrees
RT degrees

    turns the turtle clockwise by the specified angle, measured in
    degrees (1/360 of a circle).

COD***/
//CMD RIGHT 1 1 1
//CMD RT 1 1 1
DatumPtr Kernel::excRight(DatumPtr node) {
  ProcedureHelper h(this, node);
  double value = h.numberAtIndex(0);

  mainTurtle()->rotate(-value);

  return nothing;
}


/***DOC SETPOS
SETPOS pos

    moves the turtle to an absolute position in the graphics window.  The
    input is a list of two numbers, the X and Y coordinates.

COD***/
//CMD SETPOS 1 1 1
DatumPtr Kernel::excSetpos(DatumPtr node) {
  ProcedureHelper h(this, node);

  QVector<double> v;
  h.validatedDatumAtIndex(0, [&v, this](DatumPtr candidate) {
    if (!candidate.isList())
      return false;
    if (!numbersFromList(v, candidate))
      return false;
    if (v.size() != 2)
      return false;
    return true;
  });

  mainTurtle()->setxy(v[0], v[1]);

  return nothing;
}


/***DOC SETXY
SETXY xcor ycor

    moves the turtle to an absolute position in the graphics window.  The
    two inputs are numbers, the X and Y coordinates.

COD***/
//CMD SETXY 2 2 2
DatumPtr Kernel::excSetXY(DatumPtr node) {
  ProcedureHelper h(this, node);
  double x = h.numberAtIndex(0);
  double y = h.numberAtIndex(1);

  mainTurtle()->setxy(x, y);

  return nothing;
}


/***DOC SETX
SETX xcor

    moves the turtle horizontally from its old position to a new
    absolute horizontal coordinate.  The input is the new X
    coordinate.

COD***/
//CMD SETX 1 1 1
DatumPtr Kernel::excSetX(DatumPtr node) {
  ProcedureHelper h(this, node);
  double x = h.numberAtIndex(0);

  mainTurtle()->setx(x);

  return nothing;
}


/***DOC SETY
SETY ycor

    moves the turtle vertically from its old position to a new
    absolute vertical coordinate.  The input is the new Y
    coordinate.

COD***/
//CMD SETY 1 1 1
DatumPtr Kernel::excSetY(DatumPtr node) {
  ProcedureHelper h(this, node);
  double y = h.numberAtIndex(0);

  mainTurtle()->sety(y);

  return nothing;
}


/***DOC SETHEADING SETH
SETHEADING degrees
SETH degrees

    turns the turtle to a new absolute heading.  The input is
    a number, the heading in degrees clockwise from the positive
    Y axis.

COD***/
//CMD SETHEADING 1 1 1
//CMD SETH 1 1 1
DatumPtr Kernel::excSetheading(DatumPtr node) {
  ProcedureHelper h(this, node);
  double newHeading = h.numberAtIndex(0);
  double oldHeading = mainTurtle()->getHeading();

  // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
  newHeading = 360 - newHeading;

  double adjustment = newHeading - oldHeading;
  mainTurtle()->rotate(adjustment);
  return nothing;
}


/***DOC HOME
HOME

    moves the turtle to the center of the screen.  Equivalent to
    SETPOS [0 0] SETHEADING 0.

COD***/
//CMD HOME 0 0 0
DatumPtr Kernel::excHome(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->moveToHome();

  return nothing;
}


/***DOC ARC
ARC angle radius

    draws an arc of a circle, with the turtle at the center, with the
    specified radius, starting at the turtle's heading and extending
    clockwise through the specified angle.  The turtle does not move.

COD***/
//CMD ARC 2 2 2
DatumPtr Kernel::excArc(DatumPtr node) {
  ProcedureHelper h(this, node);
  double angle = h.numberAtIndex(0);
  double radius = h.numberAtIndex(1);

  // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
  angle = 0 - angle;

  if ((angle < -360) || (angle > 360))
    angle = 360;

  if ((angle != 0) && (radius != 0))
    mainTurtle()->drawArc(angle, radius);

  return nothing;
}

// TURTLE MOTION QUERIES


/***DOC POS
POS

    outputs the turtle's current position, as a list of two
    numbers, the X and Y coordinates.

COD***/
//CMD POS 0 0 0
DatumPtr Kernel::excPos(DatumPtr node) {
  ProcedureHelper h(this, node);
  double x, y;
  mainTurtle()->getxy(x, y);

  List *retval = new List();
  retval->append(DatumPtr(x));
  retval->append(DatumPtr(y));
  return h.ret(retval);
}


/***DOC HEADING
HEADING

    outputs a number, the turtle's heading in degrees.

COD***/
//CMD HEADING 0 0 0
DatumPtr Kernel::excHeading(DatumPtr node) {
  ProcedureHelper h(this, node);
  double retval = mainTurtle()->getHeading();

  // Heading is positive in the counter-clockwise direction.
  if (retval > 0)
      retval = 360 - retval;

  return h.ret(retval);
}


/***DOC TOWARDS
TOWARDS pos

    outputs a number, the heading at which the turtle should be
    facing so that it would point from its current position to
    the position given as the input.

COD***/
//CMD TOWARDS 1 1 1
DatumPtr Kernel::excTowards(DatumPtr node) {
  ProcedureHelper h(this, node);
  QVector<double> v;
  double x, y;
  h.validatedDatumAtIndex(0, [&v, this](DatumPtr candidate) {
    if (!candidate.isList())
      return false;
    if (!numbersFromList(v, candidate))
      return false;
    if (v.size() != 2)
      return false;
    return true;
  });
  mainTurtle()->getxy(x, y);
  double retval = atan2(x - v[0], v[1] - y) * (180 / M_PI);
  if (retval < 0)
    retval += 360;

  // Heading is positive in the counter-clockwise direction.
  if (retval > 0)
      retval = 360 - retval;

  return h.ret(retval);
}


/***DOC SCRUNCH
SCRUNCH

    outputs a list containing two numbers, both '1'.  This primitive is
    maintained for backward compatibility. QLogo does not use SCRUNCH.
    SCRUNCH was used by UCBLogo because older monitors had pixels with
    varying width/height proportions.


COD***/
//CMD SCRUNCH 0 0 0
DatumPtr Kernel::excScrunch(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = new List();
  retval->append(DatumPtr(1));
  retval->append(DatumPtr(1));
  return h.ret(retval);
}

// TURTLE AND WINDOW CONTROL


/***DOC SHOWTURTLE ST
SHOWTURTLE
ST

    makes the turtle visible.

COD***/
//CMD SHOWTURTLE 0 0 0
//CMD ST 0 0 0
DatumPtr Kernel::excShowturtle(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setIsTurtleVisible(true);
  mainController()->setTurtleIsVisible(true);

  return nothing;
}


/***DOC HIDETURTLE HT
HIDETURTLE
HT

    makes the turtle invisible.  It's a good idea to do this while
    you're in the middle of a complicated drawing, because hiding
    the turtle speeds up the drawing substantially.

COD***/
//CMD HIDETURTLE 0 0 0
//CMD HT 0 0 0
DatumPtr Kernel::excHideturtle(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setIsTurtleVisible(false);
  mainController()->setTurtleIsVisible(false);

  return nothing;
}


/***DOC CLEAN
CLEAN

    erases all lines that the turtle has drawn on the graphics window.
    The turtle's state (position, heading, pen mode, etc.) is not
    changed.

COD***/
//CMD CLEAN 0 0 0
DatumPtr Kernel::excClean(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->clearCanvas();
  return nothing;
}


/***DOC CLEARSCREEN CS
CLEARSCREEN
CS

    erases the graphics window and sends the turtle to its initial
    position and heading.  Like HOME and CLEAN together.

COD***/
//CMD CLEARSCREEN 0 0 0
//CMD CS 0 0 0
DatumPtr Kernel::excClearscreen(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->moveToHome();
  mainController()->clearCanvas();

  return nothing;
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
//CMD WRAP 0 0 0
DatumPtr Kernel::excWrap(DatumPtr node) {
  ProcedureHelper h(this, node);
  TurtleModeEnum newMode = turtleWrap;
  if (mainTurtle()->getMode() != newMode) {
    mainTurtle()->setMode(newMode);
      mainController()->setIsCanvasBounded(true);
  }
  return nothing;
}


/***DOC WINDOW
WINDOW

    tells the turtle to enter window mode:  From now on, if the turtle
    is asked to move past the boundary of the graphics window, it
    will move offscreen.  The visible graphics window is considered
    as just part of an infinite graphics plane; the turtle can be
    anywhere on the plane.  (If you lose the turtle, HOME will bring
    it back to the center of the window.)  Compare WRAP and FENCE.

COD***/
//CMD WINDOW 0 0 0
DatumPtr Kernel::excWindow(DatumPtr node) {
  ProcedureHelper h(this, node);
  TurtleModeEnum newMode = turtleWindow;
  if (mainTurtle()->getMode() != newMode) {
    mainTurtle()->setMode(newMode);
    mainController()->setIsCanvasBounded(false);
  }
  return nothing;
}


/***DOC FENCE
FENCE

    tells the turtle to enter fence mode:  From now on, if the turtle
    is asked to move past the boundary of the graphics window, it
    will move as far as it can and then stop at the edge with an
    "out of bounds" error message.  Compare WRAP and WINDOW.

COD***/
//CMD FENCE 0 0 0
DatumPtr Kernel::excFence(DatumPtr node) {
  ProcedureHelper h(this, node);
  TurtleModeEnum newMode = turtleFence;
  if (mainTurtle()->getMode() != newMode) {
    mainTurtle()->setMode(newMode);
      mainController()->setIsCanvasBounded(true);
  }
  return nothing;
}


/***DOC BOUNDS
BOUNDS

    outputs a list of two positive numbers [X,Y] giving the maximum bounds
    of the canvas. See SETBOUNDS.

COD***/
//CMD BOUNDS 2 2 2
DatumPtr Kernel::excBounds(DatumPtr node) {
  ProcedureHelper h(this, node);
  double x = mainController()->boundX();
  double y = mainController()->boundY();

  List *retval = new List();
  retval->append(DatumPtr(x));
  retval->append(DatumPtr(y));
  return h.ret(retval);
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
//CMD SETBOUNDS 2 2 2
DatumPtr Kernel::excSetbounds(DatumPtr node) {
  ProcedureHelper h(this, node);
  auto v = [](double candidate) { return candidate > 0; };

  double x = h.validatedNumberAtIndex(0, v);
  double y = h.validatedNumberAtIndex(1, v);

  mainController()->setBounds(x, y);

  return nothing;
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
//CMD FILLED 2 2 2
DatumPtr Kernel::excFilled(DatumPtr node) {
  ProcedureHelper h(this, node);
  QColor c;
  h.validatedDatumAtIndex(0, [&c, this](DatumPtr candidate) {
    return colorFromDatumPtr(c, candidate);
  });

  DatumPtr commandList = h.datumAtIndex(1);

  mainTurtle()->beginFillWithColor(c);
  DatumPtr retval;
  try {
    retval = runList(commandList);
  } catch (Error *e) {
    mainTurtle()->endFill();
    throw e;
  }
  mainTurtle()->endFill();
  return h.ret(retval);
}


/***DOC LABEL
LABEL text

    takes a word as input, and prints the input on the graphics window,
    starting at the turtle's position.

COD***/
// TODO: should also accept list as input.
//CMD LABEL 1 1 1
DatumPtr Kernel::excLabel(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString text = h.wordAtIndex(0).wordValue()->printValue();
  double x = 0, y = 0;
  mainTurtle()->getxy(x, y);
  mainController()->drawLabel(text);
  return nothing;
}


/***DOC SETLABELHEIGHT
SETLABELHEIGHT height

    command. Takes a positive number argument and sets the label font size.

COD***/
//CMD SETLABELHEIGHT 1 1 1
DatumPtr Kernel::excSetlabelheight(DatumPtr node) {
  ProcedureHelper h(this, node);
  double height = h.validatedNumberAtIndex(
      0, [](double candidate) { return candidate > 0; });
  mainController()->setLabelFontSize(height);
  return nothing;
}


/***DOC TEXTSCREEN TS
TEXTSCREEN
TS

    rearranges the size and position of windows to maximize the
    space available in the text window (the window used for
    interaction with Logo).  Compare SPLITSCREEN and FULLSCREEN.

COD***/
//CMD TEXTSCREEN 0 0 0
//CMD TS 0 0 0
DatumPtr Kernel::excTextscreen(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->setScreenMode(textScreenMode);
  return nothing;
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
//CMD FULLSCREEN 0 0 0
//CMD FS 0 0 0
DatumPtr Kernel::excFullscreen(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->setScreenMode(fullScreenMode);
  return nothing;
}


/***DOC SPLITSCREEN SS
SPLITSCREEN
SS

    rearranges the size and position of windows to allow some room for
    text interaction while also keeping most of the graphics window
    visible.  The proportions are 75% turtle canvas and 25% text console.
    Compare TEXTSCREEN and FULLSCREEN.

COD***/
//CMD SPLITSCREEN 0 0 0
//CMD SS 0 0 0
DatumPtr Kernel::excSplitscreen(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->setScreenMode(splitScreenMode);
  return nothing;
}


/***DOC SETSCRUNCH
SETSCRUNCH xscale yscale

    In QLogo this does nothing. See SCRUNCH.

COD***/
//CMD SETSCRUNCH 2 2 2
DatumPtr Kernel::excSetscrunch(DatumPtr node) {
  ProcedureHelper h(this, node);
  return nothing;
}

// TURTLE AND WINDOW QUERIES


/***DOC SHOWNP SHOWN?
SHOWNP
SHOWN?

    outputs TRUE if the turtle is shown (visible), FALSE if the
    turtle is hidden.  See SHOWTURTLE and HIDETURTLE.

COD***/
//CMD SHOWNP 0 0 0
//CMD SHOWN? 0 0 0
DatumPtr Kernel::excShownp(DatumPtr node) {
  ProcedureHelper h(this, node);
  bool retval = mainTurtle()->isTurtleVisible();
  return h.ret(retval);
}


/***DOC SCREENMODE
SCREENMODE

    outputs the word TEXTSCREEN, SPLITSCREEN, or FULLSCREEN depending
    on the last requested screen mode.

    In QLogo, since the user is freely able to adjust the split between
    the canvas and console, this will only return the mode set by the
    last used mode command.

COD***/
//CMD SCREENMODE 0 0 0
DatumPtr Kernel::excScreenmode(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString retval;
  switch (mainController()->getScreenMode()) {
  case textScreenMode:
  case initScreenMode:
    retval = QObject::tr("textscreen");
    break;
  case fullScreenMode:
    retval = QObject::tr("fullscreen");
    break;
  case splitScreenMode:
    retval = QObject::tr("splitscreen");
    break;
  default:
    break;
  }
  return h.ret(retval);
}


/***DOC TURTLEMODE
TURTLEMODE

    outputs the word WRAP, FENCE, or WINDOW depending on the current
    turtle mode.

COD***/
//CMD TURTLEMODE 0 0 0
DatumPtr Kernel::excTurtlemode(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString retval;
  switch (mainTurtle()->getMode()) {
  case turtleWrap:
    retval = QObject::tr("wrap");
    break;
  case turtleFence:
    retval = QObject::tr("fence");
    break;
  case turtleWindow:
    retval = QObject::tr("window");
    break;
  default:
    qDebug() << "what mode is the turtle?";
    Q_ASSERT(false);
    break;
  }
  return h.ret(retval);
}


/***DOC LABELSIZE
LABELSIZE

    outputs the height of the label font as a number. Note that QLogo only
    reports the font height as a single number, as opposed to UCBLogo which
    returned a list of two numbers. The reason is that most fonts in QLogo
    are variable-width, and therefore the width is difficult to calculate.

COD***/
//CMD LABELSIZE 0 0 0
DatumPtr Kernel::excLabelheight(DatumPtr node) {
  ProcedureHelper h(this, node);
  double retval = mainController()->getLabelFontSize();
  return h.ret(retval);
}

/***DOC MATRIX
MATRIX

    outputs a 3-by-3 transformation matrix in the form of a list of three lists,
    each list contains three numbers. This represents the state of the turtle in
    2D space, and is only present for debugging purposes. It may be removed or
    replaced in the future and should be considered DEPRICATED.

COD***/

// TODO: TURTLEMATRIX, and maybe .SETTURTLEMATRIX
// TODO: This should be an array of arrays.
//CMD MATRIX 0 0 0
DatumPtr Kernel::excMatrix(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = new List();
  const QMatrix4x4 &m = mainTurtle()->getMatrix();
  for (int row = 0; row < 4; ++row) {
    List *r = new List();
    for (int col = 0; col < 4; ++col) {
      r->append(DatumPtr(m(row, col)));
    }
    retval->append(DatumPtr(r));
  }
  return h.ret(retval);
}

// PEN AND BACKGROUND CONTROL


/***DOC PENDOWN PD
PENDOWN
PD

    sets the pen's position to DOWN, without changing its mode.

COD***/
//CMD PENDOWN 0 0 0
//CMD PD 0 0 0
DatumPtr Kernel::excPendown(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(true);

  return nothing;
}


/***DOC PENUP PU
PENUP
PU

    sets the pen's position to UP, without changing its mode.

COD***/
//CMD PENUP 0 0 0
//CMD PU 0 0 0
DatumPtr Kernel::excPenup(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(false);

  return nothing;
}


/***DOC PENPAINT PPT
PENPAINT
PPT

    sets the pen's position to DOWN and mode to PAINT.

COD***/
//CMD PENPAINT 0 0 0
//CMD PPT 0 0 0
DatumPtr Kernel::excPenpaint(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(true);
  mainTurtle()->setPenMode(penModePaint);
  return nothing;
}


/***DOC PENERASE PE
PENERASE
PE

    sets the pen's position to DOWN and mode to ERASE.

COD***/
//CMD PENERASE 0 0 0
//CMD PE 0 0 0
DatumPtr Kernel::excPenerase(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(true);
  mainTurtle()->setPenMode(penModeErase);
  return nothing;
}


/***DOC PENREVERSE PX
PENREVERSE
PX

    sets the pen's position to DOWN and mode to REVERSE.
    The pen color value is ignored while in penreverse mode.

COD***/
//CMD PENREVERSE 0 0 0
//CMD PX 0 0 0
DatumPtr Kernel::excPenreverse(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(true);
  mainTurtle()->setPenMode(penModeReverse);
  return nothing;
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

    but other colors can be assigned to numbers by the PALETTE command.

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
//CMD SETPENCOLOR 1 1 1
//CMD SETPC 1 1 1
DatumPtr Kernel::excSetpencolor(DatumPtr node) {
  ProcedureHelper h(this, node);
  QColor c;
  h.validatedDatumAtIndex(0, [&c, this](DatumPtr candidate) {
    return colorFromDatumPtr(c, candidate);
  });
  mainTurtle()->setPenColor(c);
  return nothing;
}


/***DOC ALLCOLORS
ALLCOLORS

    returns a list of all of the color names that QLogo knows about.

COD***/
//CMD ALLCOLORS 0 0 0
DatumPtr Kernel::excAllcolors(DatumPtr node) {
    ProcedureHelper h(this, node);
    List *retval = new List();
    QStringList colors = QColor::colorNames();
    for (const QString &i : colors) {
        retval->append(DatumPtr(i));
    }
    return h.ret(retval);
}


/***DOC SETPALETTE
SETPALETTE colornumber color

    sets the actual color corresponding to a given number, if allowed by
    the hardware and operating system.  Colornumber must be an integer
    greater than or equal to 8.  (Logo tries to keep the first 8 colors
    constant.)  The second input is a color. See SETPENCOLOR for different
    methods of specifying a color.

COD***/
//CMD SETPALETTE 2 2 2
DatumPtr Kernel::excSetpalette(DatumPtr node) {
  ProcedureHelper h(this, node);
  int colornumber = h.validatedIntegerAtIndex(0, [this](int candidate) {
    return (candidate >= 8) && (candidate < palette.size());
  });
  QColor c;
  h.validatedDatumAtIndex(1, [&c, this](DatumPtr candidate) {
    return colorFromDatumPtr(c, candidate);
  });
  palette[colornumber] = c;
  return nothing;
}


/***DOC SETPENSIZE
SETPENSIZE size

    sets the thickness of the pen.  The input is a single positive
    integer. Note that since QLogo uses OpenGL for drawing, the pen may either
    be vertical or horizontal depending on the direction of the line being
    drawn.

COD***/
//CMD SETPENSIZE 1 1 1
DatumPtr Kernel::excSetpensize(DatumPtr node) {
  ProcedureHelper h(this, node);
  double newSize = h.validatedNumberAtIndex(0, [](double candidate) {
    return mainTurtle()->isPenSizeValid(candidate);
  });
  mainTurtle()->setPenSize(newSize);
  return nothing;
}


/***DOC SETBACKGROUND SETBG
SETBACKGROUND color
SETBG color

    set the screen background color. See SETPENCOLOR for color details.


COD***/
//CMD SETBACKGROUND 1 1 1
//CMD SETBG 1 1 1
DatumPtr Kernel::excSetbackground(DatumPtr node) {
  ProcedureHelper h(this, node);
  QColor c;
  h.validatedDatumAtIndex(0, [&c, this](DatumPtr candidate) {
    return colorFromDatumPtr(c, candidate);
  });
  mainController()->setCanvasBackgroundColor(c);
  return nothing;
}

// PEN QUERIES


/***DOC PENDOWNP PENDOWN?
PENDOWNP
PENDOWN?

    outputs TRUE if the pen is down, FALSE if it's up.

COD***/
//CMD PENDOWNP 0 0 0
//CMD PENDOWN? 0 0 0
DatumPtr Kernel::excPendownp(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(mainTurtle()->isPenDown());
}


/***DOC PENMODE
PENMODE

    outputs one of the words PAINT, ERASE, or REVERSE according to
    the current pen mode.

COD***/
//CMD PENMODE 0 0 0
DatumPtr Kernel::excPenmode(DatumPtr node) {
  ProcedureHelper h(this, node);
  PenModeEnum pm = mainTurtle()->getPenMode();
  QString retval;
  switch (pm) {
  case penModePaint:
    retval = QObject::tr("paint");
    break;
  case penModeReverse:
    retval = QObject::tr("reverse");
    break;
  case penModeErase:
    retval = QObject::tr("erase");
    break;
  default:
    retval = "ERROR!!!";
    break;
  }
  return h.ret(retval);
}


/***DOC PENCOLOR PC
PENCOLOR
PC

    outputs a list of three nonnegative numbers less than 100 specifying
    the percent saturation of red, green, and blue in the color associated
    with the current pen color.

COD***/
//CMD PENCOLOR 0 0 0
//CMD PC 0 0 0
DatumPtr Kernel::excPencolor(DatumPtr node) {
  ProcedureHelper h(this, node);
  const QColor &c = mainTurtle()->getPenColor();
  return h.ret(listFromColor(c));
}


/***DOC PALETTE
PALETTE colornumber

    outputs a list of three nonnegative numbers less than 100 specifying
    the percent saturation of red, green, and blue in the color associated
    with the given number.

COD***/
//CMD PALETTE 1 1 1
DatumPtr Kernel::excPalette(DatumPtr node) {
  ProcedureHelper h(this, node);
  int colornumber = h.validatedIntegerAtIndex(0, [this](int candidate) {
    return (candidate >= 0) && (candidate < palette.size());
  });
  return h.ret(listFromColor(palette[colornumber]));
}


/***DOC PENSIZE
PENSIZE


    outputs a positive integer, specifying the thickness of the turtle pen.

COD***/
//CMD PENSIZE 0 0 0
DatumPtr Kernel::excPensize(DatumPtr node) {
  ProcedureHelper h(this, node);
  double retval = mainTurtle()->getPenSize();
  return h.ret(retval);
}


/***DOC BACKGROUND BG
BACKGROUND
BG

    outputs a list of three nonnegative numbers less than 100 specifying
    the percent saturation of red, green, and blue in the color associated
    with the current background color.


COD***/
//CMD BACKGROUND 0 0 0
//CMD BG 0 0 0
DatumPtr Kernel::excBackground(DatumPtr node) {
  ProcedureHelper h(this, node);
  QColor c = mainController()->getCanvasBackgroundColor();

  return h.ret(listFromColor(c));
}

// SAVING AND LOADING PICTURES


/***DOC SAVEPICT
SAVEPICT filename

    command.  Writes a file with the specified name containing the
    contents of the graphics window, in the format determined by the filename's
    extension. The dimensions of the image are determined by the canvas bounds.
    See SVGPICT to export Logo graphics as SVG.

COD***/
//CMD SAVEPICT 1 1 1
DatumPtr Kernel::excSavepict(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr filenameP = h.wordAtIndex(0);

  QString filepath = filepathForFilename(filenameP);
  QImage image = mainController()->getCanvasImage();
  bool isSuccessful = image.save(filepath);
  if (!isSuccessful) {
    return h.ret(Error::fileSystemRecoverable());
  }
  return nothing;
}

/***DOC LOADPICT
LOADPICT filename

    command.  Reads the image file with the specified filename and sets the image
    as the canvas background. The image will be stretched, if necessary, to fit
    the bounds of the canvas.

    The filename may also be an empty list, in which case any image previously
    set as the background will be cleared.

COD***/
//CMD LOADPICT 1 1 1
DatumPtr Kernel::excLoadpict(DatumPtr node) {
    ProcedureHelper h(this, node);
    DatumPtr filenameP = h.validatedDatumAtIndex(0, [](DatumPtr candidate) {
        if (candidate.isList() && (candidate.listValue()->size() == 0))
            return true;
        return candidate.isWord();
    });
    if (filenameP.isWord()) {
        QString filepath = filepathForFilename(filenameP);
        QImage image = QImage(filepath);
        if (image.isNull()) {
            return h.ret(Error::fileSystemRecoverable());
        }

        mainController()->setCanvasBackgroundImage(image);
    } else {
        mainController()->setCanvasBackgroundImage(QImage());
    }
    return nothing;
}

/***DOC SVGPICT
SVGPICT filename

    command.  Writes a file with the specified name containing the
    contents of the graphics window in SVG format. The dimensions of the image
    are determined by the canvas bounds.

COD***/
//CMD SVGPICT 1 1 1
DatumPtr Kernel::excSvgpict(DatumPtr node) {
    ProcedureHelper h(this, node);
    DatumPtr filenameP = h.wordAtIndex(0);

    QString filepath = filepathForFilename(filenameP);
    QByteArray svgImage = mainController()->getSvgImage();

    QFile file(filepath);
    bool isSuccessful = file.open(QIODevice::WriteOnly);
    if (!isSuccessful) {
        return h.ret(Error::fileSystemRecoverable());
    }

    qint64 bytesWritten = file.write(svgImage);
    if (bytesWritten != svgImage.size())
        return h.ret(Error::fileSystemRecoverable());

    return nothing;
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
//CMD MOUSEPOS 0 0 0
DatumPtr Kernel::excMousepos(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = new List();
  QVector2D position = mainController()->mousePosition();
  retval->append(DatumPtr(position.x()));
  retval->append(DatumPtr(position.y()));
  return h.ret(retval);
}


/***DOC CLICKPOS
CLICKPOS

    outputs the coordinates that the mouse was at when a mouse button
    was most recently pushed, provided that that position was within the
    graphics window, in turtle coordinates.

COD***/
//CMD CLICKPOS 0 0 0
DatumPtr Kernel::excClickpos(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = new List();
  QVector2D position = mainController()->lastMouseclickPosition();
  retval->append(DatumPtr(position.x()));
  retval->append(DatumPtr(position.y()));
  return h.ret(retval);
}


/***DOC BUTTONP BUTTON?
BUTTONP
BUTTON?

    outputs TRUE if a mouse button is down and the mouse is over the
    graphics window.  Once the button is down, BUTTONP remains true until
    the button is released, even if the mouse is dragged out of the
    graphics window.

COD***/
//CMD BUTTONP 0 0 0
//CMD BUTTON? 0 0 0
DatumPtr Kernel::excButtonp(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(mainController()->getIsMouseButtonDown());
}


/***DOC BUTTON
BUTTON

    outputs 0 if no mouse button has been pushed inside the Logo window
    since the last call to BUTTON.  Otherwise, it outputs an integer
    indicating which button was most recently pressed.
    1 means left, 2 means right.



COD***/
//CMD BUTTON 0 0 0
DatumPtr Kernel::excButton(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(mainController()->getAndResetButtonID());
}
