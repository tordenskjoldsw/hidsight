// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "receiver.h"

namespace hidpp
{

namespace
{

constexpr std::uint8_t ReadShortRegister = 0x81;
constexpr std::uint8_t ConnectionStateRegister = 0x02;
constexpr int ResourceError = 0x09;

} // namespace

Receiver::Receiver(Dispatcher &dispatcher)
    : m_dispatcher(&dispatcher)
{
}

void Receiver::readConnectedDeviceCount(Callback<std::uint8_t> done) const
{
    m_dispatcher->request(Report::shortRequest(ReceiverIndex, ReadShortRegister, ConnectionStateRegister), [done = std::move(done)](Result<Report> reply) {
        if (!reply) {
            done(reply.error());
            return;
        }
        done(parseConnectedDeviceCount(reply.value().params()));
    });
}

Result<std::uint8_t> Receiver::parseConnectedDeviceCount(std::span<const std::uint8_t> params)
{
    if (params.size() < 2 || params[1] > LastDeviceIndex) {
        return Error{ErrorKind::Malformed, 0};
    }
    return params[1];
}

bool Receiver::isEmptySlot(const Error &error)
{
    return error.kind == ErrorKind::Protocol10 && error.code == ResourceError;
}

} // namespace hidpp
