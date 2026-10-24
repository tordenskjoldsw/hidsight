// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "report.h"
#include "result.h"

#include <QDebug>

#include <algorithm>

namespace hidpp
{

std::optional<Report> Report::fromBytes(std::span<const std::uint8_t> bytes)
{
    if (bytes.empty()) {
        return std::nullopt;
    }
    std::size_t expected = 0;
    if (bytes[0] == ShortReportId) {
        expected = ShortReportSize;
    } else if (bytes[0] == LongReportId) {
        expected = LongReportSize;
    }
    if (expected == 0 || bytes.size() != expected) {
        return std::nullopt;
    }
    Report report;
    std::ranges::copy(bytes, report.m_bytes.begin());
    report.m_size = expected;
    return report;
}

Report Report::shortRequest(std::uint8_t deviceIndex, std::uint8_t subId, std::uint8_t address, std::array<std::uint8_t, 3> params)
{
    Report report;
    report.m_bytes = {ShortReportId, deviceIndex, subId, address, params[0], params[1], params[2]};
    report.m_size = ShortReportSize;
    return report;
}

std::span<const std::uint8_t> Report::params() const
{
    return std::span<const std::uint8_t>(m_bytes).subspan(HeaderSize, m_size - HeaderSize);
}

std::span<const std::uint8_t> Report::bytes() const
{
    return std::span<const std::uint8_t>(m_bytes).first(m_size);
}

Report Report::withParamsZeroed(std::size_t offset, std::size_t count) const
{
    Report copy = *this;
    const std::size_t paramsSize = m_size - HeaderSize;
    const std::size_t begin = std::min(offset, paramsSize);
    const std::size_t end = std::min(begin + count, paramsSize);
    std::ranges::fill(std::span<std::uint8_t>(copy.m_bytes).subspan(HeaderSize + begin, end - begin), std::uint8_t(0));
    return copy;
}

bool Report::operator==(const Report &other) const
{
    return std::ranges::equal(bytes(), other.bytes());
}

ReplyMatch matchReply(const Report &request, const Report &candidate)
{
    if (candidate.deviceIndex() != request.deviceIndex()) {
        return ReplyMatch::None;
    }
    if (candidate.subId() == request.subId() && candidate.address() == request.address()) {
        return ReplyMatch::Reply;
    }
    // Error replies shift the request's header bytes one position right.
    if (candidate.address() != request.subId() || candidate.params()[0] != request.address()) {
        return ReplyMatch::None;
    }
    if (candidate.subId() == Error10SubId) {
        return ReplyMatch::Error10;
    }
    if (candidate.subId() == Error20FeatureIndex) {
        return ReplyMatch::Error20;
    }
    return ReplyMatch::None;
}

QDebug operator<<(QDebug debug, const Report &report)
{
    const QDebugStateSaver saver(debug);
    const auto bytes = report.bytes();
    debug.noquote().nospace() << QByteArrayView(bytes.data(), qsizetype(bytes.size())).toByteArray().toHex(' ');
    return debug;
}

QDebug operator<<(QDebug debug, const Error &error)
{
    const QDebugStateSaver saver(debug);
    debug.nospace();
    switch (error.kind) {
    case ErrorKind::Transport:
        debug << "transport error, errno " << error.code;
        break;
    case ErrorKind::PermissionDenied:
        debug << "permission denied";
        break;
    case ErrorKind::NotFound:
        debug << "not found";
        break;
    case ErrorKind::Disconnected:
        debug << "disconnected";
        break;
    case ErrorKind::Timeout:
        debug << "timeout";
        break;
    case ErrorKind::Protocol10:
        debug << "HID++ 1.0 error " << Qt::hex << Qt::showbase << error.code;
        break;
    case ErrorKind::Protocol20:
        debug << "HID++ 2.0 error " << Qt::hex << Qt::showbase << error.code;
        break;
    case ErrorKind::Malformed:
        debug << "malformed reply";
        break;
    }
    return debug;
}

} // namespace hidpp
