// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace hidpp
{

// The kernel refuses larger report descriptors (HID_MAX_DESCRIPTOR_SIZE).
inline constexpr std::size_t MaxReportDescriptorSize = 4096;

// Whether a HID report descriptor declares HID++: the vendor usage page
// 0xFF00 with report ids 0x10 and 0x11. Verified on the desk: of the four
// hidraw nodes of the Bolt receiver only the HID++ one does. A malformed or
// oversized descriptor is not HID++.
bool isHidppReportDescriptor(std::span<const std::uint8_t> descriptor);

} // namespace hidpp
