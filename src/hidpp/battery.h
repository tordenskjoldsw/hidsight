// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "device.h"

#include <optional>

namespace hidpp
{

enum class BatteryLevel : std::uint8_t {
    Unknown,
    Critical,
    Low,
    Good,
    Full,
};

enum class ChargingState : std::uint8_t {
    Unknown,
    Discharging,
    Charging,
    ChargingSlowly,
    Complete,
    Error,
};

// Feature 0x1004. Verified on the desk: percentage, level full, discharging
// and charge complete, external power.
struct UnifiedBattery {
    static constexpr std::uint16_t FeatureId = 0x1004;

    std::optional<std::uint8_t> percentage;
    BatteryLevel level = BatteryLevel::Unknown;
    ChargingState charging = ChargingState::Unknown;
    bool externalPower = false;

    static void read(const Device &device, std::uint8_t featureIndex, Callback<UnifiedBattery> done);

    // Whether getStatus reports a percentage.
    static Result<bool> parseCapabilities(std::span<const std::uint8_t> params);
    static Result<UnifiedBattery> parseStatus(std::span<const std::uint8_t> params, bool hasPercentage);
};

// Feature 0x1000, the fallback for devices without Unified Battery. No device
// on the desk has it; the layout follows Logitech's document, unverified.
struct BatteryStatus {
    static constexpr std::uint16_t FeatureId = 0x1000;

    std::uint8_t percentage = 0;
    ChargingState charging = ChargingState::Unknown;

    static void read(const Device &device, std::uint8_t featureIndex, Callback<BatteryStatus> done);
    static Result<BatteryStatus> parse(std::span<const std::uint8_t> params);
};

// Feature 0x1001, the last fallback. No device on the desk has it; the layout
// follows Logitech's document, unverified.
struct BatteryVoltage {
    static constexpr std::uint16_t FeatureId = 0x1001;

    std::uint16_t millivolts = 0;

    static void read(const Device &device, std::uint8_t featureIndex, Callback<BatteryVoltage> done);
    static Result<BatteryVoltage> parse(std::span<const std::uint8_t> params);
};

} // namespace hidpp
