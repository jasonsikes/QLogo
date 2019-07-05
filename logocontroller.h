#ifndef LOGOCONTROLLER_H
#define LOGOCONTROLLER_H

#include "controller.h"

class LogoController : public Controller
{
protected:
    QTextStream *inStream;
    QTextStream *outStream;


public:
    LogoController(QObject *parent = NULL);
    ~LogoController();

    DatumP readRawlineWithPrompt(const QString &prompt);
    DatumP readchar();
    bool atEnd();
    void printToConsole(const QString &s);
    bool keyQueueHasChars();
    void mwait(unsigned long);
};

#endif // LOGOCONTROLLER_H
