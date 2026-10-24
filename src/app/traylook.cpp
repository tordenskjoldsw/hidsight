// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "traylook.h"
#include "devicetexts.h"

#include <KLocalizedString>

#include <QStringList>

#include <optional>

using namespace Qt::StringLiterals;
using hidpp::BatteryLevel;

namespace
{

// A charge to compare devices by: the percentage, or the reported level.
std::optional<int> charge(const hidpp::Battery &battery)
{
    if (battery.percentage) {
        return *battery.percentage;
    }
    switch (battery.level) {
    case BatteryLevel::Full:
        return 100;
    case BatteryLevel::Good:
        return 60;
    case BatteryLevel::Low:
        return 20;
    case BatteryLevel::Critical:
        return 0;
    case BatteryLevel::Unknown:
        break;
    }
    return std::nullopt;
}

} // namespace

TrayLook TrayLook::forDevices(const std::vector<hidpp::DeviceState> &devices)
{
    TrayLook look;
    look.iconName = u"de.tordenskjold.hidsight"_s;
    if (devices.empty()) {
        look.toolTip = i18nc("@info:tooltip", "No receiver found");
        return look;
    }
    look.status = Status::Active;

    const hidpp::DeviceState *weakest = nullptr;
    std::optional<int> weakestCharge;
    QStringList lines;
    for (const hidpp::DeviceState &state : devices) {
        const QString name = state.name ? state.name->name : i18nc("@info device without a name yet", "Device %1", state.index);
        lines << i18nc("@info:tooltip device name, status", "%1: %2", name.toHtmlEscaped(), DeviceTexts::statusText(state).toHtmlEscaped());

        if (state.status != hidpp::DeviceStatus::Online || !state.battery) {
            continue;
        }
        if (state.battery->level == BatteryLevel::Critical) {
            look.status = Status::NeedsAttention;
        }
        const auto value = charge(*state.battery);
        if (value && (!weakestCharge || *value < *weakestCharge)) {
            weakest = &state;
            weakestCharge = value;
        }
    }
    if (weakest) {
        look.iconName = DeviceTexts::batteryIconName(weakest->battery) + u"-symbolic"_s;
    }
    look.toolTip = lines.join(u"<br/>"_s);
    return look;
}
