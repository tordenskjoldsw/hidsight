// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "probe.h"

#include <memory>

namespace hidpp
{

namespace
{

class Probe : public std::enable_shared_from_this<Probe>
{
public:
    Probe(const Device &device, Callback<DeviceInfo> done)
        : m_device(device)
        , m_done(std::move(done))
    {
        m_info.index = device.index();
    }

    void start()
    {
        m_device.ping([self = shared_from_this()](Result<ProtocolVersion> version) {
            if (!version) {
                self->m_done(version.error());
                return;
            }
            self->m_info.protocol = version.value();
            self->readFeatureTable();
        });
    }

private:
    using Step = void (Probe::*)();

    void readFeatureTable()
    {
        m_device.readFeatureTable([self = shared_from_this()](Result<FeatureTable> table) {
            if (!table) {
                self->m_done(table.error());
                return;
            }
            self->m_info.features = std::move(table).value();
            self->readName();
        });
    }

    // Reads Feature into field when the device has it, then runs next.
    template<typename Feature>
    void readOptional(std::optional<Result<Feature>> DeviceInfo::*field, Step next)
    {
        const auto index = m_info.features.indexOf(Feature::FeatureId);
        if (!index) {
            (this->*next)();
            return;
        }
        Feature::read(m_device, *index, [self = shared_from_this(), field, next](Result<Feature> value) {
            self->m_info.*field = std::move(value);
            (self.get()->*next)();
        });
    }

    // Reads Feature as the battery when the device has it; otherwise tries
    // the fallback.
    template<typename Feature>
    void readBattery(Step fallback)
    {
        const auto index = m_info.features.indexOf(Feature::FeatureId);
        if (!index) {
            (this->*fallback)();
            return;
        }
        Feature::read(m_device, *index, [self = shared_from_this()](Result<Feature> value) {
            if (value) {
                self->m_info.battery = toBattery(value.value());
            } else {
                self->m_info.battery = value.error();
            }
            self->readDpi();
        });
    }

    void readName()
    {
        readOptional<DeviceName>(&DeviceInfo::name, &Probe::readFirmware);
    }

    void readFirmware()
    {
        readOptional<DeviceFwVersion>(&DeviceInfo::firmware, &Probe::readUnifiedBattery);
    }

    void readUnifiedBattery()
    {
        readBattery<UnifiedBattery>(&Probe::readBatteryStatus);
    }

    void readBatteryStatus()
    {
        readBattery<BatteryStatus>(&Probe::readBatteryVoltage);
    }

    void readBatteryVoltage()
    {
        readBattery<BatteryVoltage>(&Probe::readDpi);
    }

    void readDpi()
    {
        readOptional<AdjustableDpi>(&DeviceInfo::dpi, &Probe::readReportRate);
    }

    void readReportRate()
    {
        readOptional<ReportRate>(&DeviceInfo::reportRate, &Probe::finish);
    }

    void finish()
    {
        m_done(std::move(m_info));
    }

    Device m_device;
    Callback<DeviceInfo> m_done;
    DeviceInfo m_info;
};

} // namespace

void probeDevice(const Device &device, Callback<DeviceInfo> done)
{
    std::make_shared<Probe>(device, std::move(done))->start();
}

} // namespace hidpp
