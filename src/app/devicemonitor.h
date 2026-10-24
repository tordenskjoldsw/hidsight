// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QSet>
#include <QString>

#include <memory>

class QSocketNotifier;
struct udev;
struct udev_device;
struct udev_enumerate;
struct udev_monitor;

// Finds the HID++ hidraw nodes of Logitech receivers through libudev, at
// start and when they come and go.
class DeviceMonitor : public QObject
{
    Q_OBJECT

public:
    explicit DeviceMonitor(QObject *parent = nullptr);
    ~DeviceMonitor() override;

    // Reports the nodes present now through nodeAdded and then watches for
    // changes. False when udev is not available.
    bool start();

Q_SIGNALS:
    void nodeAdded(const QString &devnode);
    void nodeRemoved(const QString &devnode);

private:
    struct UdevDeleter {
        void operator()(udev *handle) const;
        void operator()(udev_monitor *handle) const;
        void operator()(udev_device *handle) const;
        void operator()(udev_enumerate *handle) const;
    };
    using UdevDevicePtr = std::unique_ptr<udev_device, UdevDeleter>;

    void handleEvent();
    void consider(udev_device *device);
    void forget(const QString &devnode);

    std::unique_ptr<udev, UdevDeleter> m_udev;
    std::unique_ptr<udev_monitor, UdevDeleter> m_monitor;
    std::unique_ptr<QSocketNotifier> m_notifier;
    QSet<QString> m_nodes;
};
