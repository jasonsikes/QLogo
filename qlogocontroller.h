#ifndef QLOGOCONTROLLER_H
#define QLOGOCONTROLLER_H

#include "controller.h"
#include "message.h"
#include <QDataStream>

class QLogoController : public Controller
{
    QDataStream guiInstream;
    QFile guiIn;
    QFile guiOut;
    message_t getMessage();
    void waitForMessage(message_t expectedType);

    // Return values from getMessage()
    QString rawLine;
public:
    QLogoController(QObject *parent = 0);
    ~QLogoController();

    void printToConsole(const QString &s);
    DatumP readRawlineWithPrompt(const QString &prompt);
};

#endif // QLOGOCONTROLLER_H
