
//===-- qlogo/mainwindow.cpp - MainWindow class implementation --*- C++ -*-===//
//
// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the MainWindow class, which is the
/// main window portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include "gui/mainwindow.h"
#include "gui/canvas.h"
#include "gui/editorwindow.h"
#include "sharedconstants.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFontInfo>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMessageBox>
#include <QScreen>
#include <QTextCursor>
#include <QTimer>
#include <array>
#include <limits>

#ifdef Q_OS_WIN
#include <windows.h>

// We use std::max later to generate a table of message handlers.
#undef max
#endif

QProcess *ProcessMessageWriter::process_ = nullptr;

qint64 ProcessMessageWriter::write(const QByteArray &buffer)
{
    return process_->write(buffer);
}

template <typename... Args>
void sendMessage(message_t header, const Args &...args)
{
    MessageTemplate<ProcessMessageWriter> msg(header);
    (void)(msg << ... << args);
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui_(std::make_unique<Ui::MainWindow>())
{
    ProcessMessageWriter::process_ = &logoProcess_;
    ui_->setupUi(this);

    windowMode_ = windowMode_t::windowMode_noWait;

    ui_->mainConsole->installEventFilter(this);
    ui_->mainCanvas->installEventFilter(this);
}

void MainWindow::show()
{
    setSplitterforMode(initScreenMode);

    QMainWindow::show();
    ui_->mainConsole->setFocus();

    startLogo();
}

MainWindow::~MainWindow() = default;

QString MainWindow::findQlogoExe()
{
#ifdef Q_OS_WIN
    QString filename("qlogo.exe");
#else
    QString filename("qlogo");
#endif

    // Build a list of candidate locations to try.
    QStringList candidates;

    // The qlogo directory relative to wherever the app binary is.
    candidates << QCoreApplication::applicationDirPath() + QDir::separator() + ".." + QDir::separator() + "qlogo" +
                      QDir::separator() + filename;
    // The same directory as the app binary.
    candidates << QCoreApplication::applicationDirPath() + QDir::separator() + filename;

    for (auto &c : candidates)
    {
        // qInfo() << "Checking: " << c;
        if (QFileInfo::exists(c))
            return c;
    }

    // TODO: How do we handle this gracefully?
    return {};
}

int MainWindow::startLogo()
{
    QString command = findQlogoExe();

    QStringList arguments;
    arguments << "--Psychi";

#ifdef Q_OS_WIN
    logoProcess_.setCreateProcessArgumentsModifier(
        [](QProcess::CreateProcessArguments *args) { args->flags |= CREATE_NO_WINDOW; });
#endif

    connect(&logoProcess_,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &MainWindow::processFinished);

    connect(&logoProcess_, &QProcess::readyReadStandardOutput, this, &MainWindow::readStandardOutput);

    connect(&logoProcess_, &QProcess::readyReadStandardError, this, &MainWindow::readStandardError);

    connect(ui_->mainConsole, &Console::sendRawlineSignal, this, &MainWindow::sendRawlineSlot);

    connect(ui_->mainConsole, &Console::sendCharSignal, this, &MainWindow::sendCharSlot);

    connect(ui_->splitter, &QSplitter::splitterMoved, this, &MainWindow::splitterHasMovedSlot);

    connect(ui_->mainCanvas, &Canvas::sendMouseclickedSignal, this, &MainWindow::mouseclickedSlot);

    connect(ui_->mainCanvas, &Canvas::sendMousemovedSignal, this, &MainWindow::mousemovedSlot);

    connect(ui_->mainCanvas, &Canvas::sendMouseReleasedSignal, this, &MainWindow::mousereleasedSlot);

    logoProcess_.start(command, arguments);
    return 0;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qint64 pid = logoProcess_.processId();
    // Tell the process to die, then ignore.
    // Because when the process dies another signal will be sent to close the application.
    if (pid > 0)
    {
        userRequestedClose_ = true;
        sendMessage(S_SYSTEM);

        logoProcess_.closeWriteChannel();

        event->ignore();
    }
    else
    {
        event->accept();
    }
}

namespace
{
bool isPhysicalControlHeld(const QKeyEvent *event)
{
    const Qt::KeyboardModifiers mods =
        event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
#ifdef Q_OS_MACOS
    // Qt maps ControlModifier to Command on macOS. The physical Control key is Meta.
    return mods == Qt::MetaModifier;
#else
    return mods == Qt::ControlModifier;
#endif
}

message_t interruptKindForKey(const QKeyEvent *keyEvent, bool copyHasSelection)
{
#ifdef Q_OS_MACOS
    Q_UNUSED(copyHasSelection);
#endif
    if (keyEvent->matches(QKeySequence::Quit))
        return S_SYSTEM;
#ifndef Q_OS_MACOS
    if (isPhysicalControlHeld(keyEvent) && (keyEvent->key() == Qt::Key_Q || keyEvent->key() == Qt::Key_Backslash))
        return S_SYSTEM;
#endif
    if (isPhysicalControlHeld(keyEvent) && keyEvent->key() == Qt::Key_Z)
        return S_PAUSE;
    if (isPhysicalControlHeld(keyEvent) && keyEvent->key() == Qt::Key_C)
    {
#ifndef Q_OS_MACOS
        if (copyHasSelection && keyEvent->matches(QKeySequence::Copy))
            return W_ZERO;
#endif
        return S_TOPLEVEL;
    }
    return W_ZERO;
}
} // namespace

bool MainWindow::maybeSendInterrupt(QKeyEvent *keyEvent)
{
    if (logoProcess_.processId() <= 0)
        return false;

    const bool copyHasSelection = ui_->mainConsole->textCursor().hasSelection();
    const message_t kind = interruptKindForKey(keyEvent, copyHasSelection);
    if (kind == W_ZERO)
        return false;

    sendMessage(kind);
    return true;
}

bool MainWindow::maybeSendKeyEvent(QKeyEvent *keyEvent)
{
    if (windowMode_ != windowMode_t::windowMode_noWait)
        return false;
    if (logoProcess_.processId() <= 0)
        return false;
    const QString text = keyEvent->text();
    if (text.isEmpty())
        return false;
    for (QChar c : text)
    {
        if (c.isNull())
            continue;
        sendMessage(C_CONSOLE_KEY_EVENT, c);
    }
    return true;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    if (event->type() != QEvent::ShortcutOverride && event->type() != QEvent::KeyPress)
        return QMainWindow::eventFilter(watched, event);

    auto *keyEvent = static_cast<QKeyEvent *>(event);
    const bool copyHasSelection = ui_->mainConsole->textCursor().hasSelection();
    if (interruptKindForKey(keyEvent, copyHasSelection) != W_ZERO)
    {
        if (event->type() == QEvent::ShortcutOverride)
        {
            event->accept();
            // Cmd-Q is an application shortcut; send here in case no KeyPress follows.
            if (keyEvent->matches(QKeySequence::Quit))
                maybeSendInterrupt(keyEvent);
            return true;
        }

        if (!keyEvent->matches(QKeySequence::Quit))
            maybeSendInterrupt(keyEvent);
        return true;
    }

    if (event->type() == QEvent::KeyPress && maybeSendKeyEvent(keyEvent))
        return true;

    return QMainWindow::eventFilter(watched, event);
}

namespace
{
/// True when Latin letters and the space share one advance.
///
/// setFixedPitch() can give i and W the same width on a proportional face
/// while the space stays narrow. That face is not monospace.
bool fontIsMonospace(const QFont &font)
{
    const QFontMetrics metrics(font);
    if (!metrics.inFont(u'i') || !metrics.inFont(u'W') || !metrics.inFont(u' '))
        return false;
    const int width = metrics.horizontalAdvance(u'i');
    return width > 0 && width == metrics.horizontalAdvance(u'W') && width == metrics.horizontalAdvance(u'M') &&
           width == metrics.horizontalAdvance(u' ');
}

QFont withMonospaceHint(QFont font)
{
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    return font;
}
} // namespace

QFont MainWindow::defaultFont()
{
    // Some platform themes name a proportional face as the fixed font.
    // Keep it only when the space advance matches the letters.
    QFont font = withMonospaceHint(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    if (!fontIsMonospace(font))
        font = withMonospaceHint(QFont(QStringLiteral("monospace")));

    if (!fontIsMonospace(font))
    {
        const QStringList families = QFontDatabase::families();
        for (const QString &family : families)
        {
            QFont candidate = withMonospaceHint(QFont(family));
            if (fontIsMonospace(candidate))
            {
                font = candidate;
                break;
            }
        }
    }

    QFontInfo info(font);
    if (!info.family().isEmpty())
    {
        QFont pinned = font;
        pinned.setFamilies(QStringList(info.family()));
        if (fontIsMonospace(pinned))
            font = pinned;
    }

    info = QFontInfo(font);
    qreal pointSize = info.pointSizeF();
    if (pointSize <= 0)
    {
        const int pixels = info.pixelSize();
        qreal dpi = 0;
        if (const QScreen *screen = QGuiApplication::primaryScreen())
            dpi = screen->logicalDotsPerInchY();
        if (pixels > 0 && dpi > 0)
            pointSize = pixels * 72.0 / dpi;
    }
    if (pointSize > 0)
        font.setPointSizeF(pointSize);
    return font;
}

void MainWindow::initialize()
{
    const QFont font = defaultFont();
    const QFontInfo fontInfo(font);
    setSplitterforMode(initScreenMode);

    sendMessage(W_INITIALIZE, QFontDatabase::families(), fontInfo.family(), static_cast<double>(font.pointSizeF()));
}

void MainWindow::fileDialogModal()
{
    QString startingDir = QDir::homePath();
    QString filePath = QFileDialog::getOpenFileName(this, tr("Choose file"), startingDir);
    sendMessage(W_FILE_DIALOG_GET_PATH, filePath);
}

void MainWindow::startWaitTimer(qint32 msecs)
{
    if (waitTimer_ == nullptr)
    {
        waitTimer_ = new QTimer(this);
        waitTimer_->setSingleShot(true);
        connect(waitTimer_, &QTimer::timeout, this, [this]() { sendMessage(W_TIMER_END); });
    }
    // Restarting cancels a timer left running after WAIT returned early for
    // BUTTONACT/KEYACT.
    waitTimer_->start(msecs);
}

void MainWindow::openEditorWindow(const QString &title, const QString &startingText)
{
    if (editWindow_ == nullptr)
    {
        editWindow_ = new EditorWindow(this);

        connect(editWindow_, &EditorWindow::editingHasEndedSignal, this, &MainWindow::editingHasEndedSlot);
    }

    editWindow_->setWindowTitle(title.isEmpty() ? tr("Editor") : title);
    editWindow_->setTextFormat(ui_->mainConsole->getFont());
    editWindow_->setContents(startingText);
    editWindow_->show();
    editWindow_->activateWindow();
}

void MainWindow::editingHasEndedSlot(bool accepted, const QString &text)
{
    sendMessage(C_CONSOLE_END_EDIT_TEXT, accepted, text);
}

void MainWindow::introduceCanvas()
{
    if (hasShownCanvas_)
        return;
    hasShownCanvas_ = true;
    setSplitterforMode(splitScreenMode);
}

void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus != QProcess::NormalExit && !userRequestedClose_)
    {
        QMessageBox msgBox;
        msgBox.setText(tr("qlogo has reached an unstable state and will be terminated."));
        msgBox.exec();
    }
    QApplication::exit(0);
}

void MainWindow::readStandardOutput()
{
    qint64 datalen;

    for (;;)
    {
        // If a message is complete then it was already sent,
        // and we can start a new one.
        if (readBuffer_.size() == readBufferLen_)
        {
            qint64 readResult = logoProcess_.read(reinterpret_cast<char *>(&datalen), sizeof(qint64));
            if (readResult != sizeof(qint64))
                return;
            readBufferLen_ = datalen;
            readBuffer_ = logoProcess_.read(readBufferLen_);
        }
        else
        {
            // We are appending the incoming message to the buffer.
            qint64 remain = readBufferLen_ - readBuffer_.size();
            QByteArray post = logoProcess_.read(remain);
            Q_ASSERT(!post.isEmpty());
            readBuffer_.append(post);
        }

        // If we don't have all of the message yet, keep what we have,
        // and wait for the next signal to come back later.
        if (readBuffer_.size() < readBufferLen_)
            return;

        // We do have a complete message.
        processReadBuffer();
    }
}

/// @brief Inbound pipe handlers, indexed by message_t.
///
/// Slots with no handler are messages Psychi sends, or codes it does not know.
struct MessageDispatch
{
    using Handler = void (*)(MainWindow &window, QDataStream &stream);

    static constexpr std::size_t count = static_cast<std::size_t>(std::numeric_limits<message_t>::max()) + 1;

    static void zero(MainWindow &, QDataStream &)
    {
        // This only exists to help catch errors.
        qInfo() << "Zero!";
    }

    static void initialize(MainWindow &window, QDataStream &)
    {
        window.initialize();
    }

    static void closePipe(MainWindow &window, QDataStream &)
    {
        window.logoProcess_.closeWriteChannel();
    }

    static void setScreenMode(MainWindow &window, QDataStream &stream)
    {
        ScreenModeEnum newMode;
        stream >> newMode;
        window.setSplitterforMode(newMode);
    }

    static void fileDialog(MainWindow &window, QDataStream &)
    {
        window.fileDialogModal();
    }

    static void timerStart(MainWindow &window, QDataStream &stream)
    {
        qint32 msecs;
        stream >> msecs;
        window.startWaitTimer(msecs);
    }

    static void printString(MainWindow &window, QDataStream &stream)
    {
        QString text;
        stream >> text;
        window.ui_->mainConsole->printString(text);
    }

    static void setConsoleFontName(MainWindow &window, QDataStream &stream)
    {
        QString name;
        stream >> name;
        window.ui_->mainConsole->setTextFontName(name);
    }

    static void setConsoleFontSize(MainWindow &window, QDataStream &stream)
    {
        qreal aSize;
        stream >> aSize;
        window.ui_->mainConsole->setTextFontSize(aSize);
    }

    static void requestLine(MainWindow &window, QDataStream &stream)
    {
        QString prompt;
        stream >> prompt;
        window.beginReadRawlineWithPrompt(prompt);
    }

    static void requestChar(MainWindow &window, QDataStream &)
    {
        window.beginReadChar();
    }

    static void beginEditText(MainWindow &window, QDataStream &stream)
    {
        QString title;
        QString startingText;
        stream >> title >> startingText;
        window.openEditorWindow(title, startingText);
    }

    static void textCursorPos(MainWindow &window, QDataStream &)
    {
        window.sendConsoleCursorPosition();
    }

    static void setTextCursorPos(MainWindow &window, QDataStream &stream)
    {
        int row, col;
        stream >> row >> col;
        window.ui_->mainConsole->setTextCursorPosition(row, col);
    }

    static void setCursorMode(MainWindow &window, QDataStream &stream)
    {
        bool mode;
        stream >> mode;
        window.ui_->mainConsole->setOverwriteMode(mode);
    }

    static void setTextColor(MainWindow &window, QDataStream &stream)
    {
        QColor foreground;
        QColor background;
        stream >> foreground >> background;
        window.ui_->mainConsole->setTextFontColor(foreground, background);
    }

    static void clearScreenText(MainWindow &window, QDataStream &)
    {
        window.ui_->mainConsole->setPlainText("");
    }

    static void updateTurtlePos(MainWindow &window, QDataStream &stream)
    {
        QTransform matrix;
        stream >> matrix;
        window.ui_->mainCanvas->setTurtleMatrix(matrix);
        window.introduceCanvas();
    }

    static void setTurtleIsVisible(MainWindow &window, QDataStream &stream)
    {
        bool isVisible;
        stream >> isVisible;
        window.ui_->mainCanvas->setTurtleIsVisible(isVisible);
        window.introduceCanvas();
    }

    static void emitVertex(MainWindow &window, QDataStream &)
    {
        window.ui_->mainCanvas->emitVertex();
        window.introduceCanvas();
    }

    static void setForegroundColor(MainWindow &window, QDataStream &stream)
    {
        QColor color;
        stream >> color;
        window.ui_->mainCanvas->setForegroundColor(color);
        window.introduceCanvas();
    }

    static void setBackgroundColor(MainWindow &window, QDataStream &stream)
    {
        QColor color;
        stream >> color;
        window.ui_->mainCanvas->setBackgroundColor(color);
        window.introduceCanvas();
    }

    static void setBackgroundImage(MainWindow &window, QDataStream &stream)
    {
        QImage image;
        stream >> image;
        window.ui_->mainCanvas->setBackgroundImage(image);
        window.introduceCanvas();
    }

    static void beginPolygon(MainWindow &window, QDataStream &stream)
    {
        QColor color;
        stream >> color;
        window.ui_->mainCanvas->beginPolygon(color);
    }

    static void endPolygon(MainWindow &window, QDataStream &)
    {
        window.ui_->mainCanvas->endPolygon();
    }

    static void clearScreen(MainWindow &window, QDataStream &)
    {
        window.ui_->mainCanvas->clearScreen();
        window.introduceCanvas();
    }

    static void setBounds(MainWindow &window, QDataStream &stream)
    {
        qreal x, y;
        stream >> x >> y;
        window.ui_->mainCanvas->setBounds(x, y);
    }

    static void setIsBounded(MainWindow &window, QDataStream &stream)
    {
        bool isBounded;
        stream >> isBounded;
        window.ui_->mainCanvas->setIsBounded(isBounded);
    }

    static void setLabelFontName(MainWindow &window, QDataStream &stream)
    {
        QString name;
        stream >> name;
        window.ui_->mainCanvas->setLabelFontName(name);
    }

    static void setLabelFontSize(MainWindow &window, QDataStream &stream)
    {
        qreal aSize;
        stream >> aSize;
        window.ui_->mainCanvas->setLabelFontSize(aSize);
    }

    static void drawLabel(MainWindow &window, QDataStream &stream)
    {
        QString aString;
        stream >> aString;
        window.ui_->mainCanvas->addLabel(aString);
        window.introduceCanvas();
    }

    static void drawArc(MainWindow &window, QDataStream &stream)
    {
        qreal angle;
        qreal radius;
        stream >> angle >> radius;
        window.ui_->mainCanvas->addArc(angle, radius);
        window.introduceCanvas();
    }

    static void setPensize(MainWindow &window, QDataStream &stream)
    {
        qreal newSize;
        stream >> newSize;
        window.ui_->mainCanvas->setPensize(newSize);
    }

    static void setPenmode(MainWindow &window, QDataStream &stream)
    {
        PenModeEnum newMode;
        stream >> newMode;
        window.ui_->mainCanvas->setPenmode(newMode);
    }

    static void setPenUpDown(MainWindow &window, QDataStream &stream)
    {
        bool penIsDown;
        stream >> penIsDown;
        window.ui_->mainCanvas->setPenIsDown(penIsDown);
    }

    static void setPenPattern(MainWindow &window, QDataStream &stream)
    {
        Qt::PenStyle style;
        Qt::PenCapStyle cap;
        Qt::PenJoinStyle join;
        QList<qreal> dashPattern;
        stream >> style >> cap >> join >> dashPattern;
        window.ui_->mainCanvas->setPenPattern(style, cap, join, dashPattern);
    }

    static void setScrunch(MainWindow &window, QDataStream &stream)
    {
        qreal x, y;
        stream >> x >> y;
        window.ui_->mainCanvas->setScrunch(x, y);
        window.introduceCanvas();
    }

    static void getImage(MainWindow &window, QDataStream &)
    {
        window.sendCanvasImage();
    }

    static void getSvg(MainWindow &window, QDataStream &)
    {
        window.sendCanvasSvg();
    }

    static const std::array<Handler, count> &handlers()
    {
        static const std::array<Handler, count> table = [] {
            std::array<Handler, count> handlers{};
            handlers[W_ZERO] = zero;
            handlers[W_INITIALIZE] = initialize;
            handlers[W_CLOSE_PIPE] = closePipe;
            handlers[W_SET_SCREENMODE] = setScreenMode;
            handlers[W_FILE_DIALOG_GET_PATH] = fileDialog;
            handlers[W_TIMER_START] = timerStart;
            handlers[C_CONSOLE_PRINT_STRING] = printString;
            handlers[C_CONSOLE_SET_FONT_NAME] = setConsoleFontName;
            handlers[C_CONSOLE_SET_FONT_SIZE] = setConsoleFontSize;
            handlers[C_CONSOLE_REQUEST_LINE] = requestLine;
            handlers[C_CONSOLE_REQUEST_CHAR] = requestChar;
            handlers[C_CONSOLE_BEGIN_EDIT_TEXT] = beginEditText;
            handlers[C_CONSOLE_TEXT_CURSOR_POS] = textCursorPos;
            handlers[C_CONSOLE_SET_TEXT_CURSOR_POS] = setTextCursorPos;
            handlers[C_CONSOLE_SET_CURSOR_MODE] = setCursorMode;
            handlers[C_CONSOLE_SET_TEXT_COLOR] = setTextColor;
            handlers[C_CONSOLE_CLEAR_SCREEN_TEXT] = clearScreenText;
            handlers[C_CANVAS_UPDATE_TURTLE_POS] = updateTurtlePos;
            handlers[C_CANVAS_SET_TURTLE_IS_VISIBLE] = setTurtleIsVisible;
            handlers[C_CANVAS_EMIT_VERTEX] = emitVertex;
            handlers[C_CANVAS_SET_FOREGROUND_COLOR] = setForegroundColor;
            handlers[C_CANVAS_SET_BACKGROUND_COLOR] = setBackgroundColor;
            handlers[C_CANVAS_SET_BACKGROUND_IMAGE] = setBackgroundImage;
            handlers[C_CANVAS_BEGIN_POLYGON] = beginPolygon;
            handlers[C_CANVAS_END_POLYGON] = endPolygon;
            handlers[C_CANVAS_CLEAR_SCREEN] = clearScreen;
            handlers[C_CANVAS_SETBOUNDS] = setBounds;
            handlers[C_CANVAS_SET_IS_BOUNDED] = setIsBounded;
            handlers[C_CANVAS_SET_FONT_NAME] = setLabelFontName;
            handlers[C_CANVAS_SET_FONT_SIZE] = setLabelFontSize;
            handlers[C_CANVAS_DRAW_LABEL] = drawLabel;
            handlers[C_CANVAS_DRAW_ARC] = drawArc;
            handlers[C_CANVAS_SET_PENSIZE] = setPensize;
            handlers[C_CANVAS_SET_PENMODE] = setPenmode;
            handlers[C_CANVAS_SET_PENUPDOWN] = setPenUpDown;
            handlers[C_CANVAS_SET_PENPATTERN] = setPenPattern;
            handlers[C_CANVAS_SET_SCRUNCH] = setScrunch;
            handlers[C_CANVAS_GET_IMAGE] = getImage;
            handlers[C_CANVAS_GET_SVG] = getSvg;
            return handlers;
        }();
        return table;
    }
};

void MainWindow::processReadBuffer()
{
    QDataStream dataStream(readBuffer_);
    message_t header;
    dataStream >> header;

    const MessageDispatch::Handler handler = MessageDispatch::handlers()[header];
    if (handler == nullptr)
    {
        qInfo() << "was not expecting" << header;
        return;
    }
    handler(*this, dataStream);
}

void MainWindow::setSplitterforMode(ScreenModeEnum mode)
{
    float canvasSize = 0;
    switch (mode)
    {
    case initScreenMode:
        canvasSize = Config::get().initScreenSize_;
        break;
    case textScreenMode:
        canvasSize = Config::get().textScreenSize_;
        break;
    case fullScreenMode:
        canvasSize = Config::get().fullScreenSize_;
        break;
    case splitScreenMode:
        canvasSize = Config::get().splitScreenSize_;
        break;
    }
    QList<int> sizes = ui_->splitter->sizes();
    float splitterSize = sizes[0] + sizes[1];
    if (splitterSize <= 0)
        splitterSize = 1000;
    canvasSize = canvasSize * splitterSize;
    const float consoleSize = splitterSize - canvasSize;
    ui_->splitter->setSizes({qRound(canvasSize), qRound(consoleSize)});
}

void MainWindow::readStandardError()
{
    QByteArray ary = logoProcess_.readAllStandardError();
    qInfo() << "stderr: " << QString(ary);
}

void MainWindow::beginReadRawlineWithPrompt(const QString &prompt)
{
    windowMode_ = windowMode_t::windowMode_waitForRawline;
    ui_->mainConsole->requestRawlineWithPrompt(prompt);
}

void MainWindow::beginReadChar()
{
    windowMode_ = windowMode_t::windowMode_waitForChar;
    ui_->mainConsole->requestChar();
}

void MainWindow::mouseclickedSlot(QPointF position, int buttonID)
{
    sendMessage(C_CANVAS_MOUSE_BUTTON_DOWN, position, buttonID);
}

void MainWindow::mousemovedSlot(QPointF position)
{
    sendMessage(C_CANVAS_MOUSE_MOVED, position);
}

void MainWindow::mousereleasedSlot()
{
    sendMessage(C_CANVAS_MOUSE_BUTTON_UP);
}

void MainWindow::sendCharSlot(QChar c)
{
    windowMode_ = windowMode_t::windowMode_noWait;
    sendMessage(C_CONSOLE_CHAR_READ, c);
}

void MainWindow::sendRawlineSlot(const QString &line)
{
    windowMode_ = windowMode_t::windowMode_noWait;
    sendMessage(C_CONSOLE_RAWLINE_READ, line);
}

void MainWindow::sendConsoleCursorPosition()
{
    const auto [row, col] = ui_->mainConsole->getCursorPos();
    sendMessage(C_CONSOLE_TEXT_CURSOR_POS, row, col);
}

void MainWindow::sendCanvasImage()
{
    QImage image(ui_->mainCanvas->getImage());
    sendMessage(C_CANVAS_GET_IMAGE, image);
}

void MainWindow::sendCanvasSvg()
{
    QByteArray svg = ui_->mainCanvas->getSvg();
    sendMessage(C_CANVAS_GET_SVG, svg);
}

void MainWindow::splitterHasMovedSlot(int, int)
{
    hasShownCanvas_ = true;
}
