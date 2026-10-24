// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "channel.h"
#include "dispatcher.h"
#include "probe.h"

#include <QObject>
#include <QTimer>

#include <chrono>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace hidpp
{

enum class DeviceStatus : std::uint8_t {
    // Paired; the first read is running.
    Reading,
    Online,
    // Paired, but asleep or switched off: the Bolt receiver answers HID++ 1.0
    // error 0x04 for both (verified on the desk). Also a device that stopped
    // answering.
    Unreachable,
};

// What Hidsight knows about the device at one index of a receiver. Values
// stay at their last reading while the device is unreachable.
struct DeviceState {
    std::uint8_t index = 0;
    DeviceStatus status = DeviceStatus::Reading;
    std::optional<Error> error;
    // The feature table and everything after it have been read.
    bool complete = false;
    ProtocolVersion protocol;
    FeatureTable features;
    std::optional<DeviceName> name;
    std::optional<Battery> battery;
    std::optional<DeviceFwVersion> firmware;
    std::optional<AdjustableDpi> dpi;
    std::optional<ReportRate> reportRate;
};

// Keeps the devices of one receiver up to date for as long as its node
// exists. Each device is first read quickly (ping, name, battery), so the list
// fills within criterion 5, and then completely. Events keep it current:
// verified on the desk, a device that is switched on or wakes up sends a
// WirelessDeviceStatus event and then its battery, and a change of charging
// sends a battery event, all without Hidsight enabling anything on the
// receiver. Devices that go quiet send nothing; the periodic re-read finds
// them.
class ReceiverSession : public QObject
{
    Q_OBJECT

public:
    static constexpr std::chrono::milliseconds DefaultRereadInterval = std::chrono::minutes(15);

    explicit ReceiverSession(std::unique_ptr<Channel> channel, QObject *parent = nullptr);
    ~ReceiverSession() override;

    void setTimeout(std::chrono::milliseconds timeout);
    void setSoftwareId(std::uint8_t softwareId);
    void setRereadInterval(std::chrono::milliseconds interval);

    void start();
    // Reads the battery of each online device and every unreachable device
    // again; the timer does this at each interval.
    void rereadAll();

    std::vector<DeviceState> devices() const;

Q_SIGNALS:
    void deviceChanged(const hidpp::DeviceState &state);
    void deviceRemoved(std::uint8_t index);
    // The node is not a receiver, or it went away.
    void failed(const hidpp::Error &error);

private:
    using Finished = std::function<void()>;
    using Job = std::function<void(Finished)>;

    void enqueue(Job job);
    void runNextJob();
    void scheduleRead(std::uint8_t index);
    void finishRead(std::uint8_t index);
    void retryRead(std::uint8_t index);

    void quickRead(std::uint8_t index, Finished finished);
    void readName(std::uint8_t index, Finished finished);
    void readBattery(std::uint8_t index, std::size_t candidate, Finished finished);
    void fullRead(std::uint8_t index, Finished finished);
    void rereadBattery(std::uint8_t index, Finished finished);

    template<typename Feature>
    void readBatteryFeature(std::uint8_t index, std::uint8_t featureIndex, Finished finished);

    // Ends a read that failed: an empty slot removes the device, anything else
    // that says the device is gone marks it unreachable. True when the read
    // cannot go on.
    bool handleReadError(std::uint8_t index, const Error &error);
    void handleUnsolicited(const Report &report);
    void handleDisconnected();

    DeviceState &stateFor(std::uint8_t index);
    void publish(std::uint8_t index);

    std::unique_ptr<Channel> m_channel;
    Dispatcher m_dispatcher;
    QTimer m_rereadTimer;

    std::map<std::uint8_t, DeviceState> m_devices;
    // Where battery events of each device come from.
    std::map<std::uint8_t, std::uint8_t> m_unifiedBatteryIndex;
    std::set<std::uint8_t> m_readsPending;
    // Devices that announced themselves while a read of them was pending.
    std::set<std::uint8_t> m_announcedDuringRead;
    // Devices whose last read failed with "connection failed" once already.
    std::set<std::uint8_t> m_connectionRetried;
    std::deque<Job> m_jobs;
    bool m_jobRunning = false;
    bool m_failed = false;
};

} // namespace hidpp
