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
    QByteArray buffer;
    forever {
        do {
            dataread = read(STDIN_FILENO, &datalen, sizeof(qint64));
            if (dataread == 0) {
                qDebug() <<"No data read";
                QThread::msleep(100);
            }
        } while(dataread == 0);
        Q_ASSERT(dataread == sizeof(qint64));
        buffer.resize(datalen);
        dataread = read(STDIN_FILENO, buffer.data(), datalen);
        Q_ASSERT(dataread == datalen);
        emit sendMessage(buffer);
    }
}


InputQueue::InputQueue(QObject *parent) : QObject(parent)
{

}

void InputQueue::startQueue()
{
    connect(&thread, SIGNAL(sendMessage(QByteArray)), this, SLOT(receiveMessage(QByteArray)), Qt::QueuedConnection);
    thread.start();
}

QByteArray InputQueue::getMessage()
{
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
    // TODO: don't crash
}
