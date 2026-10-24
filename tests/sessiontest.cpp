// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "hidpp/capture.h"
#include "hidpp/receiversession.h"

#include <QEventLoop>
#include <QFile>
#include <QTest>
#include <QTimer>

using namespace hidpp;
using namespace std::chrono_literals;

namespace
{

// Long reports in the constructed captures below are written without their
// trailing zeros.
QByteArray padLongReports(const QByteArray &text)
{
    QByteArray padded;
    for (const QByteArray &line : text.split('\n')) {
        QByteArray full = line.trimmed();
        if (full.startsWith("< 11") || full.startsWith("> 11")) {
            while (full.count(' ') < int(LongReportSize)) {
                full += " 00";
            }
        }
        padded += full + '\n';
    }
    return padded;
}

std::vector<CaptureEntry> loadFile(const QString &name)
{
    QFile file(QStringLiteral(CAPTURE_DIR "/") + name);
    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("cannot open the capture %s", qPrintable(file.fileName()));
    }
    auto entries = parseCapture(file.readAll());
    if (!entries) {
        qFatal("the capture is malformed at line %d", entries.error().code);
    }
    return std::move(entries).value();
}

// Runs a session on a capture until the capture is used up and the session
// has handled its last reports; returns every published state in order.
struct Run {
    std::vector<DeviceState> published;
    std::vector<std::uint8_t> removed;
    std::optional<Error> failure;
    bool atEnd = false;
    std::optional<Report> mismatch;
    std::vector<DeviceState> finalDevices;
};

Run run(std::vector<CaptureEntry> entries)
{
    Run result;
    auto channel = std::make_unique<ReplayChannel>(std::move(entries));
    ReplayChannel *replay = channel.get();
    ReceiverSession session(std::move(channel));
    session.setSoftwareId(replay->softwareId());
    session.setTimeout(50ms);

    QEventLoop loop;
    QObject::connect(&session, &ReceiverSession::deviceChanged, &loop, [&result](const DeviceState &state) {
        result.published.push_back(state);
    });
    QObject::connect(&session, &ReceiverSession::deviceRemoved, &loop, [&result](std::uint8_t index) {
        result.removed.push_back(index);
    });
    QObject::connect(&session, &ReceiverSession::failed, &loop, [&result, &loop](const Error &error) {
        result.failure = error;
        loop.quit();
    });
    QObject::connect(replay, &ReplayChannel::finished, &loop, [&loop] {
        QTimer::singleShot(300ms, &loop, &QEventLoop::quit);
    });
    // Longer than the one-second retry after "connection failed".
    QTimer::singleShot(10s, &loop, &QEventLoop::quit);
    session.start();
    loop.exec();

    result.atEnd = replay->isAtEnd();
    result.mismatch = replay->mismatch();
    result.finalDevices = session.devices();
    return result;
}

const DeviceState *device(const std::vector<DeviceState> &devices, std::uint8_t index)
{
    for (const DeviceState &state : devices) {
        if (state.index == index) {
            return &state;
        }
    }
    return nullptr;
}

// One device at index 1 that is asleep at start, wakes up and reports a new
// battery level; indices 2 to 6 are empty. Its features: FeatureSet at 1,
// DeviceName at 2, UnifiedBattery at 3. The device and receiver answers are
// those seen on the desk: 0x04 while asleep, 0x09 for an empty slot, a
// WirelessDeviceStatus event and a battery event on waking.
const QByteArray SleepingDevice = R"(
> 10 ff 81 02 00 00 00
< 10 ff 81 02 00 01 00
> 10 01 00 1a 00 00 5a
< 10 01 8f 00 1a 04 00
> 10 02 00 1a 00 00 5a
< 10 02 8f 00 1a 09 00
> 10 03 00 1a 00 00 5a
< 10 03 8f 00 1a 09 00
> 10 04 00 1a 00 00 5a
< 10 04 8f 00 1a 09 00
> 10 05 00 1a 00 00 5a
< 10 05 8f 00 1a 09 00
> 10 06 00 1a 00 00 5a
< 10 06 8f 00 1a 09 00
> 10 01 00 1a 00 00 5a
< 10 01 8f 00 1a 04 00
< 11 01 04 00 01 01 01
< 11 01 03 00 32 04 00 00
> 10 01 00 1a 00 00 5a
< 11 01 00 1a 04 05 5a
> 10 01 00 0a 00 05 00
< 11 01 00 0a 02
> 10 01 02 0a 00 00 00
< 11 01 02 0a 04
> 10 01 02 1a 00 00 00
< 11 01 02 1a 4b 65 79 73
> 10 01 02 2a 00 00 00
< 11 01 02 2a 00
> 10 01 00 0a 10 04 00
< 11 01 00 0a 03
> 10 01 03 0a 00 00 00
< 11 01 03 0a 0f 02
> 10 01 03 1a 00 00 00
< 11 01 03 1a 32 04 00 00
> 10 01 00 1a 00 00 5a
< 11 01 00 1a 04 05 5a
> 10 01 00 0a 00 01 00
< 11 01 00 0a 01
> 10 01 01 0a 00 00 00
< 11 01 01 0a 03
> 10 01 01 1a 01 00 00
< 11 01 01 1a 00 01
> 10 01 01 1a 02 00 00
< 11 01 01 1a 00 05
> 10 01 01 1a 03 00 00
< 11 01 01 1a 10 04
> 10 01 02 0a 00 00 00
< 11 01 02 0a 04
> 10 01 02 1a 00 00 00
< 11 01 02 1a 4b 65 79 73
> 10 01 02 2a 00 00 00
< 11 01 02 2a 00
> 10 01 03 0a 00 00 00
< 11 01 03 0a 0f 02
> 10 01 03 1a 00 00 00
< 11 01 03 1a 32 04 00 00
< 11 01 03 1b 63 08 00 00
< 11 01 03 00 31 04 00 00
)";

// One awake device at index 1 with FeatureSet at 1 and UnifiedBattery at 2,
// read completely; then a re-read that finds it asleep (0x04) twice.
const QByteArray FallsAsleep = R"(
> 10 ff 81 02 00 00 00
< 10 ff 81 02 00 01 00
> 10 01 00 1a 00 00 5a
< 11 01 00 1a 04 05 5a
> 10 01 00 0a 00 05 00
< 11 01 00 0a 00
> 10 01 00 0a 10 04 00
< 11 01 00 0a 02
> 10 01 02 0a 00 00 00
< 11 01 02 0a 0f 02
> 10 01 02 1a 00 00 00
< 11 01 02 1a 50 04 00 00
> 10 02 00 1a 00 00 5a
< 10 02 8f 00 1a 09 00
> 10 03 00 1a 00 00 5a
< 10 03 8f 00 1a 09 00
> 10 04 00 1a 00 00 5a
< 10 04 8f 00 1a 09 00
> 10 05 00 1a 00 00 5a
< 10 05 8f 00 1a 09 00
> 10 06 00 1a 00 00 5a
< 10 06 8f 00 1a 09 00
> 10 01 00 1a 00 00 5a
< 11 01 00 1a 04 05 5a
> 10 01 00 0a 00 01 00
< 11 01 00 0a 01
> 10 01 01 0a 00 00 00
< 11 01 01 0a 02
> 10 01 01 1a 01 00 00
< 11 01 01 1a 00 01
> 10 01 01 1a 02 00 00
< 11 01 01 1a 10 04
> 10 01 02 0a 00 00 00
< 11 01 02 0a 0f 02
> 10 01 02 1a 00 00 00
< 11 01 02 1a 50 04 00 00
> 10 01 00 0a 10 04 00
< 10 01 8f 00 0a 04 00
> 10 01 00 1a 00 00 5a
< 10 01 8f 00 1a 04 00
)";

} // namespace

class SessionTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void readsAndFollowsTheDeskReceiver()
    {
        const Run result = run(loadFile(QStringLiteral("bolt-session-events-2026-10-24.capture")));
        QVERIFY(!result.failure);
        QVERIFY(!result.mismatch);
        QVERIFY(result.atEnd);

        // Both devices are listed with their battery before either is read
        // completely.
        QVERIFY(result.published.size() >= 4);
        QCOMPARE(result.published[0].index, 1);
        QCOMPARE(result.published[1].index, 2);
        for (int i = 0; i < 2; ++i) {
            QCOMPARE(result.published[i].status, DeviceStatus::Online);
            QVERIFY(result.published[i].battery);
            QVERIFY(!result.published[i].complete);
        }
        QCOMPARE(result.published[0].name->name, QStringLiteral("MX Keys S"));
        QCOMPARE(result.published[1].name->name, QStringLiteral("MX Master 3S"));

        // The cable events of the keyboard, bouncing included.
        std::vector<ChargingState> keyboardCharging;
        for (const DeviceState &state : result.published) {
            if (state.index == 1 && state.complete) {
                keyboardCharging.push_back(state.battery->charging);
            }
        }
        const std::vector<ChargingState> expected{
            ChargingState::Discharging,
            ChargingState::Charging,
            ChargingState::Discharging,
            ChargingState::Charging,
            ChargingState::Discharging,
            ChargingState::Charging,
            ChargingState::Discharging,
            ChargingState::Charging,
        };
        QCOMPARE(keyboardCharging, expected);

        const DeviceState *keyboard = device(result.finalDevices, 1);
        QVERIFY(keyboard && keyboard->complete);
        QCOMPARE(keyboard->status, DeviceStatus::Online);
        QCOMPARE(keyboard->battery->externalPower, std::optional<bool>(true));
        QCOMPARE(keyboard->firmware->entries.size(), 3u);

        const DeviceState *mouse = device(result.finalDevices, 2);
        QVERIFY(mouse && mouse->complete);
        QCOMPARE(mouse->battery->percentage, std::optional<std::uint8_t>(90));
        QCOMPARE(mouse->dpi->sensorDpi, std::vector<std::uint16_t>{1000});
        QCOMPARE(result.finalDevices.size(), 2u);
    }

    void waitsForASleepingDevice()
    {
        const Run result = run(parseCapture(padLongReports(SleepingDevice)).value());
        QVERIFY(!result.failure);
        QVERIFY(!result.mismatch);
        QVERIFY(result.atEnd);

        // Asleep twice in a row: unreachable, without a name yet.
        QCOMPARE(result.published.front().status, DeviceStatus::Unreachable);
        QCOMPARE(result.published.front().error, (Error{ErrorKind::Protocol10, 0x04}));
        QVERIFY(!result.published.front().name);

        // Woken up: read again, then the battery event; the reply to
        // another program in between is ignored.
        const DeviceState *state = device(result.finalDevices, 1);
        QVERIFY(state);
        QCOMPARE(state->status, DeviceStatus::Online);
        QVERIFY(state->complete);
        QCOMPARE(state->name->name, QStringLiteral("Keys"));
        QCOMPARE(state->name->kind, DeviceKind::Keyboard);
        QCOMPARE(state->battery->percentage, std::optional<std::uint8_t>(49));
        QCOMPARE(state->battery->level, BatteryLevel::Good);
        QCOMPARE(result.finalDevices.size(), 1u);
        QVERIFY(result.removed.empty());
    }

    void marksADeviceThatFellAsleepOnReread()
    {
        auto channel = std::make_unique<ReplayChannel>(parseCapture(padLongReports(FallsAsleep)).value());
        ReplayChannel *replay = channel.get();
        ReceiverSession session(std::move(channel));
        session.setTimeout(50ms);
        std::vector<DeviceState> published;
        connect(&session, &ReceiverSession::deviceChanged, this, [&published](const DeviceState &state) {
            published.push_back(state);
        });
        session.start();

        QTRY_VERIFY_WITH_TIMEOUT(!published.empty() && published.back().complete, 5000);
        QCOMPARE(published.back().status, DeviceStatus::Online);
        QCOMPARE(published.back().battery->percentage, std::optional<std::uint8_t>(80));
        const QDateTime seen = published.back().lastSeen;
        QVERIFY(seen.isValid());

        // What the Refresh action does after the device went quiet.
        session.rereadAll();
        QTRY_VERIFY_WITH_TIMEOUT(published.back().status == DeviceStatus::Unreachable, 5000);
        QCOMPARE(published.back().error, (Error{ErrorKind::Protocol10, 0x04}));
        // The last reading stays, with the time it was taken.
        QCOMPARE(published.back().battery->percentage, std::optional<std::uint8_t>(80));
        QCOMPARE(published.back().lastSeen, seen);
        QVERIFY(!replay->mismatch());
        QVERIFY(replay->isAtEnd());
    }

    void failsOnANodeThatIsNoReceiver()
    {
        const Run result = run(parseCapture("> 10 ff 81 02 00 00 00\n< 10 ff 8f 81 02 01 00\n").value());
        QCOMPARE(result.failure, std::optional<Error>(Error{ErrorKind::Protocol10, 0x01}));
        QVERIFY(result.published.empty());
    }
};

QTEST_GUILESS_MAIN(SessionTest)

#include "sessiontest.moc"
