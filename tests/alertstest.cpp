// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "batteryalerts.h"
#include "traylook.h"

#include "hidpp/capture.h"
#include "hidpp/receiversession.h"

#include <KLocalizedString>

#include <QFile>
#include <QTest>

using namespace hidpp;
using Alert = BatteryAlerts::Alert;

namespace
{

DeviceState
device(std::uint8_t index, const QString &name, std::optional<std::uint8_t> percentage, BatteryLevel level, ChargingState charging, bool externalPower = false)
{
    DeviceState state;
    state.index = index;
    state.status = DeviceStatus::Online;
    state.name = DeviceName{name, DeviceKind::Mouse};
    Battery battery;
    battery.percentage = percentage;
    battery.level = level;
    battery.charging = charging;
    battery.externalPower = externalPower;
    state.battery = battery;
    return state;
}

const QString Mouse = QStringLiteral("/dev/hidraw3#2");

} // namespace

class AlertsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        KLocalizedString::setApplicationDomain("hidsight");
    }

    void warnsOnceWhenTheLevelFalls()
    {
        BatteryAlerts alerts;
        QCOMPARE(alerts.update(Mouse, device(2, QStringLiteral("M"), 40, BatteryLevel::Good, ChargingState::Discharging)), std::vector<Alert>{});
        QCOMPARE(alerts.update(Mouse, device(2, QStringLiteral("M"), 15, BatteryLevel::Low, ChargingState::Discharging)),
                 std::vector<Alert>{Alert::BatteryLow});
        // Still low, a lower percentage: nothing new.
        QCOMPARE(alerts.update(Mouse, device(2, QStringLiteral("M"), 12, BatteryLevel::Low, ChargingState::Discharging)), std::vector<Alert>{});
        QCOMPARE(alerts.update(Mouse, device(2, QStringLiteral("M"), 4, BatteryLevel::Critical, ChargingState::Discharging)),
                 std::vector<Alert>{Alert::BatteryCritical});
        QCOMPARE(alerts.update(Mouse, device(2, QStringLiteral("M"), 4, BatteryLevel::Critical, ChargingState::Discharging)), std::vector<Alert>{});
    }

    void warnsAboutADeviceThatIsAlreadyLow()
    {
        BatteryAlerts alerts;
        QCOMPARE(alerts.update(Mouse, device(2, QStringLiteral("M"), 10, BatteryLevel::Low, ChargingState::Discharging)),
                 std::vector<Alert>{Alert::BatteryLow});
    }

    void staysQuietOnTheCable()
    {
        BatteryAlerts alerts;
        QCOMPARE(alerts.update(Mouse, device(2, QStringLiteral("M"), 10, BatteryLevel::Low, ChargingState::Charging, true)), std::vector<Alert>{});
        QCOMPARE(alerts.update(Mouse, device(2, QStringLiteral("M"), 3, BatteryLevel::Critical, ChargingState::Unknown, true)), std::vector<Alert>{});
    }

    void ignoresUnreachableDevices()
    {
        BatteryAlerts alerts;
        DeviceState asleep = device(2, QStringLiteral("M"), 10, BatteryLevel::Low, ChargingState::Discharging);
        asleep.status = DeviceStatus::Unreachable;
        QCOMPARE(alerts.update(Mouse, asleep), std::vector<Alert>{});
    }

    void reportsACompleteCharge()
    {
        BatteryAlerts alerts;
        QCOMPARE(alerts.update(Mouse, device(2, QStringLiteral("M"), 95, BatteryLevel::Full, ChargingState::Charging, true)), std::vector<Alert>{});
        QCOMPARE(alerts.update(Mouse, device(2, QStringLiteral("M"), 100, BatteryLevel::Full, ChargingState::Complete, true)),
                 std::vector<Alert>{Alert::ChargeComplete});
        QCOMPARE(alerts.update(Mouse, device(2, QStringLiteral("M"), 100, BatteryLevel::Full, ChargingState::Complete, true)), std::vector<Alert>{});
    }

    void ignoresTheBouncingCable()
    {
        // The sequence seen on the desk when the MX Keys S cable was plugged
        // in: charging, not charging, charging within 350 ms.
        BatteryAlerts alerts;
        const QString keyboard = QStringLiteral("/dev/hidraw3#1");
        QCOMPARE(alerts.update(keyboard, device(1, QStringLiteral("K"), 100, BatteryLevel::Full, ChargingState::Discharging)), std::vector<Alert>{});
        QCOMPARE(alerts.update(keyboard, device(1, QStringLiteral("K"), 100, BatteryLevel::Full, ChargingState::Charging, true)), std::vector<Alert>{});
        QCOMPARE(alerts.update(keyboard, device(1, QStringLiteral("K"), 100, BatteryLevel::Full, ChargingState::Discharging)), std::vector<Alert>{});
        QCOMPARE(alerts.update(keyboard, device(1, QStringLiteral("K"), 100, BatteryLevel::Full, ChargingState::Charging, true)), std::vector<Alert>{});
        // A complete charge seen first, without charging before it, is not news.
        BatteryAlerts fresh;
        QCOMPARE(fresh.update(keyboard, device(1, QStringLiteral("K"), 100, BatteryLevel::Full, ChargingState::Complete, true)), std::vector<Alert>{});
    }

    void alertsOnTheDemoCapture()
    {
        QFile file(QStringLiteral(CAPTURE_DIR "/notifications-demo.capture"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        auto replay = std::make_unique<ReplayChannel>(parseCapture(file.readAll()).value());
        auto *channel = replay.get();
        ReceiverSession session(std::move(replay));
        session.setTimeout(std::chrono::milliseconds(50));

        BatteryAlerts alerts;
        std::vector<Alert> raised;
        connect(&session, &ReceiverSession::deviceChanged, this, [&](const DeviceState &state) {
            const auto now = alerts.update(QString::number(state.index), state);
            raised.insert(raised.end(), now.begin(), now.end());
        });
        session.start();
        QTRY_VERIFY_WITH_TIMEOUT(channel->isAtEnd() && raised.size() == 3, 5000);
        QCOMPARE(raised, (std::vector<Alert>{Alert::BatteryLow, Alert::BatteryCritical, Alert::ChargeComplete}));
        QVERIFY(!channel->mismatch());
    }

    void showsTheWeakestDevice()
    {
        const auto look = TrayLook::forDevices({
            device(1, QStringLiteral("MX Keys S"), 100, BatteryLevel::Full, ChargingState::Discharging),
            device(2, QStringLiteral("MX Master 3S"), 42, BatteryLevel::Good, ChargingState::Discharging),
        });
        QCOMPARE(look.status, TrayLook::Status::Active);
        QCOMPARE(look.iconName, QStringLiteral("battery-040-symbolic"));
        QCOMPARE(look.toolTip, QStringLiteral("MX Keys S: 100 %<br/>MX Master 3S: 42 %"));
    }

    void showsChargingAndAttention()
    {
        const auto charging = TrayLook::forDevices({device(1, QStringLiteral("K"), 75, BatteryLevel::Good, ChargingState::Charging, true)});
        QCOMPARE(charging.iconName, QStringLiteral("battery-080-charging-symbolic"));

        const auto critical = TrayLook::forDevices({
            device(1, QStringLiteral("K"), 100, BatteryLevel::Full, ChargingState::Discharging),
            device(2, QStringLiteral("M"), 3, BatteryLevel::Critical, ChargingState::Discharging),
        });
        QCOMPARE(critical.status, TrayLook::Status::NeedsAttention);
        QCOMPARE(critical.iconName, QStringLiteral("battery-000-symbolic"));
    }

    void skipsDevicesThatDoNotAnswer()
    {
        DeviceState asleep = device(1, QStringLiteral("K"), 5, BatteryLevel::Critical, ChargingState::Discharging);
        asleep.status = DeviceStatus::Unreachable;
        const auto look = TrayLook::forDevices({asleep, device(2, QStringLiteral("M"), 90, BatteryLevel::Full, ChargingState::Discharging)});
        QCOMPARE(look.status, TrayLook::Status::Active);
        QCOMPARE(look.iconName, QStringLiteral("battery-090-symbolic"));
        QVERIFY(look.toolTip.startsWith(QStringLiteral("K: Not connected")));
    }

    void escapesDeviceNames()
    {
        const auto look = TrayLook::forDevices({device(1, QStringLiteral("<b>Evil</b>"), 50, BatteryLevel::Good, ChargingState::Discharging)});
        QVERIFY(look.toolTip.startsWith(QStringLiteral("&lt;b&gt;Evil&lt;/b&gt;: ")));
    }

    void showsTheAppWithoutDevices()
    {
        const auto look = TrayLook::forDevices({});
        QCOMPARE(look.status, TrayLook::Status::Passive);
        QCOMPARE(look.iconName, QStringLiteral("de.tordenskjold.hidsight"));
    }
};

QTEST_GUILESS_MAIN(AlertsTest)

#include "alertstest.moc"
