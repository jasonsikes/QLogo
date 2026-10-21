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

#include "astnode.h"
#include "compiler_types.h"
#include "datum_types.h"
#include "flowcontrol.h"
#include "interface/streammanager.h"
#include "sharedconstants.h"
#include "workspace/callframe.h"
#include "workspace/exports_internal.h"
#include "workspace/kernel.h"
#include "workspace/turtle.h"

#include <QFile>
#include <QHash>
#include <QPen>
#include <cmath>

/// @brief Convert a QColor to a list of RGB values on a scale of 0 to 100.
/// @param c the QColor to convert.
/// @return a list of RGB values on a scale of 0 to 100.
DatumPtr listFromColor(const QColor &c)
{
    ListBuilder retvalBuilder;
    // Each of the RGB values are on a scale of 0 to 100.
    retvalBuilder.append(DatumPtr(c.redF() * 100.0));
    retvalBuilder.append(DatumPtr(c.greenF() * 100.0));
    retvalBuilder.append(DatumPtr(c.blueF() * 100.0));
    return retvalBuilder.finishedList();
}

EXPORTC addr_t moveTurtleForward(ASTNode *node, double distance) noexcept
{
    auto retval = Turtle::get().forward(distance);
    return reinterpret_cast<addr_t>(retval != nullptr ? retval : Datum::notADatum());
}

EXPORTC void moveTurtleRotate(double angle) noexcept
{
    Turtle::get().rotate(angle);
}

EXPORTC addr_t setTurtleXY(ASTNode *node, double x, double y) noexcept
{
    auto retval = Turtle::get().setxy(x, y);
    return reinterpret_cast<addr_t>(retval != nullptr ? retval : Datum::notADatum());
}

EXPORTC addr_t setTurtleX(ASTNode *node, double x) noexcept
{
    auto retval = Turtle::get().setx(x);
    return reinterpret_cast<addr_t>(retval != nullptr ? retval : Datum::notADatum());
}

EXPORTC addr_t setTurtleY(ASTNode *node, double y) noexcept
{
    auto retval = Turtle::get().sety(y);
    return reinterpret_cast<addr_t>(retval != nullptr ? retval : Datum::notADatum());
}

EXPORTC addr_t setTurtlePos(ASTNode *node, addr_t posAddr) noexcept
{
    const auto *pos = reinterpret_cast<const double *>(posAddr);
    double x = pos[0];
    double y = pos[1];
    auto retval = Turtle::get().setxy(x, y);
    return reinterpret_cast<addr_t>(retval != nullptr ? retval : Datum::notADatum());
}

EXPORTC void setTurtleHeading(double newHeading) noexcept
{
    double oldHeading = Turtle::get().getHeading();

    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    newHeading = 360 - newHeading;

    double adjustment = oldHeading - newHeading;
    Turtle::get().rotate(adjustment);
}

EXPORTC void setTurtleMoveToHome(void) noexcept
{
    Turtle::get().moveToHome();
}

EXPORTC void drawTurtleArc(double angle, double radius) noexcept
{
    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    angle = 0 - angle;

    if ((angle < -360) || (angle > 360))
        angle = 360;

    if ((angle != 0) && (radius != 0))
        Turtle::get().drawArc(angle, radius);
}

EXPORTC addr_t getTurtlePos(addr_t eAddr) noexcept
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

EXPORTC double getTurtleHeading(void) noexcept
{
    double retval = Turtle::get().getHeading();

    // Heading should only show two decimal places.
    retval = round(retval * 100.0) / 100.0;

    // Logo heading is positive in the clockwise direction, opposite conventional linear algebra (right-hand rule).
    if (retval > 0)
        retval = 360 - retval;

    return retval;
}

EXPORTC double getTurtleTowards(addr_t posAddr) noexcept
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

EXPORTC addr_t getScrunch(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    const QSizeF scale = Turtle::get().getScale();
    ListBuilder retvalBuilder;
    retvalBuilder.append(DatumPtr(static_cast<double>(scale.width())));
    retvalBuilder.append(DatumPtr(static_cast<double>(scale.height())));
    Datum *retval = retvalBuilder.finishedList().datumValue();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC void setScrunch(double x, double y) noexcept
{
    Turtle::get().setScale(x, y);
}

EXPORTC void setTurtleVisible(uint32_t visible) noexcept
{
    Turtle::get().setIsTurtleVisible(visible);
}

EXPORTC void clean(void) noexcept
{
    Config::get().mainInterface()->clearCanvas();
}

EXPORTC void setTurtleMode(uint32_t mode) noexcept
{
    auto newMode = static_cast<TurtleModeEnum>(mode);
    if (Turtle::get().getMode() != newMode)
    {
        bool isCanvasBounded = ((newMode == turtleWindow) || (newMode == turtleFence));
        Turtle::get().setMode(newMode);
        Config::get().mainInterface()->setIsCanvasBounded(isCanvasBounded);
    }
}

EXPORTC addr_t getBounds(addr_t eAddr) noexcept
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

EXPORTC void setBounds(double x, double y) noexcept
{
    Config::get().mainInterface()->setBounds(x, y);
}

EXPORTC addr_t beginFilledWithColor(addr_t colorAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    Kernel::get().colorFromDatumPtr(color, DatumPtr(d));
    auto *retval = Turtle::get().beginFillWithColor(color);
    return reinterpret_cast<addr_t>(retval != nullptr ? retval : Datum::notADatum());
}

EXPORTC void endFilled(void) noexcept
{
    Turtle::get().endFill();
}

EXPORTC void addLabel(addr_t textAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(textAddr);
    Config::get().mainInterface()->drawLabel(d->toString());
}

EXPORTC void setLabelHeight(double height) noexcept
{
    Config::get().mainInterface()->setLabelFontSize(height);
}

EXPORTC void setLabelFontName(addr_t nameAddr) noexcept
{
    auto *w = reinterpret_cast<Word *>(nameAddr);
    Config::get().mainInterface()->setLabelFontName(w->toString());
}

EXPORTC void setScreenMode(uint32_t mode) noexcept
{
    Config::get().mainInterface()->setScreenMode(static_cast<ScreenModeEnum>(mode));
}

EXPORTC bool isTurtleVisible(void) noexcept
{
    return Turtle::get().isTurtleVisible();
}

EXPORTC addr_t getScreenMode(addr_t eAddr) noexcept
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

EXPORTC addr_t getTurtleMode(addr_t eAddr) noexcept
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

EXPORTC addr_t getLabelSize(addr_t eAddr) noexcept
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

EXPORTC addr_t getLabelFontName(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    QString name = Config::get().mainInterface()->getLabelFontName();
    if (name.isNull())
        name = QStringLiteral("");
    auto *word = new Word(name);
    e->watch(word);
    return reinterpret_cast<addr_t>(word);
}

EXPORTC void setPenIsDown(bool isDown) noexcept
{
    Turtle::get().setPenIsDown(isDown);
}

EXPORTC void setPenMode(int32_t mode) noexcept
{
    Turtle::get().setPenMode(static_cast<PenModeEnum>(mode));
}

EXPORTC bool setPenColor(addr_t colorAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Kernel::get().colorFromDatumPtr(color, DatumPtr(d)))
        return false;
    Turtle::get().setPenColor(color);
    return true;
}

EXPORTC addr_t getAllColors(addr_t eAddr) noexcept
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

EXPORTC bool isColorIndexGood(double colorIndex, double lowerLimit) noexcept
{
    return (colorIndex == std::floor(colorIndex)) && (colorIndex >= lowerLimit) &&
           (colorIndex < Kernel::get().palette_.size());
}

EXPORTC bool setPalette(double colorIndex, addr_t colorAddr) noexcept
{
    auto index = static_cast<int>(colorIndex);
    auto *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Kernel::get().colorFromDatumPtr(color, DatumPtr(d)))
        return false;
    Kernel::get().palette_[index] = color;
    return true;
}

EXPORTC void setPenSize(double size) noexcept
{
    Turtle::get().setPenSize(size);
}

namespace
{

enum class PenAttrKind
{
    Style,
    Cap,
    Join
};

struct PenAttr
{
    PenAttrKind kind;
    int value;
};

const QHash<QString, PenAttr> &penWordDict()
{
    static const QHash<QString, PenAttr> dict = {
        {QStringLiteral("SOLIDLINE"), {PenAttrKind::Style, Qt::SolidLine}},
        {QStringLiteral("DASHLINE"), {PenAttrKind::Style, Qt::DashLine}},
        {QStringLiteral("DOTLINE"), {PenAttrKind::Style, Qt::DotLine}},
        {QStringLiteral("DASHDOTLINE"), {PenAttrKind::Style, Qt::DashDotLine}},
        {QStringLiteral("DASHDOTDOTLINE"), {PenAttrKind::Style, Qt::DashDotDotLine}},
        {QStringLiteral("FLATCAP"), {PenAttrKind::Cap, Qt::FlatCap}},
        {QStringLiteral("SQUARECAP"), {PenAttrKind::Cap, Qt::SquareCap}},
        {QStringLiteral("ROUNDCAP"), {PenAttrKind::Cap, Qt::RoundCap}},
        {QStringLiteral("MITERJOIN"), {PenAttrKind::Join, Qt::MiterJoin}},
        {QStringLiteral("BEVELJOIN"), {PenAttrKind::Join, Qt::BevelJoin}},
        {QStringLiteral("ROUNDJOIN"), {PenAttrKind::Join, Qt::RoundJoin}},
    };
    return dict;
}

struct ParsedPenPattern
{
    bool hasStyle = false;
    bool hasCap = false;
    bool hasJoin = false;
    bool hasDash = false;
    Qt::PenStyle style = Qt::SolidLine;
    Qt::PenCapStyle cap = Qt::SquareCap;
    Qt::PenJoinStyle join = Qt::BevelJoin;
    QList<qreal> dash;
};

bool parseDashPattern(List *list, QList<qreal> &out)
{
    const int n = list->count();
    if (n < 1)
        return false;

    QList<qreal> dashes;
    dashes.reserve(n);
    ListIterator it = list->newIterator();
    while (it.elementExists())
    {
        DatumPtr item = it.element();
        if (!item.isWord())
            return false;
        const double v = item.wordValue()->numberValue();
        if (!item.wordValue()->numberIsValid || !std::isfinite(v) || v <= 0)
            return false;
        dashes.append(v);
    }
    out = dashes;
    return true;
}

bool parsePenPatternList(Datum *d, ParsedPenPattern &out)
{
    if (!d->isList())
        return false;

    ParsedPenPattern parsed;
    ListIterator it = d->listValue()->newIterator();
    while (it.elementExists())
    {
        DatumPtr item = it.element();
        if (item.isWord())
        {
            const QString key = item.wordValue()->toString(Datum::ToStringFlags_Key);
            const auto found = penWordDict().constFind(key);
            if (found == penWordDict().cend())
                return false;
            switch (found->kind)
            {
            case PenAttrKind::Style:
                if (parsed.hasStyle || parsed.hasDash)
                    return false;
                parsed.hasStyle = true;
                parsed.style = static_cast<Qt::PenStyle>(found->value);
                break;
            case PenAttrKind::Cap:
                if (parsed.hasCap)
                    return false;
                parsed.hasCap = true;
                parsed.cap = static_cast<Qt::PenCapStyle>(found->value);
                break;
            case PenAttrKind::Join:
                if (parsed.hasJoin)
                    return false;
                parsed.hasJoin = true;
                parsed.join = static_cast<Qt::PenJoinStyle>(found->value);
                break;
            }
        }
        else if (item.isList())
        {
            if (parsed.hasDash || parsed.hasStyle)
                return false;
            if (!parseDashPattern(item.listValue(), parsed.dash))
                return false;
            parsed.hasDash = true;
            parsed.hasStyle = true;
            parsed.style = Qt::CustomDashLine;
        }
        else
        {
            return false;
        }
    }

    out = parsed;
    return true;
}

const char *styleWord(Qt::PenStyle style)
{
    switch (style)
    {
    case Qt::DashLine:
        return "dashline";
    case Qt::DotLine:
        return "dotline";
    case Qt::DashDotLine:
        return "dashdotline";
    case Qt::DashDotDotLine:
        return "dashdotdotline";
    case Qt::CustomDashLine:
        return nullptr;
    case Qt::SolidLine:
    default:
        return "solidline";
    }
}

const char *capWord(Qt::PenCapStyle cap)
{
    switch (cap)
    {
    case Qt::FlatCap:
        return "flatcap";
    case Qt::RoundCap:
        return "roundcap";
    case Qt::SquareCap:
    default:
        return "squarecap";
    }
}

const char *joinWord(Qt::PenJoinStyle join)
{
    switch (join)
    {
    case Qt::MiterJoin:
        return "miterjoin";
    case Qt::RoundJoin:
        return "roundjoin";
    case Qt::BevelJoin:
    default:
        return "beveljoin";
    }
}

} // namespace

EXPORTC bool isValidPenPattern(addr_t patternAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(patternAddr);
    ParsedPenPattern parsed;
    return parsePenPatternList(d, parsed);
}

EXPORTC void setPenPattern(addr_t patternAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(patternAddr);
    ParsedPenPattern parsed;
    if (!parsePenPatternList(d, parsed))
        return;

    Turtle &turtle = Turtle::get();
    Qt::PenStyle style = turtle.penStyle();
    Qt::PenCapStyle cap = turtle.penCapStyle();
    Qt::PenJoinStyle join = turtle.penJoinStyle();
    QList<qreal> dash = turtle.penDashPattern();

    if (parsed.hasStyle)
        style = parsed.style;
    if (parsed.hasCap)
        cap = parsed.cap;
    if (parsed.hasJoin)
        join = parsed.join;
    if (parsed.hasDash)
        dash = parsed.dash;
    else if (parsed.hasStyle)
        dash.clear();

    turtle.setPenPattern(style, cap, join, dash);
}

EXPORTC addr_t getPenPattern(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    const Turtle &turtle = Turtle::get();

    ListBuilder retvalBuilder;
    if (turtle.penStyle() == Qt::CustomDashLine)
    {
        ListBuilder dashBuilder;
        for (qreal dash : turtle.penDashPattern())
            dashBuilder.append(DatumPtr(static_cast<double>(dash)));
        retvalBuilder.append(dashBuilder.finishedList());
    }
    else
    {
        retvalBuilder.append(DatumPtr(styleWord(turtle.penStyle())));
    }
    retvalBuilder.append(DatumPtr(capWord(turtle.penCapStyle())));
    retvalBuilder.append(DatumPtr(joinWord(turtle.penJoinStyle())));

    DatumPtr retval = retvalBuilder.finishedList();
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC bool setBackground(addr_t colorAddr) noexcept
{
    auto *d = reinterpret_cast<Datum *>(colorAddr);
    QColor color;
    if (!Kernel::get().colorFromDatumPtr(color, DatumPtr(d)))
        return false;
    Config::get().mainInterface()->setCanvasBackgroundColor(color);
    return true;
}

EXPORTC bool isPenDown(void) noexcept
{
    return Turtle::get().isPenDown();
}

EXPORTC addr_t getPenMode(addr_t eAddr) noexcept
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

EXPORTC addr_t getPenColor(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    const QColor &color = Turtle::get().getPenColor();
    DatumPtr retval = listFromColor(color);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC addr_t getPaletteColor(addr_t eAddr, double colorIndex) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto index = static_cast<int>(colorIndex);
    const QColor &color = Kernel::get().palette_[index];
    DatumPtr retval = listFromColor(color);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC double getPenSize(void) noexcept
{
    return Turtle::get().getPenSize();
}

EXPORTC addr_t getBackground(addr_t eAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    const QColor &color = Config::get().mainInterface()->getCanvasBackgroundColor();
    DatumPtr retval = listFromColor(color);
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval.datumValue());
}

EXPORTC addr_t savePict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr) noexcept
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

EXPORTC addr_t saveSvgpict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr) noexcept
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

EXPORTC addr_t loadPict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr) noexcept
{
    auto *e = reinterpret_cast<Evaluator *>(eAddr);
    auto *dFilename = reinterpret_cast<Datum *>(filenameAddr);
    auto *retval = reinterpret_cast<Datum *>(nodeAddr);
    if (dFilename->isWord())
    {
        QString filename = reinterpret_cast<Word *>(filenameAddr)->toString();
        QString filepath = StreamManager::get().filepathForFilename(DatumPtr(filename));
        QImage image = QImage(filepath);
        if (image.isNull())
            retval = FCError::fileSystem();
        else
            Config::get().mainInterface()->setCanvasBackgroundImage(image);
    }
    else
    {
        Config::get().mainInterface()->setCanvasBackgroundImage(QImage());
    }
    e->watch(retval);
    return reinterpret_cast<addr_t>(retval);
}

EXPORTC addr_t getMousePos(addr_t eAddr) noexcept
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

EXPORTC addr_t getClickPos(addr_t eAddr) noexcept
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

EXPORTC bool isMouseButtonDown(void) noexcept
{
    return Config::get().mainInterface()->getIsMouseButtonDown();
}

EXPORTC double getMouseButton(void) noexcept
{
    return static_cast<double>(Config::get().mainInterface()->getAndResetButtonID());
}
