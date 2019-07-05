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

/// The widget where turtle graphics are drawn.
class Canvas : public QOpenGLWidget, protected QOpenGLFunctions {
  Q_OBJECT


public:
  /// Construct a Canvas
  Canvas(QWidget *parent = 0);

};

#endif // CANVAS_H
