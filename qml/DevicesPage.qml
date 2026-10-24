// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.delegates as Delegates
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
        visible: DeviceManager.accessDenied && list.count > 0
        position: Kirigami.InlineMessage.Position.Header
        type: Kirigami.MessageType.Warning
        text: i18nc("@info", "A receiver cannot be opened because the udev rule 70-hidsight.rules is missing.")
    }

    ListView {
        id: list

        model: DeviceManager.devices
        currentIndex: -1
        keyNavigationEnabled: true
        activeFocusOnTab: true
        Keys.onReturnPressed: currentItem?.clicked()
        Keys.onEnterPressed: currentItem?.clicked()

        delegate: Delegates.RoundedItemDelegate {
            id: delegate

            required property int index
            required property string name
            required property string iconName
            required property string kindText
            required property string statusText
            required property string batteryIconName
            required property bool online
            required property string key

            text: name
            icon.name: iconName
            highlighted: ListView.isCurrentItem
            Accessible.description: i18nc("@info device kind, status", "%1, %2", kindText, statusText)
            Accessible.onPressAction: clicked()

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing
                // Not connected: shown, but quieter.
                opacity: delegate.online ? 1 : 0.6

                Delegates.SubtitleContentItem {
                    itemDelegate: delegate
                    subtitle: i18nc("@info device kind, status", "%1 · %2", delegate.kindText, delegate.statusText)
                    Layout.fillWidth: true
                }

                Kirigami.Icon {
                    source: delegate.batteryIconName
                    visible: delegate.online
                    Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                }
            }

            onClicked: {
                list.currentIndex = index;
                page.openDevice(key);
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: list.count === 0

            icon.name: !DeviceManager.watching ? "dialog-error" : DeviceManager.accessDenied ? "emblem-locked" : "preferences-desktop-peripherals"
            text: !DeviceManager.watching ? i18nc("@info:placeholder", "Cannot watch for devices") : DeviceManager.accessDenied ? i18nc("@info:placeholder", "No access to the receiver") : i18nc("@info:placeholder", "No receiver found")
            explanation: !DeviceManager.watching ? i18nc("@info:placeholder", "udev is not available, so Hidsight cannot find receivers.") : DeviceManager.accessDenied ? i18nc("@info:placeholder", "Hidsight reads Logitech receivers through the udev rule 70-hidsight.rules. The package installs it; for a build from source, copy data/70-hidsight.rules to /etc/udev/rules.d/ and plug the receiver in again.") : i18nc("@info:placeholder", "Plug in a Logitech receiver, for example a Logi Bolt receiver.")
        }
    }
}
