// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "battery.h"
#include "device.h"
#include "devicefwversion.h"
#include "devicename.h"
#include "pointer.h"

#include <optional>

namespace hidpp
{

// The battery from whichever battery feature the device has, Unified Battery
// first.
struct Battery {
    enum class Source : std::uint8_t {
        UnifiedBattery,
        BatteryStatus,
        BatteryVoltage,
    };

    Source source = Source::UnifiedBattery;
    std::optional<std::uint8_t> percentage;
    std::optional<std::uint16_t> millivolts;
    BatteryLevel level = BatteryLevel::Unknown;
    ChargingState charging = ChargingState::Unknown;
    std::optional<bool> externalPower;
};

// Everything Hidsight reads from one device. A feature the device lacks is
// empty; one it has but did not answer for holds the error.
struct DeviceInfo {
    std::uint8_t index = 0;
    ProtocolVersion protocol;
    FeatureTable features;
    std::optional<Result<DeviceName>> name;
    std::optional<Result<DeviceFwVersion>> firmware;
    std::optional<Result<Battery>> battery;
    std::optional<Result<AdjustableDpi>> dpi;
    std::optional<Result<ReportRate>> reportRate;
};

// Pings the device, reads its feature table and then every feature Hidsight
// knows. Fails only when the ping or the feature table fails.
void probeDevice(const Device &device, Callback<DeviceInfo> done);

} // namespace hidpp
