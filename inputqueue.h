#ifndef INPUTQUEUE_H
#define INPUTQUEUE_H

#include <QThread>
#include <QByteArray>
#include <QEventLoop>

class InputQueueThread : public QThread
{
    Q_OBJECT

    void run() override;

public:
    explicit InputQueueThread(QObject *parent = nullptr);

signals:
    void sendMessage(QByteArray msg);
};


class InputQueue : public QObject
{
    Q_OBJECT
    InputQueueThread thread;
    QByteArray message;
    QEventLoop eventLoop;

private slots:
    // Connected to sendMessage signal from thread.
    void receiveMessage(QByteArray aMessage);

public:
    explicit InputQueue(QObject *parent = nullptr);

    /// Start the input thread.
    void startQueue();

    /// Stop the input thread.
    void stopQueue();

    /// Check for a message.
    /// Does not block.
    bool isMessageAvailable();

    /// Get a message.
    /// Will block until message is available.
    QByteArray getMessage();
};



#endif // INPUTQUEUE_H
