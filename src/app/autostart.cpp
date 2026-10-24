// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "autostart.h"
#include "app_logging.h"

#include <KConfigGroup>
#include <KLocalizedString>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QRandomGenerator>

using namespace Qt::StringLiterals;

namespace
{

const auto AutostartKey = "Autostart";

const auto PortalService = u"org.freedesktop.portal.Desktop"_s;
const auto PortalPath = u"/org/freedesktop/portal/desktop"_s;

// The documented path of the Request object for a handle token, so the
// answer cannot slip past before the connection to it exists.
QString requestPath(const QString &token)
{
    QString sender = QDBusConnection::sessionBus().baseService().mid(1);
    sender.replace(u'.', u'_');
    return u"/org/freedesktop/portal/desktop/request/%1/%2"_s.arg(sender, token);
}

} // namespace

Autostart::Autostart(KSharedConfig::Ptr config, QObject *parent)
    : QObject(parent)
    , m_config(std::move(config))
{
}

bool Autostart::enabled() const
{
    return m_config->group(u"General"_s).readEntry(AutostartKey, false);
}

void Autostart::setEnabled(bool enabled)
{
    if (busy() || enabled == this->enabled()) {
        return;
    }
    const QString token = u"hidsight%1"_s.arg(QRandomGenerator::global()->generate());
    m_requestPath = requestPath(token);
    QDBusConnection::sessionBus()
        .connect(PortalService, m_requestPath, u"org.freedesktop.portal.Request"_s, u"Response"_s, this, SLOT(handleResponse(uint, QVariantMap)));

    QDBusMessage call = QDBusMessage::createMethodCall(PortalService, PortalPath, u"org.freedesktop.portal.Background"_s, u"RequestBackground"_s);
    const QVariantMap options{
        {u"handle_token"_s, token},
        {u"reason"_s, i18nc("@info asked by the desktop", "Show the battery of your keyboards and mice in the system tray after login.")},
        {u"autostart"_s, enabled},
        {u"commandline"_s, QStringList{u"hidsight"_s, u"--background"_s}},
        {u"dbus-activatable"_s, false},
    };
    call << QString() << options;
    m_requested = enabled;
    Q_EMIT busyChanged();

    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(call), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        call->deleteLater();
        const QDBusPendingReply<QDBusObjectPath> reply = *call;
        if (reply.isError()) {
            qCDebug(HIDSIGHT_APP) << "background portal:" << reply.error().message();
            finish(this->enabled(), i18nc("@info", "The desktop does not offer starting at login (%1).", reply.error().message()));
        }
    });
}

bool Autostart::busy() const
{
    return !m_requestPath.isEmpty();
}

QString Autostart::problem() const
{
    return m_problem;
}

void Autostart::handleResponse(uint response, const QVariantMap &results)
{
    // 0 is success, 1 cancelled by the user, 2 anything else.
    const bool granted = response == 0 && results.value(u"autostart"_s).toBool() == m_requested;
    qCDebug(HIDSIGHT_APP) << "background portal answered" << response << results;
    if (granted) {
        finish(m_requested, QString());
    } else if (response == 1) {
        finish(enabled(), QString());
    } else {
        finish(enabled(), i18nc("@info", "The desktop did not allow starting at login."));
    }
}

void Autostart::finish(bool enabled, const QString &problem)
{
    QDBusConnection::sessionBus()
        .disconnect(PortalService, m_requestPath, u"org.freedesktop.portal.Request"_s, u"Response"_s, this, SLOT(handleResponse(uint, QVariantMap)));
    m_requestPath.clear();
    Q_EMIT busyChanged();

    if (enabled != this->enabled()) {
        KConfigGroup general = m_config->group(u"General"_s);
        general.writeEntry(AutostartKey, enabled);
        m_config->sync();
    }
    // Also after a refusal, so a switch the user flipped snaps back.
    Q_EMIT enabledChanged();
    if (problem != m_problem) {
        m_problem = problem;
        Q_EMIT problemChanged();
    }
}
