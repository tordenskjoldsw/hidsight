// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import de.tordenskjold.hidsight

FormCard.FormCardPage {
    id: page

    required property string deviceKey

    readonly property var details: DeviceManager.details(deviceKey)
    readonly property var device: details.device

    title: details.present ? device.name : i18nc("@title", "Device")

    // Next to the overview the page has no back button of its own; a narrow
    // window shows Kirigami's back button instead.
    actions: [
        Kirigami.Action {
            icon.name: "window-close-symbolic"
            text: i18nc("@action:button close the device page", "Close")
            displayHint: Kirigami.DisplayHint.IconOnly
            visible: page.Kirigami.PageStack.pageStack?.wideMode ?? false
            shortcut: "Escape"
            onTriggered: page.Kirigami.PageStack.pageStack.removePage(page)
        }
    ]

    Kirigami.PlaceholderMessage {
        visible: !page.details.present
        icon.name: "preferences-desktop-peripherals"
        text: i18nc("@info:placeholder", "The device is gone")
        explanation: i18nc("@info:placeholder", "Its receiver was unplugged or it was unpaired.")
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.gridUnit * 2
    }

    ColumnLayout {
        visible: page.details.present
        spacing: 0
        Layout.fillWidth: true

        // The charge at a glance, as on the device's card.
        ColumnLayout {
            spacing: Kirigami.Units.largeSpacing
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.gridUnit
            Layout.bottomMargin: Kirigami.Units.gridUnit

            GridLayout {
                columns: page.width > Kirigami.Units.gridUnit * 26 ? 2 : 1
                columnSpacing: Kirigami.Units.gridUnit * 2
                rowSpacing: Kirigami.Units.largeSpacing
                opacity: page.device.online ? 1 : 0.5
                Layout.alignment: Qt.AlignHCenter

                Behavior on opacity {
                    NumberAnimation {
                        duration: Kirigami.Units.longDuration
                    }
                }

                DeviceIllustration {
                    illustration: page.device.illustration ?? ""
                    iconName: page.device.iconName ?? ""
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 14
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 8
                    Layout.alignment: Qt.AlignCenter
                }

                BatteryRing {
                    fraction: page.device.batteryFraction ?? -1
                    tone: page.device.batteryTone ?? "unknown"
                    label: (page.device.batteryPercentage ?? -1) >= 0 ? i18nc("@info battery charge", "%1 %", page.device.batteryPercentage) : "–"
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 7
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 7
                    Layout.alignment: Qt.AlignCenter
                }
            }

            ColumnLayout {
                spacing: 0
                Layout.fillWidth: true

                Kirigami.Heading {
                    text: page.device.name ?? ""
                    level: 1
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    text: page.device.kindText ?? ""
                    color: Kirigami.Theme.disabledTextColor
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
            }

            StatusChips {
                chips: page.device.statusChips ?? []
                Layout.alignment: Qt.AlignHCenter
            }
        }

        FormCard.FormHeader {
            title: i18nc("@title:group", "Battery")
        }
        FormCard.FormCard {
            FormCard.FormTextDelegate {
                id: chargeDelegate
                // The ring shows a percentage; this row only for a device
                // that reports a voltage or a level alone.
                visible: (page.device.batteryPercentage ?? -1) < 0 && (page.device.batteryText ?? "") !== ""
                text: i18nc("@label", "Charge")
                description: page.device.batteryText ?? ""
            }
            FormCard.FormDelegateSeparator {
                visible: chargeDelegate.visible && levelDelegate.visible
            }
            FormCard.FormTextDelegate {
                id: levelDelegate
                visible: (page.device.batteryLevelText ?? "") !== ""
                text: i18nc("@label battery level reported by the device", "Level")
                description: page.device.batteryLevelText ?? ""
            }
            FormCard.FormDelegateSeparator {
                visible: (chargeDelegate.visible || levelDelegate.visible) && lastSeenDelegate.visible
            }
            FormCard.FormTextDelegate {
                id: lastSeenDelegate
                visible: (page.device.lastSeenText ?? "") !== ""
                text: i18nc("@label time of the last answer", "Last answer")
                description: page.device.lastSeenText ?? ""
            }
        }

        FormCard.FormHeader {
            title: i18nc("@title:group", "Connection")
        }
        FormCard.FormCard {
            FormCard.FormTextDelegate {
                text: i18nc("@label", "Receiver")
                description: page.device.receiverText ?? ""
            }
            FormCard.FormDelegateSeparator {
                visible: protocolDelegate.visible
            }
            FormCard.FormTextDelegate {
                id: protocolDelegate
                visible: Settings.showTechnicalDetails && (page.device.protocol ?? "") !== ""
                text: i18nc("@label", "Protocol")
                description: i18nc("@info protocol version", "HID++ %1", page.device.protocol ?? "")
            }
        }

        FormCard.FormHeader {
            title: i18nc("@title:group", "Device")
            visible: deviceCard.visible
        }
        FormCard.FormCard {
            id: deviceCard
            visible: (page.device.firmware ?? []).length > 0 || (page.device.dpi ?? 0) > 0 || (page.device.reportRate ?? 0) > 0

            Repeater {
                model: page.device.firmware ?? []

                delegate: FormCard.FormTextDelegate {
                    required property var modelData
                    text: modelData.label
                    description: modelData.version
                }
            }
            FormCard.FormTextDelegate {
                visible: (page.device.dpi ?? 0) > 0
                text: i18nc("@label pointer resolution", "Resolution")
                description: i18nc("@info dots per inch", "%1 dpi", page.device.dpi ?? 0)
            }
            FormCard.FormTextDelegate {
                visible: (page.device.reportRate ?? 0) > 0
                text: i18nc("@label", "Report rate")
                description: i18nc("@info report interval", "Every %1 ms", page.device.reportRate ?? 0)
            }
        }

        FormCard.FormHeader {
            title: i18nc("@title:group", "Features")
            visible: Settings.showTechnicalDetails
        }
        FormCard.FormCard {
            visible: Settings.showTechnicalDetails

            FormCard.FormTextDelegate {
                visible: (page.device.features ?? []).length === 0
                text: page.device.online ? i18nc("@info", "Reading…") : i18nc("@info", "Read when the device answers")
            }
            Repeater {
                model: page.device.features ?? []

                delegate: FormCard.FormTextDelegate {
                    required property var modelData
                    text: modelData.title
                    description: modelData.detail
                }
            }
        }
    }
}
