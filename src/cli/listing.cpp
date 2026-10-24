// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "listing.h"

#include "hidpp/keys.h"
#include "hidpp/receiver.h"

#include <KLocalizedString>

#include <QJsonArray>

#include <memory>

using namespace Qt::StringLiterals;
using namespace hidpp;

namespace
{

struct ListingRun {
    Dispatcher *dispatcher;
    Listing listing;
    std::function<void(Listing)> done;
};

void probeFrom(const std::shared_ptr<ListingRun> &run, std::uint8_t index)
{
    if (index > Receiver::LastDeviceIndex) {
        run->done(std::move(run->listing));
        return;
    }
    probeDevice(Device(*run->dispatcher, index), [run, index](Result<DeviceInfo> info) {
        if (info || !Receiver::isEmptySlot(info.error())) {
            run->listing.devices.emplace_back(index, std::move(info));
        }
        probeFrom(run, static_cast<std::uint8_t>(index + 1));
    });
}

QString kindText(DeviceKind kind)
{
    switch (kind) {
    case DeviceKind::Keyboard:
        return i18nc("@info device kind", "keyboard");
    case DeviceKind::RemoteControl:
        return i18nc("@info device kind", "remote control");
    case DeviceKind::Numpad:
        return i18nc("@info device kind", "numpad");
    case DeviceKind::Mouse:
        return i18nc("@info device kind", "mouse");
    case DeviceKind::Trackpad:
        return i18nc("@info device kind", "trackpad");
    case DeviceKind::Trackball:
        return i18nc("@info device kind", "trackball");
    case DeviceKind::Presenter:
        return i18nc("@info device kind", "presenter");
    case DeviceKind::Receiver:
        return i18nc("@info device kind", "receiver");
    case DeviceKind::Headset:
        return i18nc("@info device kind", "headset");
    case DeviceKind::Webcam:
        return i18nc("@info device kind", "webcam");
    case DeviceKind::SteeringWheel:
        return i18nc("@info device kind", "steering wheel");
    case DeviceKind::Joystick:
        return i18nc("@info device kind", "joystick");
    case DeviceKind::Gamepad:
        return i18nc("@info device kind", "gamepad");
    case DeviceKind::Dock:
        return i18nc("@info device kind", "dock");
    case DeviceKind::Speaker:
        return i18nc("@info device kind", "speaker");
    case DeviceKind::Microphone:
        return i18nc("@info device kind", "microphone");
    case DeviceKind::IlluminationLight:
        return i18nc("@info device kind", "illumination light");
    case DeviceKind::ProgrammableController:
        return i18nc("@info device kind", "programmable controller");
    case DeviceKind::CarSimPedals:
        return i18nc("@info device kind", "pedals");
    case DeviceKind::Adapter:
        return i18nc("@info device kind", "adapter");
    case DeviceKind::Unknown:
        break;
    }
    return i18nc("@info device kind", "unknown kind");
}

QString firmwareTypeText(FirmwareType type)
{
    switch (type) {
    case FirmwareType::MainApplication:
        return i18nc("@info firmware type", "main application");
    case FirmwareType::Bootloader:
        return i18nc("@info firmware type", "bootloader");
    case FirmwareType::Hardware:
        return i18nc("@info firmware type", "hardware");
    case FirmwareType::Touchpad:
        return i18nc("@info firmware type", "touchpad");
    case FirmwareType::OpticalSensor:
        return i18nc("@info firmware type", "optical sensor");
    case FirmwareType::Softdevice:
        return i18nc("@info firmware type", "softdevice");
    case FirmwareType::RfCompanion:
        return i18nc("@info firmware type", "radio companion");
    case FirmwareType::Other:
        break;
    }
    return i18nc("@info firmware type", "other");
}

QString levelText(BatteryLevel level)
{
    switch (level) {
    case BatteryLevel::Critical:
        return i18nc("@info battery level", "critical");
    case BatteryLevel::Low:
        return i18nc("@info battery level", "low");
    case BatteryLevel::Good:
        return i18nc("@info battery level", "good");
    case BatteryLevel::Full:
        return i18nc("@info battery level", "full");
    case BatteryLevel::Unknown:
        break;
    }
    return {};
}

QString chargingText(ChargingState state)
{
    switch (state) {
    case ChargingState::Discharging:
        return i18nc("@info charging state", "discharging");
    case ChargingState::Charging:
        return i18nc("@info charging state", "charging");
    case ChargingState::ChargingSlowly:
        return i18nc("@info charging state", "charging slowly");
    case ChargingState::Complete:
        return i18nc("@info charging state", "charge complete");
    case ChargingState::Error:
        return i18nc("@info charging state", "charging error");
    case ChargingState::Unknown:
        break;
    }
    return {};
}

QJsonObject errorJson(const Error &error)
{
    return QJsonObject{{u"error"_s, QJsonObject{{u"kind"_s, key(error.kind)}, {u"code"_s, error.code}}}};
}

QString hex4(std::uint16_t value)
{
    return u"0x%1"_s.arg(value, 4, 16, QLatin1Char('0'));
}

QString batteryText(const Battery &battery)
{
    QStringList parts;
    if (battery.percentage) {
        parts << i18nc("@info battery charge", "%1 %", *battery.percentage);
    }
    if (battery.millivolts) {
        parts << i18nc("@info battery voltage", "%1 mV", *battery.millivolts);
    }
    if (const QString level = levelText(battery.level); !level.isEmpty()) {
        parts << level;
    }
    if (const QString charging = chargingText(battery.charging); !charging.isEmpty()) {
        parts << charging;
    }
    if (battery.externalPower.value_or(false)) {
        parts << i18nc("@info", "external power");
    }
    return parts.join(u", "_s);
}

void appendDeviceText(QStringList &lines, std::uint8_t index, const DeviceInfo &info)
{
    if (info.name && info.name->hasValue()) {
        const DeviceName &name = info.name->value();
        lines << i18nc("@info device index, name, kind", "Device %1: %2, %3", index, name.name, kindText(name.kind));
    } else {
        lines << i18nc("@info", "Device %1", index);
    }
    if (info.name && !info.name->hasValue()) {
        lines << u"  "_s + i18nc("@info", "Name: %1", errorText(info.name->error()));
    }
    lines << u"  "_s + i18nc("@info HID++ protocol version", "Protocol: HID++ %1.%2", info.protocol.major, info.protocol.minor);

    if (info.firmware) {
        if (info.firmware->hasValue()) {
            for (const FirmwareEntry &entry : info.firmware->value().entries) {
                lines << u"  "_s + i18nc("@info firmware type, version", "Firmware: %1 %2", firmwareTypeText(entry.type), entry.version());
            }
        } else {
            lines << u"  "_s + i18nc("@info", "Firmware: %1", errorText(info.firmware->error()));
        }
    }
    if (info.battery) {
        lines << u"  "_s + i18nc("@info", "Battery: %1", info.battery->hasValue() ? batteryText(info.battery->value()) : errorText(info.battery->error()));
    }
    if (info.dpi) {
        if (info.dpi->hasValue()) {
            const auto &sensors = info.dpi->value().sensorDpi;
            for (std::size_t sensor = 0; sensor < sensors.size(); ++sensor) {
                lines << u"  "_s + i18nc("@info sensor number, resolution", "Sensor %1: %2 dpi", sensor, sensors[sensor]);
            }
        } else {
            lines << u"  "_s + i18nc("@info", "Resolution: %1", errorText(info.dpi->error()));
        }
    }
    if (info.reportRate) {
        lines << u"  "_s
                + i18nc("@info",
                        "Report rate: %1",
                        info.reportRate->hasValue() ? i18nc("@info report interval", "%1 ms", info.reportRate->value().milliseconds)
                                                    : errorText(info.reportRate->error()));
    }
    lines << u"  "_s + i18nc("@info", "Features:");
    for (const FeatureEntry &feature : info.features.entries()) {
        if (feature.isVisible()) {
            const QLatin1StringView name = featureName(feature.id);
            lines << (name.isEmpty() ? u"    %1"_s.arg(hex4(feature.id)) : u"    %1 %2"_s.arg(hex4(feature.id), name));
        }
    }
}

QJsonObject deviceJson(std::uint8_t index, const DeviceInfo &info)
{
    QJsonObject device{
        {u"index"_s, index},
        {u"protocol"_s, u"%1.%2"_s.arg(info.protocol.major).arg(info.protocol.minor)},
    };
    if (info.name) {
        if (info.name->hasValue()) {
            device.insert(u"name"_s, info.name->value().name);
            device.insert(u"kind"_s, key(info.name->value().kind));
        } else {
            device.insert(u"name"_s, errorJson(info.name->error()));
        }
    }
    if (info.firmware) {
        if (info.firmware->hasValue()) {
            QJsonArray firmware;
            for (const FirmwareEntry &entry : info.firmware->value().entries) {
                firmware.append(QJsonObject{{u"type"_s, key(entry.type)}, {u"version"_s, entry.version()}});
            }
            device.insert(u"firmware"_s, firmware);
        } else {
            device.insert(u"firmware"_s, errorJson(info.firmware->error()));
        }
    }
    if (info.battery) {
        if (info.battery->hasValue()) {
            const Battery &battery = info.battery->value();
            QJsonObject object{
                {u"source"_s, key(battery.source)},
                {u"level"_s, key(battery.level)},
                {u"charging"_s, key(battery.charging)},
            };
            if (battery.percentage) {
                object.insert(u"percentage"_s, *battery.percentage);
            }
            if (battery.millivolts) {
                object.insert(u"millivolts"_s, *battery.millivolts);
            }
            if (battery.externalPower) {
                object.insert(u"externalPower"_s, *battery.externalPower);
            }
            device.insert(u"battery"_s, object);
        } else {
            device.insert(u"battery"_s, errorJson(info.battery->error()));
        }
    }
    if (info.dpi) {
        if (info.dpi->hasValue()) {
            QJsonArray dpi;
            for (const std::uint16_t value : info.dpi->value().sensorDpi) {
                dpi.append(value);
            }
            device.insert(u"dpi"_s, dpi);
        } else {
            device.insert(u"dpi"_s, errorJson(info.dpi->error()));
        }
    }
    if (info.reportRate) {
        if (info.reportRate->hasValue()) {
            device.insert(u"reportRateMs"_s, info.reportRate->value().milliseconds);
        } else {
            device.insert(u"reportRateMs"_s, errorJson(info.reportRate->error()));
        }
    }
    QJsonArray features;
    for (const FeatureEntry &feature : info.features.entries()) {
        if (feature.isVisible()) {
            QJsonObject object{{u"id"_s, hex4(feature.id)}};
            if (const QLatin1StringView name = featureName(feature.id); !name.isEmpty()) {
                object.insert(u"name"_s, name);
            }
            features.append(object);
        }
    }
    device.insert(u"features"_s, features);
    return device;
}

} // namespace

void listReceiver(Dispatcher &dispatcher, const QString &source, std::function<void(Listing)> done)
{
    auto run = std::make_shared<ListingRun>(ListingRun{&dispatcher, Listing{source, std::nullopt, {}}, std::move(done)});
    Receiver(dispatcher).readConnectedDeviceCount([run](Result<std::uint8_t> count) {
        const bool isReceiver = count.hasValue();
        run->listing.connectedDevices = std::move(count);
        if (!isReceiver) {
            run->done(std::move(run->listing));
            return;
        }
        probeFrom(run, Receiver::FirstDeviceIndex);
    });
}

QString errorText(const Error &error)
{
    switch (error.kind) {
    case ErrorKind::Transport:
        return i18nc("@info error", "cannot read or write the node (error %1)", error.code);
    case ErrorKind::PermissionDenied:
        return i18nc("@info error", "no permission; the udev rule 70-hidsight.rules grants access to Logitech nodes");
    case ErrorKind::NotFound:
        return i18nc("@info error", "no such node");
    case ErrorKind::Disconnected:
        return i18nc("@info error", "disconnected");
    case ErrorKind::Timeout:
        return i18nc("@info error", "no answer");
    case ErrorKind::Protocol10:
        return i18nc("@info error", "HID++ 1.0 error %1", u"0x%1"_s.arg(error.code, 2, 16, QLatin1Char('0')));
    case ErrorKind::Protocol20:
        return i18nc("@info error", "HID++ 2.0 error %1", u"0x%1"_s.arg(error.code, 2, 16, QLatin1Char('0')));
    case ErrorKind::Malformed:
        return i18nc("@info error", "malformed reply");
    }
    return {};
}

QString formatText(const Listing &listing)
{
    QStringList lines;
    if (!listing.connectedDevices || !listing.connectedDevices->hasValue()) {
        const QString reason = listing.connectedDevices ? errorText(listing.connectedDevices->error()) : QString();
        lines << i18nc("@info node, reason", "%1: not a receiver (%2)", listing.source, reason);
        return lines.join(u'\n') + u'\n';
    }
    lines << i18ncp("@info node", "Receiver %2: 1 device connected", "Receiver %2: %1 devices connected", listing.connectedDevices->value(), listing.source);
    for (const auto &[index, info] : listing.devices) {
        lines << QString();
        if (info) {
            appendDeviceText(lines, index, info.value());
        } else {
            lines << i18nc("@info device index, reason", "Device %1: not reachable (%2)", index, errorText(info.error()));
        }
    }
    return lines.join(u'\n') + u'\n';
}

QJsonObject formatJson(const Listing &listing)
{
    QJsonObject root{{u"source"_s, listing.source}};
    if (!listing.connectedDevices || !listing.connectedDevices->hasValue()) {
        root.insert(u"receiver"_s, listing.connectedDevices ? errorJson(listing.connectedDevices->error()) : QJsonObject());
        return root;
    }
    root.insert(u"receiver"_s, QJsonObject{{u"connectedDevices"_s, listing.connectedDevices->value()}});
    QJsonArray devices;
    for (const auto &[index, info] : listing.devices) {
        if (info) {
            devices.append(deviceJson(index, info.value()));
        } else {
            QJsonObject device = errorJson(info.error());
            device.insert(u"index"_s, index);
            devices.append(device);
        }
    }
    root.insert(u"devices"_s, devices);
    return root;
}
