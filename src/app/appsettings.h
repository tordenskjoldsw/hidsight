// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include <KSharedConfig>

#include <QObject>

// The user's choices (PLAN.md section 8), in hidsightrc. Notifications are on
// by default; starting hidden is off.
class AppSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool notifyLowBattery READ notifyLowBattery WRITE setNotifyLowBattery NOTIFY changed)
    Q_PROPERTY(bool notifyChargeComplete READ notifyChargeComplete WRITE setNotifyChargeComplete NOTIFY changed)
    // Start with only the tray item, without the window.
    Q_PROPERTY(bool startHidden READ startHidden WRITE setStartHidden NOTIFY changed)
    // The features and the protocol version on the device page; off, the
    // page shows what matters day to day.
    Q_PROPERTY(bool showTechnicalDetails READ showTechnicalDetails WRITE setShowTechnicalDetails NOTIFY changed)

public:
    explicit AppSettings(KSharedConfig::Ptr config = KSharedConfig::openConfig(), QObject *parent = nullptr);

    bool notifyLowBattery() const;
    void setNotifyLowBattery(bool enabled);
    bool notifyChargeComplete() const;
    void setNotifyChargeComplete(bool enabled);
    bool startHidden() const;
    void setStartHidden(bool enabled);
    bool showTechnicalDetails() const;
    void setShowTechnicalDetails(bool enabled);

Q_SIGNALS:
    void changed();

private:
    bool read(const char *key, bool fallback) const;
    void write(const char *key, bool value);

    KSharedConfig::Ptr m_config;
};
