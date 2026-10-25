// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// The first section of a device page: the device large, its charge as a
// ring and the facts that matter day to day next to it.
QQC2.ScrollView {
    id: section

    // The device's roles by name, from DeviceDetails.
    property var device: ({})

    contentWidth: availableWidth

    ColumnLayout {
        width: section.availableWidth
        spacing: Kirigami.Units.gridUnit

        DeviceIllustration {
            illustration: section.device.illustration ?? ""
            iconName: section.device.iconName ?? ""
            opacity: section.device.online ? 1 : 0.45
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            Layout.preferredHeight: Kirigami.Units.gridUnit * 11
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Kirigami.Units.gridUnit * 2

            Behavior on opacity {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
                }
            }
        }

        RowLayout {
            spacing: Kirigami.Units.gridUnit * 1.5
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: Kirigami.Units.gridUnit

            BatteryRing {
                fraction: section.device.batteryFraction ?? -1
                tone: section.device.batteryTone ?? "unknown"
                label: (section.device.batteryPercentage ?? -1) >= 0 ? i18nc("@info battery charge", "%1 %", section.device.batteryPercentage) : "–"
                opacity: section.device.online ? 1 : 0.5
                Layout.preferredWidth: Kirigami.Units.gridUnit * 7
                Layout.preferredHeight: Kirigami.Units.gridUnit * 7
            }

            ColumnLayout {
                spacing: Kirigami.Units.smallSpacing
                Layout.alignment: Qt.AlignVCenter

                Kirigami.Heading {
                    text: section.device.online ? (section.device.batteryLevelText !== "" ? i18nc("@info battery level", "Battery %1", section.device.batteryLevelText) : (section.device.batteryText ?? "")) : (section.device.statusText ?? "")
                    level: 2
                }
                QQC2.Label {
                    visible: text !== ""
                    text: section.device.online ? (section.device.chargingText ?? "") : ""
                    color: Kirigami.Theme.disabledTextColor
                }
                QQC2.Label {
                    visible: (section.device.lastSeenText ?? "") !== ""
                    text: i18nc("@info time of the last answer", "Last answer: %1", section.device.lastSeenText ?? "")
                    color: Kirigami.Theme.disabledTextColor
                }
                QQC2.Label {
                    visible: text !== ""
                    text: (section.device.dpi ?? 0) > 0 ? i18nc("@info dots per inch", "%1 dpi", section.device.dpi) : ((section.device.firmwareVersion ?? "") !== "" ? i18nc("@info firmware version", "Firmware %1", section.device.firmwareVersion) : "")
                    color: Kirigami.Theme.disabledTextColor
                }
                StatusChips {
                    chips: section.device.statusChips ?? []
                    visible: chips.length > 0
                    Layout.topMargin: Kirigami.Units.smallSpacing
                }
            }
        }
    }
}
