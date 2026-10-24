// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "devicemanager.h"
#include "app_logging.h"

#include "hidpp/hidrawchannel.h"
#include "hidpp/keys.h"
#include "hidpp/receiversession.h"

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
{
    connect(&m_monitor, &DeviceMonitor::nodeAdded, this, &DeviceManager::addNode);
    connect(&m_monitor, &DeviceMonitor::nodeRemoved, this, &DeviceManager::removeNode);
}

DeviceManager::~DeviceManager() = default;

bool DeviceManager::start()
{
    return m_monitor.start();
}

DeviceModel *DeviceManager::devices()
{
    return &m_model;
}

bool DeviceManager::accessDenied() const
{
    return !m_deniedNodes.isEmpty();
}

void DeviceManager::addNode(const QString &node)
{
    if (m_sessions.contains(node)) {
        return;
    }
    auto channel = hidpp::HidrawChannel::open(node);
    if (!channel) {
        qCDebug(HIDSIGHT_APP) << "cannot open" << node << channel.error();
        setDenied(node, channel.error().kind == hidpp::ErrorKind::PermissionDenied);
        return;
    }
    setDenied(node, false);

    auto session = std::make_unique<hidpp::ReceiverSession>(std::move(channel).value());
    // Measures criterion 5 on the desk: from the node appearing to each
    // device in the list.
    QElapsedTimer clock;
    clock.start();
    connect(session.get(), &hidpp::ReceiverSession::deviceChanged, this, [this, node, clock](const hidpp::DeviceState &state) {
        qCDebug(HIDSIGHT_APP).nospace() << clock.elapsed() << " ms after " << node << ": device " << state.index << ' ' << hidpp::key(state.status) << ", "
                                        << (state.name ? state.name->name : QString()) << ", battery "
                                        << (state.battery && state.battery->percentage ? int(*state.battery->percentage) : -1);
        m_model.updateDevice(node, state);
    });
    connect(session.get(), &hidpp::ReceiverSession::deviceRemoved, this, [this, node](std::uint8_t index) {
        m_model.removeDevice(node, index);
    });
    connect(session.get(), &hidpp::ReceiverSession::failed, this, [this, node](const hidpp::Error &error) {
        qCDebug(HIDSIGHT_APP) << "session on" << node << "ended:" << error;
        removeNode(node);
    });
    session->start();
    m_sessions.emplace(node, std::move(session));
}

void DeviceManager::removeNode(const QString &node)
{
    setDenied(node, false);
    m_model.removeReceiver(node);
    const auto it = m_sessions.find(node);
    if (it == m_sessions.end()) {
        return;
    }
    // The session may be the sender of the signal that brought us here.
    hidpp::ReceiverSession *session = it->second.release();
    m_sessions.erase(it);
    disconnect(session, nullptr, this, nullptr);
    session->deleteLater();
}

void DeviceManager::setDenied(const QString &node, bool denied)
{
    const bool before = accessDenied();
    if (denied) {
        m_deniedNodes.insert(node);
    } else {
        m_deniedNodes.remove(node);
    }
    if (accessDenied() != before) {
        Q_EMIT accessDeniedChanged();
    }
}
