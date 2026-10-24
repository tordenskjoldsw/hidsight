// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "qmltypes.h"

DeviceManager *DeviceManagerForeign::create(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_ASSERT(instance);
    // main() owns it; the engine must not delete it.
    scriptEngine->setObjectOwnership(instance, QJSEngine::CppOwnership);
    return instance;
}
