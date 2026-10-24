// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "appsettings.h"
#include "autostart.h"
#include "devicedetails.h"
#include "devicemanager.h"
#include "devicemodel.h"

#include <QQmlEngine>

// The app classes live in a library that the tests link without QML; these
// declarations make them known to the QML module.

struct DeviceManagerForeign {
    Q_GADGET
    QML_FOREIGN(DeviceManager)
    QML_NAMED_ELEMENT(DeviceManager)
    QML_SINGLETON

public:
    // The one main() owns; set before the engine loads.
    static inline DeviceManager *instance = nullptr;

    static DeviceManager *create(QQmlEngine *engine, QJSEngine *scriptEngine);
};

struct AppSettingsForeign {
    Q_GADGET
    QML_FOREIGN(AppSettings)
    QML_NAMED_ELEMENT(Settings)
    QML_SINGLETON

public:
    static inline AppSettings *instance = nullptr;

    static AppSettings *create(QQmlEngine *engine, QJSEngine *scriptEngine);
};

struct AutostartForeign {
    Q_GADGET
    QML_FOREIGN(Autostart)
    QML_NAMED_ELEMENT(Autostart)
    QML_SINGLETON

public:
    static inline Autostart *instance = nullptr;

    static Autostart *create(QQmlEngine *engine, QJSEngine *scriptEngine);
};

struct DeviceModelForeign {
    Q_GADGET
    QML_FOREIGN(DeviceModel)
    QML_ANONYMOUS
};

struct DeviceDetailsForeign {
    Q_GADGET
    QML_FOREIGN(DeviceDetails)
    QML_ANONYMOUS
};
