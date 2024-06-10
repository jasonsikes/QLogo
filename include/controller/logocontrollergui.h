#ifndef LOGOCONTROLLERGUI_H
#define LOGOCONTROLLERGUI_H

#include "controller/logocontroller.h"
#include "sharedconstants.h"
#include <QDataStream>
#include <QFile>
#include <QFont>
#include "controller/inputqueue.h"
#include "datum.h"

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

    // cursorOverwriteMode:
    // true:  cursor overwrites previously-written text
    // false: cursor inserts text (default)
    bool cursoreModeIsOverwrite = false;

    // Text returned from editor winow
    QString editorText;

    double penSize;

    double xbound = initialBoundX;
    double ybound = initialBoundY;
    bool canvasIsBounded = true;

    QVector2D mousePos = QVector2D(0,0);
    QVector2D clickPos = QVector2D(0,0);
    int lastButtonpressID = 0;
    bool isMouseButtonDown = false;

    QColor currentBackgroundColor = initialCanvasBackgroundColor;
    QColor currentForegroundColor = initialCanvasForegroundColor;
    QImage canvasImage;
    QByteArray canvasSvg;

    QStringList allFontNames;
    QString textFontName;
    double textFontSize;

    ScreenModeEnum screenMode;

    QString labelFontName;
    double labelFontSize;

    void processInputMessageQueue();

public:
    LogoControllerGUI(QObject *parent = 0);
    ~LogoControllerGUI();
    void systemStop();

    void initialize();

    void printToConsole(QString s);
    QString inputRawlineWithPrompt(QString prompt);
    DatumPtr readchar();
    QString editText(QString startText);

    void setTurtlePos(const QTransform &newTurtlePos);
    void setTurtleIsVisible(bool isVisible);
    void setPenmode(PenModeEnum aMode);
    void emitVertex();
    void setPenIsDown(bool penIsDown);
    void setCanvasForegroundColor(const QColor &color);
    void setCanvasBackgroundColor(const QColor &color);
    void setCanvasBackgroundImage(QImage image);
    void beginPolygon(const QColor &color);
    void endPolygon();
    void clearCanvas();
    void drawLabel(QString);
    void drawArc(double angle, double radius);
    bool getIsMouseButtonDown();
    int getAndResetButtonID();
    QVector2D lastMouseclickPosition();
    QVector2D mousePosition();
    void setScreenMode(ScreenModeEnum newMode);
    ScreenModeEnum getScreenMode();

    void setBounds(double x, double y);
    double boundX() { return xbound; }
    double boundY() { return ybound; }
    const QColor getCanvasBackgroundColor(void);
    void setIsCanvasBounded(bool aIsBounded);
    bool isCanvasBounded();

    bool isPenSizeValid(double candidate) { return candidate >= 0; }
    QImage getCanvasImage();
    QByteArray getSvgImage();
    void setTextFontSize(double aSize);
    double getTextFontSize();
    QString getTextFontName();
    void setTextFontName(QString aFontName);
    QStringList getAllFontNames() { return allFontNames; }
    QString addStandoutToString(QString src);
    void getTextCursorPos(int &row, int &col);
    void setTextCursorPos(int row, int col);
    void setTextColor(const QColor &foregroundColor, const QColor &backgroundColor);
    void setCursorOverwriteMode(bool isOverwriteMode);
    bool cursorOverwriteMode();
    void setLabelFontSize(double aSize);
    double getLabelFontSize();
    QString getLabelFontName();
    void setLabelFontName(QString aName);

    void setPensize(double);
    void mwait(unsigned long msecs);
    void clearScreenText();
};

#endif // LOGOCONTROLLERGUI_H
