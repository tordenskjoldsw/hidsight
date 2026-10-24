// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "hidpp/receiversession.h"

#include <QAbstractListModel>

#include <vector>

// The devices of all receivers, one row each. QML reads names, stable keys
// such as "keyboard" or "discharging" and numbers; never reports or feature
// indices.
class DeviceModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        KindRole,
        StatusRole,
        BatteryPercentageRole,
        BatteryLevelRole,
        ChargingRole,
        ExternalPowerRole,
        FirmwareRole,
        DpiRole,
        ReportRateRole,
        CompleteRole,
        ReceiverRole,
    };
    Q_ENUM(Role)

    using QAbstractListModel::QAbstractListModel;

    int count() const;
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void updateDevice(const QString &receiver, const hidpp::DeviceState &state);
    void removeDevice(const QString &receiver, std::uint8_t index);
    void removeReceiver(const QString &receiver);

Q_SIGNALS:
    void countChanged();

private:
    struct Row {
        QString receiver;
        hidpp::DeviceState state;
    };

    std::vector<Row>::iterator find(const QString &receiver, std::uint8_t index);

    std::vector<Row> m_rows;
};
