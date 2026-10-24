// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "devicemodel.h"
#include "devicemonitor.h"

#include <QElapsedTimer>
#include <QObject>
#include <QSet>

#include <map>
#include <memory>

namespace hidpp
{
class ReceiverSession;
}

// Opens every receiver node DeviceMonitor finds, keeps a session on it for as
// long as the node exists and feeds the model.
class DeviceManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(DeviceModel *devices READ devices CONSTANT)
    // A receiver node exists but the udev rule does not grant access to it.
    Q_PROPERTY(bool accessDenied READ accessDenied NOTIFY accessDeniedChanged)

public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager() override;

    // False when udev is not available.
    bool start();

    DeviceModel *devices();
    bool accessDenied() const;

Q_SIGNALS:
    void accessDeniedChanged();

private:
    void addNode(const QString &node);
    void removeNode(const QString &node);
    void setDenied(const QString &node, bool denied);

    DeviceMonitor m_monitor;
    DeviceModel m_model;
    std::map<QString, std::unique_ptr<hidpp::ReceiverSession>> m_sessions;
    QSet<QString> m_deniedNodes;
};
