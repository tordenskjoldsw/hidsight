// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "device.h"

#include <QString>

namespace hidpp
{

// The values of getDeviceType in Logitech's document. Verified on the desk:
// Keyboard (MX Keys S) and Mouse (MX Master 3S).
enum class DeviceKind : std::uint8_t {
    Keyboard = 0,
    RemoteControl = 1,
    Numpad = 2,
    Mouse = 3,
    Trackpad = 4,
    Trackball = 5,
    Presenter = 6,
    Receiver = 7,
    Headset = 8,
    Webcam = 9,
    SteeringWheel = 10,
    Joystick = 11,
    Gamepad = 12,
    Dock = 13,
    Speaker = 14,
    Microphone = 15,
    IlluminationLight = 16,
    ProgrammableController = 17,
    CarSimPedals = 18,
    Adapter = 19,
    Unknown = 0xFF,
};

// Feature 0x0005: the marketing name and the kind of device.
struct DeviceName {
    static constexpr std::uint16_t FeatureId = 0x0005;
    // Longer than any name Logitech ships; bounds what a device can make
    // Hidsight request.
    static constexpr std::uint8_t MaxLength = 64;

    QString name;
    DeviceKind kind = DeviceKind::Unknown;

    static void read(const Device &device, std::uint8_t featureIndex, Callback<DeviceName> done);

    static Result<std::uint8_t> parseLength(std::span<const std::uint8_t> params);
    static Result<DeviceKind> parseKind(std::span<const std::uint8_t> params);
    // Printable text from the bytes up to the first NUL; control characters
    // are dropped and invalid UTF-8 is replaced.
    static QString decodeName(QByteArrayView bytes);
};

} // namespace hidpp
