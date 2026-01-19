//===-- qlogo/inputqueue.h - InputQueueThread and InputQueue class definitions -------*- C++ -*-===//
//
// Copyright 2017-2024 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definitions of the InputQueueThread and InputQueue,
/// which create a separate thread to wait for data and subsequently read data
/// from the input queue.
//===----------------------------------------------------------------------===//

#include "interface/inputqueue.h"
#include <QDebug>
#include <QMutex>
#include <QQueue>

#ifndef _WIN32
#include <unistd.h>
#endif

InputQueueThread::InputQueueThread(QQueue<QByteArray> *byteArrayQueue, QMutex *queueMutex, QObject *parent)
    : QThread(parent), byteArrayQueue(byteArrayQueue), queueMutex(queueMutex)
{
}

void InputQueueThread::run()
{
    qint64 datalen;
    qint64 dataread;
    QByteArray message;
    forever
    {
        qint64 datareadSofar = 0;
        dataread = read(STDIN_FILENO, &datalen, sizeof(qint64));
        if (dataread != sizeof(qint64))
        {
            // For some reason we didn't get the data we expected. That likely means the pipe was closed.
            return;
        }
        message.resize(datalen);
        while (datalen > 0)
        {
            dataread = read(STDIN_FILENO, message.data() + datareadSofar, datalen);
            if (dataread <= 0)
            {
                // There was a problem reading the data. Likely means the pipe was closed.
                // In any case, we are done. Exit the loop.
                return;
            }
            datareadSofar += dataread;
            datalen -= dataread;
        }
        Q_ASSERT(0 == datalen);
        queueMutex->lock();
        byteArrayQueue->enqueue(message);
        queueMutex->unlock();
        emit sendMessageSignal();
    }
}

InputQueue::InputQueue(QObject *parent) : QObject(parent), thread(&byteArrayQueue, &queueMutex, this)
{
}

void InputQueue::startQueue()
{
    connect(&thread, SIGNAL(sendMessageSignal()), this, SLOT(receiveMessageSlot()), Qt::QueuedConnection);
    thread.start();
}

// TODO: The names getMessage() and receiveMessage() are ambiguous.
QByteArray InputQueue::getMessage()
{
    QByteArray message;
    // If there is a message already in the queue, return that.
    queueMutex.lock();
    bool isbyteArrayQueued = !byteArrayQueue.isEmpty();
    if (isbyteArrayQueued)
        message = byteArrayQueue.dequeue();
    queueMutex.unlock();
    if (isbyteArrayQueued)
        return message;

    // Wait for a message.
    forever
    {
        eventLoop.exec();

        queueMutex.lock();
        isbyteArrayQueued = !byteArrayQueue.isEmpty();
        if (isbyteArrayQueued)
            message = byteArrayQueue.dequeue();
        queueMutex.unlock();
        if (isbyteArrayQueued)
            return message;
    }
}

bool InputQueue::isMessageAvailable()
{
    queueMutex.lock();
    bool retval = !byteArrayQueue.isEmpty();
    queueMutex.unlock();
    return retval;
}

void InputQueue::receiveMessageSlot()
{
    // Exit the loop to process the message.
    eventLoop.exit(0);
}

void InputQueue::stopQueue()
{
    // Exit the event loop to unblock any waiting getMessage() calls
    eventLoop.exit(0);

    if (!thread.isRunning())
    {
        // Thread already terminated, just wait for cleanup
        thread.wait();
        return;
    }

    // The QLogo GUI closes the pipe so the thread should terminate naturally.
    // Wait with a timeout to avoid hanging indefinitely if something goes wrong.
    const unsigned long timeoutMs = 5000; // 5 second timeout
    if (!thread.wait(timeoutMs))
    {
        qWarning() << "InputQueueThread did not terminate within timeout";
        thread.terminate();
        if (!thread.wait(1000))
        {
            qWarning() << "InputQueueThread failed to terminate even after request";
        }
    }
}
