// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "reportdescriptor.h"
#include "report.h"

#include <array>

namespace hidpp
{

namespace
{

constexpr std::uint32_t VendorUsagePage = 0xFF00;

// Item prefixes of the HID specification, section 6.2.2: tag in the high
// nibble, type in bits 2 and 3, data size in bits 0 and 1.
constexpr std::uint8_t LongItemPrefix = 0xFE;
constexpr std::uint8_t GlobalItem = 1;
constexpr std::uint8_t UsagePageTag = 0x0;
constexpr std::uint8_t ReportIdTag = 0x8;
constexpr std::uint8_t PushTag = 0xA;
constexpr std::uint8_t PopTag = 0xB;

// Push nests the global state; real descriptors use one level at most.
constexpr std::size_t MaxPushDepth = 8;

} // namespace

bool isHidppReportDescriptor(std::span<const std::uint8_t> descriptor)
{
    if (descriptor.size() > MaxReportDescriptorSize) {
        return false;
    }
    std::array<std::uint32_t, MaxPushDepth> pushedPages{};
    std::size_t depth = 0;
    std::uint32_t usagePage = 0;
    bool hasShortReport = false;
    bool hasLongReport = false;

    std::size_t position = 0;
    while (position < descriptor.size()) {
        const std::uint8_t prefix = descriptor[position];
        if (prefix == LongItemPrefix) {
            // Long items carry their size in the next byte and are never
            // usage pages or report ids.
            if (position + 1 >= descriptor.size()) {
                return false;
            }
            const std::size_t itemSize = 3 + std::size_t(descriptor[position + 1]);
            if (position + itemSize > descriptor.size()) {
                return false;
            }
            position += itemSize;
            continue;
        }
        const std::size_t size = (prefix & 0x03) == 3 ? 4 : (prefix & 0x03);
        if (position + 1 + size > descriptor.size()) {
            return false;
        }
        std::uint32_t data = 0;
        for (std::size_t i = 0; i < size; ++i) {
            data |= std::uint32_t(descriptor[position + 1 + i]) << (8 * i);
        }
        const std::uint8_t type = (prefix >> 2) & 0x03;
        const std::uint8_t tag = prefix >> 4;
        if (type == GlobalItem) {
            if (tag == UsagePageTag) {
                usagePage = data;
            } else if (tag == ReportIdTag && usagePage == VendorUsagePage) {
                hasShortReport = hasShortReport || data == ShortReportId;
                hasLongReport = hasLongReport || data == LongReportId;
            } else if (tag == PushTag) {
                if (depth == pushedPages.size()) {
                    return false;
                }
                pushedPages[depth++] = usagePage;
            } else if (tag == PopTag) {
                if (depth == 0) {
                    return false;
                }
                usagePage = pushedPages[--depth];
            }
        }
        position += 1 + size;
    }
    return hasShortReport && hasLongReport;
}

} // namespace hidpp
