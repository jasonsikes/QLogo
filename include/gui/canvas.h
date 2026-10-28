#ifndef CANVAS_H
#define CANVAS_H

//===-- qlogo/gui/canvas.h - Canvas class definition -------*- C++ -*-===//
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
/// This file contains the declaration of the Canvas class, which is the
/// graphics portion of the user interface, where the turtle roams.
///
//===----------------------------------------------------------------------===//

#include "sharedconstants.h"
#include <QPainter>
#include <QPen>
#include <QStaticText>
#include <QVariant>
#include <QWidget>
#include <variant>

class MainWindow;

/// Contains the information that describes a label's appearance on the Canvas.
struct Label
{
    QPointF position_;
    QStaticText text_;
    QFont font_;

    /// @brief Constructor.
    /// @param aText The text to display.
    /// @param aPosition The position to display the text.
    /// @param aFont The font to use for the text.
    Label(const QString &aText, QPointF aPosition, const QFont &aFont)
        : position_(aPosition), text_(aText), font_(aFont)
    {
    }
};

/// Contains the information that describes a polygon.
struct Polygon
{
    QColor color_;
    QPolygonF points_;
};

/// Contains the information that describes a change in how a turtle draws.
struct TurtleWriteInfo
{
    /// @brief The composition mode to use for drawing.
    ///
    /// Acceptable values are either SourceOver or Difference.
    QPainter::CompositionMode composingMode_;
    /// @brief The pen to use for drawing.
    ///
    /// Color, width, style, cap, join, and, perhaps, dash pattern.
    QPen pen_;
};

/// Contains the information that describes how to draw an arc.
struct Arc
{
    QRectF rectangle_;
    int startAngle_;
    int spanAngle_;
    Arc(QPointF center, qreal a, qreal span, qreal radiusX, qreal radiusY);
};

/// @brief A variant type that can hold any of the drawing element types.
///
/// The drawing element types are:
/// - Label - A text label.
/// - Polygon - A polygon.
/// - TurtleWriteInfo - A change in how a turtle draws.
/// - QPolygonF - A polyline.
/// - Arc - An arc.
using DrawingElementVariant = std::variant<Label, Polygon, TurtleWriteInfo, QPolygonF, Arc>;

/// @brief The canvas widget, the main widget for the turtle graphics.
///
/// This widget is responsible for drawing the turtle and the elements that the
/// turtle draws.
class Canvas : public QWidget
{
    Q_OBJECT

    // Turtle vars:
    QTransform turtleMatrix_; // The location and orientation of the turtle.
    bool turtleIsVisible_;

    QTransform turtleImageMatrix_; // The matrix applied when drawing the turtle
    QImage turtleImage_;

    bool canvasIsBounded_ = true;
    bool mouseButtonPressed_ = false;
    QTransform drawingMatrix_;        // For mapping cartesian to widget.
    QTransform inverseDrawingMatrix_; // For mapping mouse to cartesian.

    // Visible vertices on the X axis range from -boundsX to +boundsX
    qreal boundsX_;
    // Visible vertices on the Y axis range from -boundsY to +boundsY
    qreal boundsY_;

    // Display-space scrunch. Translation in turtleMatrix_ is in display units.
    qreal scrunchX_ = 1.0;
    qreal scrunchY_ = 1.0;

    QColor foregroundColor_;
    QColor backgroundColor_;
    TurtleWriteInfo currentWriteInfo_;
    PenModeEnum penMode_ = penModePaint; // paint,erase,reverse
    bool penIsDown_ = true;

    // The main data structure for all of the drawn elements on the canvas.
    QList<DrawingElementVariant> drawingElementList_;

    // The currently-constructing line sequence.
    QPolygonF lineGroup_;

    // The currently-constructing polygon.
    bool isConstructingPolygon_ = false;
    QColor polygonColor_;
    QPolygonF polygonGroup_;

    QImage backgroundImage_;

    // The current label drawing font
    QFont labelFont_;

    void initDrawingElementList();
    void initTurtleImage();

    void drawCanvas(QPainter &painter);

    void elementListSetWriteInfo(QPainter &painter, const TurtleWriteInfo &info);
    void elementListDrawBoundedBackground(QPainter &painter);
    void elementListDrawUnboundedBackground(QPainter &painter);
    void elementListDrawBackgroundImage(QPainter &painter);
    void elementListDrawTurtle(QPainter &painter);
    void elementListDrawPolyline(QPainter &painter, const QPolygonF &);
    void elementListDrawPolygon(QPainter &painter, const Polygon &p);
    void elementListDrawLabel(QPainter &painter, const Label &);
    void elementListDrawArc(QPainter &painter, const Arc &a);

    const QColor &colorForCurrentPenmode();

    void updateMatrix();

    QPointF pointFromTurtle();

    void pushLineGroup();
    void setLastWriteInfo();

    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

  public:
    /// @brief Construct a Canvas
    ///
    /// @param parent The Qt parent widget.
    Canvas(QWidget *parent = nullptr);

    /// @brief Set the position and orientation of the turtle.
    ///
    /// @param aTurtleMatrix The new position and orientation of the turtle.
    void setTurtleMatrix(const QTransform &aTurtleMatrix);

    /// @brief Record that the turtle moved.
    ///
    /// If drawing a line or polygon, add the vertex to the drawing element list.
    void emitVertex();

    /// @brief Show or hide the turtle.
    ///
    /// @param isVisible Whether the turtle should be visible.
    void setTurtleIsVisible(bool isVisible);

    /// @brief Set the pen up or down.
    ///
    /// @param aPenIsDown Whether the pen should be down.
    void setPenIsDown(bool aPenIsDown);

    /// @brief Begin the definition of a polygon.
    ///
    /// The first point of the polygon is the current position of the turtle.
    /// All future turtle movement will add a point to the polygon until
    /// endPolygon() is received.
    ///
    /// @param color The color of the polygon.
    void beginPolygon(const QColor &color);

    /// @brief End the definition of a polygon.
    ///
    /// A polygon object will be added to the drawing element list.
    void endPolygon();

    /// @brief Add an arc using the specified angle and radius.
    ///
    /// @param angle The angle of the arc.
    /// @param radius The logical radius of the arc.
    /// @note The starting angle is the current turtle orientation. The
    /// bounding rectangle is stretched by the current scrunch so a circle
    /// in logical space is an ellipse in display space.
    void addArc(qreal angle, qreal radius);

    /// @brief Add a label at the current turtle location.
    ///
    /// @param aText The text to display.
    /// @note The recorded font height is the logical size times the current
    /// Y scrunch. LABELSIZE is unaffected.
    void addLabel(const QString &aText);

    /// @brief Set the logical font size for all future text labels.
    ///
    /// @param aSize The new font size (SETLABELHEIGHT / LABELSIZE).
    void setLabelFontSize(qreal aSize);

    /// @brief Set the font name for all future text labels.
    ///
    /// @param name The new font name.
    void setLabelFontName(const QString &name);

    /// @brief Sets future lines and polygons to be drawn using newMode.
    ///
    /// @param newMode The new pen mode.
    /// @note Acceptable newMode values are draw, erase, or reverse, which
    /// correspond to PENPAINT, PENERASE, and PENREVERSE respectively.
    void setPenmode(PenModeEnum newMode);

    /// @brief Set the pen color for all future line and label drawings.
    ///
    /// @param aColor The new pen color.
    void setForegroundColor(const QColor &aColor);

    /// @brief Set the width of future lines.
    ///
    /// @param aSize The new pen size.
    void setPensize(qreal aSize);

    /// @brief Set the style, cap, join, and optional dash pattern of future lines.
    ///
    /// @param style The Qt pen style.
    /// @param cap The Qt pen cap style.
    /// @param join The Qt pen join style.
    /// @param dashPattern Dash lengths used only when style is Qt::CustomDashLine.
    ///        An odd-length list is accepted; the first length is reused as the
    ///        missing gap when the pattern repeats.
    void setPenPattern(Qt::PenStyle style, Qt::PenCapStyle cap, Qt::PenJoinStyle join, const QList<qreal> &dashPattern);

    /// @brief Set the background color.
    ///
    /// @param c The new background color.
    /// The background is drawn either as a filled rectangle when isBounded=true
    /// or the background color fills the entire widget when isBounded=false.
    void setBackgroundColor(const QColor &c);

    /// @brief Set background image.
    ///
    /// @param image The new background image.
    /// If image is a valid image, then it will be drawn onto the canvas
    /// above the background color and below everything the turtle draws. The
    /// image will be scaled to fit the bounds set by setBounds().
    void setBackgroundImage(const QImage &image);

    /// @brief Redraw the canvas onto an image object and return it.
    ///
    /// @return The image of the canvas.
    QImage getImage();

    /// @brief Draw the canvas onto an SVG object string and return it.
    ///
    /// @return The SVG string of the canvas.
    QByteArray getSvg();

    /// @brief Clears the screen and removes all painting elements from the paintList.
    void clearScreen();

    /// @brief Get the maximum X bound
    ///
    /// @return The maximum X bound
    /// @note The origin is always the center of the widget, therefore the
    /// minimum X bound is negative of the maximum.
    qreal xbound()
    {
        return boundsX_;
    }

    /// @brief Get the maximum Y bound
    ///
    /// @return The maximum Y bound
    /// @note The origin is always the center of the widget, therefore the
    /// minimum Y bound is negative of the maximum.
    qreal ybound()
    {
        return boundsY_;
    }

    /// @brief Set the maximum X and Y bounds
    ///
    /// @param x The new maximum X bound
    /// @param y The new maximum Y bound
    /// @note The origin is always the center of the widget, therefore the
    /// minimum X and Y bounds are the negatives of their maximum values.
    void setBounds(qreal x, qreal y);

    /// @brief Set the display-space scrunch factors.
    /// @param x The X scale factor.
    /// @param y The Y scale factor.
    /// @note Does not restretch already-drawn ink. The turtle icon is redrawn.
    void setScrunch(qreal x, qreal y);

    /// @brief The current X scrunch factor.
    qreal scrunchX() const
    {
        return scrunchX_;
    }

    /// @brief The current Y scrunch factor.
    qreal scrunchY() const
    {
        return scrunchY_;
    }

    /// @brief Set whether the view should use the whole widget or a box within.
    ///
    /// @param aIsBounded Set to true to specify that the widget draws a
    /// rectangle in the background color within the widget according to the
    /// ratio of the boundsX and boundsY values. Set to false to specify that
    /// the widget draws the entirety of the widget the background color.
    void setIsBounded(bool aIsBounded)
    {
        canvasIsBounded_ = aIsBounded;
        update();
    }

    /// @brief Returns true if the canvas is drawn "bounded"
    ///
    /// @return True if the canvas is drawn "bounded". See setIsBounded() for
    /// more details.
    bool isBounded()
    {
        return canvasIsBounded_;
    }

  signals:

    /// @brief Send a mouse clicked signal to the main window.
    ///
    /// @param position The position of the mouse click.
    /// @param buttonID The ID of the mouse button that was clicked.
    void sendMouseclickedSignal(QPointF position, int buttonID);

    /// @brief Send a mouse moved signal to the main window.
    /// @param position The position of the mouse move.
    void sendMousemovedSignal(QPointF position);

    /// @brief Send a mouse released signal to the main window.
    void sendMouseReleasedSignal();
};

#endif // CANVAS_H
