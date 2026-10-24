// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "dispatcher.h"
#include "result.h"

#include <cstdint>
#include <span>

namespace hidpp
{

// A receiver (Bolt, Unifying) speaking HID++ 1.0 at index 0xFF, with its
// devices at indices 1 to 6.
class Receiver
{
public:
    static constexpr std::uint8_t FirstDeviceIndex = 1;
    static constexpr std::uint8_t LastDeviceIndex = 6;

    explicit Receiver(Dispatcher &dispatcher);

    // Register 0x02, read with sub id 0x81; payload byte 1 is the number of
    // connected devices (verified on the Bolt receiver).
    void readConnectedDeviceCount(Callback<std::uint8_t> done) const;

    static Result<std::uint8_t> parseConnectedDeviceCount(std::span<const std::uint8_t> params);

    // The answer of the Bolt receiver for an index with nothing paired:
    // HID++ 1.0 error 0x09, resource error (verified on the desk).
    static bool isEmptySlot(const Error &error);

private:
    Dispatcher *m_dispatcher;
};

} // namespace hidpp
