// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "hidpp/dispatcher.h"
#include "hidpp/channel.h"

#include <QSignalSpy>
#include <QTest>

#include <vector>

using namespace hidpp;
using namespace std::chrono_literals;

namespace
{

Report report(std::vector<std::uint8_t> bytes)
{
    const auto parsed = Report::fromBytes(bytes);
    Q_ASSERT(parsed);
    return *parsed;
}

// The test decides what arrives and when; waiting holds reports that a drain
// delivers, as the kernel buffer of a hidraw node would.
class ScriptedChannel : public Channel
{
    Q_OBJECT

public:
    std::optional<Error> send(const Report &report) override
    {
        sent.push_back(report);
        return failSends ? std::optional<Error>(Error{ErrorKind::Disconnected, 0}) : std::nullopt;
    }

    void drain() override
    {
        const auto reports = std::exchange(waiting, {});
        for (const Report &report : reports) {
            Q_EMIT reportReceived(report);
        }
    }

    void deliver(const Report &report)
    {
        Q_EMIT reportReceived(report);
    }

    void unplug()
    {
        Q_EMIT disconnected();
    }

    std::vector<Report> sent;
    std::vector<Report> waiting;
    bool failSends = false;
};

// Collects the result of one request.
struct Outcome {
    std::optional<Result<Report>> result;

    Callback<Report> callback()
    {
        return [this](Result<Report> r) {
            result = std::move(r);
        };
    }
};

} // namespace

class DispatcherTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init()
    {
        m_channel = std::make_unique<ScriptedChannel>();
        m_dispatcher = std::make_unique<Dispatcher>(m_channel.get());
        m_dispatcher->setTimeout(30ms);
    }

    void cleanup()
    {
        m_dispatcher.reset();
        m_channel.reset();
    }

    void deliversTheMatchingReply()
    {
        Outcome outcome;
        m_dispatcher->request(Report::shortRequest(1, 0, functionByte(1), {0, 0, 0x5a}), outcome.callback());
        QVERIFY(!outcome.result);
        QTRY_COMPARE(m_channel->sent.size(), 1u);

        m_channel->deliver(report({0x11, 1, 0, 0x1a, 4, 5, 0x5a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));
        QVERIFY(outcome.result && outcome.result->hasValue());
        QCOMPARE(outcome.result->value().params()[0], 4);
    }

    void passesOtherReportsOn()
    {
        QSignalSpy unsolicited(m_dispatcher.get(), &Dispatcher::unsolicitedReport);
        Outcome outcome;
        m_dispatcher->request(Report::shortRequest(1, 8, functionByte(1)), outcome.callback());
        QTRY_COMPARE(m_channel->sent.size(), 1u);

        // A battery event and a reply to another tool.
        m_channel->deliver(report({0x10, 1, 8, 0x00, 85, 8, 0}));
        m_channel->deliver(report({0x10, 1, 8, 0x1b, 85, 8, 0}));
        QVERIFY(!outcome.result);
        QCOMPARE(unsolicited.size(), 2);

        m_channel->deliver(report({0x10, 1, 8, 0x1a, 85, 8, 0}));
        QVERIFY(outcome.result && outcome.result->hasValue());
    }

    void turnsErrorRepliesIntoErrors()
    {
        Outcome hidpp20;
        Outcome hidpp10;
        m_dispatcher->request(Report::shortRequest(1, 8, functionByte(1)), hidpp20.callback());
        m_dispatcher->request(Report::shortRequest(3, 0, functionByte(1)), hidpp10.callback());

        QTRY_COMPARE(m_channel->sent.size(), 1u);
        m_channel->deliver(report({0x10, 1, 0xff, 8, 0x1a, 0x05, 0}));
        QVERIFY(hidpp20.result && !hidpp20.result->hasValue());
        QCOMPARE(hidpp20.result->error(), (Error{ErrorKind::Protocol20, 0x05}));

        QTRY_COMPARE(m_channel->sent.size(), 2u);
        m_channel->deliver(report({0x10, 3, 0x8f, 0, 0x1a, 0x09, 0}));
        QVERIFY(hidpp10.result && !hidpp10.result->hasValue());
        QCOMPARE(hidpp10.result->error(), (Error{ErrorKind::Protocol10, 0x09}));
    }

    void sendsOneRequestAtATime()
    {
        Outcome first;
        Outcome second;
        m_dispatcher->request(Report::shortRequest(1, 2, functionByte(0)), first.callback());
        m_dispatcher->request(Report::shortRequest(2, 2, functionByte(0)), second.callback());
        QTRY_COMPARE(m_channel->sent.size(), 1u);
        QTest::qWait(10);
        QCOMPARE(m_channel->sent.size(), 1u);

        m_channel->deliver(report({0x10, 1, 2, 0x0a, 3, 0, 0}));
        QTRY_COMPARE(m_channel->sent.size(), 2u);
        QCOMPARE(m_channel->sent[1].deviceIndex(), 2);
    }

    void retriesOnceAfterATimeout()
    {
        Outcome outcome;
        const Report request = Report::shortRequest(1, 0, functionByte(1), {0, 0, 0x5a});
        m_dispatcher->request(request, outcome.callback());
        QTRY_COMPARE(m_channel->sent.size(), 2u);
        QVERIFY(m_channel->sent[1] == request);

        m_channel->deliver(report({0x10, 1, 0, 0x1a, 4, 5, 0x5a}));
        QVERIFY(outcome.result && outcome.result->hasValue());
    }

    void failsAfterTheRetry()
    {
        Outcome outcome;
        m_dispatcher->request(Report::shortRequest(1, 0, functionByte(1)), outcome.callback());
        QTRY_VERIFY(outcome.result);
        QCOMPARE(outcome.result->error(), (Error{ErrorKind::Timeout, 0}));
        QCOMPARE(m_channel->sent.size(), 2u);
    }

    void doesNotTakeALateReplyForTheNextRequest()
    {
        // FeatureSet getFeatureId for index 5 goes unanswered twice.
        Outcome first;
        m_dispatcher->request(Report::shortRequest(1, 1, functionByte(1), {5, 0, 0}), first.callback());
        QTRY_VERIFY(first.result);
        QCOMPARE(first.result->error().kind, ErrorKind::Timeout);

        // Its answer arrives late, while the request for index 6 is queued:
        // same header, other parameter. It must not answer index 6.
        m_channel->waiting.push_back(report({0x10, 1, 1, 0x1a, 0x10, 0x04, 0}));
        Outcome second;
        m_dispatcher->request(Report::shortRequest(1, 1, functionByte(1), {6, 0, 0}), second.callback());
        QTRY_COMPARE(m_channel->sent.size(), 3u);
        QVERIFY(!second.result);

        m_channel->deliver(report({0x10, 1, 1, 0x1a, 0x1b, 0x04, 0}));
        QVERIFY(second.result && second.result->hasValue());
        QCOMPARE(second.result->value().params()[0], 0x1b);
    }

    void failsEverythingWhenTheNodeGoesAway()
    {
        Outcome inFlight;
        Outcome queued;
        m_dispatcher->request(Report::shortRequest(1, 0, functionByte(1)), inFlight.callback());
        m_dispatcher->request(Report::shortRequest(2, 0, functionByte(1)), queued.callback());
        QTRY_COMPARE(m_channel->sent.size(), 1u);

        m_channel->unplug();
        QCOMPARE(inFlight.result->error().kind, ErrorKind::Disconnected);
        QCOMPARE(queued.result->error().kind, ErrorKind::Disconnected);

        Outcome later;
        m_dispatcher->request(Report::shortRequest(1, 0, functionByte(1)), later.callback());
        QVERIFY(!later.result);
        QTRY_VERIFY(later.result);
        QCOMPARE(later.result->error().kind, ErrorKind::Disconnected);
        QCOMPARE(m_channel->sent.size(), 1u);
    }

    void failsARequestThatCannotBeSent()
    {
        m_channel->failSends = true;
        Outcome outcome;
        m_dispatcher->request(Report::shortRequest(1, 0, functionByte(1)), outcome.callback());
        QTRY_VERIFY(outcome.result);
        QCOMPARE(outcome.result->error().kind, ErrorKind::Disconnected);
    }

private:
    std::unique_ptr<ScriptedChannel> m_channel;
    std::unique_ptr<Dispatcher> m_dispatcher;
};

QTEST_GUILESS_MAIN(DispatcherTest)

#include "dispatchertest.moc"
