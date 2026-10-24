// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "hidpp/battery.h"
#include "hidpp/capture.h"
#include "hidpp/device.h"
#include "hidpp/devicefwversion.h"
#include "hidpp/devicename.h"
#include "hidpp/pointer.h"
#include "hidpp/receiver.h"

#include <QRandomGenerator>
#include <QTest>

#include <vector>

using namespace hidpp;

namespace
{

using Bytes = std::vector<std::uint8_t>;

template<typename T>
bool isMalformed(const Result<T> &result)
{
    return !result.hasValue() && result.error().kind == ErrorKind::Malformed;
}

} // namespace

class ParserTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesRoot()
    {
        const auto version = Device::parsePing(Bytes{4, 5, 0x5a});
        QVERIFY(version);
        QCOMPARE(version.value().major, 4);
        QCOMPARE(version.value().minor, 5);
        QVERIFY(isMalformed(Device::parsePing(Bytes{4, 5, 0x00})));
        QVERIFY(isMalformed(Device::parsePing(Bytes{})));

        QCOMPARE(Device::parseFeatureIndex(Bytes{3, 0, 0}).value(), std::optional<std::uint8_t>(3));
        QCOMPARE(Device::parseFeatureIndex(Bytes{0, 0, 0}).value(), std::optional<std::uint8_t>());
    }

    void parsesTheFeatureTable()
    {
        QCOMPARE(Device::parseFeatureCount(Bytes{33}).value(), 33);
        QVERIFY(isMalformed(Device::parseFeatureCount(Bytes{0xff})));

        const auto battery = Device::parseFeatureEntry(8, Bytes{0x10, 0x04, 0x00});
        QCOMPARE(battery.value().id, 0x1004);
        QCOMPARE(battery.value().index, 8);
        QVERIFY(battery.value().isVisible());

        const auto reset = Device::parseFeatureEntry(19, Bytes{0x18, 0x02, 0x60});
        QVERIFY(!reset.value().isVisible());
        QVERIFY(!Device::parseFeatureEntry(27, Bytes{0x1e, 0x00, 0x40}).value().isVisible());
        QVERIFY(isMalformed(Device::parseFeatureEntry(1, Bytes{0x00, 0x01})));

        const FeatureTable table({FeatureEntry{0x0001, 1, 0}, FeatureEntry{0x1004, 8, 0}});
        QCOMPARE(table.indexOf(0x1004), std::optional<std::uint8_t>(8));
        QCOMPARE(table.indexOf(0x0000), std::optional<std::uint8_t>(0));
        QCOMPARE(table.indexOf(0x2201), std::optional<std::uint8_t>());
        QCOMPARE(featureName(0x1004), QLatin1StringView("UnifiedBattery"));
        QVERIFY(featureName(0x0011).isEmpty());
    }

    void parsesTheName()
    {
        QCOMPARE(DeviceName::parseLength(Bytes{9}).value(), 9);
        QVERIFY(isMalformed(DeviceName::parseLength(Bytes{0})));
        QVERIFY(isMalformed(DeviceName::parseLength(Bytes{DeviceName::MaxLength + 1})));

        QCOMPARE(DeviceName::parseKind(Bytes{0}).value(), DeviceKind::Keyboard);
        QCOMPARE(DeviceName::parseKind(Bytes{3}).value(), DeviceKind::Mouse);
        QCOMPARE(DeviceName::parseKind(Bytes{200}).value(), DeviceKind::Unknown);

        QCOMPARE(DeviceName::decodeName("MX Keys S"), QStringLiteral("MX Keys S"));
        QCOMPARE(DeviceName::decodeName(QByteArrayView("MX\x01 Master\0junk", 15)), QStringLiteral("MX Master"));
        QCOMPARE(DeviceName::decodeName("\xff\xfe"), QStringLiteral("��"));
    }

    void parsesFirmware()
    {
        QCOMPARE(DeviceFwVersion::parseEntityCount(Bytes{3}).value(), 3);
        QVERIFY(isMalformed(DeviceFwVersion::parseEntityCount(Bytes{DeviceFwVersion::MaxEntities + 1})));

        const auto main = DeviceFwVersion::parseEntry(Bytes{0x00, 'R', 'B', 'K', 0x81, 0x02, 0x00, 0x17, 0, 0, 0, 0, 0, 0, 0, 0});
        QCOMPARE(main.value().type, FirmwareType::MainApplication);
        QCOMPARE(main.value().version(), QStringLiteral("RBK81.02.B0023"));

        const auto softdevice = DeviceFwVersion::parseEntry(Bytes{0x05, 0, 0, 0, 0x00, 0x00, 0x01, 0x26, 0, 0, 0, 0, 0, 0, 0, 0});
        QCOMPARE(softdevice.value().type, FirmwareType::Softdevice);
        QCOMPARE(softdevice.value().version(), QStringLiteral("00.00.B0294"));

        const auto strange = DeviceFwVersion::parseEntry(Bytes{0x0e, '\n', 0xff, 'X', 1, 2, 0, 3});
        QCOMPARE(strange.value().type, FirmwareType::Other);
        QCOMPARE(strange.value().prefix, QStringLiteral("X"));

        QVERIFY(isMalformed(DeviceFwVersion::parseEntry(Bytes{0x00, 'R', 'B'})));
    }

    void parsesUnifiedBattery()
    {
        QCOMPARE(UnifiedBattery::parseCapabilities(Bytes{0x0f, 0x02, 0}).value(), true);
        QCOMPARE(UnifiedBattery::parseCapabilities(Bytes{0x0f, 0x00, 0}).value(), false);

        const auto mouse = UnifiedBattery::parseStatus(Bytes{85, 0x08, 0, 0}, true);
        QCOMPARE(mouse.value().percentage, std::optional<std::uint8_t>(85));
        QCOMPARE(mouse.value().level, BatteryLevel::Full);
        QCOMPARE(mouse.value().charging, ChargingState::Discharging);
        QCOMPARE(mouse.value().externalPower, false);

        const auto keyboard = UnifiedBattery::parseStatus(Bytes{100, 0x08, 3, 1}, true);
        QCOMPARE(keyboard.value().charging, ChargingState::Complete);
        QCOMPARE(keyboard.value().externalPower, true);

        const auto low = UnifiedBattery::parseStatus(Bytes{0, 0x02, 9, 0}, false);
        QCOMPARE(low.value().percentage, std::optional<std::uint8_t>());
        QCOMPARE(low.value().level, BatteryLevel::Low);
        QCOMPARE(low.value().charging, ChargingState::Unknown);

        QVERIFY(isMalformed(UnifiedBattery::parseStatus(Bytes{101, 0x08, 0, 0}, true)));
        QVERIFY(isMalformed(UnifiedBattery::parseStatus(Bytes{85, 0x08, 0}, true)));
    }

    void parsesTheBatteryFallbacks()
    {
        const auto status = BatteryStatus::parse(Bytes{50, 20, 4});
        QCOMPARE(status.value().percentage, 50);
        QCOMPARE(status.value().charging, ChargingState::ChargingSlowly);
        QVERIFY(isMalformed(BatteryStatus::parse(Bytes{150, 20, 0})));

        QCOMPARE(BatteryVoltage::parse(Bytes{0x0f, 0xa0, 0}).value().millivolts, 4000);
        QVERIFY(isMalformed(BatteryVoltage::parse(Bytes{0x0f})));
    }

    void parsesPointerFeatures()
    {
        QCOMPARE(AdjustableDpi::parseSensorCount(Bytes{1}).value(), 1);
        QVERIFY(isMalformed(AdjustableDpi::parseSensorCount(Bytes{AdjustableDpi::MaxSensors + 1})));
        QCOMPARE(AdjustableDpi::parseSensorDpi(0, Bytes{0, 0x03, 0xe8}).value(), 1000);
        QVERIFY(isMalformed(AdjustableDpi::parseSensorDpi(0, Bytes{1, 0x03, 0xe8})));

        QCOMPARE(ReportRate::parse(Bytes{8}).value().milliseconds, 8);
        QVERIFY(isMalformed(ReportRate::parse(Bytes{0})));
    }

    void parsesTheReceiver()
    {
        QCOMPARE(Receiver::parseConnectedDeviceCount(Bytes{0, 2, 0}).value(), 2);
        QVERIFY(isMalformed(Receiver::parseConnectedDeviceCount(Bytes{0, 7, 0})));
        QVERIFY(Receiver::isEmptySlot(Error{ErrorKind::Protocol10, 0x09}));
        QVERIFY(!Receiver::isEmptySlot(Error{ErrorKind::Protocol20, 0x09}));
    }

    // Random parameters of every length must give a value or an error, never
    // a read out of bounds; the asan preset turns such a read into a failure.
    void survivesRandomParameters()
    {
        QRandomGenerator random(20261024);
        for (int round = 0; round < 20000; ++round) {
            Bytes bytes(random.bounded(17));
            for (std::uint8_t &byte : bytes) {
                byte = static_cast<std::uint8_t>(random.bounded(256));
            }
            const std::span<const std::uint8_t> params(bytes);
            const auto sensor = static_cast<std::uint8_t>(random.bounded(4));

            (void)Device::parsePing(params);
            (void)Device::parseFeatureIndex(params);
            (void)Device::parseFeatureCount(params);
            (void)Device::parseFeatureEntry(1, params);
            (void)DeviceName::parseLength(params);
            (void)DeviceName::parseKind(params);
            (void)DeviceName::decodeName(QByteArrayView(params.data(), qsizetype(params.size())));
            (void)DeviceFwVersion::parseEntityCount(params);
            if (const auto entry = DeviceFwVersion::parseEntry(params)) {
                (void)entry.value().version();
            }
            (void)UnifiedBattery::parseCapabilities(params);
            (void)UnifiedBattery::parseStatus(params, true);
            (void)UnifiedBattery::parseStatus(params, false);
            (void)BatteryStatus::parse(params);
            (void)BatteryVoltage::parse(params);
            (void)AdjustableDpi::parseSensorCount(params);
            (void)AdjustableDpi::parseSensorDpi(sensor, params);
            (void)ReportRate::parse(params);
            (void)Receiver::parseConnectedDeviceCount(params);
        }
    }

    void survivesRandomReportsAndCaptures()
    {
        QRandomGenerator random(1024);
        int accepted = 0;
        for (int round = 0; round < 20000; ++round) {
            // Every second round a well-formed size, so the matching sees
            // random reports too.
            const bool wellFormed = round % 2 == 0;
            const bool isLong = random.bounded(2) == 0;
            Bytes bytes(wellFormed ? (isLong ? LongReportSize : ShortReportSize) : random.bounded(65));
            for (std::uint8_t &byte : bytes) {
                byte = static_cast<std::uint8_t>(random.bounded(256));
            }
            if (wellFormed) {
                bytes[0] = isLong ? LongReportId : ShortReportId;
            }
            if (const auto report = Report::fromBytes(bytes)) {
                ++accepted;
                (void)matchReply(Report::shortRequest(1, 2, functionByte(1)), *report);
                (void)report->errorCode();
            }

            QByteArray text;
            const char alphabet[] = "<> 0123456789abcdefXYZ#\n\t\r";
            const int length = random.bounded(200);
            for (int i = 0; i < length; ++i) {
                text.append(alphabet[random.bounded(int(sizeof(alphabet) - 1))]);
            }
            (void)parseCapture(text);
        }
        QVERIFY(accepted >= 10000);
    }
};

QTEST_GUILESS_MAIN(ParserTest)

#include "parsertest.moc"
