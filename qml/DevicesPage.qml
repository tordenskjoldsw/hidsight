// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import de.tordenskjold.hidsight

Kirigami.ScrollablePage {
    id: page

    title: i18nc("@title", "Devices")

    function openDevice(key: string) {
        const stack = page.Kirigami.PageStack.pageStack;
        stack.pop(page);
        stack.push(Qt.createComponent("de.tordenskjold.hidsight", "DevicePage"), {
            deviceKey: key
        });
    }

    header: Kirigami.InlineMessage {
        visible: DeviceManager.accessDenied && cards.count > 0
        position: Kirigami.InlineMessage.Position.Header
        type: Kirigami.MessageType.Warning
        text: i18nc("@info", "A receiver cannot be opened because the udev rule 70-hidsight.rules is missing.")
    }

    // One row per device across the width; two columns once a row would get
    // too wide to read.
    GridLayout {
        columnSpacing: Kirigami.Units.largeSpacing
        rowSpacing: Kirigami.Units.largeSpacing
        columns: width > Kirigami.Units.gridUnit * 70 && cards.count > 1 ? 2 : 1

        Repeater {
            id: cards

            model: DeviceManager.devices

            delegate: DeviceCard {
                required property string key

                Layout.fillWidth: true
                Layout.fillHeight: true
                onClicked: page.openDevice(key)
            }
        }
    }

    Kirigami.PlaceholderMessage {
        parent: page
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: cards.count === 0

        icon.name: !DeviceManager.watching ? "dialog-error" : DeviceManager.accessDenied ? "emblem-locked" : "preferences-desktop-peripherals"
        text: !DeviceManager.watching ? i18nc("@info:placeholder", "Cannot watch for devices") : DeviceManager.accessDenied ? i18nc("@info:placeholder", "No access to the receiver") : i18nc("@info:placeholder", "No receiver found")
        explanation: !DeviceManager.watching ? i18nc("@info:placeholder", "udev is not available, so Hidsight cannot find receivers.") : DeviceManager.accessDenied ? i18nc("@info:placeholder", "Hidsight reads Logitech receivers through the udev rule 70-hidsight.rules. The package installs it; for a build from source, copy data/70-hidsight.rules to /etc/udev/rules.d/ and plug the receiver in again.") : i18nc("@info:placeholder", "Plug in a Logitech receiver, for example a Logi Bolt receiver.")
    }
}
