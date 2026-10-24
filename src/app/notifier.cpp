// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "notifier.h"
#include "appsettings.h"
#include "devicemanager.h"
#include "devicetexts.h"

#include <KLocalizedString>
#include <KNotification>

using namespace Qt::StringLiterals;

Notifier::Notifier(DeviceManager *devices, AppSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    connect(devices, &DeviceManager::deviceUpdated, this, &Notifier::handle);
    connect(devices, &DeviceManager::deviceGone, this, [this](const QString &key) {
        m_alerts.forget(key);
    });
}

void Notifier::handle(const QString &key, const hidpp::DeviceState &state)
{
    const QString name = state.name ? state.name->name : i18nc("@info device without a name yet", "Device %1", state.index);
    const QString iconName = DeviceTexts::kindIconName(state.name ? state.name->kind : hidpp::DeviceKind::Unknown);
    for (const BatteryAlerts::Alert alert : m_alerts.update(key, state)) {
        QString event;
        QString title;
        QString text;
        switch (alert) {
        case BatteryAlerts::Alert::BatteryLow:
            if (!m_settings->notifyLowBattery()) {
                continue;
            }
            event = u"batteryLow"_s;
            title = i18nc("@title notification, device name", "%1: battery low", name);
            text = i18nc("@info notification, battery", "%1 left. Charge it soon.", DeviceTexts::batteryText(*state.battery));
            break;
        case BatteryAlerts::Alert::BatteryCritical:
            if (!m_settings->notifyLowBattery()) {
                continue;
            }
            event = u"batteryCritical"_s;
            title = i18nc("@title notification, device name", "%1: battery critically low", name);
            text = i18nc("@info notification, battery", "%1 left. Charge it now.", DeviceTexts::batteryText(*state.battery));
            break;
        case BatteryAlerts::Alert::ChargeComplete:
            if (!m_settings->notifyChargeComplete()) {
                continue;
            }
            event = u"chargeComplete"_s;
            title = i18nc("@title notification, device name", "%1: fully charged", name);
            text = i18nc("@info notification", "You can unplug the cable.");
            break;
        }
        // Owned by the notification server; it deletes itself once closed.
        auto *notification = new KNotification(event);
        notification->setTitle(title);
        notification->setText(text);
        notification->setIconName(iconName);
        notification->sendEvent();
    }
}
