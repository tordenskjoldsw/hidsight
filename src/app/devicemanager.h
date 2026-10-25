// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "devicemodel.h"
#include "devicemonitor.h"
#include "devicenames.h"

#include "hidpp/report.h"

#include <QObject>
#include <QSet>

#include <map>
#include <memory>

class DeviceDetails;

namespace hidpp
{
class ReceiverSession;
struct DeviceState;
}

// Opens every receiver node DeviceMonitor finds, keeps a session on it for as
// long as the node exists and feeds the model.
class DeviceManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(DeviceModel *devices READ devices CONSTANT)
    // A receiver node exists but the udev rule does not grant access to it.
    Q_PROPERTY(bool accessDenied READ accessDenied NOTIFY accessDeniedChanged)
    // False when udev is not available and no receiver can be found.
    Q_PROPERTY(bool watching READ watching NOTIFY watchingChanged)

public:
    explicit DeviceManager(DeviceNames names = DeviceNames(), QObject *parent = nullptr);
    ~DeviceManager() override;

    void start();
    // For the sessions of receivers found from now on; see Dispatcher.
    void setSoftwareId(std::uint8_t softwareId);

    DeviceModel *devices();
    bool accessDenied() const;
    bool watching() const;

    // Reads every device again: the battery of those that answer, and those
    // that did not answer last time.
    Q_INVOKABLE void refresh();
    // A live view of one device for the device page; the caller owns it.
    Q_INVOKABLE DeviceDetails *details(const QString &key);

#ifdef HIDSIGHT_DEVELOPER
    // Shows the devices of a capture instead of the receivers, to try out
    // the window, the tray and the notifications without a device that runs
    // down (PLAN.md Phase 4). False when the capture cannot be read.
    bool startReplay(const QString &path);
#endif

Q_SIGNALS:
    void accessDeniedChanged();
    void watchingChanged();
    // Every change of a device, with its remembered name filled in.
    void deviceUpdated(const QString &key, const hidpp::DeviceState &state);
    void deviceGone(const QString &key);

private:
    void addNode(const QString &node, const QString &receiverId);
    void attach(const QString &node, const QString &receiverId, std::unique_ptr<hidpp::ReceiverSession> session);
    void removeNode(const QString &node);
    void updateDevice(const QString &node, hidpp::DeviceState state);
    void setDenied(const QString &node, bool denied);

    DeviceMonitor m_monitor;
    DeviceModel m_model;
    DeviceNames m_names;
    std::map<QString, std::unique_ptr<hidpp::ReceiverSession>> m_sessions;
    std::map<QString, QString> m_receiverIds;
    QSet<QString> m_deniedNodes;
    bool m_watching = false;
    std::uint8_t m_softwareId = hidpp::DefaultSoftwareId;
};
