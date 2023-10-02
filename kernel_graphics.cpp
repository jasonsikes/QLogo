
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
#include "datum_word.h"
#include "datum_list.h"
#include "datum_astnode.h"
#include "stringconstants.h"
#include "logocontroller.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DatumPtr listFromColor(QColor c) {
  List *retval = List::alloc();
  retval->append(DatumPtr(round(c.redF() * 100)));
  retval->append(DatumPtr(round(c.greenF() * 100)));
  retval->append(DatumPtr(round(c.blueF() * 100)));
  return DatumPtr(retval);
}

char axisFromDatumPtr(DatumPtr candidate)
{
  if (!candidate.isWord())
      return 0;
  if (candidate.wordValue()->rawValue().size() != 1)
      return 0;
  char retval = candidate.wordValue()->keyValue()[0].toLatin1();
  if ((retval != 'X') && (retval != 'Y') && (retval != 'Z'))
      return 0;
  return retval;
}

// TURTLE MOTION

DatumPtr Kernel::excForward(DatumPtr node) {
  ProcedureHelper h(this, node);
  double value = h.numberAtIndex(0);

  mainTurtle()->move(0, value, 0);

  return nothing;
}

DatumPtr Kernel::excBack(DatumPtr node) {
  ProcedureHelper h(this, node);
  double value = h.numberAtIndex(0);

  mainTurtle()->move(0, -value, 0);

  return nothing;
}

DatumPtr Kernel::excLeft(DatumPtr node) {
  ProcedureHelper h(this, node);
  double value = h.numberAtIndex(0);

  mainTurtle()->rotate(value, 'Z');

  return nothing;
}

DatumPtr Kernel::excRight(DatumPtr node) {
  ProcedureHelper h(this, node);
  double value = h.numberAtIndex(0);

  mainTurtle()->rotate(-value, 'Z');

  return nothing;
}

DatumPtr Kernel::excSetpos(DatumPtr node) {
  ProcedureHelper h(this, node);

  QVector<double> v;
  h.validatedDatumAtIndex(0, [&v, this](DatumPtr candidate) {
    if (!candidate.isList())
      return false;
    if (!numbersFromList(v, candidate))
      return false;
    if ((v.size() != 2) && (v.size() != 3))
      return false;
    return true;
  });

  if (v.size() == 3) {
    mainTurtle()->setxyz(v[0], v[1], v[2]);
  } else {
    mainTurtle()->setxy(v[0], v[1]);
  }

  return nothing;
}

DatumPtr Kernel::excSetXY(DatumPtr node) {
  ProcedureHelper h(this, node);
  double x = h.numberAtIndex(0);
  double y = h.numberAtIndex(1);

  mainTurtle()->setxy(x, y);

  return nothing;
}

DatumPtr Kernel::excSetXYZ(DatumPtr node) {
  ProcedureHelper h(this, node);
  double x = h.numberAtIndex(0);
  double y = h.numberAtIndex(1);
  double z = h.numberAtIndex(2);

  mainTurtle()->setxyz(x, y, z);

  return nothing;
}

DatumPtr Kernel::excSetX(DatumPtr node) {
  ProcedureHelper h(this, node);
  double x = h.numberAtIndex(0);

  mainTurtle()->setx(x);

  return nothing;
}

DatumPtr Kernel::excSetY(DatumPtr node) {
  ProcedureHelper h(this, node);
  double y = h.numberAtIndex(0);

  mainTurtle()->sety(y);

  return nothing;
}

DatumPtr Kernel::excSetZ(DatumPtr node) {
  ProcedureHelper h(this, node);
  double z = h.numberAtIndex(0);

  mainTurtle()->setz(z);

  return nothing;
}

DatumPtr Kernel::excSetheading(DatumPtr node) {
  ProcedureHelper h(this, node);
  double newHeading = h.numberAtIndex(0);
  char axis = 'Z';
  if (node.astnodeValue()->countOfChildren() == 2) {
    h.validatedDatumAtIndex(1, [&axis](DatumPtr candidate) {
        char cAxis = axisFromDatumPtr(candidate);
        if (cAxis == 0)
            return false;
        axis = cAxis;
        return true;
    });
  }
  double oldHeading = mainTurtle()->getHeading(axis);

  // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
  newHeading = 360 - newHeading;

  double adjustment = newHeading - oldHeading;
  mainTurtle()->rotate(adjustment, axis);
  return nothing;
}

DatumPtr Kernel::excHome(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->home();

  return nothing;
}

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

DatumPtr Kernel::excPos(DatumPtr node) {
  ProcedureHelper h(this, node);
  double x, y, z;
  mainTurtle()->getxyz(x, y, z);

  List *retval = List::alloc();
  retval->append(DatumPtr(x));
  retval->append(DatumPtr(y));
  if (h.countOfChildren() > 0) {
    retval->append(DatumPtr(z));
  }
  return h.ret(retval);
}

DatumPtr Kernel::excHeading(DatumPtr node) {
  ProcedureHelper h(this, node);
  char axis = 'Z';
  if (node.astnodeValue()->countOfChildren() == 2) {
    h.validatedDatumAtIndex(1, [&axis](DatumPtr candidate) {
      char cAxis = axisFromDatumPtr(candidate);
      if (cAxis == 0)
        return false;
      axis = cAxis;
      return true;
    });
  }
  double retval = mainTurtle()->getHeading(axis);

  // Heading is positive in the counter-clockwise direction.
  if (retval > 0)
      retval = 360 - retval;

  return h.ret(retval);
}

DatumPtr Kernel::excTowards(DatumPtr node) {
  ProcedureHelper h(this, node);
  QVector<double> v;
  double x, y, z;
  h.validatedDatumAtIndex(0, [&v, this](DatumPtr candidate) {
    if (!candidate.isList())
      return false;
    if (!numbersFromList(v, candidate))
      return false;
    if (v.size() != 2)
      return false;
    return true;
  });
  mainTurtle()->getxyz(x, y, z);
  double retval = atan2(x - v[0], v[1] - y) * (180 / M_PI);
  if (retval < 0)
    retval += 360;

  // Heading is positive in the counter-clockwise direction.
  if (retval > 0)
      retval = 360 - retval;

  return h.ret(retval);
}

DatumPtr Kernel::excScrunch(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = List::alloc();
  retval->append(DatumPtr(1));
  retval->append(DatumPtr(1));
  return h.ret(retval);
}

// TURTLE AND WINDOW CONTROL

DatumPtr Kernel::excShowturtle(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setIsTurtleVisible(true);
  mainController()->setTurtleIsVisible(true);

  return nothing;
}

DatumPtr Kernel::excHideturtle(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setIsTurtleVisible(false);
  mainController()->setTurtleIsVisible(false);

  return nothing;
}

DatumPtr Kernel::excClean(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->home(false);
  mainController()->clearScreen();
  return nothing;
}

DatumPtr Kernel::excClearscreen(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->clearScreen();

  return nothing;
}

DatumPtr Kernel::excWrap(DatumPtr node) {
  ProcedureHelper h(this, node);
  TurtleModeEnum newMode = turtleWrap;
  if (mainTurtle()->getMode() != newMode) {
    mainTurtle()->setMode(newMode);
    if (mainController()->isCanvasBounded() == false) {
        mainController()->setIsCanvasBounded(true);
        mainTurtle()->home(false);
        mainController()->clearScreen();
    }
  }
  return nothing;
}

DatumPtr Kernel::excWindow(DatumPtr node) {
  ProcedureHelper h(this, node);
  TurtleModeEnum newMode = turtleWindow;
  if (mainTurtle()->getMode() != newMode) {
    mainTurtle()->setMode(newMode);
    mainController()->setIsCanvasBounded(false);
  }
  return nothing;
}

DatumPtr Kernel::excFence(DatumPtr node) {
  ProcedureHelper h(this, node);
  TurtleModeEnum newMode = turtleFence;
  if (mainTurtle()->getMode() != newMode) {
    mainTurtle()->setMode(newMode);
    if (mainController()->isCanvasBounded() == false) {
        mainController()->setIsCanvasBounded(true);
        mainTurtle()->home(false);
        mainController()->clearScreen();
    }
  }
  return nothing;
}

DatumPtr Kernel::excBounds(DatumPtr node) {
  ProcedureHelper h(this, node);
  double x = mainController()->boundX();
  double y = mainController()->boundY();

  List *retval = List::alloc();
  retval->append(DatumPtr(x));
  retval->append(DatumPtr(y));
  return h.ret(retval);
}

DatumPtr Kernel::excSetbounds(DatumPtr node) {
  ProcedureHelper h(this, node);
  auto v = [](double candidate) { return candidate > 0; };

  double x = h.validatedNumberAtIndex(0, v);
  double y = h.validatedNumberAtIndex(1, v);

  mainController()->setBounds(x, y);

  return nothing;
}

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

DatumPtr Kernel::excLabel(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString text = h.wordAtIndex(0).wordValue()->printValue();
  double x = 0, y = 0, z = 0;
  mainTurtle()->getxyz(x, y, z);
  QVector3D pos(x, y, z);
  mainController()->drawLabel(text, pos, mainTurtle()->getPenColor());
  return nothing;
}

DatumPtr Kernel::excSetlabelheight(DatumPtr node) {
  ProcedureHelper h(this, node);
  double height = h.validatedNumberAtIndex(
      0, [](double candidate) { return candidate > 0; });
  mainController()->setLabelFontSize(height);
  return nothing;
}

DatumPtr Kernel::excTextscreen(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->setScreenMode(textScreenMode);
  return nothing;
}

DatumPtr Kernel::excFullscreen(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->setScreenMode(fullScreenMode);
  return nothing;
}

DatumPtr Kernel::excSplitscreen(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainController()->setScreenMode(splitScreenMode);
  return nothing;
}

DatumPtr Kernel::excSetscrunch(DatumPtr node) {
  ProcedureHelper h(this, node);
  return nothing;
}

// TURTLE AND WINDOW QUERIES

DatumPtr Kernel::excShownp(DatumPtr node) {
  ProcedureHelper h(this, node);
  bool retval = mainTurtle()->isTurtleVisible();
  return h.ret(retval);
}

DatumPtr Kernel::excScreenmode(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString retval;
  switch (mainController()->getScreenMode()) {
  case textScreenMode:
  case initScreenMode:
    retval = k.textscreen();
    break;
  case fullScreenMode:
    retval = k.fullscreen();
    break;
  case splitScreenMode:
    retval = k.splitscreen();
    break;
  default:
    break;
  }
  return h.ret(retval);
}

DatumPtr Kernel::excTurtlemode(DatumPtr node) {
  ProcedureHelper h(this, node);
  QString retval;
  switch (mainTurtle()->getMode()) {
  case turtleWrap:
    retval = k.wrap();
    break;
  case turtleFence:
    retval = k.fence();
    break;
  case turtleWindow:
    retval = k.window();
    break;
  default:
    qDebug() << "what mode is the turtle?";
    Q_ASSERT(false);
    break;
  }
  return h.ret(retval);
}

DatumPtr Kernel::excLabelheight(DatumPtr node) {
  ProcedureHelper h(this, node);
  double retval = mainController()->getLabelFontSize();
  return h.ret(retval);
}

DatumPtr Kernel::excMatrix(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = List::alloc();
  const QMatrix4x4 &m = mainTurtle()->getMatrix();
  for (int row = 0; row < 4; ++row) {
    List *r = List::alloc();
    for (int col = 0; col < 4; ++col) {
      r->append(DatumPtr(m(row, col)));
    }
    retval->append(DatumPtr(r));
  }
  return h.ret(retval);
}

// PEN AND BACKGROUND CONTROL

DatumPtr Kernel::excPendown(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(true);

  return nothing;
}

DatumPtr Kernel::excPenup(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(false);

  return nothing;
}

DatumPtr Kernel::excPenpaint(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(true);
  mainTurtle()->setPenMode(penModePaint);
  return nothing;
}

DatumPtr Kernel::excPenerase(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(true);
  mainTurtle()->setPenMode(penModeErase);
  return nothing;
}

DatumPtr Kernel::excPenreverse(DatumPtr node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(true);
  mainTurtle()->setPenMode(penModeReverse);
  return nothing;
}

DatumPtr Kernel::excSetpencolor(DatumPtr node) {
  ProcedureHelper h(this, node);
  QColor c;
  h.validatedDatumAtIndex(0, [&c, this](DatumPtr candidate) {
    return colorFromDatumPtr(c, candidate);
  });
  mainTurtle()->setPenColor(c);
  return nothing;
}

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

DatumPtr Kernel::excSetpensize(DatumPtr node) {
  ProcedureHelper h(this, node);
  double newSize = h.validatedNumberAtIndex(0, [](double candidate) {
    return mainTurtle()->isPenSizeValid(candidate);
  });
  mainTurtle()->setPenSize(newSize);
  return nothing;
}

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

DatumPtr Kernel::excPendownp(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(mainTurtle()->isPenDown());
}

DatumPtr Kernel::excPenmode(DatumPtr node) {
  ProcedureHelper h(this, node);
  PenModeEnum pm = mainTurtle()->getPenMode();
  QString retval;
  switch (pm) {
  case penModePaint:
    retval = k.paint();
    break;
  case penModeReverse:
    retval = k.reverse();
    break;
  case penModeErase:
    retval = k.erase();
    break;
  default:
    retval = "ERROR!!!";
    break;
  }
  return h.ret(retval);
}

DatumPtr Kernel::excPencolor(DatumPtr node) {
  ProcedureHelper h(this, node);
  const QColor &c = mainTurtle()->getPenColor();
  return h.ret(listFromColor(c));
}

DatumPtr Kernel::excPalette(DatumPtr node) {
  ProcedureHelper h(this, node);
  int colornumber = h.validatedIntegerAtIndex(0, [this](int candidate) {
    return (candidate >= 0) && (candidate < palette.size());
  });
  return h.ret(listFromColor(palette[colornumber]));
}

DatumPtr Kernel::excPensize(DatumPtr node) {
  ProcedureHelper h(this, node);
  double retval = mainTurtle()->getPenSize();
  return h.ret(retval);
}

DatumPtr Kernel::excBackground(DatumPtr node) {
  ProcedureHelper h(this, node);
  QColor c = mainController()->getCanvasBackgroundColor();

  return h.ret(listFromColor(c));
}

// SAVING AND LOADING PICTURES

DatumPtr Kernel::excSavepict(DatumPtr node) {
  ProcedureHelper h(this, node);
  DatumPtr filenameP = h.wordAtIndex(0);

  const QString filepath = filepathForFilename(filenameP);
  QImage image = mainController()->getCanvasImage();
  bool isSuccessful = image.save(filepath);
  if (!isSuccessful) {
    return h.ret(Error::fileSystemRecoverable());
  }
  return nothing;
}

// MOUSE QUERIES

DatumPtr Kernel::excMousepos(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = List::alloc();
  QVector2D position = mainController()->mousePosition();
  retval->append(DatumPtr(position.x()));
  retval->append(DatumPtr(position.y()));
  return h.ret(retval);
}

DatumPtr Kernel::excClickpos(DatumPtr node) {
  ProcedureHelper h(this, node);
  List *retval = List::alloc();
  QVector2D position = mainController()->lastMouseclickPosition();
  retval->append(DatumPtr(position.x()));
  retval->append(DatumPtr(position.y()));
  return h.ret(retval);
}

DatumPtr Kernel::excButtonp(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(mainController()->getIsMouseButtonDown());
}

DatumPtr Kernel::excButton(DatumPtr node) {
  ProcedureHelper h(this, node);
  return h.ret(mainController()->getAndResetButtonID());
}
