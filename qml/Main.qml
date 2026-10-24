// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import org.kde.kirigami as Kirigami
import de.tordenskjold.hidsight

Kirigami.ApplicationWindow {
    id: root

    title: i18nc("@title:window", "Hidsight")

    width: Kirigami.Units.gridUnit * 44
    height: Kirigami.Units.gridUnit * 32
    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 15

    pageStack.initialPage: DevicesPage {}
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.ToolBar

    // The app-wide actions, the same on every page, also when a narrow window
    // shows only a device.
    globalDrawer: Kirigami.GlobalDrawer {
        isMenu: true
        actions: [
            Kirigami.Action {
                icon.name: "view-refresh"
                text: i18nc("@action:inmenu", "Refresh")
                shortcut: StandardKey.Refresh
                onTriggered: DeviceManager.refresh()
            },
            Kirigami.Action {
                separator: true
            },
            Kirigami.Action {
                icon.name: "help-about"
                text: i18nc("@action:inmenu", "About Hidsight")
                onTriggered: root.pageStack.layers.push(Qt.createComponent("org.kde.kirigamiaddons.formcard", "AboutPage"))
            },
            Kirigami.Action {
                icon.name: "application-exit"
                text: i18nc("@action:inmenu", "Quit")
                shortcut: StandardKey.Quit
                onTriggered: Qt.quit()
            }
        ]
    }
}
