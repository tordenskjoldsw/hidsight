// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "batteryalerts.h"

#include <QObject>

class AppSettings;
class DeviceManager;

// Sends the notifications BatteryAlerts decides on, as far as the settings
// allow; the events are in hidsight.notifyrc.
class Notifier : public QObject
{
    Q_OBJECT

public:
    Notifier(DeviceManager *devices, AppSettings *settings, QObject *parent = nullptr);

private:
    void handle(const QString &key, const hidpp::DeviceState &state);

    AppSettings *m_settings;
    BatteryAlerts m_alerts;
};
