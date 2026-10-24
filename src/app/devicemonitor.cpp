// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "devicemonitor.h"
#include "app_logging.h"

#include "hidpp/reportdescriptor.h"

#include <QFile>
#include <QSocketNotifier>

#include <libudev.h>

#include <optional>

using namespace Qt::StringLiterals;

namespace
{

constexpr unsigned UsbBus = 0x0003;
constexpr unsigned BluetoothBus = 0x0005;
constexpr unsigned LogitechVendor = 0x046D;

struct HidId {
    unsigned bus = 0;
    unsigned vendor = 0;
};

// HID_ID of the parent hid device, for example "0003:0000046D:0000C548".
std::optional<HidId> parseHidId(const char *value)
{
    if (!value) {
        return std::nullopt;
    }
    const QList<QByteArray> parts = QByteArray(value).split(':');
    if (parts.size() != 3) {
        return std::nullopt;
    }
    bool busOk = false;
    bool vendorOk = false;
    const HidId id{parts[0].toUInt(&busOk, 16), parts[1].toUInt(&vendorOk, 16)};
    if (!busOk || !vendorOk) {
        return std::nullopt;
    }
    return id;
}

bool hasHidppDescriptor(udev_device *hid)
{
    const char *syspath = udev_device_get_syspath(hid);
    if (!syspath) {
        return false;
    }
    QFile file(QFile::decodeName(syspath) + u"/report_descriptor"_s);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray descriptor = file.read(qint64(hidpp::MaxReportDescriptorSize) + 1);
    return hidpp::isHidppReportDescriptor(
        std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t *>(descriptor.constData()), std::size_t(descriptor.size())));
}

} // namespace

void DeviceMonitor::UdevDeleter::operator()(udev *handle) const
{
    udev_unref(handle);
}

void DeviceMonitor::UdevDeleter::operator()(udev_monitor *handle) const
{
    udev_monitor_unref(handle);
}

void DeviceMonitor::UdevDeleter::operator()(udev_device *handle) const
{
    udev_device_unref(handle);
}

void DeviceMonitor::UdevDeleter::operator()(udev_enumerate *handle) const
{
    udev_enumerate_unref(handle);
}

DeviceMonitor::DeviceMonitor(QObject *parent)
    : QObject(parent)
{
}

DeviceMonitor::~DeviceMonitor() = default;

bool DeviceMonitor::start()
{
    m_udev.reset(udev_new());
    if (!m_udev) {
        qCWarning(HIDSIGHT_APP) << "udev is not available";
        return false;
    }

    // The monitor first, so a node that appears during the enumeration is not
    // missed; consider() ignores nodes it already knows.
    m_monitor.reset(udev_monitor_new_from_netlink(m_udev.get(), "udev"));
    if (!m_monitor || udev_monitor_filter_add_match_subsystem_devtype(m_monitor.get(), "hidraw", nullptr) < 0
        || udev_monitor_enable_receiving(m_monitor.get()) < 0) {
        qCWarning(HIDSIGHT_APP) << "cannot watch for hidraw nodes";
        m_monitor.reset();
        return false;
    }
    m_notifier = std::make_unique<QSocketNotifier>(udev_monitor_get_fd(m_monitor.get()), QSocketNotifier::Read);
    connect(m_notifier.get(), &QSocketNotifier::activated, this, &DeviceMonitor::handleEvent);

    const std::unique_ptr<udev_enumerate, UdevDeleter> enumerate(udev_enumerate_new(m_udev.get()));
    if (!enumerate) {
        return false;
    }
    udev_enumerate_add_match_subsystem(enumerate.get(), "hidraw");
    udev_enumerate_scan_devices(enumerate.get());
    udev_list_entry *entry = nullptr;
    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(enumerate.get()))
    {
        const UdevDevicePtr device(udev_device_new_from_syspath(m_udev.get(), udev_list_entry_get_name(entry)));
        if (device) {
            consider(device.get());
        }
    }
    return true;
}

void DeviceMonitor::handleEvent()
{
    const UdevDevicePtr device(udev_monitor_receive_device(m_monitor.get()));
    if (!device) {
        return;
    }
    const char *action = udev_device_get_action(device.get());
    const char *devnode = udev_device_get_devnode(device.get());
    if (!action || !devnode) {
        return;
    }
    const QByteArrayView actionView(action);
    if (actionView == "add") {
        consider(device.get());
    } else if (actionView == "remove") {
        forget(QFile::decodeName(devnode));
    }
}

void DeviceMonitor::consider(udev_device *device)
{
    const char *devnodeName = udev_device_get_devnode(device);
    if (!devnodeName) {
        return;
    }
    const QString devnode = QFile::decodeName(devnodeName);
    if (m_nodes.contains(devnode)) {
        return;
    }
    // Owned by device; not unreferenced here.
    udev_device *hid = udev_device_get_parent_with_subsystem_devtype(device, "hid", nullptr);
    if (!hid) {
        return;
    }
    const auto id = parseHidId(udev_device_get_property_value(hid, "HID_ID"));
    if (!id || id->vendor != LogitechVendor) {
        return;
    }
    if (!hasHidppDescriptor(hid)) {
        return;
    }
    if (id->bus == BluetoothBus) {
        // Devices connected over Bluetooth wait for the decision in PLAN.md
        // section 12.
        qCDebug(HIDSIGHT_APP) << "skipping the Bluetooth HID++ node" << devnode;
        return;
    }
    if (id->bus != UsbBus) {
        return;
    }
    qCDebug(HIDSIGHT_APP) << "HID++ node" << devnode;
    m_nodes.insert(devnode);
    Q_EMIT nodeAdded(devnode);
}

void DeviceMonitor::forget(const QString &devnode)
{
    if (m_nodes.remove(devnode)) {
        qCDebug(HIDSIGHT_APP) << "HID++ node removed" << devnode;
        Q_EMIT nodeRemoved(devnode);
    }
}
