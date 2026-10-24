// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include <KSharedConfig>

#include <QObject>
#include <QString>

// Starting at login through the background portal (PLAN.md section 12):
// RequestBackground with autostart writes or removes the autostart entry, and
// the user's desktop may ask them first. Hidsight then starts with
// --background, as a tray item without the window.
class Autostart : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    // A request is waiting for the portal's answer.
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    // Why the last request failed; empty when it did not.
    Q_PROPERTY(QString problem READ problem NOTIFY problemChanged)

public:
    explicit Autostart(KSharedConfig::Ptr config = KSharedConfig::openConfig(), QObject *parent = nullptr);

    bool enabled() const;
    // Asks the portal; enabled changes once it agrees.
    void setEnabled(bool enabled);
    bool busy() const;
    QString problem() const;

Q_SIGNALS:
    void enabledChanged();
    void busyChanged();
    void problemChanged();

private Q_SLOTS:
    // org.freedesktop.portal.Request.Response
    void handleResponse(uint response, const QVariantMap &results);

private:
    void finish(bool enabled, const QString &problem);

    KSharedConfig::Ptr m_config;
    QString m_requestPath;
    bool m_requested = false;
    QString m_problem;
};
