#include "inputqueue.h"
#include <unistd.h>
#include <QDebug>
//#include "constants.h"

InputQueueThread::InputQueueThread(QObject *parent) : QThread(parent)
{

}

// TODO: This spin loop is an antipattern. Fix it.
QByteArray InputQueueThread::getMessage()
{
    QByteArray retval;

    do {
        mutex.lock();
        if (! list.isEmpty()) {
            retval = list.takeFirst();
        }
        mutex.unlock();
        if (retval.size() == 0)
            msleep(100);
    } while (retval.size() == 0);

    return retval;
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
        {
            // Force a deep copy for thread safety.
            QByteArray *newAry = new QByteArray(buffer.data(), buffer.size());
            mutex.lock();
            list.append(*newAry);
            delete newAry;
            mutex.unlock();
        }
    }
}


InputQueue::InputQueue(QObject *parent) : QObject(parent)
{

}

void InputQueue::startQueue()
{
    // TODO: connections
    thread.start();
    // TODO: Wait until thread has started
}

QByteArray InputQueue::getMessage()
{
    return thread.getMessage();
}

void InputQueue::stopQueue()
{
    // TODO: don't crash
}
