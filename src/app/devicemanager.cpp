// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "devicemanager.h"
#include "app_logging.h"
#include "devicedetails.h"

#include "hidpp/capture.h"
#include "hidpp/hidrawchannel.h"
#include "hidpp/keys.h"
#include "hidpp/receiversession.h"

#include <QElapsedTimer>
#include <QFile>

using namespace Qt::StringLiterals;

DeviceManager::DeviceManager(DeviceNames names, QObject *parent)
    : QObject(parent)
    , m_names(std::move(names))
{
    connect(&m_monitor, &DeviceMonitor::nodeAdded, this, &DeviceManager::addNode);
    connect(&m_monitor, &DeviceMonitor::nodeRemoved, this, &DeviceManager::removeNode);
}

DeviceManager::~DeviceManager() = default;

void DeviceManager::start()
{
    m_watching = m_monitor.start();
    Q_EMIT watchingChanged();
}

DeviceModel *DeviceManager::devices()
{
    return &m_model;
}

bool DeviceManager::accessDenied() const
{
    return !m_deniedNodes.isEmpty();
}

bool DeviceManager::watching() const
{
    return m_watching;
}

void DeviceManager::refresh()
{
    for (const auto &[node, session] : m_sessions) {
        session->rereadAll();
    }
}

DeviceDetails *DeviceManager::details(const QString &key)
{
    // Without a parent; QML takes ownership of what an invokable returns.
    return new DeviceDetails(&m_model, key);
}

void DeviceManager::addNode(const QString &node, const QString &receiverId)
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
    attach(node, receiverId, std::make_unique<hidpp::ReceiverSession>(std::move(channel).value()));
}

void DeviceManager::attach(const QString &node, const QString &receiverId, std::unique_ptr<hidpp::ReceiverSession> session)
{
    m_receiverIds[node] = receiverId;
    // Measures criterion 5 on the desk: from the node appearing to each
    // device in the list.
    QElapsedTimer clock;
    clock.start();
    connect(session.get(), &hidpp::ReceiverSession::deviceChanged, this, [this, node, clock](const hidpp::DeviceState &state) {
        qCDebug(HIDSIGHT_APP).nospace() << clock.elapsed() << " ms after " << node << ": device " << state.index << ' ' << hidpp::key(state.status) << ", "
                                        << (state.name ? state.name->name : QString()) << ", battery "
                                        << (state.battery && state.battery->percentage ? int(*state.battery->percentage) : -1);
        updateDevice(node, state);
    });
    connect(session.get(), &hidpp::ReceiverSession::deviceRemoved, this, [this, node](std::uint8_t index) {
        m_names.forget(m_receiverIds[node], index);
        m_model.removeDevice(node, index);
        Q_EMIT deviceGone(DeviceModel::keyFor(node, index));
    });
    connect(session.get(), &hidpp::ReceiverSession::failed, this, [this, node](const hidpp::Error &error) {
        qCDebug(HIDSIGHT_APP) << "session on" << node << "ended:" << error;
        removeNode(node);
    });
    session->start();
    m_sessions.emplace(node, std::move(session));
}

#ifdef HIDSIGHT_DEVELOPER
bool DeviceManager::startReplay(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(HIDSIGHT_APP) << "cannot open the capture" << path;
        return false;
    }
    auto entries = hidpp::parseCapture(file.readAll());
    if (!entries) {
        qCWarning(HIDSIGHT_APP) << "the capture is malformed at line" << entries.error().code;
        return false;
    }
    auto channel = std::make_unique<hidpp::ReplayChannel>(std::move(entries).value());
    const std::uint8_t softwareId = channel->softwareId();
    auto session = std::make_unique<hidpp::ReceiverSession>(std::move(channel));
    session->setSoftwareId(softwareId);
    // Replayed answers arrive at once.
    session->setTimeout(std::chrono::milliseconds(50));
    m_watching = true;
    Q_EMIT watchingChanged();
    attach(u"replay"_s, u"046d:c548"_s, std::move(session));
    return true;
}
#endif

void DeviceManager::removeNode(const QString &node)
{
    setDenied(node, false);
    const QStringList gone = m_model.keysOf(node);
    m_model.removeReceiver(node);
    for (const QString &key : gone) {
        Q_EMIT deviceGone(key);
    }
    m_receiverIds.erase(node);
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

void DeviceManager::updateDevice(const QString &node, hidpp::DeviceState state)
{
    const QString &receiverId = m_receiverIds[node];
    if (state.name) {
        m_names.remember(receiverId, state.index, *state.name);
    } else {
        state.name = m_names.recall(receiverId, state.index);
    }
    m_model.updateDevice(node, receiverId, state);
    Q_EMIT deviceUpdated(DeviceModel::keyFor(node, state.index), state);
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
