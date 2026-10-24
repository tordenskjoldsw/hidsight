// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "hidpp/devicename.h"

#include <KSharedConfig>

#include <optional>

// Remembers the name and kind of the device in each slot of a receiver
// model, so a device that is asleep when Hidsight starts is listed by name
// (PLAN.md section 12). Nothing else is stored.
class DeviceNames
{
public:
    explicit DeviceNames(KSharedConfig::Ptr config = KSharedConfig::openStateConfig());

    std::optional<hidpp::DeviceName> recall(const QString &receiverId, std::uint8_t index) const;
    void remember(const QString &receiverId, std::uint8_t index, const hidpp::DeviceName &name);
    void forget(const QString &receiverId, std::uint8_t index);

private:
    KConfigGroup group(const QString &receiverId, std::uint8_t index) const;

    KSharedConfig::Ptr m_config;
};
