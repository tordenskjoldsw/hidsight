// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "pointer.h"
#include "sequence_p.h"

namespace hidpp
{

namespace
{

enum AdjustableDpiFunction : std::uint8_t {
    GetSensorCount = 0,
    GetSensorDpi = 2,
};

enum ReportRateFunction : std::uint8_t {
    GetReportRate = 1,
};

} // namespace

void AdjustableDpi::read(const Device &device, std::uint8_t featureIndex, Callback<AdjustableDpi> done)
{
    device.request<std::uint8_t>(featureIndex, GetSensorCount, {}, &AdjustableDpi::parseSensorCount, [device, featureIndex, done](Result<std::uint8_t> count) {
        if (!count) {
            done(count.error());
            return;
        }
        detail::readSequentially<std::uint16_t>(
            count.value(),
            [device, featureIndex](std::size_t position, Callback<std::uint16_t> one) {
                const auto sensor = static_cast<std::uint8_t>(position);
                device.request<std::uint16_t>(
                    featureIndex,
                    GetSensorDpi,
                    {sensor, 0, 0},
                    [sensor](std::span<const std::uint8_t> params) {
                        return AdjustableDpi::parseSensorDpi(sensor, params);
                    },
                    std::move(one));
            },
            [done](Result<std::vector<std::uint16_t>> dpi) {
                if (!dpi) {
                    done(dpi.error());
                    return;
                }
                done(AdjustableDpi{std::move(dpi).value()});
            });
    });
}

Result<std::uint8_t> AdjustableDpi::parseSensorCount(std::span<const std::uint8_t> params)
{
    if (params.empty() || params[0] > MaxSensors) {
        return Error{ErrorKind::Malformed, 0};
    }
    return params[0];
}

Result<std::uint16_t> AdjustableDpi::parseSensorDpi(std::uint8_t sensor, std::span<const std::uint8_t> params)
{
    if (params.size() < 3 || params[0] != sensor) {
        return Error{ErrorKind::Malformed, 0};
    }
    return static_cast<std::uint16_t>((params[1] << 8) | params[2]);
}

void ReportRate::read(const Device &device, std::uint8_t featureIndex, Callback<ReportRate> done)
{
    device.request<ReportRate>(featureIndex, GetReportRate, {}, &ReportRate::parse, std::move(done));
}

Result<ReportRate> ReportRate::parse(std::span<const std::uint8_t> params)
{
    if (params.empty() || params[0] == 0) {
        return Error{ErrorKind::Malformed, 0};
    }
    return ReportRate{params[0]};
}

} // namespace hidpp
