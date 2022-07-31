#include "inputqueue.h"
#include <unistd.h>
#include <QDebug>

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
        emit sendMessage(message);
    }
}


InputQueue::InputQueue(QObject *parent) : QObject(parent)
{

}

void InputQueue::startQueue()
{
    // The QueuedConnection allows us to queue the messages
    connect(&thread, SIGNAL(sendMessage(QByteArray)),
            this, SLOT(receiveMessage(QByteArray)), Qt::QueuedConnection);
    thread.start();
}

// TODO: The names getMessage() and receiveMessage() are ambiguous.
QByteArray InputQueue::getMessage()
{
    // Wait for a message signal from thread.
    eventLoop.exec();
    return message;
}

void InputQueue::receiveMessage(QByteArray aMessage)
{
    message = aMessage;
    eventLoop.exit(0);
}

void InputQueue::stopQueue()
{
    // The QLogo GUI closes the pipe so there is nothing to do except wait.
    thread.wait();
}
