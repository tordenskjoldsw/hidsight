// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "report.h"
#include "result.h"

#include <QObject>

#include <optional>

namespace hidpp
{

// The transport of HID++ reports. Production uses HidrawChannel; tests replay
// recorded captures through ReplayChannel.
class Channel : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    // Writes one report. An error means the channel is no longer usable.
    virtual std::optional<Error> send(const Report &report) = 0;

    // Delivers the reports that are already waiting, so a late reply to an
    // earlier request is not taken for the answer to the next one.
    virtual void drain() = 0;

Q_SIGNALS:
    void reportReceived(const hidpp::Report &report);
    void disconnected();
};

} // namespace hidpp
