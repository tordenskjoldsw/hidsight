// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "hidpp/dispatcher.h"
#include "hidpp/probe.h"
#include "hidpp/receiversession.h"

#include <QJsonObject>
#include <QString>

#include <functional>
#include <optional>
#include <utility>
#include <vector>

struct Listing {
    QString source;
    std::optional<hidpp::Result<std::uint8_t>> connectedDevices;
    // Indices with a device or with an error; empty slots are left out.
    std::vector<std::pair<std::uint8_t, hidpp::Result<hidpp::DeviceInfo>>> devices;
};

// Reads the receiver and then each device index, one after another.
void listReceiver(hidpp::Dispatcher &dispatcher, const QString &source, std::function<void(Listing)> done);

QString formatText(const Listing &listing);
QJsonObject formatJson(const Listing &listing);
QString errorText(const hidpp::Error &error);
// One line for a device of a watched receiver.
QString formatState(const hidpp::DeviceState &state);
