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
