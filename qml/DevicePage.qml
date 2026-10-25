// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.delegates as Delegates
import de.tordenskjold.hidsight

// One device, in place of the overview: its battery and connection in the
// header, its sections in a side navigation, or as tabs in a narrow window.
Kirigami.Page {
    id: page

    required property string deviceKey

    readonly property var details: DeviceManager.details(deviceKey)
    readonly property var device: details.device
    readonly property bool wide: width > Kirigami.Units.gridUnit * 36
    property int section: 0

    // The sections of a device page; the settings join them later.
    readonly property list<QtObject> sections: [
        QtObject {
            readonly property string text: i18nc("@title:tab device page section", "Overview")
            readonly property string iconName: "dashboard-show-symbolic"
        },
        QtObject {
            readonly property string text: i18nc("@title:tab device page section", "Details")
            readonly property string iconName: "documentinfo-symbolic"
        }
    ]

    title: details.present ? device.name : i18nc("@title", "Device")
    padding: 0

    function goBack() {
        page.Kirigami.PageStack.pageStack.removePage(page);
    }

    titleDelegate: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        QQC2.ToolButton {
            icon.name: "go-previous-symbolic"
            text: i18nc("@action:button back to the device overview", "Devices")
            display: QQC2.AbstractButton.TextBesideIcon
            onClicked: page.goBack()
        }

        Kirigami.Heading {
            text: page.title
            level: 1
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        Kirigami.Icon {
            visible: page.details.present && (page.device.online ?? false)
            source: (page.device.batteryIconName ?? "battery-missing") + "-symbolic"
            color: Tones.color(page.device.batteryTone ?? "unknown")
            isMask: true
            Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
            Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
        }
        QQC2.Label {
            visible: page.details.present && (page.device.online ?? false) && (page.device.batteryPercentage ?? -1) >= 0
            text: i18nc("@info battery charge", "%1 %", page.device.batteryPercentage ?? 0)
            color: Tones.color(page.device.batteryTone ?? "unknown")
            font.weight: Font.DemiBold
        }
        Pill {
            visible: page.details.present
            text: (page.device.online ?? false) ? i18nc("@info:status", "Connected") : (page.device.status ?? "") === "reading" ? i18nc("@info:status", "Reading") : i18nc("@info:status", "Not connected")
            dotColor: (page.device.online ?? false) ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.disabledTextColor
            Layout.rightMargin: Kirigami.Units.smallSpacing
        }
    }

    Shortcut {
        sequences: ["Escape", StandardKey.Back]
        onActivated: page.goBack()
    }

    // The back button of a mouse.
    TapHandler {
        acceptedButtons: Qt.BackButton
        onTapped: page.goBack()
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: !page.details.present
        icon.name: "preferences-desktop-peripherals"
        text: i18nc("@info:placeholder", "The device is gone")
        explanation: i18nc("@info:placeholder", "Its receiver was unplugged or it was unpaired.")
    }

    RowLayout {
        anchors.fill: parent
        visible: page.details.present
        spacing: 0

        // The side navigation of a wide window.
        Rectangle {
            visible: page.wide
            color: Kirigami.Theme.backgroundColor
            Kirigami.Theme.colorSet: Kirigami.Theme.Window
            Kirigami.Theme.inherit: false
            Layout.preferredWidth: Kirigami.Units.gridUnit * 11
            Layout.fillHeight: true

            ListView {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing
                model: page.sections
                currentIndex: page.section
                keyNavigationEnabled: true
                activeFocusOnTab: true
                Keys.onReturnPressed: page.section = currentIndex

                delegate: Delegates.RoundedItemDelegate {
                    required property int index
                    required property var modelData

                    text: modelData.text
                    icon.name: modelData.iconName
                    highlighted: index === page.section
                    onClicked: page.section = index
                }
            }

            Kirigami.Separator {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
            }
        }

        ColumnLayout {
            spacing: 0
            Layout.fillWidth: true
            Layout.fillHeight: true

            // The same sections as tabs in a narrow window.
            QQC2.TabBar {
                visible: !page.wide
                currentIndex: page.section
                Layout.fillWidth: true

                Repeater {
                    model: page.sections

                    delegate: QQC2.TabButton {
                        required property int index
                        required property var modelData

                        text: modelData.text
                        icon.name: modelData.iconName
                        onClicked: page.section = index
                    }
                }
            }

            StackLayout {
                currentIndex: page.section
                Layout.fillWidth: true
                Layout.fillHeight: true

                DeviceOverviewSection {
                    device: page.device
                }
                DeviceDetailsSection {
                    device: page.device
                }
            }
        }
    }
}
