// ===-- qlogo/exports/exports_graphics.cpp - Exports for the QLogo language -------*- C++ -*-===//
//
// Copyright 2017-2026 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the C library routines that are used by the QLogo compiler
/// for graphical canvas operations.
///
//===----------------------------------------------------------------------===//

#include "interface/streammanager.h"
#include "sharedconstants.h"
#include "workspace/turtle.h"
#include "workspace/exports_internal.h"
#include "datum_types.h"
#include "workspace/kernel.h"
#include "astnode.h"
#include "flowcontrol.h"
#include "compiler_types.h"
#include "workspace/callframe.h"

#include <QFile>

/// @brief Convert a QColor to a list of RGB values on a scale of 0 to 100.
/// @param c the QColor to convert.
/// @return a list of RGB values on a scale of 0 to 100.
List *listFromColor(const QColor &c)
{
    ListBuilder retvalBuilder;
    // Each of the RGB values are on a scale of 0 to 100.
    retvalBuilder.append(DatumPtr(c.redF() * 100.0));
    retvalBuilder.append(DatumPtr(c.greenF() * 100.0));
    retvalBuilder.append(DatumPtr(c.blueF() * 100.0));
    return retvalBuilder.finishedList().listValue();
}

EXPORTC addr_t moveTurtleForward(ASTNode *node, double distance)
{
    auto retval = Turtle::get().forward(distance);
    return reinterpret_cast<addr_t>(retval != nullptr ? retval : Datum::notADatum());
}

EXPORTC void moveTurtleRotate(double angle)
{
    Turtle::get().rotate(angle);
}

EXPORTC addr_t setTurtleXY(ASTNode *node, double x, double y)
{
    auto retval = Turtle::get().setxy(x, y);
    return reinterpret_cast<addr_t>(retval != nullptr ? retval : Datum::notADatum());
}

EXPORTC addr_t setTurtleX(ASTNode *node, double x)
{
    auto retval = Turtle::get().setx(x);
    return reinterpret_cast<addr_t>(retval != nullptr ? retval : Datum::notADatum());
}

EXPORTC addr_t setTurtleY(ASTNode *node, double y)
{
    auto retval = Turtle::get().sety(y);
    return reinterpret_cast<addr_t>(retval != nullptr ? retval : Datum::notADatum());
}

EXPORTC addr_t setTurtlePos(ASTNode *node, addr_t posAddr)
{
    const auto *pos = reinterpret_cast<const double *>(posAddr);
    double x = pos[0];
    double y = pos[1];
    auto retval = Turtle::get().setxy(x, y);
    return reinterpret_cast<addr_t>(retval != nullptr ? retval : Datum::notADatum());
}

EXPORTC void setTurtleHeading(double newHeading)
{
    double oldHeading = Turtle::get().getHeading();

    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    newHeading = 360 - newHeading;

    double adjustment = oldHeading - newHeading;
    Turtle::get().rotate(adjustment);
}

EXPORTC void setTurtleMoveToHome(void)
{
    Turtle::get().moveToHome();
}

EXPORTC void drawTurtleArc(double angle, double radius)
{
    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    angle = 0 - angle;

    if ((angle < -360) || (angle > 360))
        angle = 360;

    if ((angle != 0) && (radius != 0))
        Turtle::get().drawArc(angle, radius);
}

EXPORTC addr_t getTurtlePos(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto xy = Turtle::get().getxy();
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(xy.x()));
    retvalBuilder.append(DatumPtr(xy.y()));
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC double getTurtleHeading(void)
{
    double retval = Turtle::get().getHeading();

    // Heading should only show two decimal places.
    retval = round(retval * 100.0) / 100.0;

    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    if (retval > 0)
        retval = 360 - retval;

    return retval;
}

EXPORTC double getTurtleTowards(addr_t posAddr)
{
    auto xy = Turtle::get().getxy();
    const auto *pos = reinterpret_cast<const double *>(posAddr);
    double vx = pos[0];
    double vy = pos[1];
    double retval = atan2(xy.x() - vx, vy - xy.y()) * (180 / PI);

    // Heading should only show two decimal places.
    retval = round(retval * 100.0) / 100.0;

    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    retval = 0 - retval;
    if (retval < 0)
        retval = 360 + retval;

    return retval;
}

EXPORTC addr_t getScrunch(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(1));
    retvalBuilder.append(DatumPtr(1));
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC void setTurtleVisible(uint32_t visible)
{
    Turtle::get().setIsTurtleVisible(visible);
}

EXPORTC void clean(void)
{
    Config::get().mainInterface()->clearCanvas();
}

EXPORTC void setTurtleMode(uint32_t mode)
{
    auto newMode = static_cast<TurtleModeEnum>(mode);
    if (Turtle::get().getMode() != newMode)
    {
        bool isCanvasBounded = ((newMode == turtleWindow) || (newMode == turtleFence));
        Turtle::get().setMode(newMode);
        Config::get().mainInterface()->setIsCanvasBounded(isCanvasBounded);
    }
}

EXPORTC addr_t getBounds(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    double x = Config::get().mainInterface()->boundX();
    double y = Config::get().mainInterface()->boundY();

    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(x));
    retvalBuilder.append(DatumPtr(y));
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC void setBounds(double x, double y)
{
    Config::get().mainInterface()->setBounds(x, y);
}

EXPORTC int32_t beginFilledWithColor(addr_t colorAddr)
{
    auto *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Kernel::get().colorFromDatumPtr(color, DatumPtr(d)))
        return 0;
    Turtle::get().beginFillWithColor(color);
    return 1;
}

EXPORTC void endFilled(void)
{
    Turtle::get().endFill();
}

EXPORTC void addLabel(addr_t textAddr)
{
    auto *d = reinterpret_cast<Datum *>(textAddr);
    Config::get().mainInterface()->drawLabel(d->toString());
}

EXPORTC void setLabelHeight(double height)
{
    Config::get().mainInterface()->setLabelFontSize(height);
}

EXPORTC void setLabelFontName(addr_t nameAddr)
{
    auto *w = reinterpret_cast<Word *>(nameAddr);
    Config::get().mainInterface()->setLabelFontName(w->toString());
}

EXPORTC void setScreenMode(uint32_t mode)
{
    Config::get().mainInterface()->setScreenMode(static_cast<ScreenModeEnum>(mode));
}

EXPORTC bool isTurtleVisible(void)
{
    return Turtle::get().isTurtleVisible();
}

EXPORTC addr_t getScreenMode(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    ScreenModeEnum mode = Config::get().mainInterface()->getScreenMode();
    QString modeStr;
    switch (mode)
    {
    case textScreenMode:
    case initScreenMode:
        modeStr = QObject::tr("textscreen");
        break;
    case splitScreenMode:
        modeStr = QObject::tr("splitscreen");
        break;
    case fullScreenMode:
        modeStr = QObject::tr("fullscreen");
        break;
    }
    auto *retval = new Word(modeStr);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t getTurtleMode(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    TurtleModeEnum mode = Turtle::get().getMode();
    QString modeStr;
    switch (mode)
    {
    case turtleWrap:
        modeStr = QObject::tr("wrap");
        break;
    case turtleFence:
        modeStr = QObject::tr("fence");
        break;
    case turtleWindow:
        modeStr = QObject::tr("window");
        break;
    }
    auto *retval = new Word(modeStr);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t getLabelSize(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    double height = Config::get().mainInterface()->getLabelFontSize();
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(height));
    retvalBuilder.append(DatumPtr(height));
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t getLabelFontName(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QString name = Config::get().mainInterface()->getLabelFontName();
    if (name.isNull())
        name = QStringLiteral("");
    auto *word = new Word(name);
    e->watch(word);
    return reinterpret_cast<addr_t>(word);
}

EXPORTC void setPenIsDown(bool isDown)
{
    Turtle::get().setPenIsDown(isDown);
}

EXPORTC void setPenMode(int32_t mode)
{
    Turtle::get().setPenMode(static_cast<PenModeEnum>(mode));
}

EXPORTC bool setPenColor(addr_t colorAddr)
{
    auto *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Kernel::get().colorFromDatumPtr(color, DatumPtr(d)))
        return false;
    Turtle::get().setPenColor(color);
    return true;
}

EXPORTC addr_t getAllColors(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    ListBuilder lb;
    QStringList colors = QColor::colorNames();
    for (const QString &i : colors)
    {
        lb.append(DatumPtr(new Word(i)));
    }
    DatumPtr retval = lb.finishedList();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC bool isColorIndexGood(addr_t colorIndexAddr, double lowerLimit)
{
    auto *d = reinterpret_cast<Datum *>(colorIndexAddr);
    if (!d->isWord())
        return false;
    auto *w = static_cast<Word *>(d);
    double colorIndex = w->numberValue();

    return (w->numberIsValid) && (colorIndex == floor(colorIndex)) && (colorIndex >= lowerLimit) &&
           (colorIndex < Kernel::get().palette_.size());
}

EXPORTC bool setPalette(addr_t colorIndexAddr, addr_t colorAddr)
{
    auto colorIndex = static_cast<int>((reinterpret_cast<Word *>(colorIndexAddr))->numberValue());
    auto *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Kernel::get().colorFromDatumPtr(color, DatumPtr(d)))
        return false;
    Kernel::get().palette_[colorIndex] = color;
    return true;
}

EXPORTC void setPenSize(double size)
{
    Turtle::get().setPenSize(size);
}

EXPORTC bool setBackground(addr_t colorAddr)
{
    auto *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Kernel::get().colorFromDatumPtr(color, DatumPtr(d)))
        return false;
    Config::get().mainInterface()->setCanvasBackgroundColor(color);
    return true;
}

EXPORTC bool isPenDown(void)
{
    return Turtle::get().isPenDown();
}

EXPORTC addr_t getPenMode(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    PenModeEnum pm = Turtle::get().getPenMode();
    QString retval;
    switch (pm)
    {
    case penModePaint:
        retval = QObject::tr("paint");
        break;
    case penModeReverse:
        retval = QObject::tr("reverse");
        break;
    case penModeErase:
        retval = QObject::tr("erase");
        break;
    }
    auto *w = new Word(retval);
    e->watch(w);
    return reinterpret_cast<addr_t>(w);
}

EXPORTC addr_t getPenColor(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    const QColor &color = Turtle::get().getPenColor();
    List *retval = listFromColor(color);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t getPaletteColor(addr_t eAddr, addr_t colorIndexAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto colorIndex = static_cast<int>((reinterpret_cast<Word *>(colorIndexAddr))->numberValue());
    const QColor &color = Kernel::get().palette_[colorIndex];
    List *retval = listFromColor(color);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC double getPenSize(void)
{
    return Turtle::get().getPenSize();
}

EXPORTC addr_t getBackground(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    const QColor &color = Config::get().mainInterface()->getCanvasBackgroundColor();
    List *retval = listFromColor(color);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t savePict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QString filename = reinterpret_cast<Word *>(filenameAddr)->toString();
    QString filepath = StreamManager::get().filepathForFilename(DatumPtr(filename));
    QImage image = Config::get().mainInterface()->getCanvasImage();
    bool isSuccessful = image.save(filepath);
    auto *retval = reinterpret_cast<Datum *>(nodeAddr);
    if (!isSuccessful)
    {
        retval = FCError::fileSystem();
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t saveSvgpict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QString filename = reinterpret_cast<Word *>(filenameAddr)->toString();
    QString filepath = StreamManager::get().filepathForFilename(DatumPtr(filename));
    QByteArray svgImage = Config::get().mainInterface()->getSvgImage();

    auto *retval = reinterpret_cast<Datum *>(nodeAddr);
    QFile file(filepath);
    bool isSuccessful = file.open(QIODevice::WriteOnly);
    if (!isSuccessful)
    {
        retval = FCError::fileSystem();
    }

    qint64 bytesWritten = file.write(svgImage);
    if (bytesWritten != svgImage.size())
        retval = FCError::fileSystem();

    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t loadPict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *dFilename = reinterpret_cast<Datum *>(filenameAddr);
    auto *retval = reinterpret_cast<Datum *>(nodeAddr);
    if (dFilename->isa_ == Datum::typeWord)
    {
        QString filename = reinterpret_cast<Word *>(filenameAddr)->toString();
        QString filepath = StreamManager::get().filepathForFilename(DatumPtr(filename));
        QImage image = QImage(filepath);
        if (image.isNull())
        {
            retval = FCError::fileSystem();
        }
        Config::get().mainInterface()->setCanvasBackgroundImage(image);
        goto done;
    }
    if (dFilename->isList())
    {
        if (dFilename->listValue()->isEmpty())
        {
            Config::get().mainInterface()->setCanvasBackgroundImage(QImage());
            goto done;
        }
    }
    retval = FCError::doesntLike(DatumPtr(reinterpret_cast<ASTNode *>(nodeAddr)->nodeName_), DatumPtr(dFilename));
done:
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t getMousePos(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QVector2D position = Config::get().mainInterface()->mousePosition();
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(position.x()));
    retvalBuilder.append(DatumPtr(position.y()));
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t getClickPos(addr_t eAddr)
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QVector2D position = Config::get().mainInterface()->lastMouseclickPosition();
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(position.x()));
    retvalBuilder.append(DatumPtr(position.y()));
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC bool isMouseButtonDown(void)
{
    return Config::get().mainInterface()->getIsMouseButtonDown();
}

EXPORTC double getMouseButton(void)
{
    return static_cast<double>(Config::get().mainInterface()->getAndResetButtonID());
}

