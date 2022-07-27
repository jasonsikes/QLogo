#include "inputqueue.h"
#include <unistd.h>
#include <QDebug>
#include <fcntl.h>


/// The message sending event blocks the sending thread until the message has been received.
/// Since I can't find a way to non-block query the event queue, I'll block the sender.
/// (The pipe should be able to handle any backlog.)
static volatile bool isSendingSignal = false;

InputQueueThread::InputQueueThread(QObject *parent) : QThread(parent)
{

}


void InputQueueThread::run()
{
    qint64 datalen;
    qint64 dataread;
    QByteArray message;
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    forever {
        dataread = read(STDIN_FILENO, &datalen, sizeof(datalen));
        if (dataread <= 0) {
            // We didn't get data.
            if ((EAGAIN == errno) || (EWOULDBLOCK == errno)) {
                msleep(30);
                continue;
            }
            // Like tears in rain. Time to die.
            return;
        }
        Q_ASSERT(dataread == sizeof(qint64));
        message.resize(datalen);
        dataread = read(STDIN_FILENO, message.data(), datalen);
        Q_ASSERT(dataread == datalen);
        isSendingSignal = true;
        emit sendMessage(message);
        isSendingSignal = false;
    }
}


InputQueue::InputQueue(QObject *parent) : QObject(parent)
{

}

void InputQueue::startQueue()
{
    connect(&thread, SIGNAL(sendMessage(QByteArray)),
            this, SLOT(receiveMessage(QByteArray)), Qt::BlockingQueuedConnection);
    thread.start();
}

bool InputQueue::isMessageAvailable()
{
    return isSendingSignal;
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
    // Close stdin which will interrupt the thread.
    close(STDIN_FILENO);
    thread.wait();
}
