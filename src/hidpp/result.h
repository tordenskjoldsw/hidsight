// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include <QtGlobal>

#include <functional>
#include <utility>
#include <variant>

class QDebug;

namespace hidpp
{

enum class ErrorKind {
    // Opening, reading or writing the node failed; code is the errno.
    Transport,
    // The node exists but the udev rule does not grant access; code is the errno.
    PermissionDenied,
    // The node does not exist; code is the errno.
    NotFound,
    // The node went away, for example because the receiver was unplugged.
    Disconnected,
    // No matching reply within the timeout, also not after the retry.
    Timeout,
    // A HID++ 1.0 error reply; code is the error code.
    Protocol10,
    // A HID++ 2.0 error reply; code is the error code.
    Protocol20,
    // A reply that is too short or carries a value outside its documented range.
    Malformed,
};

struct Error {
    ErrorKind kind = ErrorKind::Transport;
    int code = 0;

    bool operator==(const Error &) const = default;
};

QDebug operator<<(QDebug debug, const Error &error);

// A value or the error that prevented it. Exceptions are disabled in this
// code base, so failures travel as values.
template<typename T>
class Result
{
public:
    Result(T value)
        : m_value(std::move(value))
    {
    }

    Result(Error error)
        : m_value(error)
    {
    }

    bool hasValue() const
    {
        return std::holds_alternative<T>(m_value);
    }

    explicit operator bool() const
    {
        return hasValue();
    }

    const T &value() const &
    {
        Q_ASSERT(hasValue());
        return *std::get_if<T>(&m_value);
    }

    T &&value() &&
    {
        Q_ASSERT(hasValue());
        return std::move(*std::get_if<T>(&m_value));
    }

    const Error &error() const
    {
        Q_ASSERT(!hasValue());
        return *std::get_if<Error>(&m_value);
    }

private:
    std::variant<T, Error> m_value;
};

template<typename T>
using Callback = std::function<void(Result<T>)>;

} // namespace hidpp
