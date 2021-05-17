#ifndef CANVAS_H
#define CANVAS_H

//===-- qlogo/canvas.h - Canvas class definition -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
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

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QMatrix4x4>
#include "constants.h"

class QOpenGLShaderProgram;
class QOpenGLBuffer;
class QOpenGLVertexArrayObject;

/// Contains the information that describes a label's appearance on the Canvas.
class Label {
public:
  QString text;
  QVector4D position;
  QColor color;
  QFont font;

  Label(const QString &aText, const QVector3D &aPosition, const QColor &aColor,
        const QFont &aFont)
      : text(aText), color(aColor), font(aFont) { position = QVector4D(aPosition, 1); }
};

enum CanvasDrawingElementType {
  canvasDrawArrayType,
  canvasDrawSetPenmodeType,
  canvasDrawSetPensizeType
};

struct CanvasDrawingArrayElement {
  GLenum mode;   // GL_LINES or GL_TRIANGLE_FAN
  GLint first;   // the index of the first vertex in the vertex array
  GLsizei count; // the number of vertices to draw
};

struct CanvasDrawingSetPenmodeElement {
  PenModeEnum penMode;
};

struct CanvasDrawingSetPensizeElement {
  GLfloat width;
};

union CanvasDrawingElementU {
  CanvasDrawingArrayElement drawArrayElement;
  CanvasDrawingSetPenmodeElement penmodeElement;
  CanvasDrawingSetPensizeElement pensizeElement;
};

struct CanvasDrawingElement {
  CanvasDrawingElementType type;
  CanvasDrawingElementU u;
};

/// The widget where turtle graphics are drawn.
class Canvas : public QOpenGLWidget, protected QOpenGLFunctions {
  Q_OBJECT
  QMatrix4x4 turtleMatrix;
  bool turtleIsVisible;

  bool canvasIsBounded;

  // Visible vertices on the X axis range from -boundsX to +boundsX
  qreal boundsX;
  // Visible vertices on the Y axis range from -boundsY to +boundsY
  qreal boundsY;

  // The main data structure for all of the drawn elements on the canvas
  // (sans labels).
  QList<CanvasDrawingElement> drawingElementList;

  GLclampf backgroundColor[4];

  // Some initializers
  void initTurtleVBO(void);
  void initSurfaceVBO(void);
  void setSurfaceVertices(void);


  // The matrix that converts world coordinates to screen coordinates.
  QMatrix4x4 matrix;
  GLuint matrixUniformID; // matrix ID for GLSL

  // the inverse of matrix. Used for converting mouse coordinates to world
  // coordinates.
  QMatrix4x4 invertedMatrix;
  int widgetWidth;
  int widgetHeight;

  void initializeGL() override;
  void resizeGL(int width, int height) override;
  void paintGL() override;

  // The collection of text labels
  QList<Label> labels;

  QOpenGLShaderProgram *shaderProgram;

  // Border Surface VBO
  QOpenGLVertexArrayObject *surfaceArrayObject;
  QOpenGLBuffer *surfaceVertexBufferObject;
  QOpenGLBuffer *surfaceColorBufferObject;

  // LOGO Drawing VBO
  QOpenGLVertexArrayObject *linesObject = NULL;
  QOpenGLBuffer *linesVertexBufferObject;
  QOpenGLBuffer *linesColorBufferObject;

  // Turtle Drawing VBO
  QOpenGLVertexArrayObject *t_object = NULL;
  QOpenGLBuffer *t_vertex_bo;
  QOpenGLBuffer *t_color_bo;
  QOpenGLBuffer *t_index_bo;


  // Vertices information for user-generated lines and polygons
  QVector<GLfloat> vertices;
  QVector<GLubyte> vertexColors;

  // draw, erase, or reverse
  PenModeEnum currentPenMode;

  GLfloat pensizeRange[2]; // Minimum and maximum valid pen sizes
  GLfloat currentPensize = 0;


  // These are called by paintGL()
  void paintSurface();
  void paintTurtle();
  void paintElements();
  void paintLabels(QPainter *painter);

  void updateMatrix(void);

public:
  /// Construct a Canvas
  Canvas(QWidget *parent = 0);

  void setTurtleMatrix(const QMatrix4x4 &matrix);
  void setTurtleIsVisible(bool isVisible);
  void addLine(const QVector3D &vertexA, const QVector3D &vertexB, const QColor &color);
  void addPolygon(const QList<QVector3D> &points, const QList<QColor> &colors);
  void addLabel(const QString &aText, const QVector3D &aLocation,
                const QColor &aColor, const QFont &aFont);

  /// Sets future lines and polygons to be drawn using newMode.
  void setPenmode(PenModeEnum newMode);

  /// Sets the width of future lines drawn to aSize.
  void setPensize(GLfloat aSize);

  /// Returns true if aSize is a valid pen size.
  bool isPenSizeValid(GLfloat aSize);

  /// Sets the background color to c.
  ///
  /// The background is drawn either as a filled rectangle when isBounded=true
  /// or the background color fills the entire widget when isBounded=false.
  void setBackgroundColor(const QColor &c);

  QPointF worldToScreen(const QVector4D &world);

  /// Clears the screen and removes all drawing elements from their respective
  /// lists.
  void clearScreen();

  /// Get the minimum pen size
  double minimumPenSize() { return pensizeRange[0];}

  /// Get the maximum pen size
  double maximumPenSize() { return pensizeRange[1];}

};

#endif // CANVAS_H
