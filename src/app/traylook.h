// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "hidpp/receiversession.h"

#include <QString>

#include <vector>

// What the tray item shows for a set of devices (PLAN.md section 8): the
// battery of the device with the lowest charge, as one of Breeze's symbolic
// battery icons, and every device in the tooltip.
struct TrayLook {
    enum class Status {
        // No device: the item may hide in the tray's overflow.
        Passive,
        Active,
        // A device reports a critical battery.
        NeedsAttention,
    };

    QString iconName;
    Status status = Status::Passive;
    // Rich text; device names are escaped.
    QString toolTip;

    static TrayLook forDevices(const std::vector<hidpp::DeviceState> &devices);
};
