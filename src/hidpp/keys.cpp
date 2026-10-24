// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "keys.h"

using namespace Qt::StringLiterals;

namespace hidpp
{

QLatin1StringView key(DeviceKind kind)
{
    switch (kind) {
    case DeviceKind::Keyboard:
        return "keyboard"_L1;
    case DeviceKind::RemoteControl:
        return "remoteControl"_L1;
    case DeviceKind::Numpad:
        return "numpad"_L1;
    case DeviceKind::Mouse:
        return "mouse"_L1;
    case DeviceKind::Trackpad:
        return "trackpad"_L1;
    case DeviceKind::Trackball:
        return "trackball"_L1;
    case DeviceKind::Presenter:
        return "presenter"_L1;
    case DeviceKind::Receiver:
        return "receiver"_L1;
    case DeviceKind::Headset:
        return "headset"_L1;
    case DeviceKind::Webcam:
        return "webcam"_L1;
    case DeviceKind::SteeringWheel:
        return "steeringWheel"_L1;
    case DeviceKind::Joystick:
        return "joystick"_L1;
    case DeviceKind::Gamepad:
        return "gamepad"_L1;
    case DeviceKind::Dock:
        return "dock"_L1;
    case DeviceKind::Speaker:
        return "speaker"_L1;
    case DeviceKind::Microphone:
        return "microphone"_L1;
    case DeviceKind::IlluminationLight:
        return "illuminationLight"_L1;
    case DeviceKind::ProgrammableController:
        return "programmableController"_L1;
    case DeviceKind::CarSimPedals:
        return "carSimPedals"_L1;
    case DeviceKind::Adapter:
        return "adapter"_L1;
    case DeviceKind::Unknown:
        break;
    }
    return "unknown"_L1;
}

QLatin1StringView key(FirmwareType type)
{
    switch (type) {
    case FirmwareType::MainApplication:
        return "mainApplication"_L1;
    case FirmwareType::Bootloader:
        return "bootloader"_L1;
    case FirmwareType::Hardware:
        return "hardware"_L1;
    case FirmwareType::Touchpad:
        return "touchpad"_L1;
    case FirmwareType::OpticalSensor:
        return "opticalSensor"_L1;
    case FirmwareType::Softdevice:
        return "softdevice"_L1;
    case FirmwareType::RfCompanion:
        return "rfCompanion"_L1;
    case FirmwareType::Other:
        break;
    }
    return "other"_L1;
}

QLatin1StringView key(BatteryLevel level)
{
    switch (level) {
    case BatteryLevel::Critical:
        return "critical"_L1;
    case BatteryLevel::Low:
        return "low"_L1;
    case BatteryLevel::Good:
        return "good"_L1;
    case BatteryLevel::Full:
        return "full"_L1;
    case BatteryLevel::Unknown:
        break;
    }
    return "unknown"_L1;
}

QLatin1StringView key(ChargingState state)
{
    switch (state) {
    case ChargingState::Discharging:
        return "discharging"_L1;
    case ChargingState::Charging:
        return "charging"_L1;
    case ChargingState::ChargingSlowly:
        return "chargingSlowly"_L1;
    case ChargingState::Complete:
        return "complete"_L1;
    case ChargingState::Error:
        return "error"_L1;
    case ChargingState::Unknown:
        break;
    }
    return "unknown"_L1;
}

QLatin1StringView key(Battery::Source source)
{
    switch (source) {
    case Battery::Source::UnifiedBattery:
        return "unifiedBattery"_L1;
    case Battery::Source::BatteryStatus:
        return "batteryStatus"_L1;
    case Battery::Source::BatteryVoltage:
        return "batteryVoltage"_L1;
    }
    return {};
}

QLatin1StringView key(ErrorKind kind)
{
    switch (kind) {
    case ErrorKind::Transport:
        return "transport"_L1;
    case ErrorKind::PermissionDenied:
        return "permissionDenied"_L1;
    case ErrorKind::NotFound:
        return "notFound"_L1;
    case ErrorKind::Disconnected:
        return "disconnected"_L1;
    case ErrorKind::Timeout:
        return "timeout"_L1;
    case ErrorKind::Protocol10:
        return "hidpp10"_L1;
    case ErrorKind::Protocol20:
        return "hidpp20"_L1;
    case ErrorKind::Malformed:
        return "malformed"_L1;
    }
    return {};
}

} // namespace hidpp
