// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>

#include <memory>

class DeviceModel;
class KStatusNotifierItem;
class QWindow;

// The tray item (PLAN.md section 8): the battery of the weakest device as its
// icon, every device in the tooltip and the menu. A click shows or hides the
// window.
class BatteryTray : public QObject
{
    Q_OBJECT

public:
    explicit BatteryTray(DeviceModel *model, QObject *parent = nullptr);
    ~BatteryTray() override;

    void setWindow(QWindow *window);

private:
    void update();

    DeviceModel *m_model;
    std::unique_ptr<KStatusNotifierItem> m_item;
};
