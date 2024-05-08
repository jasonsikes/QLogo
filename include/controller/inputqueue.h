#ifndef INPUTQUEUE_H
#define INPUTQUEUE_H

#include <QThread>
#include <QByteArray>
#include <QEventLoop>

class InputQueueThread : public QThread
{
    Q_OBJECT

    void run() Q_DECL_OVERRIDE;

public:
    explicit InputQueueThread(QObject *parent = nullptr);

signals:
    void sendMessage();
};


class InputQueue : public QObject
{
    Q_OBJECT
    InputQueueThread thread;
    QByteArray message;
    QEventLoop eventLoop;

private slots:
    // Connected to sendMessage signal from thread.
    void receiveMessage();

public:
    explicit InputQueue(QObject *parent = nullptr);

    /// Start the input thread.
    void startQueue();

    /// Stop the input thread.
    void stopQueue();

    /// Get a message.
    /// Will wait until message is available.
    QByteArray getMessage();

    /// Ask if there is a message in the queue
    /// Returns immediately
    bool isMessageAvailable();
};



#endif // INPUTQUEUE_H
