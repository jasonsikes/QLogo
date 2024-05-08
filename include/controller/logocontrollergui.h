#ifndef LOGOCONTROLLERGUI_H
#define LOGOCONTROLLERGUI_H

#include "controller/logocontroller.h"
#include "sharedconstants.h"
#include <QDataStream>
#include <QFile>
#include <QFont>
#include "controller/inputqueue.h"
#include "datum/datum.h"

class LogoControllerGUI : public LogoController
{
    InputQueue messageQueue;
    message_t getMessage();
    void waitForMessage(message_t expectedType);

    // Return values from getMessage()
    QString rawLine;
    QChar rawChar;

    int cursorRow;
    int cursorCol;

    /// cursorOverwriteMode:
    /// true:  cursor overwrites previously-written text
    /// false: cursor inserts text (default)
    bool cursoreModeIsOverwrite = false;

    // Text returned from editor winow
    QString editorText;

    double minPensize;
    double maxPensize;
    double penSize;

    double xbound;
    double ybound;
    bool canvasIsBounded = true;

    QColor currentBackgroundColor;
    QImage canvasImage;

    QStringList allFontNames;
    QString textFontName;
    qreal textFontSize;

    ScreenModeEnum screenMode;

    QString labelFontName;
    qreal labelFontSize;

    void processInputMessageQueue();

public:
    LogoControllerGUI(QObject *parent = 0);
    ~LogoControllerGUI();
    void systemStop();

    void initialize();

    void printToConsole(const QString &s);
    QString inputRawlineWithPrompt(const QString prompt);
    DatumPtr readchar();
    const QString editText(const QString startText);

    void setTurtlePos(const QMatrix4x4 &newTurtlePos);
    void setTurtleIsVisible(bool isVisible);
    void setPenmode(PenModeEnum aMode);
    void drawLine(const QVector3D &start, const QVector3D &end, const QColor &startColor, const QColor &endColor);
    void drawPolygon(const QList<QVector3D> &points, const QList<QColor> &colors);
    void clearScreen();
    void drawLabel(const QString &, const QVector3D &, const QColor &);
    void setCanvasBackgroundColor(QColor);
    bool getIsMouseButtonDown();
    int getAndResetButtonID();
    QVector2D lastMouseclickPosition();
    QVector2D mousePosition();
    void setScreenMode(ScreenModeEnum newMode);
    ScreenModeEnum getScreenMode();

    void setBounds(double x, double y);
    double boundX() { return xbound; }
    double boundY() { return ybound; }
    QColor getCanvasBackgroundColor(void);
    void setIsCanvasBounded(bool aIsBounded);
    bool isCanvasBounded();

    bool isPenSizeValid(double candidate) { return ((candidate >= minPensize) && (candidate <= maxPensize)); }
    QImage getCanvasImage();
    void setTextFontSize(double aSize);
    double getTextFontSize();
    const QString getTextFontName();
    void setTextFontName(const QString aFontName);
    const QStringList getAllFontNames() { return allFontNames; }
    QString addStandoutToString(const QString src);
    void getTextCursorPos(int &row, int &col);
    void setTextCursorPos(int row, int col);
    void setTextColor(const QColor foregroundColor, const QColor backgroundColor);
    void setCursorOverwriteMode(bool isOverwriteMode);
    bool cursorOverwriteMode();
    void setLabelFontSize(double aSize);
    double getLabelFontSize();
    const QString getLabelFontName();
    void setLabelFontName(const QString &aName);

    void setPensize(double);
    void mwait(unsigned long msecs);
    void clearScreenText();
};

#endif // LOGOCONTROLLERGUI_H
