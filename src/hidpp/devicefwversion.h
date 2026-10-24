// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "device.h"

#include <QString>

#include <vector>

namespace hidpp
{

// Verified on the desk: MainApplication, Bootloader and Softdevice.
enum class FirmwareType : std::uint8_t {
    MainApplication = 0,
    Bootloader = 1,
    Hardware = 2,
    Touchpad = 3,
    OpticalSensor = 4,
    Softdevice = 5,
    RfCompanion = 6,
    Other = 0xFF,
};

struct FirmwareEntry {
    FirmwareType type = FirmwareType::Other;
    QString prefix;
    std::uint8_t number = 0;
    std::uint8_t revision = 0;
    std::uint16_t build = 0;

    // As the spike printed it, for example "RBK81.02.B0023"; whether
    // Logitech prints the build number in decimal is unverified.
    QString version() const;
};

// Feature 0x0003: the firmware entities of the device.
struct DeviceFwVersion {
    static constexpr std::uint16_t FeatureId = 0x0003;
    static constexpr std::uint8_t MaxEntities = 16;

    std::vector<FirmwareEntry> entries;

    static void read(const Device &device, std::uint8_t featureIndex, Callback<DeviceFwVersion> done);

    static Result<std::uint8_t> parseEntityCount(std::span<const std::uint8_t> params);
    static Result<FirmwareEntry> parseEntry(std::span<const std::uint8_t> params);
};

} // namespace hidpp
