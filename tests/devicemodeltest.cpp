// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "devicemodel.h"

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTest>

using namespace hidpp;

namespace
{

DeviceState keyboard()
{
    DeviceState state;
    state.index = 1;
    state.status = DeviceStatus::Online;
    state.name = DeviceName{QStringLiteral("MX Keys S"), DeviceKind::Keyboard};
    Battery battery;
    battery.percentage = 100;
    battery.level = BatteryLevel::Full;
    battery.charging = ChargingState::Complete;
    battery.externalPower = true;
    state.battery = battery;
    return state;
}

DeviceState mouse()
{
    DeviceState state;
    state.index = 2;
    state.status = DeviceStatus::Online;
    state.name = DeviceName{QStringLiteral("MX Master 3S"), DeviceKind::Mouse};
    state.dpi = AdjustableDpi{{1000}};
    return state;
}

} // namespace

class DeviceModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void keepsTheRowsOrdered()
    {
        DeviceModel model;
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        QSignalSpy count(&model, &DeviceModel::countChanged);

        model.updateDevice(QStringLiteral("/dev/hidraw7"), mouse());
        model.updateDevice(QStringLiteral("/dev/hidraw3"), mouse());
        model.updateDevice(QStringLiteral("/dev/hidraw3"), keyboard());
        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(count.size(), 3);
        QCOMPARE(model.index(0).data(DeviceModel::NameRole).toString(), QStringLiteral("MX Keys S"));
        QCOMPARE(model.index(1).data(DeviceModel::ReceiverRole).toString(), QStringLiteral("/dev/hidraw3"));
        QCOMPARE(model.index(2).data(DeviceModel::ReceiverRole).toString(), QStringLiteral("/dev/hidraw7"));
    }

    void exposesKeysAndNumbers()
    {
        DeviceModel model;
        model.updateDevice(QStringLiteral("/dev/hidraw3"), keyboard());
        model.updateDevice(QStringLiteral("/dev/hidraw3"), mouse());

        const QModelIndex first = model.index(0);
        QCOMPARE(first.data(DeviceModel::KindRole).toString(), QStringLiteral("keyboard"));
        QCOMPARE(first.data(DeviceModel::StatusRole).toString(), QStringLiteral("online"));
        QCOMPARE(first.data(DeviceModel::BatteryPercentageRole).toInt(), 100);
        QCOMPARE(first.data(DeviceModel::BatteryLevelRole).toString(), QStringLiteral("full"));
        QCOMPARE(first.data(DeviceModel::ChargingRole).toString(), QStringLiteral("complete"));
        QCOMPARE(first.data(DeviceModel::ExternalPowerRole).toBool(), true);
        QCOMPARE(first.data(DeviceModel::DpiRole).toInt(), 0);

        const QModelIndex second = model.index(1);
        QCOMPARE(second.data(DeviceModel::KindRole).toString(), QStringLiteral("mouse"));
        QCOMPARE(second.data(DeviceModel::BatteryPercentageRole).toInt(), -1);
        QCOMPARE(second.data(DeviceModel::ChargingRole).toString(), QStringLiteral("unknown"));
        QCOMPARE(second.data(DeviceModel::DpiRole).toInt(), 1000);
        QCOMPARE(model.roleNames().value(DeviceModel::BatteryPercentageRole), QByteArray("batteryPercentage"));
    }

    void updatesAndRemoves()
    {
        DeviceModel model;
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        model.updateDevice(QStringLiteral("/dev/hidraw3"), keyboard());
        model.updateDevice(QStringLiteral("/dev/hidraw3"), mouse());
        model.updateDevice(QStringLiteral("/dev/hidraw7"), keyboard());

        QSignalSpy changed(&model, &DeviceModel::dataChanged);
        DeviceState asleep = keyboard();
        asleep.status = DeviceStatus::Unreachable;
        model.updateDevice(QStringLiteral("/dev/hidraw3"), asleep);
        QCOMPARE(changed.size(), 1);
        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(model.index(0).data(DeviceModel::StatusRole).toString(), QStringLiteral("unreachable"));

        model.removeDevice(QStringLiteral("/dev/hidraw3"), 2);
        QCOMPARE(model.rowCount(), 2);
        model.removeReceiver(QStringLiteral("/dev/hidraw3"));
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.index(0).data(DeviceModel::ReceiverRole).toString(), QStringLiteral("/dev/hidraw7"));
        model.removeReceiver(QStringLiteral("/dev/hidraw9"));
        QCOMPARE(model.rowCount(), 1);
    }
};

QTEST_GUILESS_MAIN(DeviceModelTest)

#include "devicemodeltest.moc"
