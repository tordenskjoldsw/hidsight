// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "devicemodel.h"

#include "hidpp/keys.h"

#include <algorithm>

using namespace Qt::StringLiterals;

namespace
{

QString mainFirmware(const hidpp::DeviceState &state)
{
    if (!state.firmware) {
        return {};
    }
    for (const hidpp::FirmwareEntry &entry : state.firmware->entries) {
        if (entry.type == hidpp::FirmwareType::MainApplication) {
            return entry.version();
        }
    }
    return {};
}

} // namespace

int DeviceModel::count() const
{
    return int(m_rows.size());
}

int DeviceModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant DeviceModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return {};
    }
    const Row &row = m_rows[std::size_t(index.row())];
    const hidpp::DeviceState &state = row.state;
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return state.name ? state.name->name : QString();
    case KindRole:
        return QString(hidpp::key(state.name ? state.name->kind : hidpp::DeviceKind::Unknown));
    case StatusRole:
        return QString(hidpp::key(state.status));
    case BatteryPercentageRole:
        return state.battery && state.battery->percentage ? int(*state.battery->percentage) : -1;
    case BatteryLevelRole:
        return QString(hidpp::key(state.battery ? state.battery->level : hidpp::BatteryLevel::Unknown));
    case ChargingRole:
        return QString(hidpp::key(state.battery ? state.battery->charging : hidpp::ChargingState::Unknown));
    case ExternalPowerRole:
        return state.battery && state.battery->externalPower.value_or(false);
    case FirmwareRole:
        return mainFirmware(state);
    case DpiRole:
        return state.dpi && !state.dpi->sensorDpi.empty() ? int(state.dpi->sensorDpi.front()) : 0;
    case ReportRateRole:
        return state.reportRate ? int(state.reportRate->milliseconds) : 0;
    case CompleteRole:
        return state.complete;
    case ReceiverRole:
        return row.receiver;
    default:
        return {};
    }
}

QHash<int, QByteArray> DeviceModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {KindRole, "kind"},
        {StatusRole, "status"},
        {BatteryPercentageRole, "batteryPercentage"},
        {BatteryLevelRole, "batteryLevel"},
        {ChargingRole, "charging"},
        {ExternalPowerRole, "externalPower"},
        {FirmwareRole, "firmware"},
        {DpiRole, "dpi"},
        {ReportRateRole, "reportRate"},
        {CompleteRole, "complete"},
        {ReceiverRole, "receiver"},
    };
}

void DeviceModel::updateDevice(const QString &receiver, const hidpp::DeviceState &state)
{
    const auto it = find(receiver, state.index);
    if (it != m_rows.end()) {
        it->state = state;
        const QModelIndex changed = index(int(it - m_rows.begin()));
        Q_EMIT dataChanged(changed, changed);
        return;
    }
    // Ordered by receiver and device index, so the list does not jump.
    const auto position = std::ranges::find_if(m_rows, [&](const Row &row) {
        return row.receiver > receiver || (row.receiver == receiver && row.state.index > state.index);
    });
    const int rowNumber = int(position - m_rows.begin());
    beginInsertRows({}, rowNumber, rowNumber);
    m_rows.insert(position, Row{receiver, state});
    endInsertRows();
    Q_EMIT countChanged();
}

void DeviceModel::removeDevice(const QString &receiver, std::uint8_t index)
{
    const auto it = find(receiver, index);
    if (it == m_rows.end()) {
        return;
    }
    const int rowNumber = int(it - m_rows.begin());
    beginRemoveRows({}, rowNumber, rowNumber);
    m_rows.erase(it);
    endRemoveRows();
    Q_EMIT countChanged();
}

void DeviceModel::removeReceiver(const QString &receiver)
{
    const auto first = std::ranges::find(m_rows, receiver, &Row::receiver);
    if (first == m_rows.end()) {
        return;
    }
    const auto last = std::find_if(first, m_rows.end(), [&](const Row &row) {
        return row.receiver != receiver;
    });
    beginRemoveRows({}, int(first - m_rows.begin()), int(last - m_rows.begin()) - 1);
    m_rows.erase(first, last);
    endRemoveRows();
    Q_EMIT countChanged();
}

std::vector<DeviceModel::Row>::iterator DeviceModel::find(const QString &receiver, std::uint8_t index)
{
    return std::ranges::find_if(m_rows, [&](const Row &row) {
        return row.receiver == receiver && row.state.index == index;
    });
}
