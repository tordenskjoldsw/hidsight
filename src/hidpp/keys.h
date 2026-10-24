// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "battery.h"
#include "devicefwversion.h"
#include "devicename.h"
#include "result.h"

#include <QLatin1StringView>

namespace hidpp
{

// Stable, untranslated keys of the enum values, for JSON and for the model
// that QML reads, for example "keyboard", "full" or "discharging".
QLatin1StringView key(DeviceKind kind);
QLatin1StringView key(FirmwareType type);
QLatin1StringView key(BatteryLevel level);
QLatin1StringView key(ChargingState state);
QLatin1StringView key(Battery::Source source);
QLatin1StringView key(ErrorKind kind);

} // namespace hidpp
