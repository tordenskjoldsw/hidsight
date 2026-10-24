// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "devicemodel.h"
#include "devicetexts.h"

#include "hidpp/keys.h"

#include <KLocalizedString>

#include <algorithm>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace
{

QVariantList firmware(const hidpp::DeviceState &state)
{
    QVariantList entries;
    if (!state.firmware) {
        return entries;
    }
    for (const hidpp::FirmwareEntry &entry : state.firmware->entries) {
        entries.append(QVariantMap{{u"label"_s, DeviceTexts::firmwareTypeText(entry.type)}, {u"version"_s, entry.version()}});
    }
    return entries;
}

QVariantList features(const hidpp::DeviceState &state)
{
    QVariantList entries;
    for (const hidpp::FeatureEntry &feature : state.features.entries()) {
        if (feature.isVisible()) {
            entries.append(QVariantMap{{u"title"_s, DeviceTexts::featureTitle(feature.id)}, {u"detail"_s, DeviceTexts::featureDetail(feature.id)}});
        }
    }
    return entries;
}

} // namespace

DeviceModel::DeviceModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_clock.setInterval(1min);
    connect(&m_clock, &QTimer::timeout, this, [this] {
        if (!m_rows.empty()) {
            Q_EMIT dataChanged(index(0), index(count() - 1), {LastSeenTextRole});
        }
    });
    m_clock.start();
}

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
    const hidpp::DeviceKind kind = state.name ? state.name->kind : hidpp::DeviceKind::Unknown;
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return state.name ? state.name->name : i18nc("@info device without a name yet", "Device %1", state.index);
    case KindRole:
        return QString(hidpp::key(kind));
    case KindTextRole:
        return DeviceTexts::kindText(kind);
    case IconNameRole:
        return DeviceTexts::kindIconName(kind);
    case StatusRole:
        return QString(hidpp::key(state.status));
    case StatusTextRole:
        return DeviceTexts::statusText(state);
    case OnlineRole:
        return state.status == hidpp::DeviceStatus::Online;
    case BatteryPercentageRole:
        return state.battery && state.battery->percentage ? int(*state.battery->percentage) : -1;
    case BatteryLevelRole:
        return QString(hidpp::key(state.battery ? state.battery->level : hidpp::BatteryLevel::Unknown));
    case BatteryLevelTextRole:
        return state.battery ? DeviceTexts::levelText(state.battery->level) : QString();
    case ChargingRole:
        return QString(hidpp::key(state.battery ? state.battery->charging : hidpp::ChargingState::Unknown));
    case ChargingTextRole:
        return state.battery ? DeviceTexts::chargingText(state.battery->charging) : QString();
    case ExternalPowerRole:
        return state.battery && state.battery->externalPower.value_or(false);
    case BatteryTextRole:
        return state.battery ? DeviceTexts::batteryText(*state.battery) : QString();
    case BatteryIconNameRole:
        return DeviceTexts::batteryIconName(state.battery);
    case LastSeenTextRole:
        return DeviceTexts::lastSeenText(state.lastSeen);
    case FirmwareRole:
        return firmware(state);
    case DpiRole:
        return state.dpi && !state.dpi->sensorDpi.empty() ? int(state.dpi->sensorDpi.front()) : 0;
    case ReportRateRole:
        return state.reportRate ? int(state.reportRate->milliseconds) : 0;
    case ProtocolRole:
        return state.protocol.major > 0 ? u"%1.%2"_s.arg(state.protocol.major).arg(state.protocol.minor) : QString();
    case FeaturesRole:
        return features(state);
    case CompleteRole:
        return state.complete;
    case ReceiverRole:
        return row.receiver;
    case ReceiverTextRole:
        return DeviceTexts::receiverText(row.receiverId);
    case KeyRole:
        return keyFor(row.receiver, state.index);
    default:
        return {};
    }
}

QHash<int, QByteArray> DeviceModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {KindRole, "kind"},
        {KindTextRole, "kindText"},
        {IconNameRole, "iconName"},
        {StatusRole, "status"},
        {StatusTextRole, "statusText"},
        {OnlineRole, "online"},
        {BatteryPercentageRole, "batteryPercentage"},
        {BatteryLevelRole, "batteryLevel"},
        {BatteryLevelTextRole, "batteryLevelText"},
        {ChargingRole, "charging"},
        {ChargingTextRole, "chargingText"},
        {ExternalPowerRole, "externalPower"},
        {BatteryTextRole, "batteryText"},
        {BatteryIconNameRole, "batteryIconName"},
        {LastSeenTextRole, "lastSeenText"},
        {FirmwareRole, "firmware"},
        {DpiRole, "dpi"},
        {ReportRateRole, "reportRate"},
        {ProtocolRole, "protocol"},
        {FeaturesRole, "features"},
        {CompleteRole, "complete"},
        {ReceiverRole, "receiver"},
        {ReceiverTextRole, "receiverText"},
        {KeyRole, "key"},
    };
}

int DeviceModel::rowOf(const QString &deviceKey) const
{
    for (std::size_t row = 0; row < m_rows.size(); ++row) {
        if (keyFor(m_rows[row].receiver, m_rows[row].state.index) == deviceKey) {
            return int(row);
        }
    }
    return -1;
}

QString DeviceModel::keyFor(const QString &receiver, std::uint8_t index)
{
    return u"%1#%2"_s.arg(receiver).arg(index);
}

std::vector<hidpp::DeviceState> DeviceModel::states() const
{
    std::vector<hidpp::DeviceState> states;
    states.reserve(m_rows.size());
    for (const Row &row : m_rows) {
        states.push_back(row.state);
    }
    return states;
}

QStringList DeviceModel::keysOf(const QString &receiver) const
{
    QStringList keys;
    for (const Row &row : m_rows) {
        if (row.receiver == receiver) {
            keys << keyFor(row.receiver, row.state.index);
        }
    }
    return keys;
}

void DeviceModel::updateDevice(const QString &receiver, const QString &receiverId, const hidpp::DeviceState &state)
{
    const auto it = find(receiver, state.index);
    if (it != m_rows.end()) {
        it->receiverId = receiverId;
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
    m_rows.insert(position, Row{receiver, receiverId, state});
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
