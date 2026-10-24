// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "battery.h"

namespace hidpp
{

namespace
{

enum UnifiedBatteryFunction : std::uint8_t {
    GetCapabilities = 0,
    GetStatus = 1,
};

enum BatteryStatusFunction : std::uint8_t {
    GetBatteryLevelStatus = 0,
};

enum BatteryVoltageFunction : std::uint8_t {
    GetBatteryVoltage = 0,
};

constexpr std::uint8_t PercentageCapability = 0x02;
constexpr std::uint8_t ExternalPowerFlag = 0x01;
constexpr std::uint8_t MaxPercentage = 100;

BatteryLevel unifiedLevel(std::uint8_t bits)
{
    if (bits & 0x01) {
        return BatteryLevel::Critical;
    }
    if (bits & 0x02) {
        return BatteryLevel::Low;
    }
    if (bits & 0x04) {
        return BatteryLevel::Good;
    }
    if (bits & 0x08) {
        return BatteryLevel::Full;
    }
    return BatteryLevel::Unknown;
}

ChargingState unifiedCharging(std::uint8_t raw)
{
    switch (raw) {
    case 0:
        return ChargingState::Discharging;
    case 1:
        return ChargingState::Charging;
    case 2:
        return ChargingState::ChargingSlowly;
    case 3:
        return ChargingState::Complete;
    case 4:
        return ChargingState::Error;
    default:
        return ChargingState::Unknown;
    }
}

ChargingState batteryStatusCharging(std::uint8_t raw)
{
    switch (raw) {
    case 0:
        return ChargingState::Discharging;
    case 1:
    case 2:
        return ChargingState::Charging;
    case 3:
        return ChargingState::Complete;
    case 4:
        return ChargingState::ChargingSlowly;
    case 5:
    case 6:
    case 7:
        return ChargingState::Error;
    default:
        return ChargingState::Unknown;
    }
}

} // namespace

void UnifiedBattery::read(const Device &device, std::uint8_t featureIndex, Callback<UnifiedBattery> done)
{
    device.request<bool>(featureIndex, GetCapabilities, {}, &UnifiedBattery::parseCapabilities, [device, featureIndex, done](Result<bool> hasPercentage) {
        if (!hasPercentage) {
            done(hasPercentage.error());
            return;
        }
        device.request<UnifiedBattery>(
            featureIndex,
            GetStatus,
            {},
            [hasPercentage = hasPercentage.value()](std::span<const std::uint8_t> params) {
                return UnifiedBattery::parseStatus(params, hasPercentage);
            },
            done);
    });
}

Result<bool> UnifiedBattery::parseCapabilities(std::span<const std::uint8_t> params)
{
    if (params.size() < 2) {
        return Error{ErrorKind::Malformed, 0};
    }
    return (params[1] & PercentageCapability) != 0;
}

Result<UnifiedBattery> UnifiedBattery::parseStatus(std::span<const std::uint8_t> params, bool hasPercentage)
{
    if (params.size() < 4 || (hasPercentage && params[0] > MaxPercentage)) {
        return Error{ErrorKind::Malformed, 0};
    }
    UnifiedBattery status;
    if (hasPercentage) {
        status.percentage = params[0];
    }
    status.level = unifiedLevel(params[1]);
    status.charging = unifiedCharging(params[2]);
    status.externalPower = (params[3] & ExternalPowerFlag) != 0;
    return status;
}

void BatteryStatus::read(const Device &device, std::uint8_t featureIndex, Callback<BatteryStatus> done)
{
    device.request<BatteryStatus>(featureIndex, GetBatteryLevelStatus, {}, &BatteryStatus::parse, std::move(done));
}

Result<BatteryStatus> BatteryStatus::parse(std::span<const std::uint8_t> params)
{
    if (params.size() < 3 || params[0] > MaxPercentage) {
        return Error{ErrorKind::Malformed, 0};
    }
    return BatteryStatus{params[0], batteryStatusCharging(params[2])};
}

void BatteryVoltage::read(const Device &device, std::uint8_t featureIndex, Callback<BatteryVoltage> done)
{
    device.request<BatteryVoltage>(featureIndex, GetBatteryVoltage, {}, &BatteryVoltage::parse, std::move(done));
}

Result<BatteryVoltage> BatteryVoltage::parse(std::span<const std::uint8_t> params)
{
    if (params.size() < 2) {
        return Error{ErrorKind::Malformed, 0};
    }
    return BatteryVoltage{static_cast<std::uint16_t>((params[0] << 8) | params[1])};
}

} // namespace hidpp
