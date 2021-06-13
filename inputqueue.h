#ifndef INPUTQUEUE_H
#define INPUTQUEUE_H

#include <QThread>
#include <QByteArrayList>
#include <QMutexLocker>
#include <QEventLoop>

class InputQueueThread : public QThread
{
    Q_OBJECT
    QByteArrayList list;
    QMutex mutex;

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
public:
    explicit InputQueue(QObject *parent = nullptr);
    void stopQueue();
    void startQueue();

    /// Get a message.
    /// Will wait until message is available.
    QByteArray getMessage();

public slots:
    void receiveMessage(QByteArray aMessage);
};



#endif // INPUTQUEUE_H
