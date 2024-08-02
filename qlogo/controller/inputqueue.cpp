
//===-- qlogo/reader.h - Reader class definition -------*- C++ -*-===//
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

#include "controller/inputqueue.h"
#include <QDebug>
#include <QMutex>
#include <QQueue>
#include <unistd.h>

/// @brief The mutex for the message queue
QMutex queueLock;

/// @brief The message queue
QQueue<QByteArray> messageQueue;

InputQueueThread::InputQueueThread(QObject *parent) : QThread(parent)
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
        if (dataread <= 0)
        {
            // I guess we're done.
            return;
        }
        Q_ASSERT(dataread == sizeof(qint64));
        message.resize(datalen);
        while (datalen > 0)
        {
            dataread = read(STDIN_FILENO, message.data() + datareadSofar, datalen);
            if (dataread <= 0)
            {
                // Like tears in rain, time to die.
                return;
            }
            datareadSofar += dataread;
            datalen -= dataread;
        }
        Q_ASSERT(0 == datalen);
        queueLock.lock();
        messageQueue.enqueue(message);
        queueLock.unlock();
        emit sendMessageSignal();
    }
}

InputQueue::InputQueue(QObject *parent) : QObject(parent)
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
    queueLock.lock();
    bool isMessageQueued = !messageQueue.isEmpty();
    if (isMessageQueued)
        message = messageQueue.dequeue();
    queueLock.unlock();
    if (isMessageQueued)
        return message;

    // Wait for a message.
    forever
    {
        eventLoop.exec();

        queueLock.lock();
        isMessageQueued = !messageQueue.isEmpty();
        if (isMessageQueued)
            message = messageQueue.dequeue();
        queueLock.unlock();
        if (isMessageQueued)
            return message;
    }
}

bool InputQueue::isMessageAvailable()
{
    queueLock.lock();
    bool retval = !messageQueue.isEmpty();
    queueLock.unlock();
    return retval;
}

void InputQueue::receiveMessageSlot()
{
    eventLoop.exit(0);
}

void InputQueue::stopQueue()
{
    // The QLogo GUI closes the pipe so there is nothing to do except wait.
    thread.wait();
}
