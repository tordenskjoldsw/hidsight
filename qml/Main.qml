// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    title: i18nc("@title:window", "Hidsight")

    width: Kirigami.Units.gridUnit * 44
    height: Kirigami.Units.gridUnit * 32
    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 15

    pageStack.initialPage: DevicesPage {}
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.ToolBar

    Shortcut {
        sequences: [StandardKey.Quit]
        onActivated: Qt.quit()
    }
}
