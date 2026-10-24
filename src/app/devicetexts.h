// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "hidpp/receiversession.h"

#include <QString>

// The translated words and Breeze icon names the UI shows for a device, so
// QML only places them.
namespace DeviceTexts
{

QString kindText(hidpp::DeviceKind kind);
QString kindIconName(hidpp::DeviceKind kind);

QString batteryText(const hidpp::Battery &battery);
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

} // namespace DeviceTexts
