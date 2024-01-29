#include "inputqueue.h"
#include <unistd.h>
#include <QDebug>
#include <QQueue>
#include <QMutex>

QMutex queueLock;
QQueue<QByteArray> messageQueue;

InputQueueThread::InputQueueThread(QObject *parent) : QThread(parent)
{

}


void InputQueueThread::run()
{
    qint64 datalen;
    qint64 dataread;
    QByteArray message;
    forever {
        dataread = read(STDIN_FILENO, &datalen, sizeof(qint64));
        if (dataread <= 0) {
            // I guess we're done.
            return;
        }
        Q_ASSERT(dataread == sizeof(qint64));
        message.resize(datalen);
        dataread = read(STDIN_FILENO, message.data(), datalen);
        if (dataread <= 0) {
            // Like tears in rain, time to die.
            return;
        }
        Q_ASSERT(dataread == datalen);
        queueLock.lock();
        messageQueue.enqueue(message);
        queueLock.unlock();
        emit sendMessage();
    }
}


InputQueue::InputQueue(QObject *parent) : QObject(parent)
{

}

void InputQueue::startQueue()
{
    connect(&thread, SIGNAL(sendMessage()),
            this, SLOT(receiveMessage()), Qt::QueuedConnection);
    thread.start();
}

// TODO: The names getMessage() and receiveMessage() are ambiguous.
QByteArray InputQueue::getMessage()
{
    QByteArray message;
    // If there is a message already in the queue, return that.
    queueLock.lock();
    bool isMessageQueued = ! messageQueue.isEmpty();
    if (isMessageQueued)
        message = messageQueue.dequeue();
    queueLock.unlock();
    if (isMessageQueued) return message;

    // Wait for a message.
    forever {
        eventLoop.exec();

        queueLock.lock();
        isMessageQueued = ! messageQueue.isEmpty();
        if (isMessageQueued)
            message = messageQueue.dequeue();
        queueLock.unlock();
        if (isMessageQueued) return message;
    }
}

bool InputQueue::isMessageAvailable()
{
    queueLock.lock();
    bool retval = ! messageQueue.isEmpty();
    queueLock.unlock();
    return retval;
}

void InputQueue::receiveMessage()
{
    eventLoop.exit(0);
}

void InputQueue::stopQueue()
{
    // The QLogo GUI closes the pipe so there is nothing to do except wait.
    thread.wait();
}
