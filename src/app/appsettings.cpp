// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "appsettings.h"

#include <KConfigGroup>

using namespace Qt::StringLiterals;

namespace
{

const auto NotifyLowBatteryKey = "NotifyLowBattery";
const auto NotifyChargeCompleteKey = "NotifyChargeComplete";
const auto StartHiddenKey = "StartHidden";
const auto ShowTechnicalDetailsKey = "ShowTechnicalDetails";

} // namespace

AppSettings::AppSettings(KSharedConfig::Ptr config, QObject *parent)
    : QObject(parent)
    , m_config(std::move(config))
{
}

bool AppSettings::notifyLowBattery() const
{
    return read(NotifyLowBatteryKey, true);
}

void AppSettings::setNotifyLowBattery(bool enabled)
{
    write(NotifyLowBatteryKey, enabled);
}

bool AppSettings::notifyChargeComplete() const
{
    return read(NotifyChargeCompleteKey, true);
}

void AppSettings::setNotifyChargeComplete(bool enabled)
{
    write(NotifyChargeCompleteKey, enabled);
}

bool AppSettings::startHidden() const
{
    return read(StartHiddenKey, false);
}

void AppSettings::setStartHidden(bool enabled)
{
    write(StartHiddenKey, enabled);
}

bool AppSettings::showTechnicalDetails() const
{
    return read(ShowTechnicalDetailsKey, false);
}

void AppSettings::setShowTechnicalDetails(bool enabled)
{
    write(ShowTechnicalDetailsKey, enabled);
}

bool AppSettings::read(const char *key, bool fallback) const
{
    return m_config->group(u"General"_s).readEntry(key, fallback);
}

void AppSettings::write(const char *key, bool value)
{
    if (read(key, !value) == value) {
        return;
    }
    KConfigGroup general = m_config->group(u"General"_s);
    general.writeEntry(key, value);
    m_config->sync();
    Q_EMIT changed();
}
