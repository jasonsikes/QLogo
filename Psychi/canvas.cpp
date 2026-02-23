//===-- qlogo/canvas.cpp - Canvas class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Canvas class, which is the
/// graphics portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include "gui/canvas.h"
#include "sharedconstants.h"
#include <QBuffer>
#include <QColor>
#include <QMouseEvent>
#include <QSvgGenerator>

Arc::Arc(QPointF center, qreal a, qreal span, qreal radius)
{
    rectangle_ = QRectF(center.x() - radius, center.y() - radius, radius * 2, radius * 2);
    startAngle_ = (a - 90) * 16;
    spanAngle_ = span * -16;
}

Canvas::Canvas(QWidget *parent) : QWidget(parent)
{
    boundsX_ = Config::get().initialBoundX;
    boundsY_ = Config::get().initialBoundY;
    backgroundColor_ = Config::get().initialCanvasBackgroundColor;
    foregroundColor_ = Config::get().initialCanvasForegroundColor;
    currentWriteInfo_.pen_ = QPen(foregroundColor_);
    currentWriteInfo_.pen_.setCapStyle(Qt::RoundCap);
    currentWriteInfo_.pen_.setJoinStyle(Qt::RoundJoin);
    currentWriteInfo_.composingMode_ = QPainter::CompositionMode_SourceOver;
    turtleMatrix_ = QTransform();
    turtleIsVisible_ = true;
    initDrawingElementList();
    initTurtleImage();
}

void Canvas::initDrawingElementList()
{
    drawingElementList_.push_back({DrawingElementIDTurtle, DrawingElementVariant(currentWriteInfo_)});
    if (penIsDown_)
        lineGroup_.push_back(pointFromTurtle());
}

/// @brief Initialize the turtle image.
void Canvas::initTurtleImage()
{
    qreal multiplier = 5;
    qreal height = 7 * multiplier * 2;    // vertical distance from origin to head
    qreal halfwidth = 3 * multiplier * 2; // horizontal distance from origin to edge
    qreal aft = -2 * multiplier * 2;      // vertical distance from origin to butt

    QPolygonF turtlePolygon;
    turtlePolygon << QPointF(0, 0)            // Origin open
                  << QPointF(halfwidth, aft)  // Right aft
                  << QPointF(0, height)       // Head
                  << QPointF(-halfwidth, aft) // Left aft
                  << QPointF(0, 0)            // Origin close
        ;

    turtleImage_ =
        QImage(halfwidth * 2 + multiplier * 2, height - aft + multiplier * 2, QImage::Format_ARGB32_Premultiplied);
    turtleImage_.fill(Qt::transparent);

    QPainter painter(&turtleImage_);
    painter.translate(halfwidth + multiplier, multiplier - aft);

    QPen pen = QPen(Config::get().initialCanvasForegroundColor, multiplier * 2);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(QBrush(Config::get().initialCanvasBackgroundColor));
    painter.drawPolygon(turtlePolygon);

    // Whenever we draw the turtle, transform a bit.
    turtleImageMatrix_.scale(0.5 / multiplier, 0.5 / multiplier);
    turtleImageMatrix_.translate(-halfwidth - multiplier, aft);
}

void Canvas::clearScreen()
{
    drawingElementList_.clear();
    lineGroup_.clear();
    initDrawingElementList();
    update();
}

// Call this when we are about to add something to the DrawingElementList.
// (Except LineGroup, of course.)
void Canvas::pushLineGroup()
{
    if (lineGroup_.size() > 1)
    {
        drawingElementList_.push_back({DrawingElementIDPolyline, DrawingElementVariant(lineGroup_)});
        lineGroup_.clear();
        if (penIsDown_)
            lineGroup_.push_back(pointFromTurtle());
    }
}

void Canvas::setBounds(qreal x, qreal y)
{
    boundsX_ = x;
    boundsY_ = y;
    updateMatrix();
    update();
}

void Canvas::setLastWriteInfo()
{
    Q_ASSERT(drawingElementList_.size() > 0);
    int lastElementID = drawingElementList_.last().elementID_;
    // If the last drawing element is not a TurtleWriteInfo, then create it and
    // push it onto the list.
    if (lastElementID != DrawingElementIDTurtle)
    {
        drawingElementList_.push_back({DrawingElementIDTurtle, DrawingElementVariant(currentWriteInfo_)});
    }
    else
    {
        // replace the drawing element at the end of the list.
        std::get<TurtleWriteInfo>(drawingElementList_.last().element_) = currentWriteInfo_;
    }
}

void Canvas::setPenIsDown(bool aPenIsDown)
{
    if (aPenIsDown == penIsDown_)
        return;

    penIsDown_ = aPenIsDown;

    if (penIsDown_)
    {
        Q_ASSERT(lineGroup_.size() < 2);
        lineGroup_.clear();
        lineGroup_.push_back(pointFromTurtle());
    }
    else
    {
        pushLineGroup();
    }
}

void Canvas::setPenmode(PenModeEnum newMode)
{
    if (newMode == penMode_)
        return;

    pushLineGroup();

    penMode_ = newMode;
    currentWriteInfo_.composingMode_ =
        (penMode_ == penModeReverse) ? QPainter::CompositionMode_Difference : QPainter::CompositionMode_SourceOver;
    currentWriteInfo_.pen_.setColor(colorForCurrentPenmode());
    setLastWriteInfo();
}

void Canvas::setPensize(qreal aSize)
{
    if (currentWriteInfo_.pen_.widthF() == aSize)
        return;

    pushLineGroup();

    currentWriteInfo_.pen_.setWidthF(aSize);

    setLastWriteInfo();
}

const QColor &Canvas::colorForCurrentPenmode()
{
    if (penMode_ == penModePaint)
        return foregroundColor_;
    if (penMode_ == penModeErase)
        return backgroundColor_;
    // Else it must be penModeReverse. Return white for full reverse effect.
    return QColorConstants::White;
}

void Canvas::setLabelFontName(const QString &name)
{
    labelFont_.setFamily(name);
}

void Canvas::setLabelFontSize(qreal aSize)
{
    labelFont_.setPointSizeF(aSize);
}

void Canvas::addLabel(const QString &aText)
{
    // The "minus-dy" is because we have to flip the coordinate system when
    // drawing text. This is the most efficient place to do it.
    Label l(aText, QPointF(turtleMatrix_.dx(), -turtleMatrix_.dy()), labelFont_);
    pushLineGroup();
    drawingElementList_.push_back({DrawingElementIDLabel, DrawingElementVariant(l)});
    update();
}

void Canvas::addArc(qreal angle, qreal radius)
{
    if (!penIsDown_)
        return;

    qreal s = turtleMatrix_.m21();
    qreal c = turtleMatrix_.m11();

    qreal a = atan2(s, c) * (180.0 / PI);

    if (radius < 0)
    {
        radius *= -1;
        a = 180 - a;
    }

    Arc arc(pointFromTurtle(), a, angle, radius);
    pushLineGroup();
    drawingElementList_.push_back({DrawingElementIDArc, DrawingElementVariant(arc)});
    update();
}

void Canvas::setTurtleIsVisible(bool isVisible)
{
    if (turtleIsVisible_ != isVisible)
    {
        turtleIsVisible_ = isVisible;
        update();
    }
}

void Canvas::setTurtleMatrix(const QTransform &aTurtleMatrix)
{
    turtleMatrix_ = aTurtleMatrix;
    update();
}

void Canvas::setBackgroundColor(const QColor &c)
{
    backgroundColor_ = c;
    update();
}

void Canvas::setForegroundColor(const QColor &c)
{
    if (foregroundColor_ == c)
        return;

    pushLineGroup();

    foregroundColor_ = c;
    currentWriteInfo_.pen_.setColor(colorForCurrentPenmode());

    setLastWriteInfo();
}

void Canvas::setBackgroundImage(const QImage &image)
{
    backgroundImage_ = image;
    update();
}

QImage Canvas::getImage()
{
    QImage retval(boundsX_ * 2, boundsY_ * 2, QImage::Format_ARGB32_Premultiplied);

    QPainter imagePainter = QPainter(&retval);
    retval.fill(backgroundColor_);
    painter = &imagePainter;
    painter->translate(boundsX_, boundsY_);
    painter->scale(1, -1);

    drawCanvas();

    return retval;
}

QByteArray Canvas::getSvg()
{
    QByteArray retval;
    QBuffer bufferStream(&retval);
    QSvgGenerator generator;
    generator.setOutputDevice(&bufferStream);

    generator.setSize(QSize(boundsX_ * 2, boundsY_ * 2));

    QPainter svgPainter = QPainter(&generator);
    painter = &svgPainter;
    painter->translate(boundsX_, boundsY_);
    painter->scale(1, -1);

    drawCanvas();

    return retval;
}

void Canvas::paintEvent(QPaintEvent *event)
{

    // If any of our dimensions are zero then we can't draw.
    if ((width() == 0) || (height() == 0) || (boundsX_ == 0) || (boundsY_ == 0))
        return;

    QPainter eventPainter = QPainter(this);
    painter = &eventPainter;

    if (!canvasIsBounded_)
        elementListDrawUnboundedBackground();

    painter->setWorldTransform(drawingMatrix_);

    if (canvasIsBounded_)
        elementListDrawBoundedBackground();

    drawCanvas();
}

void Canvas::drawCanvas()
{
    painter->setRenderHint(QPainter::Antialiasing);

    elementListDrawBackgroundImage();

    for (auto &drawCommand : drawingElementList_)
    {
        switch (drawCommand.elementID_)
        {
        case DrawingElementIDLabel:
            elementListDrawLabel(std::get<Label>(drawCommand.element_));
            break;
        case DrawingElementIDTurtle:
            elementListSetWriteInfo(std::get<TurtleWriteInfo>(drawCommand.element_));
            break;
        case DrawingElementIDPolyline:
            elementListDrawPolyline(std::get<QPolygonF>(drawCommand.element_));
            break;
        case DrawingElementIDPolygon:
            elementListDrawPolygon(std::get<Polygon>(drawCommand.element_));
            break;
        case DrawingElementIDArc:
            elementListDrawArc(std::get<Arc>(drawCommand.element_));
            break;
        default:
            Q_ASSERT(false);
        }
    }

    // Draw the in-progress line group.
    painter->drawPolyline(lineGroup_);

    elementListDrawTurtle();
}

void Canvas::elementListDrawUnboundedBackground()
{
    painter->fillRect(rect(), backgroundColor_);
}

void Canvas::elementListDrawBoundedBackground()
{
    QRectF rect(-boundsX_, -boundsY_, 2 * boundsX_, 2 * boundsY_);
    painter->setClipRect(rect);
    painter->fillRect(rect, backgroundColor_);
}

void Canvas::elementListDrawBackgroundImage()
{
    if (backgroundImage_.isNull())
        return;

    QRectF rect(-boundsX_, -boundsY_, 2 * boundsX_, 2 * boundsY_);

    painter->scale(1, -1);
    painter->drawImage(rect, backgroundImage_);
    painter->scale(1, -1);
}

void Canvas::elementListDrawLabel(const Label &label)
{
    painter->setFont(label.font_);

    painter->scale(1, -1);
    painter->drawStaticText(label.position_, label.text_);
    painter->scale(1, -1);
}

void Canvas::elementListDrawPolyline(const QPolygonF &polyLine)
{
    painter->drawPolyline(polyLine);
}

void Canvas::elementListDrawPolygon(const Polygon &p)
{
    static QPen noPen = QPen();
    noPen.setStyle(Qt::NoPen);

    QPen pen = painter->pen();
    painter->setPen(noPen);
    painter->setBrush(QBrush(p.color_));
    painter->drawPolygon(p.points_);
    painter->setPen(pen);
}

void Canvas::elementListDrawArc(const Arc &a)
{
    painter->drawArc(a.rectangle_, a.startAngle_, a.spanAngle_);
}

void Canvas::elementListDrawTurtle()
{
    if (turtleIsVisible_)
    {
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter->save();
        painter->setTransform(turtleMatrix_, true);
        painter->setTransform(turtleImageMatrix_, true);
        painter->drawImage(QPointF(0, 0), turtleImage_);
        painter->restore();
    }
}

// The pen controls composition mode, color and size
void Canvas::elementListSetWriteInfo(const TurtleWriteInfo &info)
{
    painter->setPen(info.pen_);
    painter->setCompositionMode(info.composingMode_);
}

void Canvas::emitVertex()
{
    if (penIsDown_)
        lineGroup_ << pointFromTurtle();
    if (isConstructingPolygon_)
        polygonGroup_ << pointFromTurtle();
    update();
}

void Canvas::updateMatrix()
{
    // Set coordinate system so that background box fits in widget and fills
    // without stretching.
    qreal widgetHWRatio = (qreal)height() / (qreal)width();
    qreal boundsHWRatio = boundsY_ / boundsX_;
    qreal hwRatio;
    if (widgetHWRatio > boundsHWRatio)
    {
        // the bounds are hugging the left and right edges
        hwRatio = width() / boundsX_ / 2;
    }
    else
    {
        // the bounds are hugging the top and bottom edges
        hwRatio = height() / boundsY_ / 2;
    }

    drawingMatrix_.reset();
    drawingMatrix_.translate(width() / 2.0, height() / 2.0);
    drawingMatrix_.scale(hwRatio, -hwRatio);

    inverseDrawingMatrix_ = drawingMatrix_.inverted();
}

QPointF Canvas::pointFromTurtle()
{
    return {turtleMatrix_.dx(), turtleMatrix_.dy()};
}

void Canvas::beginPolygon(const QColor &color)
{
    Q_ASSERT(isConstructingPolygon_ == false);
    Q_ASSERT(polygonGroup_.size() == 0);
    isConstructingPolygon_ = true;

    polygonColor_ = (penMode_ == penModeReverse) ? QColorConstants::White : color;
    polygonGroup_ << pointFromTurtle();
}

void Canvas::endPolygon()
{
    Q_ASSERT(isConstructingPolygon_ == true);
    // A polygon needs at least three vertices.
    if (polygonGroup_.size() >= 3)
    {
        pushLineGroup();
        drawingElementList_.push_back(
            {DrawingElementIDPolygon, DrawingElementVariant(Polygon({polygonColor_, polygonGroup_}))});
    }
    polygonGroup_.clear();
    isConstructingPolygon_ = false;
}

void Canvas::resizeEvent(QResizeEvent *event)
{
    updateMatrix();
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    int buttonID = 0;
    Qt::MouseButton button = event->button();
    if (button & Qt::MiddleButton)
        buttonID = 3;
    if (button & Qt::RightButton)
        buttonID = 2;
    if (button & Qt::LeftButton)
        buttonID = 1;
    QPointF mousePos = inverseDrawingMatrix_.map(event->position());
    if (!canvasIsBounded_ || ((mousePos.x() <= boundsX_) && (mousePos.y() <= boundsY_) && (mousePos.x() >= -boundsX_) &&
                             (mousePos.y() >= -boundsY_)))
    {
        mouseButtonPressed_ = true;
        emit sendMouseclickedSignal(mousePos, buttonID);
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    QPointF mousePos = inverseDrawingMatrix_.map(event->position());
    if (mouseButtonPressed_ || !canvasIsBounded_ ||
        ((mousePos.x() <= boundsX_) && (mousePos.y() <= boundsY_) && (mousePos.x() >= -boundsX_) &&
         (mousePos.y() >= -boundsY_)))
        emit sendMousemovedSignal(mousePos);
}

void Canvas::mouseReleaseEvent(QMouseEvent *)
{
    if (mouseButtonPressed_)
    {
        mouseButtonPressed_ = false;
        emit sendMouseReleasedSignal();
    }
}
