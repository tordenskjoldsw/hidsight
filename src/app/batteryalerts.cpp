// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "batteryalerts.h"

using hidpp::BatteryLevel;
using hidpp::ChargingState;

namespace
{

// How urgent a level is; only a rise in urgency raises an alert.
int urgency(BatteryLevel level)
{
    switch (level) {
    case BatteryLevel::Critical:
        return 2;
    case BatteryLevel::Low:
        return 1;
    case BatteryLevel::Good:
    case BatteryLevel::Full:
    case BatteryLevel::Unknown:
        break;
    }
    return 0;
}

bool isCharging(ChargingState state)
{
    return state == ChargingState::Charging || state == ChargingState::ChargingSlowly;
}

} // namespace

std::vector<BatteryAlerts::Alert> BatteryAlerts::update(const QString &key, const hidpp::DeviceState &state)
{
    std::vector<Alert> alerts;
    if (!state.battery || state.status != hidpp::DeviceStatus::Online) {
        return alerts;
    }
    const hidpp::Battery &battery = *state.battery;
    const auto previous = m_known.find(key);
    const Known before = previous == m_known.end() ? Known{} : previous->second;

    const bool onCable = isCharging(battery.charging) || battery.charging == ChargingState::Complete || battery.externalPower.value_or(false);
    if (!onCable && urgency(battery.level) > urgency(before.level)) {
        alerts.push_back(battery.level == BatteryLevel::Critical ? Alert::BatteryCritical : Alert::BatteryLow);
    }
    if (battery.charging == ChargingState::Complete && isCharging(before.charging)) {
        alerts.push_back(Alert::ChargeComplete);
    }

    m_known[key] = Known{battery.level, battery.charging};
    return alerts;
}

void BatteryAlerts::forget(const QString &key)
{
    m_known.erase(key);
}
