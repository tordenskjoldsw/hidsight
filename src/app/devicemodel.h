// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "hidpp/receiversession.h"

#include <QAbstractListModel>
#include <QTimer>

#include <vector>

// The devices of all receivers, one row each. QML reads translated texts,
// icon names, stable keys such as "keyboard" and numbers; never reports or
// feature indices.
class DeviceModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        KindRole,
        KindTextRole,
        IconNameRole,
        IllustrationRole,
        SlotTextRole,
        StatusRole,
        StatusTextRole,
        OnlineRole,
        BatteryPercentageRole,
        BatteryLevelRole,
        BatteryLevelTextRole,
        ChargingRole,
        ChargingTextRole,
        ExternalPowerRole,
        BatteryTextRole,
        BatteryIconNameRole,
        // 0 to 1 for the ring; -1 without a reading.
        BatteryFractionRole,
        BatteryToneRole,
        StatusChipsRole,
        LastSeenTextRole,
        FirmwareRole,
        // The version of the main application firmware alone.
        FirmwareVersionRole,
        DpiRole,
        ReportRateRole,
        ProtocolRole,
        FeaturesRole,
        CompleteRole,
        ReceiverRole,
        ReceiverTextRole,
        KeyRole,
    };
    Q_ENUM(Role)

    explicit DeviceModel(QObject *parent = nullptr);

    int count() const;
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // The row of the device with this key, or -1.
    int rowOf(const QString &key) const;
    // What identifies a device for QML and the notifications.
    static QString keyFor(const QString &receiver, std::uint8_t index);
    // The devices in row order, for the tray.
    std::vector<hidpp::DeviceState> states() const;
    QStringList keysOf(const QString &receiver) const;
    Q_INVOKABLE QString keyAt(int row) const;

    void updateDevice(const QString &receiver, const QString &receiverId, const hidpp::DeviceState &state);
    void removeDevice(const QString &receiver, std::uint8_t index);
    void removeReceiver(const QString &receiver);

Q_SIGNALS:
    void countChanged();

private:
    struct Row {
        QString receiver;
        QString receiverId;
        hidpp::DeviceState state;
    };

    std::vector<Row>::iterator find(const QString &receiver, std::uint8_t index);

    std::vector<Row> m_rows;
    // "5 minutes ago" ages; a minute is the finest step it shows.
    QTimer m_clock;
};
