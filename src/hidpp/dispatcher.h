// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "report.h"
#include "result.h"

#include <QObject>
#include <QPointer>
#include <QTimer>

#include <chrono>
#include <deque>
#include <optional>

namespace hidpp
{

class Channel;

// Sends requests over a channel one at a time and hands each reply to its
// request. Devices wake slowly: verified on the desk, the first request after
// plugging in the receiver can go unanswered, so a request that times out is
// sent once more before it fails.
class Dispatcher : public QObject
{
    Q_OBJECT

public:
    static constexpr std::chrono::milliseconds DefaultTimeout{2000};

    explicit Dispatcher(Channel *channel, QObject *parent = nullptr);

    void setTimeout(std::chrono::milliseconds timeout);

    // Queues a request. done runs later from the event loop with the reply or
    // an error, never from inside this call. Callbacks still pending when the
    // dispatcher is destroyed are dropped without running; callers that
    // capture a QObject guard it with QPointer.
    void request(const Report &request, Callback<Report> done);

Q_SIGNALS:
    // A report that answers no request in flight: a notification from a
    // device or the receiver, or a reply to another tool on the same node.
    void unsolicitedReport(const hidpp::Report &report);

private:
    struct Pending {
        Report request;
        Callback<Report> done;
        int attempts = 0;
    };

    void scheduleNext();
    void sendNext();
    void transmit();
    void handleReport(const Report &report);
    void handleTimeout();
    void handleDisconnected();
    void finish(Result<Report> result);

    QPointer<Channel> m_channel;
    std::deque<Pending> m_queue;
    std::optional<Pending> m_inFlight;
    QTimer m_timer;
    bool m_nextScheduled = false;
    bool m_disconnected = false;
};

} // namespace hidpp
