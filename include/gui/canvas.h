#ifndef CANVAS_H
#define CANVAS_H

//===-- qlogo/gui/canvas.h - Canvas class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Canvas class, which is the
/// graphics portion of the user interface, where the turtle roams.
///
//===----------------------------------------------------------------------===//

#include <QStaticText>
#include <QWidget>
#include <QPen>
#include <QVariant>
#include <QPainter>
#include "sharedconstants.h"

class MainWindow;

/// Contains the information that describes a label's appearance on the Canvas.
struct Label {
  QPointF position;
  QStaticText text;
  QFont font;

  Label(QString aText, QPointF aPosition, QFont aFont)
      : text(aText),position(aPosition), font(aFont)
  {}
};


/// Contains information that describes a polygon.
struct Polygon {
    QColor color;
    QPolygonF points;
};


/// Contains information that describes a change in how a turtle draws.
struct TurtleWriteInfo {
    QPainter::CompositionMode composingMode; // either SourceOver or Difference
    QPen pen; // color and size
};


using deVariant = std::variant<Label, Polygon, TurtleWriteInfo, QPolygonF>;



/// The variant structure for the individual drawing elements.
struct DrawingElement {
    int eID;
    deVariant element;
};

enum ElementID {
    labelTypeID,
    polygonTypeID,
    turtleWriteInfoID,
    polylineTypeID,
};


/// The widget where turtle graphics are drawn.
class Canvas : public QWidget {
    Q_OBJECT

    // Turtle vars:
    QTransform turtleMatrix; // The location and orientation of the turtle.
    bool turtleIsVisible;

    QTransform turtleImageMatrix; // The matrix applied when drawing the turtle
    QImage turtleImage;

  bool canvasIsBounded = true;
  bool mouseButtonPressed = false;
  QTransform drawingMatrix; // For mapping cartesian to widget.
  QTransform inverseDrawingMatrix; // For mapping mouse to cartesian.

  // Visible vertices on the X axis range from -boundsX to +boundsX
  qreal boundsX;
  // Visible vertices on the Y axis range from -boundsY to +boundsY
  qreal boundsY;

  QColor foregroundColor;
  QColor backgroundColor;
  TurtleWriteInfo currentWriteInfo;
  PenModeEnum penMode = penModePaint; // paint,erase,reverse
  bool penIsDown = true;

  // The main data structure for all of the drawn elements on the canvas.
  QList<DrawingElement> drawingElementList;


  // The currently-constructing line sequence.
  QPolygonF lineGroup;

  // The currently-constructing polygon.
  bool isConstructingPolygon = false;
  QColor polygonColor;
  QPolygonF polygonGroup;

  QImage backgroundImage;

  // The QPainter object. This is only valid during the paintEvent().
  QPainter *painter;

  // The current label drawing font
  QFont labelFont;


  void initDrawingElementList();
  void initTurtleImage();

  void drawCanvas();
  // the "el*" methods are the executors of each element of the DrawingElementList.
  void elSetWriteInfo(const TurtleWriteInfo &info);
  void elDrawBoundedBackground();
  void elDrawUnboundedBackground();
  void elDrawTurtle();
  void elDrawPolyline(const QPolygonF &);
  void elDrawPolygon(const Polygon &p);
  void elDrawLabel(const Label &);

  // Return the appropriate color for the current penMode.
  const QColor& colorForPenmode();

  void updateMatrix(void);

  QPointF pointFromTurtle();

  void pushLineGroup();
  void setLastWriteInfo();

  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

public:
  /// Construct a Canvas
  Canvas(QWidget *parent = 0);

  /// Sets the position and orientation of the turtle.
  void setTurtleMatrix(const QTransform &aTurtleMatrix);

  /// Record that the turtle moved. If drawing lines and/or polygons, add vertex
  /// to their lists.
  void emitVertex();

  /// Show or hide the turtle.
  void setTurtleIsVisible(bool isVisible);

  /// Set the pen up or down.
  void setPenIsDown(bool aPenIsDown);

  /// Begin the definition of a polygon. The first point of the polygon is the
  /// current posion of the turtle. All future turtle movement will add a point
  /// to the polygon until endPolygon() is received.
  void beginPolygon(const QColor &color);

  /// End the definition of a polygon. A polygon object will be added to the
  /// paintElements list.
  void endPolygon();

  /// Add a label at the current mouse location.
  void addLabel(QString aText);

  /// Set the font size for all future text labels.
  void setLabelFontSize(qreal aSize);

  /// Set the font name for all future text labels.
  void setLabelFontName(QString name);

  /// Sets future lines and polygons to be drawn using newMode.
  /// (draw, erase, or reverse)
  void setPenmode(PenModeEnum newMode);

  /// Set the pen color for all future line and label drawings.
  void setForegroundColor(const QColor &aColor);

  /// Sets the width of future lines.
  void setPensize(qreal aSize);

  /// Sets the background color to c.
  ///
  /// The background is drawn either as a filled rectangle when isBounded=true
  /// or the background color fills the entire widget when isBounded=false.
  void setBackgroundColor(const QColor &c);

  /// Set background image.
  ///
  /// If image is a valid image, then it will be drawn onto the canvas
  /// above the background color and below everything the turtle draws. The
  /// image will be scaled to fit the bounds set by setBounds().
  void setBackgroundImage(QImage image);

  /// Redraw the canvas onto an image object and return it.
  QImage getImage();

  /// Clears the screen and removes all painting elements from the paintList.
  void clearScreen();

  /// Get the maximum X bound
  qreal xbound() { return boundsX; }

  /// Get the maximum Y bound
  qreal ybound() { return boundsY; }

  /// Set the maximum X and Y bounds
  void setBounds(qreal x, qreal y);

  /// Set whether the view should use the whole widget or a box within.
  void setIsBounded(bool aIsBounded) { canvasIsBounded = aIsBounded; update(); }

  /// Returns true if the bounds are drawn
  bool isBounded() { return canvasIsBounded; }

signals:
  void sendMouseclickedSignal(QPointF position, int buttonID);
  void sendMousemovedSignal(QPointF position);
  void sendMouseReleasedSignal();

};

#endif // CANVAS_H
