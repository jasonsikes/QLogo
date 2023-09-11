
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

DatumP listFromColor(QColor c) {
  List *retval = new List;
  retval->append(DatumP(round(c.redF() * 100)));
  retval->append(DatumP(round(c.greenF() * 100)));
  retval->append(DatumP(round(c.blueF() * 100)));
  return DatumP(retval);
}

char axisFromDatump(DatumP candidate)
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

DatumP Kernel::excForward(DatumP node) {
  ProcedureHelper h(this, node);
  double value = h.numberAtIndex(0);

  mainTurtle()->move(0, value, 0);

  return nothing;
}

DatumP Kernel::excBack(DatumP node) {
  ProcedureHelper h(this, node);
  double value = h.numberAtIndex(0);

  mainTurtle()->move(0, -value, 0);

  return nothing;
}

DatumP Kernel::excLeft(DatumP node) {
  ProcedureHelper h(this, node);
  double value = h.numberAtIndex(0);

  mainTurtle()->rotate(value, 'Z');

  return nothing;
}

DatumP Kernel::excRight(DatumP node) {
  ProcedureHelper h(this, node);
  double value = h.numberAtIndex(0);

  mainTurtle()->rotate(-value, 'Z');

  return nothing;
}

DatumP Kernel::excSetpos(DatumP node) {
  ProcedureHelper h(this, node);

  QVector<double> v;
  h.validatedDatumAtIndex(0, [&v, this](DatumP candidate) {
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

DatumP Kernel::excSetXY(DatumP node) {
  ProcedureHelper h(this, node);
  double x = h.numberAtIndex(0);
  double y = h.numberAtIndex(1);

  mainTurtle()->setxy(x, y);

  return nothing;
}

DatumP Kernel::excSetXYZ(DatumP node) {
  ProcedureHelper h(this, node);
  double x = h.numberAtIndex(0);
  double y = h.numberAtIndex(1);
  double z = h.numberAtIndex(2);

  mainTurtle()->setxyz(x, y, z);

  return nothing;
}

DatumP Kernel::excSetX(DatumP node) {
  ProcedureHelper h(this, node);
  double x = h.numberAtIndex(0);

  mainTurtle()->setx(x);

  return nothing;
}

DatumP Kernel::excSetY(DatumP node) {
  ProcedureHelper h(this, node);
  double y = h.numberAtIndex(0);

  mainTurtle()->sety(y);

  return nothing;
}

DatumP Kernel::excSetZ(DatumP node) {
  ProcedureHelper h(this, node);
  double z = h.numberAtIndex(0);

  mainTurtle()->setz(z);

  return nothing;
}

DatumP Kernel::excSetheading(DatumP node) {
  ProcedureHelper h(this, node);
  double newHeading = h.numberAtIndex(0);
  char axis = 'Z';
  if (node.astnodeValue()->countOfChildren() == 2) {
    h.validatedDatumAtIndex(1, [&axis](DatumP candidate) {
        char cAxis = axisFromDatump(candidate);
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

DatumP Kernel::excHome(DatumP node) {
  ProcedureHelper h(this, node);
  mainTurtle()->home();

  return nothing;
}

DatumP Kernel::excArc(DatumP node) {
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

DatumP Kernel::excPos(DatumP node) {
  ProcedureHelper h(this, node);
  double x, y, z;
  mainTurtle()->getxyz(x, y, z);

  List *retval = new List;
  retval->append(DatumP(x));
  retval->append(DatumP(y));
  if (h.countOfChildren() > 0) {
    retval->append(DatumP(z));
  }
  return h.ret(retval);
}

DatumP Kernel::excHeading(DatumP node) {
  ProcedureHelper h(this, node);
  char axis = 'Z';
  if (node.astnodeValue()->countOfChildren() == 2) {
    h.validatedDatumAtIndex(1, [&axis](DatumP candidate) {
      char cAxis = axisFromDatump(candidate);
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

DatumP Kernel::excTowards(DatumP node) {
  ProcedureHelper h(this, node);
  QVector<double> v;
  double x, y, z;
  h.validatedDatumAtIndex(0, [&v, this](DatumP candidate) {
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

DatumP Kernel::excScrunch(DatumP node) {
  ProcedureHelper h(this, node);
  List *retval = new List;
  retval->append(DatumP(1));
  retval->append(DatumP(1));
  return h.ret(retval);
}

// TURTLE AND WINDOW CONTROL

DatumP Kernel::excShowturtle(DatumP node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setIsTurtleVisible(true);
  mainController()->setTurtleIsVisible(true);

  return nothing;
}

DatumP Kernel::excHideturtle(DatumP node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setIsTurtleVisible(false);
  mainController()->setTurtleIsVisible(false);

  return nothing;
}

DatumP Kernel::excClean(DatumP node) {
  ProcedureHelper h(this, node);
  mainTurtle()->home(false);
  mainController()->clearScreen();
  return nothing;
}

DatumP Kernel::excClearscreen(DatumP node) {
  ProcedureHelper h(this, node);
  mainController()->clearScreen();

  return nothing;
}

DatumP Kernel::excWrap(DatumP node) {
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

DatumP Kernel::excWindow(DatumP node) {
  ProcedureHelper h(this, node);
  TurtleModeEnum newMode = turtleWindow;
  if (mainTurtle()->getMode() != newMode) {
    mainTurtle()->setMode(newMode);
    mainController()->setIsCanvasBounded(false);
  }
  return nothing;
}

DatumP Kernel::excFence(DatumP node) {
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

DatumP Kernel::excBounds(DatumP node) {
  ProcedureHelper h(this, node);
  double x = mainController()->boundX();
  double y = mainController()->boundY();

  List *retval = new List;
  retval->append(new Word(x));
  retval->append(new Word(y));
  return h.ret(retval);
}

DatumP Kernel::excSetbounds(DatumP node) {
  ProcedureHelper h(this, node);
  auto v = [](double candidate) { return candidate > 0; };

  double x = h.validatedNumberAtIndex(0, v);
  double y = h.validatedNumberAtIndex(1, v);

  mainController()->setBounds(x, y);

  return nothing;
}

DatumP Kernel::excFilled(DatumP node) {
  ProcedureHelper h(this, node);
  QColor c;
  h.validatedDatumAtIndex(0, [&c, this](DatumP candidate) {
    return colorFromDatumP(c, candidate);
  });

  DatumP commandList = h.datumAtIndex(1);

  mainTurtle()->beginFillWithColor(c);
  DatumP retval;
  try {
    retval = runList(commandList);
  } catch (Error *e) {
    mainTurtle()->endFill();
    throw e;
  }
  mainTurtle()->endFill();
  return h.ret(retval);
}

DatumP Kernel::excLabel(DatumP node) {
  ProcedureHelper h(this, node);
  QString text = h.wordAtIndex(0).wordValue()->printValue();
  double x = 0, y = 0, z = 0;
  mainTurtle()->getxyz(x, y, z);
  QVector3D pos(x, y, z);
  mainController()->drawLabel(text, pos, mainTurtle()->getPenColor());
  return nothing;
}

DatumP Kernel::excSetlabelheight(DatumP node) {
  ProcedureHelper h(this, node);
  double height = h.validatedNumberAtIndex(
      0, [](double candidate) { return candidate > 0; });
  mainController()->setLabelFontSize(height);
  return nothing;
}

DatumP Kernel::excTextscreen(DatumP node) {
  ProcedureHelper h(this, node);
  mainController()->setScreenMode(textScreenMode);
  return nothing;
}

DatumP Kernel::excFullscreen(DatumP node) {
  ProcedureHelper h(this, node);
  mainController()->setScreenMode(fullScreenMode);
  return nothing;
}

DatumP Kernel::excSplitscreen(DatumP node) {
  ProcedureHelper h(this, node);
  mainController()->setScreenMode(splitScreenMode);
  return nothing;
}

DatumP Kernel::excSetscrunch(DatumP node) {
  ProcedureHelper h(this, node);
  return nothing;
}

// TURTLE AND WINDOW QUERIES

DatumP Kernel::excShownp(DatumP node) {
  ProcedureHelper h(this, node);
  bool retval = mainTurtle()->isTurtleVisible();
  return h.ret(retval);
}

DatumP Kernel::excScreenmode(DatumP node) {
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

DatumP Kernel::excTurtlemode(DatumP node) {
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

DatumP Kernel::excLabelheight(DatumP node) {
  ProcedureHelper h(this, node);
  double retval = mainController()->getLabelFontSize();
  return h.ret(retval);
}

DatumP Kernel::excMatrix(DatumP node) {
  ProcedureHelper h(this, node);
  List *retval = new List;
  const QMatrix4x4 &m = mainTurtle()->getMatrix();
  for (int row = 0; row < 4; ++row) {
    List *r = new List;
    for (int col = 0; col < 4; ++col) {
      r->append(DatumP(m(row, col)));
    }
    retval->append(DatumP(r));
  }
  return h.ret(retval);
}

// PEN AND BACKGROUND CONTROL

DatumP Kernel::excPendown(DatumP node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(true);

  return nothing;
}

DatumP Kernel::excPenup(DatumP node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(false);

  return nothing;
}

DatumP Kernel::excPenpaint(DatumP node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(true);
  mainTurtle()->setPenMode(penModePaint);
  return nothing;
}

DatumP Kernel::excPenerase(DatumP node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(true);
  mainTurtle()->setPenMode(penModeErase);
  return nothing;
}

DatumP Kernel::excPenreverse(DatumP node) {
  ProcedureHelper h(this, node);
  mainTurtle()->setPenIsDown(true);
  mainTurtle()->setPenMode(penModeReverse);
  return nothing;
}

DatumP Kernel::excSetpencolor(DatumP node) {
  ProcedureHelper h(this, node);
  QColor c;
  h.validatedDatumAtIndex(0, [&c, this](DatumP candidate) {
    return colorFromDatumP(c, candidate);
  });
  mainTurtle()->setPenColor(c);
  return nothing;
}

DatumP Kernel::excSetpalette(DatumP node) {
  ProcedureHelper h(this, node);
  int colornumber = h.validatedIntegerAtIndex(0, [this](int candidate) {
    return (candidate >= 8) && (candidate < palette.size());
  });
  QColor c;
  h.validatedDatumAtIndex(1, [&c, this](DatumP candidate) {
    return colorFromDatumP(c, candidate);
  });
  palette[colornumber] = c;
  return nothing;
}

DatumP Kernel::excSetpensize(DatumP node) {
  ProcedureHelper h(this, node);
  double newSize = h.validatedNumberAtIndex(0, [](double candidate) {
    return mainTurtle()->isPenSizeValid(candidate);
  });
  mainTurtle()->setPenSize(newSize);
  return nothing;
}

DatumP Kernel::excSetbackground(DatumP node) {
  ProcedureHelper h(this, node);
  QColor c;
  h.validatedDatumAtIndex(0, [&c, this](DatumP candidate) {
    return colorFromDatumP(c, candidate);
  });
  mainController()->setCanvasBackgroundColor(c);
  return nothing;
}

// PEN QUERIES

DatumP Kernel::excPendownp(DatumP node) {
  ProcedureHelper h(this, node);
  return h.ret(mainTurtle()->isPenDown());
}

DatumP Kernel::excPenmode(DatumP node) {
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

DatumP Kernel::excPencolor(DatumP node) {
  ProcedureHelper h(this, node);
  const QColor &c = mainTurtle()->getPenColor();
  return h.ret(listFromColor(c));
}

DatumP Kernel::excPalette(DatumP node) {
  ProcedureHelper h(this, node);
  int colornumber = h.validatedIntegerAtIndex(0, [this](int candidate) {
    return (candidate >= 0) && (candidate < palette.size());
  });
  return h.ret(listFromColor(palette[colornumber]));
}

DatumP Kernel::excPensize(DatumP node) {
  ProcedureHelper h(this, node);
  double retval = mainTurtle()->getPenSize();
  return h.ret(retval);
}

DatumP Kernel::excBackground(DatumP node) {
  ProcedureHelper h(this, node);
  QColor c = mainController()->getCanvasBackgroundColor();

  return h.ret(listFromColor(c));
}

// SAVING AND LOADING PICTURES

DatumP Kernel::excSavepict(DatumP node) {
  ProcedureHelper h(this, node);
  DatumP filenameP = h.wordAtIndex(0);

  const QString filepath = filepathForFilename(filenameP);
  QImage image = mainController()->getCanvasImage();
  bool isSuccessful = image.save(filepath);
  if (!isSuccessful) {
    return h.ret(Error::fileSystemRecoverable());
  }
  return nothing;
}

// MOUSE QUERIES

DatumP Kernel::excMousepos(DatumP node) {
  ProcedureHelper h(this, node);
  List *retval = new List;
  QVector2D position = mainController()->mousePosition();
  retval->append(DatumP(position.x()));
  retval->append(DatumP(position.y()));
  return h.ret(retval);
}

DatumP Kernel::excClickpos(DatumP node) {
  ProcedureHelper h(this, node);
  List *retval = new List;
  QVector2D position = mainController()->lastMouseclickPosition();
  retval->append(DatumP(position.x()));
  retval->append(DatumP(position.y()));
  return h.ret(retval);
}

DatumP Kernel::excButtonp(DatumP node) {
  ProcedureHelper h(this, node);
  return h.ret(mainController()->getIsMouseButtonDown());
}

DatumP Kernel::excButton(DatumP node) {
  ProcedureHelper h(this, node);
  return h.ret(new Word(mainController()->getAndResetButtonID()));
}
