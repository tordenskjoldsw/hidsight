// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "devicenames.h"

#include <KConfigGroup>

using namespace Qt::StringLiterals;

namespace
{

const auto NameKey = "Name";
const auto KindKey = "Kind";

} // namespace

DeviceNames::DeviceNames(KSharedConfig::Ptr config)
    : m_config(std::move(config))
{
}

std::optional<hidpp::DeviceName> DeviceNames::recall(const QString &receiverId, std::uint8_t index) const
{
    const KConfigGroup slot = group(receiverId, index);
    const QString name = slot.readEntry(NameKey, QString());
    // The file is the user's; bound it like a name read from a device.
    if (name.isEmpty() || name.size() > hidpp::DeviceName::MaxLength) {
        return std::nullopt;
    }
    const int kind = slot.readEntry(KindKey, int(hidpp::DeviceKind::Unknown));
    const bool knownKind = kind >= 0 && kind <= int(hidpp::DeviceKind::Adapter);
    return hidpp::DeviceName{name, knownKind ? hidpp::DeviceKind(kind) : hidpp::DeviceKind::Unknown};
}

void DeviceNames::remember(const QString &receiverId, std::uint8_t index, const hidpp::DeviceName &name)
{
    const auto known = recall(receiverId, index);
    if (known && known->name == name.name && known->kind == name.kind) {
        return;
    }
    KConfigGroup slot = group(receiverId, index);
    slot.writeEntry(NameKey, name.name);
    slot.writeEntry(KindKey, int(name.kind));
    m_config->sync();
}

void DeviceNames::forget(const QString &receiverId, std::uint8_t index)
{
    KConfigGroup slot = group(receiverId, index);
    if (slot.exists()) {
        slot.deleteGroup();
        m_config->sync();
    }
}

KConfigGroup DeviceNames::group(const QString &receiverId, std::uint8_t index) const
{
    return m_config->group(u"Devices"_s).group(u"%1 slot %2"_s.arg(receiverId).arg(index));
}
