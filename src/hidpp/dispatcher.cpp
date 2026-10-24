// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "dispatcher.h"
#include "channel.h"
#include "hidpp_logging.h"

namespace hidpp
{

namespace
{

constexpr int MaxAttempts = 2;

} // namespace

Dispatcher::Dispatcher(Channel *channel, QObject *parent)
    : QObject(parent)
    , m_channel(channel)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(DefaultTimeout);
    connect(&m_timer, &QTimer::timeout, this, &Dispatcher::handleTimeout);
    connect(channel, &Channel::reportReceived, this, &Dispatcher::handleReport);
    connect(channel, &Channel::disconnected, this, &Dispatcher::handleDisconnected);
    connect(channel, &QObject::destroyed, this, &Dispatcher::handleDisconnected);
}

void Dispatcher::setTimeout(std::chrono::milliseconds timeout)
{
    m_timer.setInterval(timeout);
}

void Dispatcher::request(const Report &request, Callback<Report> done)
{
    if (m_disconnected) {
        QTimer::singleShot(0, this, [done = std::move(done)] {
            done(Error{ErrorKind::Disconnected, 0});
        });
        return;
    }
    m_queue.push_back(Pending{request, std::move(done)});
    scheduleNext();
}

void Dispatcher::scheduleNext()
{
    if (m_nextScheduled || m_inFlight || m_queue.empty()) {
        return;
    }
    m_nextScheduled = true;
    QMetaObject::invokeMethod(this, &Dispatcher::sendNext, Qt::QueuedConnection);
}

void Dispatcher::sendNext()
{
    m_nextScheduled = false;
    if (m_inFlight || m_queue.empty() || !m_channel) {
        return;
    }
    // Drained while nothing is in flight, waiting reports cannot be matched
    // to the next request: a late reply to a request with the same header but
    // other parameters would otherwise be taken for its answer.
    m_channel->drain();
    if (m_inFlight || m_queue.empty() || m_disconnected) {
        return;
    }
    m_inFlight = std::move(m_queue.front());
    m_queue.pop_front();
    transmit();
}

void Dispatcher::transmit()
{
    ++m_inFlight->attempts;
    qCDebug(HIDSIGHT_HIDPP).nospace() << "request to device " << m_inFlight->request.deviceIndex() << ", header " << Qt::hex << m_inFlight->request.subId()
                                      << " " << m_inFlight->request.address() << ", attempt " << Qt::dec << m_inFlight->attempts;
    if (const auto error = m_channel->send(m_inFlight->request)) {
        finish(*error);
        return;
    }
    m_timer.start();
}

void Dispatcher::handleReport(const Report &report)
{
    if (!m_inFlight) {
        Q_EMIT unsolicitedReport(report);
        return;
    }
    switch (matchReply(m_inFlight->request, report)) {
    case ReplyMatch::Reply:
        finish(report);
        return;
    case ReplyMatch::Error10:
        finish(Error{ErrorKind::Protocol10, report.errorCode()});
        return;
    case ReplyMatch::Error20:
        finish(Error{ErrorKind::Protocol20, report.errorCode()});
        return;
    case ReplyMatch::None:
        Q_EMIT unsolicitedReport(report);
        return;
    }
}

void Dispatcher::handleTimeout()
{
    if (!m_inFlight || !m_channel) {
        return;
    }
    if (m_inFlight->attempts >= MaxAttempts) {
        finish(Error{ErrorKind::Timeout, 0});
        return;
    }
    // The same request again, so a late reply to the first attempt that the
    // drain delivers is a valid answer.
    m_channel->drain();
    if (m_inFlight) {
        transmit();
    }
}

void Dispatcher::handleDisconnected()
{
    if (m_disconnected) {
        return;
    }
    m_disconnected = true;
    m_timer.stop();
    std::deque<Pending> failed;
    if (m_inFlight) {
        failed.push_back(std::move(*m_inFlight));
        m_inFlight.reset();
    }
    while (!m_queue.empty()) {
        failed.push_back(std::move(m_queue.front()));
        m_queue.pop_front();
    }
    for (const Pending &pending : failed) {
        pending.done(Error{ErrorKind::Disconnected, 0});
    }
}

void Dispatcher::finish(Result<Report> result)
{
    m_timer.stop();
    const Pending pending = std::move(*m_inFlight);
    m_inFlight.reset();
    if (!result) {
        qCDebug(HIDSIGHT_HIDPP) << "request to device" << pending.request.deviceIndex() << "failed:" << result.error();
    }
    pending.done(std::move(result));
    scheduleNext();
}

} // namespace hidpp
