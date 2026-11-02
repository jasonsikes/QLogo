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
#include "gui/mainwindow.h"
#include "sharedconstants.h"
#include <QBuffer>
#include <QColor>
#include <QMouseEvent>
#include <QSvgGenerator>

namespace
{
template <class... Ts>
struct Overload : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
Overload(Ts...) -> Overload<Ts...>;

/// Paint a turtle stroke.
///
/// QPainter::drawPolyline leaves no pixels when every vertex is the same
/// point. Logo still inks that stroke: FD 0, and SETPOS of the current
/// position, deposit a dot the size of the pen. drawLine paints the pen
/// cap there (a square for SQUARECAP, a circle for ROUNDCAP, nothing for
/// FLATCAP). A stroke that actually moves is drawn as a polyline.
void drawStroke(QPainter &painter, const QPolygonF &polyLine)
{
    if (polyLine.size() >= 2)
    {
        const QPointF origin = polyLine.first();
        bool hasLength = false;
        for (const QPointF &point : polyLine)
        {
            if (point != origin)
            {
                hasLength = true;
                break;
            }
        }
        if (!hasLength)
        {
            painter.drawLine(origin, origin);
            return;
        }
    }
    painter.drawPolyline(polyLine);
}
} // namespace

Arc::Arc(QPointF center, qreal a, qreal span, qreal radiusX, qreal radiusY)
{
    rectangle_ = QRectF(center.x() - radiusX, center.y() - radiusY, radiusX * 2, radiusY * 2);
    startAngle_ = (a - 90) * 16;
    spanAngle_ = span * -16;
}

Canvas::Canvas(QWidget *parent) : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    boundsX_ = Config::get().initialBoundX_;
    boundsY_ = Config::get().initialBoundY_;
    backgroundColor_ = Config::get().initialCanvasBackgroundColor_;
    foregroundColor_ = Config::get().initialCanvasForegroundColor_;
    currentWriteInfo_.pen_ = QPen(foregroundColor_);
    currentWriteInfo_.pen_.setCapStyle(Qt::SquareCap);
    currentWriteInfo_.pen_.setJoinStyle(Qt::BevelJoin);
    currentWriteInfo_.composingMode_ = QPainter::CompositionMode_SourceOver;
    turtleMatrix_ = QTransform();
    turtleIsVisible_ = true;
    labelFont_ = MainWindow::defaultFont();
    initDrawingElementList();
    initTurtleImage();
}

void Canvas::initDrawingElementList()
{
    drawingElementList_.emplace_back(currentWriteInfo_);
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

    QPen pen = QPen(Config::get().initialCanvasForegroundColor_, multiplier * 2);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(QBrush(Config::get().initialCanvasBackgroundColor_));
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
        drawingElementList_.emplace_back(lineGroup_);
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

void Canvas::setScrunch(qreal x, qreal y)
{
    if (scrunchX_ == x && scrunchY_ == y)
        return;
    scrunchX_ = x;
    scrunchY_ = y;
    update();
}

void Canvas::setLastWriteInfo()
{
    Q_ASSERT(!drawingElementList_.isEmpty());
    // If the last drawing element is not a TurtleWriteInfo, then create it and
    // push it onto the list.
    if (!std::holds_alternative<TurtleWriteInfo>(drawingElementList_.last()))
        drawingElementList_.emplace_back(currentWriteInfo_);
    else
        std::get<TurtleWriteInfo>(drawingElementList_.last()) = currentWriteInfo_;
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

void Canvas::setPenPattern(Qt::PenStyle style,
                           Qt::PenCapStyle cap,
                           Qt::PenJoinStyle join,
                           const QList<qreal> &dashPattern)
{
    QList<qreal> pattern = dashPattern;
    // QPen requires an even-length pattern. For an odd list, append the first
    // length so the last dash is followed by a gap of that size when the
    // pattern repeats.
    if (style == Qt::CustomDashLine && pattern.size() % 2 == 1)
        pattern.append(pattern.first());

    const QPen &pen = currentWriteInfo_.pen_;
    if (pen.style() == style && pen.capStyle() == cap && pen.joinStyle() == join &&
        (style != Qt::CustomDashLine || pen.dashPattern() == pattern))
        return;

    pushLineGroup();

    currentWriteInfo_.pen_.setStyle(style);
    currentWriteInfo_.pen_.setCapStyle(cap);
    currentWriteInfo_.pen_.setJoinStyle(join);
    if (style == Qt::CustomDashLine)
        currentWriteInfo_.pen_.setDashPattern(pattern);

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
    labelFont_.setFamilies(QStringList(name));
}

void Canvas::setLabelFontSize(qreal aSize)
{
    labelFont_.setPointSizeF(aSize);
}

void Canvas::addLabel(const QString &aText)
{
    // The "minus-dy" is because we have to flip the coordinate system when
    // drawing text. This is the most efficient place to do it.
    // SETLABELHEIGHT is logical; UCBLogo draws at y_scale * height.
    QFont drawnFont = labelFont_;
    const qreal logicalSize = labelFont_.pointSizeF();
    if (logicalSize > 0)
        drawnFont.setPointSizeF(logicalSize * qAbs(scrunchY_));
    Label l(aText, QPointF(turtleMatrix_.dx(), (logicalSize * -1) - turtleMatrix_.dy()), drawnFont);
    pushLineGroup();
    drawingElementList_.emplace_back(l);
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

    Arc arc(pointFromTurtle(), a, angle, radius * scrunchX_, radius * scrunchY_);
    pushLineGroup();
    drawingElementList_.emplace_back(arc);
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

QImage Canvas::getImage() const
{
    const int imageWidth = qRound(boundsX_ * 2);
    const int imageHeight = qRound(boundsY_ * 2);
    QImage retval(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);

    QPainter imagePainter(&retval);
    retval.fill(backgroundColor_);
    imagePainter.translate(imageWidth / 2.0, imageHeight / 2.0);
    imagePainter.scale(1, -1);

    drawCanvas(imagePainter);

    return retval;
}

QByteArray Canvas::getSvg() const
{
    QByteArray retval;
    QBuffer bufferStream(&retval);
    QSvgGenerator generator;
    generator.setOutputDevice(&bufferStream);

    generator.setSize(QSize(boundsX_ * 2, boundsY_ * 2));

    QPainter svgPainter(&generator);
    svgPainter.translate(boundsX_, boundsY_);
    svgPainter.scale(1, -1);

    drawCanvas(svgPainter);

    return retval;
}

void Canvas::paintEvent(QPaintEvent *event)
{

    // If any of our dimensions are zero then we can't draw.
    if ((width() == 0) || (height() == 0) || (boundsX_ == 0) || (boundsY_ == 0))
        return;

    QPainter eventPainter(this);

    if (!canvasIsBounded_)
        elementListDrawUnboundedBackground(eventPainter);

    eventPainter.setWorldTransform(drawingMatrix_);

    if (canvasIsBounded_)
        elementListDrawBoundedBackground(eventPainter);

    drawCanvas(eventPainter);
}

void Canvas::drawCanvas(QPainter &painter) const
{
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, false);

    elementListDrawBackgroundImage(painter);

    for (const auto &drawCommand : drawingElementList_)
    {
        std::visit(Overload{
                       [this, &painter](const Label &label) { elementListDrawLabel(painter, label); },
                       [this, &painter](const TurtleWriteInfo &info) { elementListSetWriteInfo(painter, info); },
                       [this, &painter](const QPolygonF &polyline) { elementListDrawPolyline(painter, polyline); },
                       [this, &painter](const Polygon &polygon) { elementListDrawPolygon(painter, polygon); },
                       [this, &painter](const Arc &arc) { elementListDrawArc(painter, arc); },
                   },
                   drawCommand);
    }

    // Draw the in-progress line group.
    drawStroke(painter, lineGroup_);

    elementListDrawTurtle(painter);
}

void Canvas::elementListDrawUnboundedBackground(QPainter &painter) const
{
    painter.fillRect(rect(), backgroundColor_);
}

void Canvas::elementListDrawBoundedBackground(QPainter &painter) const
{
    QRectF rect(-boundsX_, -boundsY_, 2 * boundsX_, 2 * boundsY_);
    painter.setClipRect(rect);
    painter.fillRect(rect, backgroundColor_);
}

void Canvas::elementListDrawBackgroundImage(QPainter &painter) const
{
    if (backgroundImage_.isNull())
        return;

    QRectF rect(-boundsX_, -boundsY_, 2 * boundsX_, 2 * boundsY_);

    painter.scale(1, -1);
    painter.drawImage(rect, backgroundImage_);
    painter.scale(1, -1);
}

void Canvas::elementListDrawLabel(QPainter &painter, const Label &label) const
{
    painter.setFont(label.font_);

    painter.scale(1, -1);
    painter.drawStaticText(label.position_, label.text_);
    painter.scale(1, -1);
}

void Canvas::elementListDrawPolyline(QPainter &painter, const QPolygonF &polyLine) const
{
    drawStroke(painter, polyLine);
}

void Canvas::elementListDrawPolygon(QPainter &painter, const Polygon &p) const
{
    const QPen noPen(Qt::NoPen);

    QPen pen = painter.pen();
    painter.setPen(noPen);
    painter.setBrush(QBrush(p.color_));
    painter.drawPolygon(p.points_);
    painter.setPen(pen);
}

void Canvas::elementListDrawArc(QPainter &painter, const Arc &a) const
{
    painter.drawArc(a.rectangle_, a.startAngle_, a.spanAngle_);
}

void Canvas::elementListDrawTurtle(QPainter &painter) const
{
    if (turtleIsVisible_)
    {
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.save();
        // Scale the rotated axes in display space (T × S × R). Scaling after
        const QTransform scrunched(turtleMatrix_.m11() * scrunchX_,
                                   turtleMatrix_.m12() * scrunchY_,
                                   turtleMatrix_.m13(),
                                   turtleMatrix_.m21() * scrunchX_,
                                   turtleMatrix_.m22() * scrunchY_,
                                   turtleMatrix_.m23(),
                                   turtleMatrix_.dx(),
                                   turtleMatrix_.dy(),
                                   turtleMatrix_.m33());
        painter.setTransform(scrunched, true);
        painter.setTransform(turtleImageMatrix_, true);
        painter.drawImage(QPointF(0, 0), turtleImage_);
        painter.restore();
    }
}

// The pen controls composition mode, color, size, style, cap, and join.
void Canvas::elementListSetWriteInfo(QPainter &painter, const TurtleWriteInfo &info) const
{
    painter.setPen(info.pen_);
    painter.setCompositionMode(info.composingMode_);
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
    qreal widgetHWRatio = static_cast<qreal>(height()) / static_cast<qreal>(width());
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
        drawingElementList_.emplace_back(Polygon{polygonColor_, polygonGroup_});
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
    setFocus();
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
