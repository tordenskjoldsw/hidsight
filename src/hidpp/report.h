// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

class QDebug;

namespace hidpp
{

inline constexpr std::uint8_t ShortReportId = 0x10;
inline constexpr std::uint8_t LongReportId = 0x11;
inline constexpr std::size_t ShortReportSize = 7;
inline constexpr std::size_t LongReportSize = 20;
inline constexpr std::size_t HeaderSize = 4;

// The receiver itself; a device connected without a receiver also answers here.
inline constexpr std::uint8_t ReceiverIndex = 0xFF;

// Marks Hidsight's HID++ 2.0 requests, so replies to other tools on the same
// node (Solaar, logiops) are not taken for ours.
inline constexpr std::uint8_t SoftwareId = 0x0A;

// Byte 2 of an error reply: HID++ 1.0 sub id and HID++ 2.0 feature index.
inline constexpr std::uint8_t Error10SubId = 0x8F;
inline constexpr std::uint8_t Error20FeatureIndex = 0xFF;

// Byte 3 of a HID++ 2.0 request: the function in the high nibble, the
// software id in the low one.
constexpr std::uint8_t functionByte(std::uint8_t function)
{
    return static_cast<std::uint8_t>((function << 4) | SoftwareId);
}

// One HID++ report: report id, device index, two header bytes, parameters.
// In HID++ 1.0 the header bytes are the sub id and the register address; in
// HID++ 2.0 they are the feature index and the function byte.
class Report
{
public:
    // Accepts only a short report of exactly 7 bytes or a long report of
    // exactly 20; anything else read from a node is not HID++.
    static std::optional<Report> fromBytes(std::span<const std::uint8_t> bytes);

    // Every request Hidsight sends fits a short report.
    static Report shortRequest(std::uint8_t deviceIndex, std::uint8_t subId, std::uint8_t address, std::array<std::uint8_t, 3> params = {});

    std::uint8_t reportId() const
    {
        return m_bytes[0];
    }

    std::uint8_t deviceIndex() const
    {
        return m_bytes[1];
    }

    std::uint8_t subId() const
    {
        return m_bytes[2];
    }

    std::uint8_t address() const
    {
        return m_bytes[3];
    }

    // 3 bytes in a short report, 16 in a long one.
    std::span<const std::uint8_t> params() const;
    std::span<const std::uint8_t> bytes() const;

    // The error code of an error reply.
    std::uint8_t errorCode() const
    {
        return m_bytes[5];
    }

    bool operator==(const Report &other) const;

private:
    Report() = default;

    std::array<std::uint8_t, LongReportSize> m_bytes{};
    std::size_t m_size = ShortReportSize;
};

enum class ReplyMatch {
    None,
    Reply,
    Error10,
    Error20,
};

// Whether candidate answers request: same device index and the same two
// header bytes, or an error reply that repeats them.
ReplyMatch matchReply(const Report &request, const Report &candidate);

// Hex bytes; only for the hidsight.raw category.
QDebug operator<<(QDebug debug, const Report &report);

} // namespace hidpp
