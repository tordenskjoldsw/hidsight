// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "device.h"
#include "sequence_p.h"

#include <algorithm>

using namespace Qt::StringLiterals;

namespace hidpp
{

namespace
{

constexpr std::uint8_t RootIndex = 0x00;

enum RootFunction : std::uint8_t {
    GetFeature = 0,
    GetProtocolVersion = 1,
};

enum FeatureSetFunction : std::uint8_t {
    GetCount = 0,
    GetFeatureId = 1,
};

// Echoed by the device in the ping reply.
constexpr std::uint8_t PingData = 0x5A;

struct NamedFeature {
    std::uint16_t id;
    QLatin1StringView name;
};

// The features of the devices on the desk and those Hidsight reads, with the
// names of Logitech's HID++ documents.
constexpr std::array KnownFeatures{
    NamedFeature{0x0000, "Root"_L1},
    NamedFeature{0x0001, "FeatureSet"_L1},
    NamedFeature{0x0003, "DeviceFwVersion"_L1},
    NamedFeature{0x0005, "DeviceName"_L1},
    NamedFeature{0x0007, "DeviceFriendlyName"_L1},
    NamedFeature{0x0020, "ConfigChange"_L1},
    NamedFeature{0x0021, "UniqueIdentifier"_L1},
    NamedFeature{0x00c3, "SecureDfuControl"_L1},
    NamedFeature{0x1000, "BatteryStatus"_L1},
    NamedFeature{0x1001, "BatteryVoltage"_L1},
    NamedFeature{0x1004, "UnifiedBattery"_L1},
    NamedFeature{0x1814, "ChangeHost"_L1},
    NamedFeature{0x1815, "HostsInfo"_L1},
    NamedFeature{0x1982, "Backlight2"_L1},
    NamedFeature{0x1b04, "ReprogControlsV4"_L1},
    NamedFeature{0x1d4b, "WirelessDeviceStatus"_L1},
    NamedFeature{0x2110, "SmartShift"_L1},
    NamedFeature{0x2121, "HiResWheel"_L1},
    NamedFeature{0x2150, "Thumbwheel"_L1},
    NamedFeature{0x2201, "AdjustableDpi"_L1},
    NamedFeature{0x2250, "ExtendedAdjustableDpi"_L1},
    NamedFeature{0x2251, "MouseWheelAnalytics"_L1},
    NamedFeature{0x40a3, "FnInversion"_L1},
    NamedFeature{0x4220, "LockKeyState"_L1},
    NamedFeature{0x4521, "DisableKeys"_L1},
    NamedFeature{0x4531, "MultiPlatform"_L1},
    NamedFeature{0x4540, "KeyboardLayout2"_L1},
    NamedFeature{0x8060, "ReportRate"_L1},
};

} // namespace

FeatureTable::FeatureTable(std::vector<FeatureEntry> entries)
    : m_entries(std::move(entries))
{
}

std::optional<std::uint8_t> FeatureTable::indexOf(std::uint16_t featureId) const
{
    if (featureId == Device::RootFeatureId) {
        return RootIndex;
    }
    const auto it = std::ranges::find(m_entries, featureId, &FeatureEntry::id);
    if (it == m_entries.end()) {
        return std::nullopt;
    }
    return it->index;
}

QLatin1StringView featureName(std::uint16_t featureId)
{
    const auto it = std::ranges::find(KnownFeatures, featureId, &NamedFeature::id);
    return it == KnownFeatures.end() ? QLatin1StringView() : it->name;
}

Device::Device(Dispatcher &dispatcher, std::uint8_t index)
    : m_dispatcher(&dispatcher)
    , m_index(index)
{
}

void Device::call(std::uint8_t featureIndex, std::uint8_t function, std::array<std::uint8_t, 3> params, Callback<Report> done) const
{
    m_dispatcher->request(Report::shortRequest(m_index, featureIndex, functionByte(function, m_dispatcher->softwareId()), params), std::move(done));
}

void Device::ping(Callback<ProtocolVersion> done) const
{
    request<ProtocolVersion>(RootIndex, GetProtocolVersion, {0, 0, PingData}, &Device::parsePing, std::move(done));
}

void Device::featureIndex(std::uint16_t featureId, Callback<std::optional<std::uint8_t>> done) const
{
    const std::array<std::uint8_t, 3> params{static_cast<std::uint8_t>(featureId >> 8), static_cast<std::uint8_t>(featureId & 0xFF), 0};
    request<std::optional<std::uint8_t>>(RootIndex, GetFeature, params, &Device::parseFeatureIndex, std::move(done));
}

void Device::readFeatureTable(Callback<FeatureTable> done) const
{
    const Device device = *this;
    featureIndex(FeatureSetFeatureId, [device, done](Result<std::optional<std::uint8_t>> featureSet) {
        if (!featureSet) {
            done(featureSet.error());
            return;
        }
        if (!featureSet.value()) {
            // Every HID++ 2.0 device has FeatureSet.
            done(Error{ErrorKind::Malformed, 0});
            return;
        }
        const std::uint8_t featureSetIndex = *featureSet.value();
        device.request<std::uint8_t>(featureSetIndex, GetCount, {}, &Device::parseFeatureCount, [device, featureSetIndex, done](Result<std::uint8_t> count) {
            if (!count) {
                done(count.error());
                return;
            }
            detail::readSequentially<FeatureEntry>(
                count.value(),
                [device, featureSetIndex](std::size_t position, Callback<FeatureEntry> one) {
                    // Index 0 is Root, which FeatureSet does not count.
                    const auto index = static_cast<std::uint8_t>(position + 1);
                    device.request<FeatureEntry>(
                        featureSetIndex,
                        GetFeatureId,
                        {index, 0, 0},
                        [index](std::span<const std::uint8_t> params) {
                            return Device::parseFeatureEntry(index, params);
                        },
                        std::move(one));
                },
                [done](Result<std::vector<FeatureEntry>> entries) {
                    if (!entries) {
                        done(entries.error());
                        return;
                    }
                    done(FeatureTable(std::move(entries).value()));
                });
        });
    });
}

Result<ProtocolVersion> Device::parsePing(std::span<const std::uint8_t> params)
{
    if (params.size() < 3 || params[2] != PingData) {
        return Error{ErrorKind::Malformed, 0};
    }
    return ProtocolVersion{params[0], params[1]};
}

Result<std::optional<std::uint8_t>> Device::parseFeatureIndex(std::span<const std::uint8_t> params)
{
    if (params.empty()) {
        return Error{ErrorKind::Malformed, 0};
    }
    if (params[0] == RootIndex) {
        return std::optional<std::uint8_t>();
    }
    return std::optional<std::uint8_t>(params[0]);
}

Result<std::uint8_t> Device::parseFeatureCount(std::span<const std::uint8_t> params)
{
    // Index 0xFF marks error replies, so a device has at most 0xFE features.
    if (params.empty() || params[0] >= Error20FeatureIndex) {
        return Error{ErrorKind::Malformed, 0};
    }
    return params[0];
}

Result<FeatureEntry> Device::parseFeatureEntry(std::uint8_t index, std::span<const std::uint8_t> params)
{
    if (params.size() < 3) {
        return Error{ErrorKind::Malformed, 0};
    }
    return FeatureEntry{static_cast<std::uint16_t>((params[0] << 8) | params[1]), index, params[2]};
}

} // namespace hidpp
