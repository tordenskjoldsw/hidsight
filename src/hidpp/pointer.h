// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "device.h"

#include <vector>

namespace hidpp
{

// Feature 0x2201: the current resolution of each sensor. Verified on the
// desk: one sensor at 1000 dpi on the MX Master 3S. Only the getters exist
// here; Hidsight changes no setting.
struct AdjustableDpi {
    static constexpr std::uint16_t FeatureId = 0x2201;
    static constexpr std::uint8_t MaxSensors = 8;

    std::vector<std::uint16_t> sensorDpi;

    static void read(const Device &device, std::uint8_t featureIndex, Callback<AdjustableDpi> done);

    static Result<std::uint8_t> parseSensorCount(std::span<const std::uint8_t> params);
    static Result<std::uint16_t> parseSensorDpi(std::uint8_t sensor, std::span<const std::uint8_t> params);
};

// Feature 0x8060: the interval between reports. No device on the desk has
// it; the layout follows Logitech's document, unverified.
struct ReportRate {
    static constexpr std::uint16_t FeatureId = 0x8060;

    std::uint8_t milliseconds = 0;

    static void read(const Device &device, std::uint8_t featureIndex, Callback<ReportRate> done);
    static Result<ReportRate> parse(std::span<const std::uint8_t> params);
};

} // namespace hidpp
