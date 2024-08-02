#ifndef INPUTQUEUE_H
#define INPUTQUEUE_H

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
/// This file contains the declarations of the InputQueueThread and InputQueue,
/// which create a separate thread to wait for data and subsequently read data
/// from the input queue.
//===----------------------------------------------------------------------===//

#include <QByteArray>
#include <QEventLoop>
#include <QThread>

class InputQueueThread : public QThread
{
    Q_OBJECT

    void run() Q_DECL_OVERRIDE;

  public:
    /// @brief Constructor
    /// @param parent The Qt parent object
    explicit InputQueueThread(QObject *parent = nullptr);

  signals:
    /// @brief Signal to indicate that a message is available
    void sendMessageSignal();
};

class InputQueue : public QObject
{
    Q_OBJECT
    InputQueueThread thread;
    QByteArray message;
    QEventLoop eventLoop;

  private slots:
    // Connected to sendMessage signal from thread.
    void receiveMessageSlot();

  public:
    /// @brief Constructor
    /// @param parent The Qt parent object
    explicit InputQueue(QObject *parent = nullptr);

    /// @brief Start the input thread.
    void startQueue();

    /// @brief Stop the input thread.
    void stopQueue();

    /// @brief Get a message.
    /// @return The message.
    /// Will halt until message is available.
    QByteArray getMessage();

    /// @brief Ask if there is a message in the queue
    /// @return True if there is a message, false otherwise
    bool isMessageAvailable();
};

#endif // INPUTQUEUE_H
