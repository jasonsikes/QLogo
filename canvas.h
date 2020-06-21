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
/// graphics portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QMatrix4x4>

class QOpenGLShaderProgram;
class QOpenGLBuffer;
class QOpenGLVertexArrayObject;


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



  // These are called by paintGL()
  void paintSurface();
  void paintTurtle();
  //void paintElements();
  //void paintLabels(QPainter *painter);

  void updateMatrix(void);

public:
  /// Construct a Canvas
  Canvas(QWidget *parent = 0);

  void setTurtleMatrix(const QMatrix4x4 &matrix);
  void setTurtleIsVisible(bool isVisible);

};

#endif // CANVAS_H
