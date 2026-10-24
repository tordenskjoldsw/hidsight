// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "qmltypes.h"

namespace
{

// main() owns the singletons; the engine must not delete them.
template<typename T>
T *keepOwnership(T *instance, QJSEngine *scriptEngine)
{
    Q_ASSERT(instance);
    scriptEngine->setObjectOwnership(instance, QJSEngine::CppOwnership);
    return instance;
}

} // namespace

DeviceManager *DeviceManagerForeign::create(QQmlEngine *, QJSEngine *scriptEngine)
{
    return keepOwnership(instance, scriptEngine);
}

AppSettings *AppSettingsForeign::create(QQmlEngine *, QJSEngine *scriptEngine)
{
    return keepOwnership(instance, scriptEngine);
}

Autostart *AutostartForeign::create(QQmlEngine *, QJSEngine *scriptEngine)
{
    return keepOwnership(instance, scriptEngine);
}
