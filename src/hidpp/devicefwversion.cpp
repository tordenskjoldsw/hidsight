// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "devicefwversion.h"
#include "sequence_p.h"

using namespace Qt::StringLiterals;

namespace hidpp
{

namespace
{

enum Function : std::uint8_t {
    GetEntityCount = 0,
    GetFwInfo = 1,
};

FirmwareType firmwareType(std::uint8_t raw)
{
    if (raw > static_cast<std::uint8_t>(FirmwareType::RfCompanion)) {
        return FirmwareType::Other;
    }
    return static_cast<FirmwareType>(raw);
}

} // namespace

QString FirmwareEntry::version() const
{
    return u"%1%2.%3.B%4"_s.arg(prefix).arg(number, 2, 16, QLatin1Char('0')).arg(revision, 2, 16, QLatin1Char('0')).arg(build, 4, 10, QLatin1Char('0'));
}

void DeviceFwVersion::read(const Device &device, std::uint8_t featureIndex, Callback<DeviceFwVersion> done)
{
    device.request<std::uint8_t>(featureIndex,
                                 GetEntityCount,
                                 {},
                                 &DeviceFwVersion::parseEntityCount,
                                 [device, featureIndex, done](Result<std::uint8_t> count) {
                                     if (!count) {
                                         done(count.error());
                                         return;
                                     }
                                     detail::readSequentially<FirmwareEntry>(
                                         count.value(),
                                         [device, featureIndex](std::size_t entity, Callback<FirmwareEntry> one) {
                                             device.request<FirmwareEntry>(featureIndex,
                                                                           GetFwInfo,
                                                                           {static_cast<std::uint8_t>(entity), 0, 0},
                                                                           &DeviceFwVersion::parseEntry,
                                                                           std::move(one));
                                         },
                                         [done](Result<std::vector<FirmwareEntry>> entries) {
                                             if (!entries) {
                                                 done(entries.error());
                                                 return;
                                             }
                                             done(DeviceFwVersion{std::move(entries).value()});
                                         });
                                 });
}

Result<std::uint8_t> DeviceFwVersion::parseEntityCount(std::span<const std::uint8_t> params)
{
    if (params.empty() || params[0] > MaxEntities) {
        return Error{ErrorKind::Malformed, 0};
    }
    return params[0];
}

Result<FirmwareEntry> DeviceFwVersion::parseEntry(std::span<const std::uint8_t> params)
{
    if (params.size() < 8) {
        return Error{ErrorKind::Malformed, 0};
    }
    FirmwareEntry entry;
    entry.type = firmwareType(params[0] & 0x0F);
    // The prefix is three ASCII letters; the softdevice entry has none.
    for (const std::uint8_t c : params.subspan(1, 3)) {
        if (c > 0x20 && c < 0x7F) {
            entry.prefix.append(QLatin1Char(static_cast<char>(c)));
        }
    }
    entry.number = params[4];
    entry.revision = params[5];
    entry.build = static_cast<std::uint16_t>((params[6] << 8) | params[7]);
    return entry;
}

} // namespace hidpp
