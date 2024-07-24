#ifndef INPUTQUEUE_H
#define INPUTQUEUE_H

//===-- qlogo/reader.h - Reader class definition -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// QLogo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with QLogo.  If not, see <http://www.gnu.org/licenses/>.
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
    void sendMessage();
};

class InputQueue : public QObject
{
    Q_OBJECT
    InputQueueThread thread;
    QByteArray message;
    QEventLoop eventLoop;

  private slots:
    // Connected to sendMessage signal from thread.
    void receiveMessage();

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
