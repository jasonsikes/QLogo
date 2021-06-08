#include "inputqueuethread.h"
#include <unistd.h>
//#include "constants.h"

InputQueueThread::InputQueueThread(QObject *parent) : QThread(parent)
{

}

QByteArray InputQueueThread::getMessage()
{
    QByteArray retval;
    QMutexLocker locker(&mutex);

    if (! list.isEmpty()) {
        retval = list.takeFirst();
    }
    dataIsAvailable = ! list.isEmpty();
    return retval;
}


void InputQueueThread::clearQueue()
{
    QMutexLocker locker(&mutex);
    dataIsAvailable = false;
    list.clear();
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
            dataIsAvailable = true;
        }
    }
}
