// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "hidpp/capture.h"
#include "hidpp/probe.h"
#include "hidpp/receiver.h"

#include <QBuffer>
#include <QEventLoop>
#include <QFile>
#include <QTest>
#include <QTimer>

using namespace hidpp;
using namespace std::chrono_literals;

namespace
{

std::vector<CaptureEntry> loadCapture(const QString &name)
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

template<typename T>
std::optional<Result<T>> await(const std::function<void(Callback<T>)> &start)
{
    std::optional<Result<T>> result;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    start([&result, &loop](Result<T> value) {
        result = std::move(value);
        loop.quit();
    });
    if (!result) {
        loop.exec();
    }
    if (!result) {
        qFatal("no result within the timeout");
    }
    return result;
}

} // namespace

class CaptureTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rejectsMalformedCaptures_data()
    {
        QTest::addColumn<QByteArray>("text");
        QTest::addColumn<int>("line");
        QTest::newRow("no direction") << QByteArray("# fine\n10 ff 81 02 00 00 00\n") << 2;
        QTest::newRow("no space") << QByteArray(">10 ff 81 02 00 00 00\n") << 1;
        QTest::newRow("odd digit") << QByteArray("> 10 ff 81 02 00 00 0\n") << 1;
        QTest::newRow("not hex") << QByteArray("> 10 ff 81 02 00 00 zz\n") << 1;
        QTest::newRow("wrong size") << QByteArray("> 10 ff 81 02 00 00\n") << 1;
        QTest::newRow("too many bytes") << QByteArray("< 11" + QByteArray(" 00").repeated(20) + "\n") << 1;
    }

    void rejectsMalformedCaptures()
    {
        QFETCH(QByteArray, text);
        QFETCH(int, line);
        const auto entries = parseCapture(text);
        QVERIFY(!entries);
        QCOMPARE(entries.error(), (Error{ErrorKind::Malformed, line}));
    }

    void recordsWhatItReplays()
    {
        const QByteArray text = "> 10 ff 81 02 00 00 00\n< 10 ff 81 02 00 02 00\n";
        QBuffer output;
        output.open(QIODevice::WriteOnly);
        auto replay = std::make_unique<ReplayChannel>(parseCapture(text).value());
        auto *replayChannel = replay.get();
        RecordingChannel recording(std::move(replay), &output);
        Dispatcher dispatcher(&recording);

        const auto count = await<std::uint8_t>([&dispatcher](Callback<std::uint8_t> done) {
            Receiver(dispatcher).readConnectedDeviceCount(std::move(done));
        });
        QCOMPARE(count->value(), 2);
        QVERIFY(replayChannel->isAtEnd());
        QCOMPARE(output.data(), text);
    }

    void redactsIdentifiers()
    {
        CaptureFilter filter;
        const auto exchange = [&filter](const QByteArray &request, const QByteArray &reply) {
            filter.observeRequest(parseCapture(QByteArray("> " + request)).value().front().report);
            const auto kept = filter.filter(parseCapture(QByteArray("< " + reply)).value().front().report);
            return kept ? formatCaptureLine({CaptureEntry::Direction::Received, *kept}) : QByteArray();
        };
        const QByteArray zeros(" 00 00 00 00 00 00 00 00 00 00 00 00 00");

        // Device 1 names the index of DeviceFwVersion through Root getFeature.
        exchange("10 01 00 0a 00 03 00", "11 01 00 0a 02 00 00" + zeros);
        QCOMPARE(exchange("10 01 02 0a 00 00 00", "11 01 02 0a 03 de ad be ef 00 02 b3 78 00 00 00 00 00 01 00"),
                 "< 11 01 02 0a 03 00 00 00 00 00 02 b3 78 00 00 00 00 00 01 00\n");
        QCOMPARE(exchange("10 01 02 1a 01 00 00", "11 01 02 1a 00 52 42 4b 81 02 00 17 01 b3 78 12 34 56 78 00"),
                 "< 11 01 02 1a 00 52 42 4b 81 02 00 17 01 b3 78 00 00 00 00 00\n");

        // Device 2 names it through FeatureSet, as readFeatureTable does.
        exchange("10 02 00 0a 00 01 00", "11 02 00 0a 01 00 00" + zeros);
        exchange("10 02 01 1a 02 00 00", "11 02 01 1a 00 03 00" + zeros);
        QCOMPARE(exchange("10 02 02 0a 00 00 00", "11 02 02 0a 03 ca fe ba be 00 02 b0 34 00 00 00 00 0a 01 00"),
                 "< 11 02 02 0a 03 00 00 00 00 00 02 b0 34 00 00 00 00 0a 01 00\n");

        // Other features and devices pass unchanged.
        QCOMPARE(exchange("10 02 08 1a 00 00 00", "11 02 08 1a 55 08 00 00 00 00 00 00 00 00 00 00 00 00 00 00"),
                 "< 11 02 08 1a 55 08 00 00 00 00 00 00 00 00 00 00 00 00 00 00\n");
        QCOMPARE(exchange("10 03 02 0a 00 00 00", "11 03 02 0a 03 de ad be ef 00 02 b3 78 00 00 00 00 00 01 00"),
                 "< 11 03 02 0a 03 de ad be ef 00 02 b3 78 00 00 00 00 00 01 00\n");
    }

    void keepsOnlyOwnRepliesAndEvents()
    {
        CaptureFilter filter;
        const auto received = [&filter](const QByteArray &reply) {
            const auto kept = filter.filter(parseCapture(QByteArray("< " + reply)).value().front().report);
            return kept.has_value();
        };
        filter.observeRequest(parseCapture("> 10 01 08 1b 00 00 00").value().front().report);

        // The reply to this request and its error form.
        QVERIFY(received("10 01 08 1b 55 08 00"));
        QVERIFY(received("10 01 ff 08 1b 05 00"));
        // A battery event and a WirelessDeviceStatus event.
        QVERIFY(received("11 01 08 00 64 08 01 01 00 00 00 00 00 00 00 00 00 00 00 00"));
        QVERIFY(received("11 02 04 00 01 01 01 00 00 00 00 00 00 00 00 00 00 00 00 00"));
        // Replies and errors for another program, also at the receiver.
        QVERIFY(!received("11 01 02 0a 03 de ad be ef 00 02 b3 78 00 00 00 00 00 01 00"));
        QVERIFY(!received("10 01 ff 02 0a 05 00"));
        QVERIFY(!received("10 ff 83 b5 51 00 00"));
        QVERIFY(!received("10 02 8f 00 1a 04 00"));
    }

    void retriesWhereTheCaptureHasNoAnswer()
    {
        const QByteArray text =
            "> 10 01 00 1a 00 00 5a\n"
            "> 10 01 00 1a 00 00 5a\n"
            "< 11 01 00 1a 04 05 5a 00 00 00 00 00 00 00 00 00 00 00 00 00\n";
        ReplayChannel channel(parseCapture(text).value());
        Dispatcher dispatcher(&channel);
        dispatcher.setTimeout(20ms);

        const auto version = await<ProtocolVersion>([&dispatcher](Callback<ProtocolVersion> done) {
            Device(dispatcher, 1).ping(std::move(done));
        });
        QCOMPARE(version->value().major, 4);
        QVERIFY(channel.isAtEnd());
    }

    void readsTheDeskReceiver()
    {
        ReplayChannel channel(loadCapture(QStringLiteral("bolt-mxkeys-s-mxmaster-3s-2026-10-24.capture")));
        Dispatcher dispatcher(&channel);
        dispatcher.setTimeout(50ms);

        const auto count = await<std::uint8_t>([&dispatcher](Callback<std::uint8_t> done) {
            Receiver(dispatcher).readConnectedDeviceCount(std::move(done));
        });
        QCOMPARE(count->value(), 2);

        const auto keyboard = await<DeviceInfo>([&dispatcher](Callback<DeviceInfo> done) {
            probeDevice(Device(dispatcher, 1), std::move(done));
        });
        QVERIFY(keyboard->hasValue());
        checkKeyboard(keyboard->value());

        const auto mouse = await<DeviceInfo>([&dispatcher](Callback<DeviceInfo> done) {
            probeDevice(Device(dispatcher, 2), std::move(done));
        });
        QVERIFY(mouse->hasValue());
        checkMouse(mouse->value());

        for (std::uint8_t index = 3; index <= Receiver::LastDeviceIndex; ++index) {
            const auto empty = await<DeviceInfo>([&dispatcher, index](Callback<DeviceInfo> done) {
                probeDevice(Device(dispatcher, index), std::move(done));
            });
            QVERIFY(!empty->hasValue());
            QVERIFY(Receiver::isEmptySlot(empty->error()));
        }

        QVERIFY(!channel.mismatch());
        QVERIFY(channel.isAtEnd());
    }

private:
    static void checkKeyboard(const DeviceInfo &info)
    {
        QCOMPARE(info.protocol.major, 4);
        QCOMPARE(info.protocol.minor, 5);
        QCOMPARE(info.features.entries().size(), 33u);
        QCOMPARE(info.features.indexOf(0x1004), std::optional<std::uint8_t>(8));

        QCOMPARE(info.name->value().name, QStringLiteral("MX Keys S"));
        QCOMPARE(info.name->value().kind, DeviceKind::Keyboard);

        const auto &firmware = info.firmware->value().entries;
        QCOMPARE(firmware.size(), 3u);
        QCOMPARE(firmware[0].type, FirmwareType::Bootloader);
        QCOMPARE(firmware[0].version(), QStringLiteral("BL188.02.B0023"));
        QCOMPARE(firmware[1].type, FirmwareType::MainApplication);
        QCOMPARE(firmware[1].version(), QStringLiteral("RBK81.02.B0023"));
        QCOMPARE(firmware[2].type, FirmwareType::Softdevice);
        QCOMPARE(firmware[2].version(), QStringLiteral("00.00.B0294"));

        const Battery &battery = info.battery->value();
        QCOMPARE(battery.source, Battery::Source::UnifiedBattery);
        QCOMPARE(battery.percentage, std::optional<std::uint8_t>(100));
        QCOMPARE(battery.level, BatteryLevel::Full);
        QCOMPARE(battery.charging, ChargingState::Complete);
        QCOMPARE(battery.externalPower, std::optional<bool>(true));

        QVERIFY(!info.dpi);
        QVERIFY(!info.reportRate);
    }

    static void checkMouse(const DeviceInfo &info)
    {
        QCOMPARE(info.protocol.major, 4);
        QCOMPARE(info.protocol.minor, 5);
        QCOMPARE(info.features.entries().size(), 36u);

        QCOMPARE(info.name->value().name, QStringLiteral("MX Master 3S"));
        QCOMPARE(info.name->value().kind, DeviceKind::Mouse);

        const auto &firmware = info.firmware->value().entries;
        QCOMPARE(firmware.size(), 3u);
        QCOMPARE(firmware[0].version(), QStringLiteral("BL169.02.B0009"));
        QCOMPARE(firmware[1].version(), QStringLiteral("RBM22.02.B0009"));

        const Battery &battery = info.battery->value();
        QCOMPARE(battery.percentage, std::optional<std::uint8_t>(85));
        QCOMPARE(battery.level, BatteryLevel::Full);
        QCOMPARE(battery.charging, ChargingState::Discharging);
        QCOMPARE(battery.externalPower, std::optional<bool>(false));

        QCOMPARE(info.dpi->value().sensorDpi, std::vector<std::uint16_t>{1000});
        QVERIFY(!info.reportRate);
    }
};

QTEST_GUILESS_MAIN(CaptureTest)

#include "capturetest.moc"
