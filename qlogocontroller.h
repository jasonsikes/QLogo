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

    double xbound;
    double ybound;

    QStringList allFontNames;
    QString textFontName;
    qreal textFontSize;

    QString labelFontName;
    qreal labelFontSize;

public:
    QLogoController(QObject *parent = 0);
    ~QLogoController();

    void initialize();

    void printToConsole(const QString &s);
    DatumP readRawlineWithPrompt(const QString &prompt);
    DatumP readchar();

    void setTurtlePos(const QMatrix4x4 &newTurtlePos);
    void drawLine(const QVector3D &start, const QVector3D &end, const QColor &startColor, const QColor &endColor);
    void drawPolygon(const QList<QVector3D> &points, const QList<QColor> &colors);
    void clearScreen();
    void drawLabel(const QString &, const QVector3D &, const QColor &);
    void setCanvasBackgroundColor(QColor);

    void setBounds(double x, double y);
    double boundX() { return xbound; }
    double boundY() { return ybound; }

    bool isPenSizeValid(double candidate) { return ((candidate >= minPensize) && (candidate <= maxPensize)); }

    void setTextFontSize(double aSize);
    double getTextFontSize();
    const QString getTextFontName();
    void setTextFontName(const QString aFontName);
    const QStringList getAllFontNames() { return allFontNames; }

    void setLabelFontSize(double aSize);
    double getLabelFontSize();
    const QString getLabelFontName();
    void setLabelFontName(const QString &aName);

    void setPensize(double);
};

#endif // QLOGOCONTROLLER_H
