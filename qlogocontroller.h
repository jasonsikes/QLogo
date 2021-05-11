#ifndef QLOGOCONTROLLER_H
#define QLOGOCONTROLLER_H

#include "controller.h"
#include "constants.h"
#include <QDataStream>
#include <QFile>

class QLogoController : public Controller
{
    message_t getMessage();
    void waitForMessage(message_t expectedType);

    // Return values from getMessage()
    QString rawLine;
    QChar rawChar;

public:
    QLogoController(QObject *parent = 0);
    ~QLogoController();

    void printToConsole(const QString &s);
    DatumP readRawlineWithPrompt(const QString &prompt);
    DatumP readchar();

    void setTurtlePos(const QMatrix4x4 &newTurtlePos);
    void drawLine(const QVector3D &start, const QVector3D &end, const QColor &color);
    void clearScreen();

};

#endif // QLOGOCONTROLLER_H
