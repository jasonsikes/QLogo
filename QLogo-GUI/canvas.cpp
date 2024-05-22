//===-- qlogo/canvas.cpp - Canvas class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Canvas class, which is the
/// graphics portion of the user interface.
///
//===----------------------------------------------------------------------===//

#define _USE_MATH_DEFINES

#include "sharedconstants.h"
#include "gui/canvas.h"
#include "math.h"
#include <QColor>
#include <QMouseEvent>
#include <QSvgGenerator>
#include <QBuffer>

Arc::Arc(QPointF center, qreal a, qreal span, qreal radius)
{
    rectangle = QRectF(center.x() - radius,center.y() - radius,
                       radius * 2,radius * 2);
    startAngle = (a - 90) * 16;
    spanAngle = span * -16;
}

Canvas::Canvas(QWidget *parent) : QWidget(parent) {
    boundsX = initialBoundX;
    boundsY = initialBoundY;
    backgroundColor = initialCanvasBackgroundColor;
    foregroundColor = initialCanvasForegroundColor;
    currentWriteInfo.pen = QPen(foregroundColor);
    currentWriteInfo.pen.setCapStyle(Qt::RoundCap);
    currentWriteInfo.pen.setJoinStyle(Qt::RoundJoin);
    currentWriteInfo.composingMode = QPainter::CompositionMode_SourceOver;
    turtleMatrix = QTransform();
    turtleIsVisible = true;
    initDrawingElementList();
    initTurtleImage();
}


void Canvas::initDrawingElementList() {
    drawingElementList.push_back( { turtleWriteInfoID,deVariant(currentWriteInfo) } );
    if (penIsDown)
        lineGroup.push_back(pointFromTurtle());
}


void Canvas::initTurtleImage()
{
    qreal multiplier = 5;
    qreal height = 7 * multiplier * 2;    // vertical distance from origin to head
    qreal halfwidth = 3 * multiplier * 2; // horizontal distance from origin to edge
    qreal aft = -2 * multiplier * 2;      // vertical distance from origin to butt

    QPolygonF turtlePolygon;
    turtlePolygon << QPointF(0,0) // Origin open
                  << QPointF(halfwidth, aft) // Right aft
                  << QPointF(0,height) // Head
                  << QPointF(-halfwidth, aft) // Left aft
                  << QPointF(0,0) // Origin close
        ;

    turtleImage = QImage(halfwidth * 2 + multiplier * 2,
                         height -aft + multiplier * 2,
                         QImage::Format_ARGB32_Premultiplied);
    turtleImage.fill(Qt::transparent);

    QPainter painter(&turtleImage);
    painter.translate(halfwidth + multiplier, multiplier-aft);

    QPen pen = QPen(initialCanvasForegroundColor,multiplier*2);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(QBrush(initialCanvasBackgroundColor));
    painter.drawPolygon(turtlePolygon);

    // Whenever we draw the turtle, transform a bit.
    turtleImageMatrix.scale(0.5 / multiplier, 0.5 / multiplier);
    turtleImageMatrix.translate(-halfwidth - multiplier, aft);
}


void Canvas::clearScreen()
{
    drawingElementList.clear();
    lineGroup.clear();
    initDrawingElementList();
    update();
}


// Call this when we are about to add something to the DrawingElementList.
// (Except LineGroup, of course.)
void Canvas::pushLineGroup()
{
    if (lineGroup.size() > 1) {
        drawingElementList.push_back( { polylineTypeID, deVariant(lineGroup) } );
        lineGroup.clear();
        if (penIsDown)
            lineGroup.push_back(pointFromTurtle());
    }
}


void Canvas::setBounds(qreal x, qreal y)
{
    boundsX = x;
    boundsY = y;
    update();
}


void Canvas::setLastWriteInfo()
{
    Q_ASSERT(drawingElementList.size() > 0);
    int lastElementID = drawingElementList.last().eID;
    // If the last drawing element is not a TurtleWriteInfo, then create it and
    // push it onto the list.
    if (lastElementID != turtleWriteInfoID) {
        drawingElementList.push_back(
            { turtleWriteInfoID, deVariant( currentWriteInfo) }
            );
    } else {
        // replace the drawing element at the end of the list.
        std::get<TurtleWriteInfo>(drawingElementList.last().element) = currentWriteInfo;
    }
}


void Canvas::setPenIsDown(bool aPenIsDown)
{
    if (aPenIsDown == penIsDown) return;

    penIsDown = aPenIsDown;

    if (penIsDown) {
        Q_ASSERT(lineGroup.size() < 2);
        lineGroup.clear();
        lineGroup.push_back(pointFromTurtle());
    } else {
        pushLineGroup();
    }
}


void Canvas::setPenmode(PenModeEnum newMode)
{
    if (newMode == penMode) return;

    pushLineGroup();

    penMode = newMode;
    currentWriteInfo.composingMode = (penMode == penModeReverse)
                                         ? QPainter::CompositionMode_Difference
                                         : QPainter::CompositionMode_SourceOver;
    currentWriteInfo.pen.setColor(colorForPenmode());
    setLastWriteInfo();
}


void Canvas::setPensize(qreal aSize)
{
    if (currentWriteInfo.pen.widthF() == aSize) return;

    pushLineGroup();

    currentWriteInfo.pen.setWidthF(aSize);

    setLastWriteInfo();
}


const QColor& Canvas::colorForPenmode() {
    if (penMode == penModePaint)
        return foregroundColor;
    if (penMode == penModeErase)
        return backgroundColor;
    // Else it must be penModeReverse. Return white for full reverse effect.
    return QColorConstants::White;
}





void Canvas::setLabelFontName(QString name)
{
    labelFont.setFamily(name);
}


void Canvas::setLabelFontSize(qreal aSize)
{
    labelFont.setPointSizeF(aSize);
}


void Canvas::addLabel(QString aText)
{
    // The "minus-dy" is because we have to flip the coordinate system when
    // drawing text. This is the most efficient place to do it.
    Label l(aText, QPointF(turtleMatrix.dx(), -turtleMatrix.dy()), labelFont);
    pushLineGroup();
    drawingElementList.push_back( { labelTypeID, deVariant(l) } );
    update();
}


void Canvas::addArc(qreal angle, qreal radius)
{
    if ( ! penIsDown) return;

    qreal s = turtleMatrix.m21();
    qreal c = turtleMatrix.m11();

    qreal a = atan2(s, c) * 180 / M_PI;

    if (radius < 0) {
        radius *= -1;
        a = 180 - a;
    }

    Arc arc(pointFromTurtle(),a, angle,radius);
    pushLineGroup();
    drawingElementList.push_back( { arcTypeID, deVariant(arc) } );
    update();
}


void Canvas::setTurtleIsVisible(bool isVisible)
{
    if (turtleIsVisible != isVisible) {
        turtleIsVisible = isVisible;
        update();
    }
}

void Canvas::setTurtleMatrix(const QTransform &aTurtleMatrix)
{
    turtleMatrix = aTurtleMatrix;
    update();
}


void Canvas::setBackgroundColor(const QColor &c)
{
    backgroundColor = c;
    update();
}


void Canvas::setForegroundColor(const QColor &c)
{
    if (foregroundColor == c) return;

    pushLineGroup();

    foregroundColor = c;
    currentWriteInfo.pen.setColor(colorForPenmode());

    setLastWriteInfo();
}


void Canvas::setBackgroundImage(QImage image)
{
    backgroundImage = image;
    update();
}


QImage Canvas::getImage()
{
    QImage retval(boundsX * 2, boundsY * 2, QImage::Format_ARGB32_Premultiplied);

    QPainter imagePainter = QPainter(&retval);
    retval.fill(backgroundColor);
    painter = &imagePainter;
    painter->translate(boundsX, boundsY);
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

    generator.setSize(QSize(boundsX * 2, boundsY * 2));

    QPainter svgPainter = QPainter(&generator);
    painter = &svgPainter;
    painter->translate(boundsX, boundsY);
    painter->scale(1, -1);

    drawCanvas();

    return retval;
}


void Canvas::paintEvent(QPaintEvent *event)
{

    // If any of our dimensions are zero then we can't draw.
    if ((width() == 0) || (height() == 0) || (boundsX == 0) || (boundsY == 0))
        return;

    QPainter eventPainter = QPainter(this);
    painter = &eventPainter;

    if ( ! canvasIsBounded)
        elDrawUnboundedBackground();

    painter->setWorldTransform(drawingMatrix);

    if (canvasIsBounded)
        elDrawBoundedBackground();

    drawCanvas();
}


void Canvas::drawCanvas()
{
    painter->setRenderHint(QPainter::Antialiasing);

    elDrawBackgroundImage();

    for (auto &drawCommand : drawingElementList) {
        switch(drawCommand.eID) {
        case labelTypeID:
            elDrawLabel(std::get<Label>(drawCommand.element));
            break;
        case turtleWriteInfoID:
            elSetWriteInfo(std::get<TurtleWriteInfo>(drawCommand.element));
            break;
        case polylineTypeID:
            elDrawPolyline(std::get<QPolygonF>(drawCommand.element));
            break;
        case polygonTypeID:
            elDrawPolygon(std::get<Polygon>(drawCommand.element));
            break;
        case arcTypeID:
            elDrawArc(std::get<Arc>(drawCommand.element));
            break;
        default:
            Q_ASSERT(false);
        }
    }

    // Draw the in-progress line group.
    painter->drawPolyline(lineGroup);

    elDrawTurtle();
}


void Canvas::elDrawUnboundedBackground()
{
    painter->fillRect(rect(), backgroundColor);
}

void Canvas::elDrawBoundedBackground()
{
        QRectF rect(-boundsX, -boundsY, 2*boundsX, 2*boundsY);
        painter->setClipRect(rect);
        painter->fillRect(rect, backgroundColor);
}


void Canvas::elDrawBackgroundImage()
{
    if (backgroundImage.isNull()) return;

    QRectF rect(-boundsX, -boundsY, 2*boundsX, 2*boundsY);

    painter->scale(1, -1);
    painter->drawImage(rect, backgroundImage);
    painter->scale(1, -1);
}


void Canvas::elDrawLabel(const Label &label)
{
    painter->setFont(label.font);

    painter->scale(1, -1);
    painter->drawStaticText(label.position, label.text);
    painter->scale(1, -1);
}


void Canvas::elDrawPolyline(const QPolygonF &polyLine)
{
    painter->drawPolyline(polyLine);
}


void Canvas::elDrawPolygon(const Polygon &p)
{
    static QPen noPen = QPen();
    noPen.setStyle(Qt::NoPen);

    QPen pen = painter->pen();
    painter->setPen(noPen);
    painter->setBrush(QBrush(p.color));
    painter->drawPolygon(p.points);
    painter->setPen(pen);
}


void Canvas::elDrawArc(const Arc &a)
{
    painter->drawArc(a.rectangle, a.startAngle, a.spanAngle);
}


void Canvas::elDrawTurtle()
{
    if (turtleIsVisible) {
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter->save();
        painter->setTransform(turtleMatrix, true);
        painter->setTransform(turtleImageMatrix, true);
        painter->drawImage(QPointF(0,0),turtleImage);
        painter->restore();
    }
}


// The pen controls composition mode, color and size
void Canvas::elSetWriteInfo(const TurtleWriteInfo &info)
{
    painter->setPen(info.pen);
    painter->setCompositionMode(info.composingMode);
}


void Canvas::emitVertex()
{
    if (penIsDown)
        lineGroup << pointFromTurtle();
    if (isConstructingPolygon)
        polygonGroup << pointFromTurtle();
    update();
}


QPointF Canvas::pointFromTurtle()
{
    return QPointF(turtleMatrix.dx(), turtleMatrix.dy());
}


void Canvas::beginPolygon(const QColor &color)
{
    Q_ASSERT(isConstructingPolygon == false);
    Q_ASSERT(polygonGroup.size() == 0);
    isConstructingPolygon = true;

    polygonColor = (penMode == penModeReverse) ? QColorConstants::White : color;
    polygonGroup << pointFromTurtle();
}


void Canvas::endPolygon()
{
    Q_ASSERT(isConstructingPolygon == true);
    // A polygon needs at least three vertices.
    if (polygonGroup.size() >= 3) {
        pushLineGroup();
        drawingElementList.push_back(
            { polygonTypeID, deVariant( Polygon( { polygonColor, polygonGroup})) } );
    }
    polygonGroup.clear();
    isConstructingPolygon = false;
}

void Canvas::resizeEvent(QResizeEvent *event)
{
    // Set coordinate system so that background box fits in widget and fills
    // without stretching.
    qreal widgetHWRatio = (qreal)height() / (qreal)width();
    qreal boundsHWRatio = boundsY / boundsX;
    qreal hwRatio;
    if (widgetHWRatio > boundsHWRatio) {
        // the bounds are hugging the left and right edges
        hwRatio = width() / boundsX / 2;
    } else {
        // the bounds are hugging the top and bottom edges
        hwRatio = height() / boundsY / 2;
    }

    drawingMatrix.reset();
    drawingMatrix.translate(width() / 2.0, height() / 2.0);
    drawingMatrix.scale(hwRatio, -hwRatio);

    inverseDrawingMatrix = drawingMatrix.inverted();
}



void Canvas::mousePressEvent(QMouseEvent *event) {
    int buttonID = 0;
    Qt::MouseButton button = event->button();
    if (button & Qt::MiddleButton)
        buttonID = 3;
    if (button & Qt::RightButton)
        buttonID = 2;
    if (button & Qt::LeftButton)
        buttonID = 1;
    QPointF mousePos = inverseDrawingMatrix.map(event->position());
    if (  ! canvasIsBounded
        || ((mousePos.x() <= boundsX)
            && (mousePos.y() <= boundsY)
            && (mousePos.x() >= -boundsX)
            && (mousePos.y() >= -boundsY))) {
        mouseButtonPressed = true;
        emit sendMouseclickedSignal(mousePos, buttonID);
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
    QPointF mousePos = inverseDrawingMatrix.map(event->position());
    if (mouseButtonPressed
        || ! canvasIsBounded
        || ((mousePos.x() <= boundsX)
            && (mousePos.y() <= boundsY)
            && (mousePos.x() >= -boundsX)
            && (mousePos.y() >= -boundsY)))
        emit sendMousemovedSignal(mousePos);

}

void Canvas::mouseReleaseEvent(QMouseEvent *) {
    if (mouseButtonPressed) {
        mouseButtonPressed = false;
        emit sendMouseReleasedSignal();
    }
}
