// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "receiversession.h"
#include "hidpp_logging.h"
#include "receiver.h"

#include <array>

namespace hidpp
{

namespace
{

// Unified Battery first, the older features as fallbacks.
constexpr std::array BatteryFeatures{UnifiedBattery::FeatureId, BatteryStatus::FeatureId, BatteryVoltage::FeatureId};

constexpr std::uint8_t SoftwareIdMask = 0x0F;

// HID++ 1.0 error of the receiver for a device it cannot reach.
constexpr int ConnectionFailed = 0x04;
constexpr std::chrono::milliseconds ConnectionRetryDelay{1000};
constexpr std::uint8_t EventFunction = 0;

template<typename T>
void takeValue(std::optional<T> &target, std::optional<Result<T>> &source)
{
    if (source && source->hasValue()) {
        target = std::move(*source).value();
    }
}

} // namespace

ReceiverSession::ReceiverSession(std::unique_ptr<Channel> channel, QObject *parent)
    : QObject(parent)
    , m_channel(std::move(channel))
    , m_dispatcher(m_channel.get())
{
    m_rereadTimer.setInterval(DefaultRereadInterval);
    connect(&m_rereadTimer, &QTimer::timeout, this, &ReceiverSession::rereadAll);
    connect(&m_dispatcher, &Dispatcher::unsolicitedReport, this, &ReceiverSession::handleUnsolicited);
    connect(m_channel.get(), &Channel::disconnected, this, &ReceiverSession::handleDisconnected);
}

ReceiverSession::~ReceiverSession() = default;

void ReceiverSession::setTimeout(std::chrono::milliseconds timeout)
{
    m_dispatcher.setTimeout(timeout);
}

void ReceiverSession::setSoftwareId(std::uint8_t softwareId)
{
    m_dispatcher.setSoftwareId(softwareId);
}

void ReceiverSession::setRereadInterval(std::chrono::milliseconds interval)
{
    m_rereadTimer.setInterval(interval);
}

void ReceiverSession::start()
{
    Receiver(m_dispatcher).readConnectedDeviceCount([this](Result<std::uint8_t> count) {
        if (!count) {
            qCDebug(HIDSIGHT_HIDPP) << "not a receiver:" << count.error();
            m_failed = true;
            Q_EMIT failed(count.error());
            return;
        }
        // All quick reads before any full read, so every device is listed
        // with its battery as early as possible.
        for (std::uint8_t index = Receiver::FirstDeviceIndex; index <= Receiver::LastDeviceIndex; ++index) {
            m_readsPending.insert(index);
            enqueue([this, index](Finished finished) {
                quickRead(index, std::move(finished));
            });
        }
        for (std::uint8_t index = Receiver::FirstDeviceIndex; index <= Receiver::LastDeviceIndex; ++index) {
            enqueue([this, index](Finished finished) {
                fullRead(index, std::move(finished));
            });
        }
        m_rereadTimer.start();
    });
}

void ReceiverSession::rereadAll()
{
    for (const auto &[index, state] : m_devices) {
        if (state.status == DeviceStatus::Online) {
            enqueue([this, index](Finished finished) {
                rereadBattery(index, std::move(finished));
            });
        } else if (state.status == DeviceStatus::Unreachable) {
            scheduleRead(index);
        }
    }
}

std::vector<DeviceState> ReceiverSession::devices() const
{
    std::vector<DeviceState> devices;
    devices.reserve(m_devices.size());
    for (const auto &[index, state] : m_devices) {
        devices.push_back(state);
    }
    return devices;
}

void ReceiverSession::enqueue(Job job)
{
    if (m_failed) {
        return;
    }
    m_jobs.push_back(std::move(job));
    if (!m_jobRunning) {
        runNextJob();
    }
}

void ReceiverSession::runNextJob()
{
    if (m_failed || m_jobs.empty()) {
        m_jobRunning = false;
        return;
    }
    m_jobRunning = true;
    Job job = std::move(m_jobs.front());
    m_jobs.pop_front();
    job([this] {
        // From the event loop, so a job that finishes at once does not
        // recurse.
        QMetaObject::invokeMethod(this, &ReceiverSession::runNextJob, Qt::QueuedConnection);
    });
}

void ReceiverSession::scheduleRead(std::uint8_t index)
{
    if (m_failed) {
        return;
    }
    if (m_readsPending.contains(index)) {
        // The pending read may have started before the device woke up and
        // end with it unreachable; finishRead() reads it once more then.
        m_announcedDuringRead.insert(index);
        return;
    }
    m_readsPending.insert(index);
    enqueue([this, index](Finished finished) {
        quickRead(index, std::move(finished));
    });
    enqueue([this, index](Finished finished) {
        fullRead(index, std::move(finished));
    });
}

void ReceiverSession::finishRead(std::uint8_t index)
{
    m_readsPending.erase(index);
    if (m_announcedDuringRead.erase(index) == 0) {
        return;
    }
    const auto it = m_devices.find(index);
    if (it == m_devices.end() || it->second.status != DeviceStatus::Online) {
        scheduleRead(index);
    }
}

void ReceiverSession::retryRead(std::uint8_t index)
{
    if (m_failed) {
        return;
    }
    m_readsPending.insert(index);
    enqueue([this, index](Finished finished) {
        quickRead(index, std::move(finished));
    });
    enqueue([this, index](Finished finished) {
        fullRead(index, std::move(finished));
    });
}

void ReceiverSession::quickRead(std::uint8_t index, Finished finished)
{
    Device(m_dispatcher, index).ping([this, index, finished](Result<ProtocolVersion> version) {
        if (!version) {
            handleReadError(index, version.error());
            finished();
            return;
        }
        stateFor(index).protocol = version.value();
        readName(index, finished);
    });
}

void ReceiverSession::readName(std::uint8_t index, Finished finished)
{
    const Device device(m_dispatcher, index);
    device.featureIndex(DeviceName::FeatureId, [this, device, index, finished](Result<std::optional<std::uint8_t>> featureIndex) {
        if (!featureIndex || !featureIndex.value()) {
            if (!featureIndex && handleReadError(index, featureIndex.error())) {
                finished();
                return;
            }
            readBattery(index, 0, finished);
            return;
        }
        DeviceName::read(device, *featureIndex.value(), [this, index, finished](Result<DeviceName> name) {
            if (name) {
                stateFor(index).name = std::move(name).value();
            } else if (handleReadError(index, name.error())) {
                finished();
                return;
            }
            readBattery(index, 0, finished);
        });
    });
}

void ReceiverSession::readBattery(std::uint8_t index, std::size_t candidate, Finished finished)
{
    if (candidate >= BatteryFeatures.size()) {
        DeviceState &state = stateFor(index);
        state.status = DeviceStatus::Online;
        state.error.reset();
        m_connectionRetried.erase(index);
        publish(index);
        finished();
        return;
    }
    Device(m_dispatcher, index).featureIndex(BatteryFeatures[candidate], [this, index, candidate, finished](Result<std::optional<std::uint8_t>> featureIndex) {
        if (!featureIndex) {
            if (handleReadError(index, featureIndex.error())) {
                finished();
                return;
            }
            readBattery(index, candidate + 1, finished);
            return;
        }
        if (!featureIndex.value()) {
            readBattery(index, candidate + 1, finished);
            return;
        }
        const std::uint8_t found = *featureIndex.value();
        switch (candidate) {
        case 0:
            m_unifiedBatteryIndex[index] = found;
            readBatteryFeature<UnifiedBattery>(index, found, finished);
            return;
        case 1:
            readBatteryFeature<BatteryStatus>(index, found, finished);
            return;
        default:
            readBatteryFeature<BatteryVoltage>(index, found, finished);
            return;
        }
    });
}

template<typename Feature>
void ReceiverSession::readBatteryFeature(std::uint8_t index, std::uint8_t featureIndex, Finished finished)
{
    Feature::read(Device(m_dispatcher, index), featureIndex, [this, index, finished](Result<Feature> value) {
        if (value) {
            stateFor(index).battery = toBattery(value.value());
        } else if (handleReadError(index, value.error())) {
            finished();
            return;
        }
        // Past the last candidate: marks the device online and publishes it.
        readBattery(index, BatteryFeatures.size(), finished);
    });
}

void ReceiverSession::fullRead(std::uint8_t index, Finished finished)
{
    const auto it = m_devices.find(index);
    if (it == m_devices.end() || it->second.status != DeviceStatus::Online) {
        finishRead(index);
        finished();
        return;
    }
    probeDevice(Device(m_dispatcher, index), [this, index, finished](Result<DeviceInfo> result) {
        if (!result) {
            handleReadError(index, result.error());
            finishRead(index);
            finished();
            return;
        }
        DeviceInfo &&info = std::move(result).value();
        DeviceState &state = stateFor(index);
        state.protocol = info.protocol;
        state.features = std::move(info.features);
        takeValue(state.name, info.name);
        takeValue(state.battery, info.battery);
        takeValue(state.firmware, info.firmware);
        takeValue(state.dpi, info.dpi);
        takeValue(state.reportRate, info.reportRate);
        state.complete = true;
        if (const auto batteryIndex = state.features.indexOf(UnifiedBattery::FeatureId)) {
            m_unifiedBatteryIndex[index] = *batteryIndex;
        }
        publish(index);
        finishRead(index);
        finished();
    });
}

void ReceiverSession::rereadBattery(std::uint8_t index, Finished finished)
{
    const auto it = m_devices.find(index);
    if (it == m_devices.end() || it->second.status != DeviceStatus::Online) {
        finished();
        return;
    }
    readBattery(index, 0, std::move(finished));
}

bool ReceiverSession::handleReadError(std::uint8_t index, const Error &error)
{
    if (Receiver::isEmptySlot(error)) {
        if (m_devices.erase(index) > 0) {
            m_unifiedBatteryIndex.erase(index);
            Q_EMIT deviceRemoved(index);
        }
        return true;
    }
    if (error.kind == ErrorKind::Protocol10 && error.code == ConnectionFailed && !m_connectionRetried.contains(index)) {
        // Asleep or switched off, or a request of another program to the
        // same device at the same moment, which the receiver also refuses
        // (seen on the desk). A second try a moment later tells them apart.
        m_connectionRetried.insert(index);
        QTimer::singleShot(ConnectionRetryDelay, this, [this, index] {
            retryRead(index);
        });
        return true;
    }
    switch (error.kind) {
    case ErrorKind::Protocol20:
    case ErrorKind::Malformed:
        // The device answered; only this feature failed.
        return false;
    case ErrorKind::Disconnected:
        return true;
    default:
        break;
    }
    DeviceState &state = stateFor(index);
    state.status = DeviceStatus::Unreachable;
    state.error = error;
    publish(index);
    return true;
}

void ReceiverSession::handleUnsolicited(const Report &report)
{
    const std::uint8_t index = report.deviceIndex();
    if (index < Receiver::FirstDeviceIndex || index > Receiver::LastDeviceIndex) {
        return;
    }
    // Events carry software id 0; anything else answers a request of another
    // tool on the same node.
    if ((report.address() & SoftwareIdMask) != 0 || report.subId() == Error10SubId || report.subId() == Error20FeatureIndex) {
        return;
    }
    const auto it = m_devices.find(index);
    if (it == m_devices.end() || it->second.status != DeviceStatus::Online) {
        // A device that was switched on or woke up announces itself.
        scheduleRead(index);
        return;
    }
    const auto batteryIndex = m_unifiedBatteryIndex.find(index);
    if (batteryIndex == m_unifiedBatteryIndex.end() || report.subId() != batteryIndex->second || (report.address() >> 4) != EventFunction) {
        return;
    }
    // Verified on the desk: the battery event has the layout of getStatus.
    DeviceState &state = it->second;
    const bool hasPercentage = state.battery && state.battery->percentage;
    const auto status = UnifiedBattery::parseStatus(report.params(), hasPercentage);
    if (!status) {
        qCDebug(HIDSIGHT_HIDPP) << "ignored a malformed battery event from device" << index;
        return;
    }
    state.battery = toBattery(status.value());
    publish(index);
}

void ReceiverSession::handleDisconnected()
{
    if (m_failed) {
        return;
    }
    m_failed = true;
    m_rereadTimer.stop();
    m_jobs.clear();
    Q_EMIT failed(Error{ErrorKind::Disconnected, 0});
}

DeviceState &ReceiverSession::stateFor(std::uint8_t index)
{
    const auto [it, inserted] = m_devices.try_emplace(index);
    if (inserted) {
        it->second.index = index;
    }
    return it->second;
}

void ReceiverSession::publish(std::uint8_t index)
{
    DeviceState &state = m_devices.at(index);
    if (state.status == DeviceStatus::Online) {
        state.lastSeen = QDateTime::currentDateTimeUtc();
    }
    Q_EMIT deviceChanged(state);
}

} // namespace hidpp
