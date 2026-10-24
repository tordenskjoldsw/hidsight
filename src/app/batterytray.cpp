// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "batterytray.h"
#include "devicemodel.h"
#include "devicetexts.h"
#include "traylook.h"

#include <KLocalizedString>
#include <KStatusNotifierItem>

#include <QAction>
#include <QMenu>

using namespace Qt::StringLiterals;

namespace
{

KStatusNotifierItem::ItemStatus itemStatus(TrayLook::Status status)
{
    switch (status) {
    case TrayLook::Status::Passive:
        return KStatusNotifierItem::Passive;
    case TrayLook::Status::Active:
        return KStatusNotifierItem::Active;
    case TrayLook::Status::NeedsAttention:
        return KStatusNotifierItem::NeedsAttention;
    }
    return KStatusNotifierItem::Active;
}

} // namespace

BatteryTray::BatteryTray(DeviceModel *model, QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_item(std::make_unique<KStatusNotifierItem>(u"hidsight"_s))
{
    m_item->setTitle(i18nc("@title", "Hidsight"));
    m_item->setCategory(KStatusNotifierItem::Hardware);
    connect(model, &QAbstractItemModel::dataChanged, this, &BatteryTray::update);
    connect(model, &QAbstractItemModel::rowsInserted, this, &BatteryTray::update);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &BatteryTray::update);
    connect(model, &QAbstractItemModel::modelReset, this, &BatteryTray::update);
    update();
}

BatteryTray::~BatteryTray() = default;

void BatteryTray::setWindow(QWindow *window)
{
    m_item->setAssociatedWindow(window);
}

void BatteryTray::update()
{
    const auto states = m_model->states();
    const TrayLook look = TrayLook::forDevices(states);
    m_item->setIconByName(look.iconName);
    m_item->setStatus(itemStatus(look.status));
    m_item->setToolTip(look.iconName, i18nc("@title", "Hidsight"), look.toolTip);

    // The devices at the top of the menu, read-only; the item adds Quit.
    QMenu *menu = m_item->contextMenu();
    const auto old = menu->actions();
    for (QAction *action : old) {
        if (action->property("hidsightDevice").toBool()) {
            menu->removeAction(action);
            action->deleteLater();
        }
    }
    QAction *first = menu->actions().value(0);
    for (const hidpp::DeviceState &state : states) {
        const QString name = state.name ? state.name->name : i18nc("@info device without a name yet", "Device %1", state.index);
        auto *action = new QAction(QIcon::fromTheme(DeviceTexts::kindIconName(state.name ? state.name->kind : hidpp::DeviceKind::Unknown)),
                                   i18nc("@action:inmenu device name, status", "%1 – %2", name, DeviceTexts::statusText(state)),
                                   menu);
        action->setEnabled(false);
        action->setProperty("hidsightDevice", true);
        menu->insertAction(first, action);
    }
    if (!states.empty() && first && !first->isSeparator()) {
        QAction *separator = menu->insertSeparator(first);
        separator->setProperty("hidsightDevice", true);
    }
}
