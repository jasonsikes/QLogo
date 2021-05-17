#ifndef QLOGOCONTROLLER_H
#define QLOGOCONTROLLER_H

#include "controller.h"
#include "constants.h"
#include <QDataStream>
#include <QFile>
#include <QFont>

class QLogoController : public Controller
{
    message_t getMessage();
    void waitForMessage(message_t expectedType);

    // Return values from getMessage()
    QString rawLine;
    QChar rawChar;

    double minPensize;
    double maxPensize;
    double penSize;

    QStringList allFontNames;
    QFont textFont;
    QFont labelFont;

public:
    QLogoController(QObject *parent = 0);
    ~QLogoController();

    void initialize();

    void printToConsole(const QString &s);
    DatumP readRawlineWithPrompt(const QString &prompt);
    DatumP readchar();

    void setTurtlePos(const QMatrix4x4 &newTurtlePos);
    void drawLine(const QVector3D &start, const QVector3D &end, const QColor &color);
    void drawPolygon(const QList<QVector3D> &points, const QList<QColor> &colors);
    void clearScreen();
    void drawLabel(const QString &, const QVector3D &, const QColor &,
                           const QFont &);
    void setCanvasBackgroundColor(QColor);
    bool isPenSizeValid(double candidate) { return ((candidate >= minPensize) && (candidate <= maxPensize)); }

    void setTextSize(double aSize) { textFont.setPointSizeF(aSize); } // Move to cpp
    double getTextSize() { return textFont.pointSizeF(); }
    QString getFontName() { return textFont.family(); }
    void setFontName(const QString aFamily) { textFont.setFamily(aFamily); } // Move to cpp
    const QStringList getAllFontNames() { return allFontNames; }

    void setLabelSize(double aSize) { labelFont.setPointSizeF(aSize); } // Move to cpp
    double getLabelSize() { return labelFont.pointSizeF(); }
    QFont getLabelFont() { return labelFont; }
    void setLabelFontName(const QString &aName) { labelFont.setFamily(aName); } // Move to cpp

    void setPensize(double);
};

#endif // QLOGOCONTROLLER_H
