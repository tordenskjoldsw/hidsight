// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "hidpp/receiversession.h"

#include <QString>
#include <QVariantList>

#include <optional>

// The translated words and Breeze icon names the UI shows for a device, so
// QML only places them.
namespace DeviceTexts
{

QString kindText(hidpp::DeviceKind kind);
QString kindIconName(hidpp::DeviceKind kind);
// Hidsight's own drawing of the kind of device ("keyboard", "mouse"), or
// empty for a kind without one, which shows its icon instead.
QString illustrationName(hidpp::DeviceKind kind);

QString batteryText(const hidpp::Battery &battery);
// The charge in percent, from the percentage or from the reported level;
// nothing when the device tells neither.
std::optional<int> chargeEstimate(const hidpp::Battery &battery);
// How the battery is drawn: "charging", "critical", "low", "normal", or
// "unknown" for a device without a reading.
QString batteryTone(const hidpp::DeviceState &state);
// Short states for chips, each {text, iconName}, at most two: how the
// battery charges, or the cable and how low it is; not connected.
QVariantList statusChips(const hidpp::DeviceState &state);
QString batteryIconName(const std::optional<hidpp::Battery> &battery);
QString chargingText(hidpp::ChargingState state);
QString levelText(hidpp::BatteryLevel level);

// "Not connected" for a device that does not answer; the battery for one
// that does.
QString statusText(const hidpp::DeviceState &state);
// "15 minutes ago", "Today at 3:00 PM"; empty before the first answer.
QString lastSeenText(const QDateTime &lastSeen);

QString firmwareTypeText(hidpp::FirmwareType type);

// A feature in plain words, for example "Battery level and charging".
QString featureTitle(std::uint16_t featureId);
// Logitech's name and the id, for example "UnifiedBattery, 0x1004".
QString featureDetail(std::uint16_t featureId);

// The receiver by its USB product id, for example "046d:c548".
QString receiverText(const QString &receiverId);
// Where the device is paired, for example "Slot 1 · Logi Bolt receiver".
QString slotText(const QString &receiverId, std::uint8_t index);

} // namespace DeviceTexts
