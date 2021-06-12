#ifndef INPUTQUEUE_H
#define INPUTQUEUE_H

#include <QThread>
#include <QByteArrayList>
#include <QMutexLocker>

class InputQueueThread : public QThread
{
    QByteArrayList list;
    QMutex mutex;

    void run() override;
public:
    explicit InputQueueThread(QObject *parent = nullptr);

    /// Get a message.
    /// Will wait until message is available.
    QByteArray getMessage();


};

class InputQueue : public QObject
{
    InputQueueThread thread;
public:
    explicit InputQueue(QObject *parent = nullptr);
    void stopQueue();
    void startQueue();
    QByteArray getMessage();
};



#endif // INPUTQUEUE_H
