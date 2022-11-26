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

#include "constants.h"
#include "canvas.h"
#include "math.h"

#include <QPainter>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QMouseEvent>


inline GLfloat lerp(float a, float b, float t) { return a * t + b * (1 - t); }

static const char *vertexShaderSource = "attribute highp vec4 posAttr;\n"
                                        "attribute lowp vec4 colAttr;\n"
                                        "varying lowp vec4 col;\n"
                                        "uniform highp mat4 matrix;\n"
                                        "void main() {\n"
                                        "   col = colAttr;\n"
                                        "   gl_Position = matrix * posAttr;\n"
                                        "}\n";

static const char *fragmentShaderSource = "varying lowp vec4 col;"
                                          "void main() {\n"
                                          "   gl_FragColor = col;\n"
                                          "}\n";

// The length of the turtle. All other turtle vertices are derived from this.
const GLfloat turtleLength = 15;

void Canvas::initSurfaceVBO() {
  surfaceArrayObject = new QOpenGLVertexArrayObject(this);
  surfaceArrayObject->create();

  surfaceVertexBufferObject = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  surfaceVertexBufferObject->create();
  surfaceVertexBufferObject->setUsagePattern(QOpenGLBuffer::StaticDraw);

  surfaceColorBufferObject = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  surfaceColorBufferObject->create();
  surfaceColorBufferObject->setUsagePattern(QOpenGLBuffer::StaticDraw);

  surfaceArrayObject->release();
}

// Turtle colors
#define T_SHELL_TOP 0, 0, 1, 1
#define T_SHELL_SIDE .2f, .2f, .4f, 1
#define T_HEAD_TOP 0, 1, 0, 1
#define T_HEAD_SIDE .2f, .3f, .2f, 1
#define T_FLIPPER_TIP 1, 0, 0, 1
#define T_FLIPPER_SHOULDER .3f, .2f, .2f, 1
#define T_FLIPPER_JOINT .5f, .4f, .4f, 1

void Canvas::initTurtleVBO(void) {
  const GLfloat u = turtleLength;      // length of turtle
  const GLfloat sr = u * 0.33333f;     // shell radius
  const float a = (float)M_PI * 2 / 5; // shell tile inner angle
  const GLfloat he = 1.2f;             // head side proportion from neck
  GLfloat p2x = sr * sinf(a);
  GLfloat p2y = sr - sr * cosf(a);
  GLfloat p3x = sr * sinf(2 * a);
  GLfloat p3y = sr - sr * cosf(2 * a);
  GLfloat p13x = lerp(p2x, p3x, .05f);
  GLfloat p13y = lerp(p2y, p3y, .05f);
  GLfloat p14x = lerp(p2x, p3x, .45f);
  GLfloat p14y = lerp(p2y, p3y, .45f);
  GLfloat p12x = lerp(p14x, u, .75);
  GLfloat p23x = lerp(p2x, 0, .25);
  GLfloat p23y = lerp(p2y, 0, .25);
  GLfloat p24x = lerp(p2x, 0, .75);
  GLfloat p24y = lerp(p2y, 0, .75);
  GLfloat p22x = lerp(p24x, p23x, 1.5);
  GLfloat p22y = -0.1 * u;

  GLfloat tsy = lerp(p3y, u, .7f);

  GLfloat t_vertices[] = {
      0,         sr,   sr, 1, // 0  shell point
      0,         0,    0,  1, // 1 turtle's butt and Origin
      p2x,       p2y,  0,  1, // 2 turtle's shell right
      p3x,       p3y,  0,  1, // 3 turtle's neck right
      -p3x,      p3y,  0,  1, // 4 turtle's neck left
      -p2x,      p2y,  0,  1, // 5 turtle's shell left

      0,         u,    0,  1, // 6 head tip
      -p3x * he, tsy,  0,  1, // 7 turtle's left ear
      -p3x,      p3y,  0,  1, // 8 turtle's neck left
      p3x,       p3y,  0,  1, // 9 turtle's neck right
      p3x * he,  tsy,  0,  1, // 10 turtle's right ear
      0.8f * u,  p14y, 0,  1, // 11 right flipper tip
      p12x,      p3y,  0,  1, // 12 right flipper joint
      p13x,      p13y, 0,  1, // 13 right flipper shoulder
      p14x,      p14y, 0,  1, // 14 right flipper pit
      -0.8f * u, p14y, 0,  1, // 15 -11
      -p14x,     p14y, 0,  1, // 16 -14
      -p13x,     p13y, 0,  1, // 17 -13
      -p12x,     p3y,  0,  1, // 18 -12
      -p22x,     p22y, 0,  1, // 19 -22
      -p24x,     p24y, 0,  1, // 20 -24
      -p23x,     p23y, 0,  1, // 21 -23
      p22x,      p22y, 0,  1, // 22
      p23x,      p23y, 0,  1, // 23
      p24x,      p24y, 0,  1  // 24
  };

  GLfloat t_colors[] = {
      T_SHELL_TOP,        // 0
      T_SHELL_SIDE,       // 1
      T_SHELL_SIDE,       // 2
      T_SHELL_SIDE,       // 3
      T_SHELL_SIDE,       // 4
      T_SHELL_SIDE,       // 5
      T_HEAD_TOP,         // 6
      T_HEAD_SIDE,        // 7
      T_HEAD_SIDE,        // 8
      T_HEAD_SIDE,        // 9
      T_HEAD_SIDE,        // 10
      T_FLIPPER_TIP,      // 11
      T_FLIPPER_JOINT,    // 12
      T_FLIPPER_SHOULDER, // 13
      T_FLIPPER_SHOULDER, // 14
      T_FLIPPER_SHOULDER, // 15
      T_FLIPPER_SHOULDER, // 16
      T_FLIPPER_SHOULDER, // 17
      T_FLIPPER_SHOULDER, // 18
      T_HEAD_SIDE,        // 19
      T_HEAD_SIDE,        // 20
      T_HEAD_SIDE,        // 21
      T_HEAD_SIDE,        // 22
      T_HEAD_SIDE,        // 23
      T_HEAD_SIDE,        // 24
  };

  GLuint t_indices[17 * 3] = {
      0,  1,  2,  0,  2,  3,  0,  3,  4,  0,  4,  5,  0,  5,  1,  1,  5,
      4,  1,  4,  3,  1,  3,  2,  6,  7,  8,  6,  8,  9,  6,  9,  10, 11,
      12, 13, 11, 13, 14, 15, 16, 17, 15, 17, 18, 19, 20, 21, 22, 23, 24};

  t_object = new QOpenGLVertexArrayObject(this);
  t_object->create();
  t_object->bind();

  t_vertex_bo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  t_vertex_bo->create();
  t_vertex_bo->setUsagePattern(QOpenGLBuffer::StaticDraw);
  t_vertex_bo->bind();
  t_vertex_bo->allocate(t_vertices, 25 * 4 * sizeof(GLfloat));

  t_index_bo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
  t_index_bo->create();
  t_index_bo->setUsagePattern(QOpenGLBuffer::StaticDraw);
  t_index_bo->bind();
  t_index_bo->allocate(t_indices, 17 * 3 * sizeof(GL_UNSIGNED_INT));

  t_index_bo->release();

  t_vertex_bo->release();

  t_color_bo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  t_color_bo->create();
  t_color_bo->setUsagePattern(QOpenGLBuffer::StaticDraw);
  t_color_bo->bind();
  t_color_bo->allocate(t_colors, 25 * 4 * sizeof(GLfloat));

  t_color_bo->release();

  t_object->release();
}

void Canvas::initLinesVBO()
{
    linesObject = new QOpenGLVertexArrayObject(this);
    bool isOK = linesObject->create();

    // If this fails, then I don't know what to do.
    if ( ! isOK) {
        qDebug() <<"Failed OpenGL init!";
        qDebug() << "This won't work.";
    }

    linesVertexBufferObject = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    linesVertexBufferObject->create();
    linesVertexBufferObject->setUsagePattern(QOpenGLBuffer::StaticDraw);

    linesColorBufferObject = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    linesColorBufferObject->create();
    linesColorBufferObject->setUsagePattern(QOpenGLBuffer::StaticDraw);
}


Canvas::Canvas(QWidget *parent) : QOpenGLWidget(parent) {
  boundsX = initialBoundX;
  boundsY = initialBoundY;
  backgroundColor[0] = 0;
  backgroundColor[1] = 0;
  backgroundColor[2] = 0;
  backgroundColor[3] = 1;
  turtleMatrix.setToIdentity();
  turtleIsVisible = true;
}

void Canvas::initializeGL() {
  initializeOpenGLFunctions();
  shaderProgram = new QOpenGLShaderProgram(this);
  shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                         vertexShaderSource);
  shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                         fragmentShaderSource);
  shaderProgram->link();
  shaderProgram->bind();

  initTurtleVBO();
  initSurfaceVBO();
  setSurfaceVertices();
  initLinesVBO();

  shaderProgram->release();

  matrixUniformID = shaderProgram->uniformLocation("matrix");

  glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, pensizeRange);

  setPensize(startingPensize);
  setPenmode(penModePaint);
}

void Canvas::updateMatrix() {
  float aspect = (widgetHeight == 0) ? 1 : (float)widgetWidth / widgetHeight;
  float boundsAspect = boundsX / boundsY;
  float largestBound = (boundsX > boundsY) ? boundsX : boundsY;
  // Sqrt(3) is the Z-axis view angle (in radians) where things start looking
  // distorted (in my opinion).
  float zPlane = sqrt(3) * largestBound;
  float fovy = (2 * 180 / M_PI) * atan(boundsY / zPlane);
  if (boundsAspect > aspect) {
    float fovx = (2 * 180 / M_PI) * atan(boundsX / zPlane);
    fovy = (2 * 180 / M_PI) * atan(tan((M_PI / 180 / 2) * fovx) / aspect);
  }
  matrix.setToIdentity();
  matrix.perspective(fovy, aspect, zPlane * 0.01f, zPlane * 100.0f);
  matrix.translate(0, 0, -zPlane);

  invertedMatrix = matrix.inverted();
}

void Canvas::setTurtleMatrix(const QMatrix4x4 &matrix)
{
  turtleMatrix = matrix;
  update();
}


void Canvas::setTurtleIsVisible(bool isVisible)
{
  turtleIsVisible = isVisible;
  update();
}

QPointF Canvas::worldToScreen(const QVector4D &world) {
  QVector2D pv = (world * matrix).toVector2DAffine();
  return QPointF((pv.x() + 1) * widgetWidth / 2,
                 widgetHeight - (pv.y() + 1) * widgetHeight / 2);
}

// Create a line from the near to far frustum at the mouse click.
// Calculate where that line intersects with the Z=0 plane.
QVector2D Canvas::screenToWorld(const QPointF &p) {
  QPointF q(2 * p.x() / widgetWidth - 1,
            -2 * (p.y() - widgetHeight) / widgetHeight - 1);
  QVector4D s0(q.x(), q.y(), 0, 1);
  QVector4D s1(q.x(), q.y(), 1, 1);
  QVector3D p0 = (s0 * invertedMatrix).toVector3DAffine();
  QVector3D p1 = (s1 * invertedMatrix).toVector3DAffine();
  float u = -p0.z() / (p1.z() - p0.z());
  return QVector2D(p0.x() + u * (p1.x() - p0.x()),
                   p0.y() + u * (p1.y() - p0.y()));
}


void Canvas::addLine(const QVector3D &vertexA, const QVector3D &vertexB, const QColor &color)
{
  if (drawingElementList.isEmpty() ||
      (drawingElementList.last().type != canvasDrawArrayType) ||
      (drawingElementList.last().u.drawArrayElement.mode != GL_LINES)) {
    CanvasDrawingElement cde;
    cde.type = canvasDrawArrayType;
    cde.u.drawArrayElement.mode = GL_LINES;
    cde.u.drawArrayElement.first = vertexColors.size() / 4;
    cde.u.drawArrayElement.count = 0;
    drawingElementList.push_back(cde);
  }

  drawingElementList.last().u.drawArrayElement.count += 2;

  vertices.push_back(vertexA.x());
  vertices.push_back(vertexA.y());
  vertices.push_back(vertexA.z());
  vertices.push_back(1);
  if (currentPenMode == penModeReverse) {
    vertexColors.push_back(UCHAR_MAX);
    vertexColors.push_back(UCHAR_MAX);
    vertexColors.push_back(UCHAR_MAX);
    vertexColors.push_back(UCHAR_MAX);
  } else {
    vertexColors.push_back(color.red());
    vertexColors.push_back(color.green());
    vertexColors.push_back(color.blue());
    vertexColors.push_back(color.alpha());
  }

  vertices.push_back(vertexB.x());
  vertices.push_back(vertexB.y());
  vertices.push_back(vertexB.z());
  vertices.push_back(1);
  if (currentPenMode == penModeReverse) {
    vertexColors.push_back(UCHAR_MAX);
    vertexColors.push_back(UCHAR_MAX);
    vertexColors.push_back(UCHAR_MAX);
    vertexColors.push_back(UCHAR_MAX);
  } else {
    vertexColors.push_back(color.red());
    vertexColors.push_back(color.green());
    vertexColors.push_back(color.blue());
    vertexColors.push_back(color.alpha());
  }

  update();
}

void Canvas::addPolygon(const QList<QVector3D> &points,
                        const QList<QColor> &colors) {
  CanvasDrawingElement cde;
  cde.type = canvasDrawArrayType;
  cde.u.drawArrayElement.mode = GL_TRIANGLE_FAN;
  cde.u.drawArrayElement.first = vertexColors.size() / 4;
  cde.u.drawArrayElement.count = points.size();
  drawingElementList.push_back(cde);

  auto pIter = points.begin();
  for (auto cIter = colors.begin(); cIter != colors.end(); ++cIter, ++pIter) {
    vertices.push_back(pIter->x());
    vertices.push_back(pIter->y());
    vertices.push_back(pIter->z());
    vertices.push_back(1);
    vertexColors.push_back(cIter->red());
    vertexColors.push_back(cIter->green());
    vertexColors.push_back(cIter->blue());
    vertexColors.push_back(cIter->alpha());
  }

  update();
}

void Canvas::addLabel(const QString &aText, const QVector3D &aLocation,
                      const QColor &aColor) {
  labels.push_back(Label(aText, aLocation, aColor, labelFont));
  update();
}

void Canvas::setBackgroundColor(const QColor &c) {
  backgroundColor[0] = c.redF();
  backgroundColor[1] = c.greenF();
  backgroundColor[2] = c.blueF();
  backgroundColor[3] = c.alphaF();
  setSurfaceVertices();
  update();
}

void Canvas::setLabelFontName(const QString name)
{
    labelFont.setFamily(name);
}

void Canvas::setLabelFontSize(double aSize)
{
    labelFont.setPointSizeF(aSize);
}

void Canvas::setBounds(double x, double y)
{
    boundsX = x;
    boundsY = y;
    updateMatrix();
    update();
}

void Canvas::resizeGL(int width, int height) {
  widgetWidth = width;
  widgetHeight = height;
  glViewport(0, 0, widgetWidth, widgetHeight);
  updateMatrix();
}

void Canvas::paintSurface() {
  if (canvasIsBounded) {

    // Draw the Qt-default background color
    const QColor &bg = QWidget::palette().color(QWidget::backgroundRole());
    glClearColor(bg.redF(), bg.greenF(), bg.blueF(), bg.alphaF());
    glClear(GL_COLOR_BUFFER_BIT);

    // draw the surface rectangle
    surfaceArrayObject->bind();
    surfaceVertexBufferObject->bind();
    shaderProgram->enableAttributeArray("posAttr");
    shaderProgram->setAttributeBuffer("posAttr", GL_FLOAT, 0, 4);

    surfaceColorBufferObject->bind();
    shaderProgram->enableAttributeArray("colAttr");
    shaderProgram->setAttributeBuffer("colAttr", GL_FLOAT, 0, 4);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    surfaceColorBufferObject->release();
    surfaceVertexBufferObject->release();
    surfaceArrayObject->release();
  } else {
    glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2],
                 backgroundColor[3]);
    glClear(GL_COLOR_BUFFER_BIT);
  }
}

void Canvas::paintTurtle() {
  t_object->bind();
  t_vertex_bo->bind();

  shaderProgram->enableAttributeArray("posAttr");
  shaderProgram->setAttributeBuffer("posAttr", GL_FLOAT, 0, 4);

  t_index_bo->bind();
  t_color_bo->bind();

  shaderProgram->enableAttributeArray("colAttr");
  shaderProgram->setAttributeBuffer("colAttr", GL_FLOAT, 0, 4);

  QMatrix4x4 t_matrix = matrix * turtleMatrix;

  shaderProgram->setUniformValue(matrixUniformID, t_matrix);

  glDrawElements(GL_TRIANGLES, 17 * 3, GL_UNSIGNED_INT, 0);
  t_color_bo->release();
  t_index_bo->release();
  t_vertex_bo->release();
  t_object->release();
}

void Canvas::paintElements() {
  linesObject->bind();
  void *addrVertices = (vertices.size() > 0) ? &vertices[0] : NULL;
  void *addrVerticexColors =
      (vertexColors.size() > 0) ? &vertexColors[0] : NULL;

  linesVertexBufferObject->bind();
  linesVertexBufferObject->allocate(addrVertices,
                                    vertices.size() * sizeof(GLfloat));

  shaderProgram->enableAttributeArray("posAttr");
  shaderProgram->setAttributeBuffer("posAttr", GL_FLOAT, 0, 4);

  linesColorBufferObject->bind();
  linesColorBufferObject->allocate(addrVerticexColors,
                                   vertexColors.size() * sizeof(GLubyte));

  shaderProgram->enableAttributeArray("colAttr");
  shaderProgram->setAttributeBuffer("colAttr", GL_UNSIGNED_BYTE, 0, 4);

  glEnable(GL_BLEND);

  for (auto iter = drawingElementList.begin(); iter != drawingElementList.end();
       ++iter) {

    switch (iter->type) {
    case canvasDrawArrayType: {
      CanvasDrawingArrayElement &cdae = iter->u.drawArrayElement;
      glDrawArrays(cdae.mode, cdae.first, cdae.count);
      break;
    }
    case canvasDrawSetPenmodeType: {
      CanvasDrawingSetPenmodeElement &spme = iter->u.penmodeElement;
      switch (spme.penMode) {
      case penModePaint:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
      case penModeReverse:
        glBlendColor(0, 0, 0, 1);
        glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_CONSTANT_COLOR);
        break;
      case penModeErase:
        glBlendColor(backgroundColor[0], backgroundColor[1], backgroundColor[2],
                     backgroundColor[3]);
        glBlendFunc(GL_CONSTANT_COLOR, GL_ZERO);
        break;
      default:
        qDebug() << "I'm not even supposed to be here";
        Q_ASSERT(false);
      }
      break;
    }
    case canvasDrawSetPensizeType: {
      CanvasDrawingSetPensizeElement &pse = iter->u.pensizeElement;
      glLineWidth(pse.width);
      break;
    }
    default:
      qDebug() << "This is weird";
      Q_ASSERT(false);
    }
  } // /for drawingElementList
  linesColorBufferObject->release();
  linesVertexBufferObject->release();
  linesObject->release();
}

void Canvas::paintLabels(QPainter *painter) {
  for (QList<Label>::iterator iter = labels.begin(); iter != labels.end();
       ++iter) {
    Label &l = *iter;
    QPointF p = worldToScreen(l.position);
    painter->setPen(l.color);
    painter->setFont(l.font);
    painter->drawText(p, l.text);
  }
}


void Canvas::paintGL() {
  QPainter painter(this);
  painter.beginNativePainting();
  shaderProgram->bind();

  shaderProgram->setUniformValue(matrixUniformID, matrix);

  paintSurface();

  paintElements();

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  if (turtleIsVisible) {
    paintTurtle();
  }
  shaderProgram->release();

  painter.endNativePainting();

  paintLabels(&painter);
}

void Canvas::clearScreen() {
  drawingElementList.clear();

  vertices.clear();
  vertexColors.clear();
  labels.clear();

  setPenmode(currentPenMode);
  setPensize(currentPensize);

  update();
}

void Canvas::setSurfaceVertices() {
  GLfloat surfaceVertices[] = {
      (GLfloat)boundsX,  (GLfloat)-boundsY, 0, 1, // 3
      (GLfloat)boundsX,  (GLfloat)boundsY,  0, 1, // 2
      (GLfloat)-boundsX, (GLfloat)boundsY,  0, 1, // 1
      (GLfloat)-boundsX, -(GLfloat)boundsY, 0, 1, // 0
  };

  GLfloat surfaceVerticexColors[] = {
      backgroundColor[0], backgroundColor[1], backgroundColor[2],
      backgroundColor[3], backgroundColor[0], backgroundColor[1],
      backgroundColor[2], backgroundColor[3], backgroundColor[0],
      backgroundColor[1], backgroundColor[2], backgroundColor[3],
      backgroundColor[0], backgroundColor[1], backgroundColor[2],
      backgroundColor[3],
  };

  surfaceVertexBufferObject->bind();
  surfaceVertexBufferObject->allocate(surfaceVertices, 4 * 4 * sizeof(GLfloat));
  shaderProgram->enableAttributeArray("posAttr");
  shaderProgram->setAttributeBuffer("posAttr", GL_FLOAT, 0, 4);
  surfaceVertexBufferObject->release();

  surfaceColorBufferObject->bind();
  surfaceColorBufferObject->allocate(surfaceVerticexColors,
                                     4 * 4 * sizeof(GLfloat));
  shaderProgram->enableAttributeArray("colAttr");
  shaderProgram->setAttributeBuffer("colAttr", GL_FLOAT, 0, 4);
  surfaceColorBufferObject->release();
}


void Canvas::setPenmode(PenModeEnum newMode) {
  currentPenMode = newMode;
  if (drawingElementList.isEmpty() ||
      (drawingElementList.last().type != canvasDrawSetPenmodeType)) {
    CanvasDrawingElement cde;
    cde.type = canvasDrawSetPenmodeType;
    cde.u.penmodeElement.penMode = newMode;
    drawingElementList.push_back(cde);
  } else {
    drawingElementList.last().u.penmodeElement.penMode = newMode;
  }
}

void Canvas::setPensize(GLfloat aSize) {
  currentPensize = aSize;
  if (drawingElementList.isEmpty() ||
      (drawingElementList.last().type != canvasDrawSetPensizeType)) {
    CanvasDrawingElement cde;
    cde.type = canvasDrawSetPensizeType;
    cde.u.pensizeElement.width = aSize;
    drawingElementList.push_back(cde);
  } else {
    drawingElementList.last().u.pensizeElement.width = aSize;
  }
}


QImage Canvas::getImage()
{
    QImage framebuffer = grabFramebuffer();

    if (framebuffer.width() * framebuffer.height() == 0)
        return framebuffer;

    // If our canvas has bounds, then clip
    if (canvasIsBounded) {
        double fbwidth = framebuffer.width();
        double fbheight = framebuffer.height();
        double fbaspect = fbheight / fbwidth;
        double canvasaspect = boundsY / boundsX;
        double x = 0;
        double y = 0;
        double width = fbwidth;
        double height = fbheight;

        if (fbaspect > canvasaspect) {
            // clip top and bottom
            height = fbwidth * canvasaspect;
            if (height > fbheight) height = fbheight;
            y = (fbheight - height) / 2;
        } else {
            // clip left and right
            width = fbheight / canvasaspect;
            if (width > fbwidth) width = fbwidth;
            x = (fbwidth - width) / 2;
        }
        framebuffer = framebuffer.copy((int)x, (int)y, (int)width, (int)height);
    }
    return framebuffer;

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
  QVector2D clickPos = screenToWorld(event->position());
  if ( ! canvasIsBounded || (fabsf(clickPos.x()) <= boundsX) && (fabsf(clickPos.y()) <= boundsY)) {
    mouseButtonPressed = true;
    emit sendMouseclickedSignal(clickPos, buttonID);
  }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
    QVector2D mousePos = screenToWorld(event->position());
    if (mouseButtonPressed ||
            ! canvasIsBounded ||
            (fabsf(mousePos.x()) <= boundsX) && (fabsf(mousePos.y()) <= boundsY))
        emit sendMousemovedSignal(mousePos);

}

void Canvas::mouseReleaseEvent(QMouseEvent *) {
  mouseButtonPressed = false;
  emit sendMouseReleasedSignal();
}
