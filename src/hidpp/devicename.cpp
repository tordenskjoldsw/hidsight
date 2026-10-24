// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "devicename.h"

#include <QByteArray>

#include <algorithm>
#include <memory>

namespace hidpp
{

namespace
{

enum Function : std::uint8_t {
    GetDeviceNameCount = 0,
    GetDeviceName = 1,
    GetDeviceType = 2,
};

struct NameRead {
    Device device;
    std::uint8_t featureIndex;
    std::uint8_t length;
    QByteArray bytes;
    Callback<DeviceName> done;
};

void continueNameRead(const std::shared_ptr<NameRead> &read)
{
    if (read->bytes.size() >= read->length) {
        read->device.request<DeviceKind>(read->featureIndex, GetDeviceType, {}, &DeviceName::parseKind, [read](Result<DeviceKind> kind) {
            if (!kind) {
                read->done(kind.error());
                return;
            }
            read->done(DeviceName{DeviceName::decodeName(read->bytes), kind.value()});
        });
        return;
    }
    const auto offset = static_cast<std::uint8_t>(read->bytes.size());
    read->device.call(read->featureIndex, GetDeviceName, {offset, 0, 0}, [read](Result<Report> reply) {
        if (!reply) {
            read->done(reply.error());
            return;
        }
        const auto chunk = reply.value().params();
        const auto wanted = std::min<std::size_t>(chunk.size(), static_cast<std::size_t>(read->length - read->bytes.size()));
        if (wanted == 0) {
            read->done(Error{ErrorKind::Malformed, 0});
            return;
        }
        read->bytes.append(QByteArrayView(chunk.first(wanted)));
        continueNameRead(read);
    });
}

} // namespace

void DeviceName::read(const Device &device, std::uint8_t featureIndex, Callback<DeviceName> done)
{
    device.request<std::uint8_t>(featureIndex, GetDeviceNameCount, {}, &DeviceName::parseLength, [device, featureIndex, done](Result<std::uint8_t> length) {
        if (!length) {
            done(length.error());
            return;
        }
        continueNameRead(std::make_shared<NameRead>(NameRead{device, featureIndex, length.value(), {}, done}));
    });
}

Result<std::uint8_t> DeviceName::parseLength(std::span<const std::uint8_t> params)
{
    if (params.empty() || params[0] == 0 || params[0] > MaxLength) {
        return Error{ErrorKind::Malformed, 0};
    }
    return params[0];
}

Result<DeviceKind> DeviceName::parseKind(std::span<const std::uint8_t> params)
{
    if (params.empty()) {
        return Error{ErrorKind::Malformed, 0};
    }
    if (params[0] > static_cast<std::uint8_t>(DeviceKind::Adapter)) {
        return DeviceKind::Unknown;
    }
    return static_cast<DeviceKind>(params[0]);
}

QString DeviceName::decodeName(QByteArrayView bytes)
{
    const qsizetype end = bytes.indexOf('\0');
    const QString text = QString::fromUtf8(end < 0 ? bytes : bytes.first(end));
    QString printable;
    printable.reserve(text.size());
    for (const QChar c : text) {
        if (c.isPrint()) {
            printable.append(c);
        }
    }
    return printable.trimmed();
}

} // namespace hidpp
