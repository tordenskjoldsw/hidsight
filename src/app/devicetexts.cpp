// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "devicetexts.h"

#include <KFormat>
#include <KLocalizedString>

#include <QStringList>

using namespace Qt::StringLiterals;
using namespace hidpp;

namespace DeviceTexts
{

QString kindText(DeviceKind kind)
{
    switch (kind) {
    case DeviceKind::Keyboard:
        return i18nc("@info device kind", "Keyboard");
    case DeviceKind::RemoteControl:
        return i18nc("@info device kind", "Remote control");
    case DeviceKind::Numpad:
        return i18nc("@info device kind", "Numeric keypad");
    case DeviceKind::Mouse:
        return i18nc("@info device kind", "Mouse");
    case DeviceKind::Trackpad:
        return i18nc("@info device kind", "Touchpad");
    case DeviceKind::Trackball:
        return i18nc("@info device kind", "Trackball");
    case DeviceKind::Presenter:
        return i18nc("@info device kind", "Presenter");
    case DeviceKind::Receiver:
        return i18nc("@info device kind", "Receiver");
    case DeviceKind::Headset:
        return i18nc("@info device kind", "Headset");
    case DeviceKind::Webcam:
        return i18nc("@info device kind", "Webcam");
    case DeviceKind::SteeringWheel:
        return i18nc("@info device kind", "Steering wheel");
    case DeviceKind::Joystick:
        return i18nc("@info device kind", "Joystick");
    case DeviceKind::Gamepad:
        return i18nc("@info device kind", "Gamepad");
    case DeviceKind::Dock:
        return i18nc("@info device kind", "Dock");
    case DeviceKind::Speaker:
        return i18nc("@info device kind", "Speaker");
    case DeviceKind::Microphone:
        return i18nc("@info device kind", "Microphone");
    case DeviceKind::IlluminationLight:
        return i18nc("@info device kind", "Light");
    case DeviceKind::ProgrammableController:
        return i18nc("@info device kind", "Controller");
    case DeviceKind::CarSimPedals:
        return i18nc("@info device kind", "Pedals");
    case DeviceKind::Adapter:
        return i18nc("@info device kind", "Adapter");
    case DeviceKind::Unknown:
        break;
    }
    return i18nc("@info device kind", "Device");
}

QString kindIconName(DeviceKind kind)
{
    switch (kind) {
    case DeviceKind::Keyboard:
        return u"input-keyboard"_s;
    case DeviceKind::Numpad:
        return u"input-numpad"_s;
    case DeviceKind::Mouse:
    case DeviceKind::Trackball:
        return u"input-mouse"_s;
    case DeviceKind::Trackpad:
        return u"input-touchpad"_s;
    case DeviceKind::RemoteControl:
        return u"input-tablet"_s;
    case DeviceKind::Presenter:
        return u"x-office-presentation"_s;
    case DeviceKind::Headset:
        return u"audio-headset"_s;
    case DeviceKind::Webcam:
        return u"camera-web"_s;
    case DeviceKind::SteeringWheel:
    case DeviceKind::Joystick:
    case DeviceKind::Gamepad:
    case DeviceKind::CarSimPedals:
        return u"input-gaming"_s;
    case DeviceKind::Speaker:
        return u"audio-speakers"_s;
    case DeviceKind::Microphone:
        return u"audio-input-microphone"_s;
    case DeviceKind::Receiver:
        return u"network-receiver"_s;
    case DeviceKind::Dock:
    case DeviceKind::IlluminationLight:
    case DeviceKind::ProgrammableController:
    case DeviceKind::Adapter:
    case DeviceKind::Unknown:
        break;
    }
    return u"preferences-desktop-peripherals"_s;
}

QString illustrationName(DeviceKind kind)
{
    switch (kind) {
    case DeviceKind::Keyboard:
    case DeviceKind::Numpad:
        return u"keyboard"_s;
    case DeviceKind::Mouse:
    case DeviceKind::Trackball:
    case DeviceKind::Trackpad:
        return u"mouse"_s;
    default:
        return {};
    }
}

QString chargingText(ChargingState state)
{
    switch (state) {
    case ChargingState::Discharging:
        return i18nc("@info charging state", "Not charging");
    case ChargingState::Charging:
        return i18nc("@info charging state", "Charging");
    case ChargingState::ChargingSlowly:
        return i18nc("@info charging state", "Charging slowly");
    case ChargingState::Complete:
        return i18nc("@info charging state", "Fully charged");
    case ChargingState::Error:
        return i18nc("@info charging state", "Charging error");
    case ChargingState::Unknown:
        break;
    }
    return {};
}

QString levelText(BatteryLevel level)
{
    switch (level) {
    case BatteryLevel::Critical:
        return i18nc("@info battery level", "Critical");
    case BatteryLevel::Low:
        return i18nc("@info battery level", "Low");
    case BatteryLevel::Good:
        return i18nc("@info battery level", "Good");
    case BatteryLevel::Full:
        return i18nc("@info battery level", "Full");
    case BatteryLevel::Unknown:
        break;
    }
    return {};
}

QString batteryText(const Battery &battery)
{
    QString charge;
    if (battery.percentage) {
        charge = i18nc("@info battery charge", "%1 %", *battery.percentage);
    } else if (battery.millivolts) {
        charge = i18nc("@info battery voltage", "%1 mV", *battery.millivolts);
    } else {
        switch (battery.level) {
        case BatteryLevel::Critical:
            charge = i18nc("@info battery level", "Battery critical");
            break;
        case BatteryLevel::Low:
            charge = i18nc("@info battery level", "Battery low");
            break;
        case BatteryLevel::Good:
            charge = i18nc("@info battery level", "Battery good");
            break;
        case BatteryLevel::Full:
            charge = i18nc("@info battery level", "Battery full");
            break;
        case BatteryLevel::Unknown:
            break;
        }
    }
    if (charge.isEmpty()) {
        return chargingText(battery.charging);
    }
    switch (battery.charging) {
    case ChargingState::Charging:
        return i18nc("@info battery charge", "%1, charging", charge);
    case ChargingState::ChargingSlowly:
        return i18nc("@info battery charge", "%1, charging slowly", charge);
    case ChargingState::Complete:
        return i18nc("@info battery charge", "%1, fully charged", charge);
    case ChargingState::Error:
        return i18nc("@info battery charge", "%1, charging error", charge);
    case ChargingState::Discharging:
    case ChargingState::Unknown:
        break;
    }
    return charge;
}

std::optional<int> chargeEstimate(const Battery &battery)
{
    if (battery.percentage) {
        return *battery.percentage;
    }
    switch (battery.level) {
    case BatteryLevel::Full:
        return 100;
    case BatteryLevel::Good:
        return 60;
    case BatteryLevel::Low:
        return 20;
    case BatteryLevel::Critical:
        return 0;
    case BatteryLevel::Unknown:
        break;
    }
    return std::nullopt;
}

QString batteryTone(const DeviceState &state)
{
    if (state.status != DeviceStatus::Online || !state.battery || !chargeEstimate(*state.battery)) {
        return u"unknown"_s;
    }
    const Battery &battery = *state.battery;
    if (battery.charging == ChargingState::Charging || battery.charging == ChargingState::ChargingSlowly || battery.charging == ChargingState::Complete) {
        return u"charging"_s;
    }
    if (battery.level == BatteryLevel::Critical) {
        return u"critical"_s;
    }
    if (battery.level == BatteryLevel::Low) {
        return u"low"_s;
    }
    return u"normal"_s;
}

QVariantList statusChips(const DeviceState &state)
{
    const auto chip = [](const QString &text, const QString &iconName) {
        return QVariantMap{{u"text"_s, text}, {u"iconName"_s, iconName}};
    };
    QVariantList chips;
    switch (state.status) {
    case DeviceStatus::Reading:
        chips << chip(i18nc("@info:status", "Reading"), u"view-refresh-symbolic"_s);
        return chips;
    case DeviceStatus::Unreachable:
        chips << chip(i18nc("@info:status", "Not connected"), u"network-disconnect-symbolic"_s);
        return chips;
    case DeviceStatus::Online:
        break;
    }
    if (!state.battery) {
        return chips;
    }
    const Battery &battery = *state.battery;
    // At most two chips: how the battery charges, and how low it is when it
    // does not. Charging implies the cable, so the cable chip only stands for
    // a cable without charging.
    switch (battery.charging) {
    case ChargingState::Charging:
        chips << chip(i18nc("@info:status", "Charging"), u"battery-100-charging-symbolic"_s);
        return chips;
    case ChargingState::ChargingSlowly:
        chips << chip(i18nc("@info:status", "Charging slowly"), u"battery-100-charging-symbolic"_s);
        return chips;
    case ChargingState::Complete:
        chips << chip(i18nc("@info:status", "Fully charged"), u"battery-full-charged-symbolic"_s);
        return chips;
    case ChargingState::Error:
        chips << chip(i18nc("@info:status", "Charging error"), u"emblem-error-symbolic"_s);
        return chips;
    case ChargingState::Discharging:
    case ChargingState::Unknown:
        break;
    }
    if (battery.externalPower.value_or(false)) {
        chips << chip(i18nc("@info:status", "On cable"), u"network-connect-symbolic"_s);
    }
    if (battery.level == BatteryLevel::Critical) {
        chips << chip(i18nc("@info:status", "Battery critical"), u"battery-caution-symbolic"_s);
    } else if (battery.level == BatteryLevel::Low) {
        chips << chip(i18nc("@info:status", "Battery low"), u"battery-low-symbolic"_s);
    }
    return chips;
}

QString batteryIconName(const std::optional<Battery> &battery)
{
    if (!battery) {
        return u"battery-missing"_s;
    }
    const auto estimate = chargeEstimate(*battery);
    if (!estimate) {
        return u"battery-missing"_s;
    }
    const int percentage = *estimate;
    // Breeze has an icon for every ten percent.
    const int step = (percentage + 5) / 10 * 10;
    const bool charging = battery->charging == ChargingState::Charging || battery->charging == ChargingState::ChargingSlowly;
    return u"battery-%1%2"_s.arg(step, 3, 10, QLatin1Char('0')).arg(charging ? u"-charging"_s : QString());
}

QString statusText(const DeviceState &state)
{
    switch (state.status) {
    case DeviceStatus::Reading:
        return i18nc("@info device status", "Reading…");
    case DeviceStatus::Unreachable:
        return i18nc("@info device status", "Not connected – asleep or switched off");
    case DeviceStatus::Online:
        break;
    }
    return state.battery ? batteryText(*state.battery) : i18nc("@info device status", "Connected");
}

QString lastSeenText(const QDateTime &lastSeen)
{
    if (!lastSeen.isValid()) {
        return {};
    }
    return KFormat().formatRelativeDateTime(lastSeen.toLocalTime(), QLocale::ShortFormat);
}

QString firmwareTypeText(FirmwareType type)
{
    switch (type) {
    case FirmwareType::MainApplication:
        return i18nc("@label firmware part", "Firmware");
    case FirmwareType::Bootloader:
        return i18nc("@label firmware part", "Bootloader");
    case FirmwareType::Hardware:
        return i18nc("@label firmware part", "Hardware");
    case FirmwareType::Touchpad:
        return i18nc("@label firmware part", "Touchpad firmware");
    case FirmwareType::OpticalSensor:
        return i18nc("@label firmware part", "Sensor firmware");
    case FirmwareType::Softdevice:
        return i18nc("@label firmware part", "Radio stack");
    case FirmwareType::RfCompanion:
        return i18nc("@label firmware part", "Radio companion");
    case FirmwareType::Other:
        break;
    }
    return i18nc("@label firmware part", "Other firmware");
}

QString featureTitle(std::uint16_t featureId)
{
    switch (featureId) {
    case 0x0001:
        return i18nc("@info device feature", "List of features");
    case 0x0003:
        return i18nc("@info device feature", "Firmware information");
    case 0x0005:
        return i18nc("@info device feature", "Name and kind");
    case 0x0007:
        return i18nc("@info device feature", "Custom name");
    case 0x0020:
        return i18nc("@info device feature", "Configuration change detection");
    case 0x0021:
        return i18nc("@info device feature", "Unique identifier");
    case 0x00c3:
        return i18nc("@info device feature", "Firmware update");
    case 0x1000:
        return i18nc("@info device feature", "Battery level");
    case 0x1001:
        return i18nc("@info device feature", "Battery voltage");
    case 0x1004:
        return i18nc("@info device feature", "Battery level and charging");
    case 0x1814:
        return i18nc("@info device feature", "Switching between computers");
    case 0x1815:
        return i18nc("@info device feature", "Paired computers");
    case 0x1982:
        return i18nc("@info device feature", "Backlight");
    case 0x1b04:
        return i18nc("@info device feature", "Programmable keys and buttons");
    case 0x1d4b:
        return i18nc("@info device feature", "Wireless connection status");
    case 0x2110:
        return i18nc("@info device feature", "SmartShift wheel");
    case 0x2121:
        return i18nc("@info device feature", "High-resolution scrolling");
    case 0x2150:
        return i18nc("@info device feature", "Thumb wheel");
    case 0x2201:
        return i18nc("@info device feature", "Adjustable resolution");
    case 0x2250:
        return i18nc("@info device feature", "Adjustable resolution per axis");
    case 0x2251:
        return i18nc("@info device feature", "Wheel usage statistics");
    case 0x40a3:
        return i18nc("@info device feature", "Fn key swap");
    case 0x4220:
        return i18nc("@info device feature", "Lock key state");
    case 0x4521:
        return i18nc("@info device feature", "Disabling keys");
    case 0x4531:
        return i18nc("@info device feature", "Layout for several operating systems");
    case 0x4540:
        return i18nc("@info device feature", "Keyboard layout");
    case 0x8060:
        return i18nc("@info device feature", "Report rate");
    default:
        return i18nc("@info device feature", "Unknown feature");
    }
}

QString featureDetail(std::uint16_t featureId)
{
    const QString id = u"0x%1"_s.arg(featureId, 4, 16, QLatin1Char('0'));
    const QLatin1StringView name = featureName(featureId);
    return name.isEmpty() ? id : i18nc("@info feature name, feature id", "%1, %2", name, id);
}

QString receiverText(const QString &receiverId)
{
    // Verified on the desk; other receivers are named generically until one
    // is seen.
    if (receiverId == "046d:c548"_L1) {
        return i18nc("@info receiver model", "Logi Bolt receiver");
    }
    return i18nc("@info receiver model", "Logitech receiver");
}

QString slotText(const QString &receiverId, std::uint8_t index)
{
    return i18nc("@info receiver slot, receiver model", "Slot %1 · %2", index, receiverText(receiverId));
}

} // namespace DeviceTexts
