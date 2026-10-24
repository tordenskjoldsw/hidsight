// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "hidpp/receiversession.h"

#include <QString>

#include <map>
#include <vector>

// Decides which notifications a change of a device deserves (PLAN.md
// section 8): the level the device reports falling to low or critical while
// it is not on its cable, and charging reaching complete. Repeated states and
// the bouncing of a cable contact (seen on the desk: charging, not charging,
// charging within 350 ms) raise nothing.
class BatteryAlerts
{
public:
    enum class Alert {
        BatteryLow,
        BatteryCritical,
        ChargeComplete,
    };

    // The alerts for the new state of the device with this key.
    std::vector<Alert> update(const QString &key, const hidpp::DeviceState &state);
    void forget(const QString &key);

private:
    struct Known {
        hidpp::BatteryLevel level = hidpp::BatteryLevel::Unknown;
        hidpp::ChargingState charging = hidpp::ChargingState::Unknown;
    };

    std::map<QString, Known> m_known;
};
