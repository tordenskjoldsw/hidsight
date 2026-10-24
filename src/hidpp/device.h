// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "dispatcher.h"
#include "report.h"
#include "result.h"

#include <QLatin1StringView>

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace hidpp
{

struct ProtocolVersion {
    std::uint8_t major = 0;
    std::uint8_t minor = 0;
};

struct FeatureEntry {
    // Verified on the desk: hidden and engineering features carry these
    // flags; they never reach the UI.
    static constexpr std::uint8_t ObsoleteFlag = 0x80;
    static constexpr std::uint8_t HiddenFlag = 0x40;
    static constexpr std::uint8_t EngineeringFlag = 0x20;

    std::uint16_t id = 0;
    std::uint8_t index = 0;
    std::uint8_t flags = 0;

    bool isVisible() const
    {
        return (flags & (HiddenFlag | EngineeringFlag)) == 0;
    }
};

class FeatureTable
{
public:
    FeatureTable() = default;
    explicit FeatureTable(std::vector<FeatureEntry> entries);

    std::optional<std::uint8_t> indexOf(std::uint16_t featureId) const;

    const std::vector<FeatureEntry> &entries() const
    {
        return m_entries;
    }

private:
    std::vector<FeatureEntry> m_entries;
};

// Logitech's name of a feature, for example "UnifiedBattery"; empty when
// Hidsight does not know the feature.
QLatin1StringView featureName(std::uint16_t featureId);

// A HID++ 2.0 device at an index on a channel: on a receiver at 1 to 6. A
// cheap handle that the asynchronous reads copy into their callbacks.
class Device
{
public:
    static constexpr std::uint16_t RootFeatureId = 0x0000;
    static constexpr std::uint16_t FeatureSetFeatureId = 0x0001;

    Device(Dispatcher &dispatcher, std::uint8_t index);

    std::uint8_t index() const
    {
        return m_index;
    }

    // One request to a feature; the whole reply goes to done.
    void call(std::uint8_t featureIndex, std::uint8_t function, std::array<std::uint8_t, 3> params, Callback<Report> done) const;

    // One request whose reply parameters parse turns into a value.
    template<typename T, typename Parse>
    void request(std::uint8_t featureIndex, std::uint8_t function, std::array<std::uint8_t, 3> params, Parse parse, Callback<T> done) const
    {
        call(featureIndex, function, params, [parse = std::move(parse), done = std::move(done)](Result<Report> reply) {
            if (!reply) {
                done(reply.error());
                return;
            }
            done(parse(reply.value().params()));
        });
    }

    // Root getProtocolVersion; also the ping that tells whether the device is
    // awake.
    void ping(Callback<ProtocolVersion> done) const;

    // Root getFeature; nothing when the device lacks the feature.
    void featureIndex(std::uint16_t featureId, Callback<std::optional<std::uint8_t>> done) const;

    // All features through FeatureSet, one request per feature.
    void readFeatureTable(Callback<FeatureTable> done) const;

    static Result<ProtocolVersion> parsePing(std::span<const std::uint8_t> params);
    static Result<std::optional<std::uint8_t>> parseFeatureIndex(std::span<const std::uint8_t> params);
    static Result<std::uint8_t> parseFeatureCount(std::span<const std::uint8_t> params);
    static Result<FeatureEntry> parseFeatureEntry(std::uint8_t index, std::span<const std::uint8_t> params);

private:
    Dispatcher *m_dispatcher;
    std::uint8_t m_index;
};

} // namespace hidpp
