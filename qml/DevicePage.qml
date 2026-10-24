// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
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

        RowLayout {
            spacing: Kirigami.Units.largeSpacing
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Kirigami.Units.gridUnit
            Layout.bottomMargin: Kirigami.Units.largeSpacing

            Kirigami.Icon {
                source: page.device.iconName ?? ""
                Layout.preferredWidth: Kirigami.Units.iconSizes.huge
                Layout.preferredHeight: Kirigami.Units.iconSizes.huge
            }

            ColumnLayout {
                spacing: 0

                Kirigami.Heading {
                    text: page.device.name ?? ""
                    level: 1
                }
                Kirigami.Heading {
                    text: page.device.kindText ?? ""
                    level: 4
                    type: Kirigami.Heading.Type.Secondary
                }
            }
        }

        FormCard.FormHeader {
            title: i18nc("@title:group", "Battery")
        }
        FormCard.FormCard {
            FormCard.FormTextDelegate {
                icon.name: page.device.batteryIconName ?? ""
                text: i18nc("@label", "Charge")
                description: page.device.batteryText || i18nc("@info", "Unknown")
            }
            FormCard.FormDelegateSeparator {
                visible: levelDelegate.visible
            }
            FormCard.FormTextDelegate {
                id: levelDelegate
                visible: (page.device.batteryLevelText ?? "") !== ""
                text: i18nc("@label battery level reported by the device", "Level")
                description: page.device.batteryLevelText ?? ""
            }
            FormCard.FormDelegateSeparator {
                visible: chargingDelegate.visible
            }
            FormCard.FormTextDelegate {
                id: chargingDelegate
                visible: (page.device.chargingText ?? "") !== ""
                text: i18nc("@label", "Charging")
                description: page.device.chargingText ?? ""
            }
            FormCard.FormDelegateSeparator {}
            FormCard.FormTextDelegate {
                text: i18nc("@label", "External power")
                description: page.device.externalPower ? i18nc("@info external power", "Connected") : i18nc("@info external power", "Not connected")
            }
        }

        FormCard.FormHeader {
            title: i18nc("@title:group", "Connection")
        }
        FormCard.FormCard {
            FormCard.FormTextDelegate {
                text: i18nc("@label", "Status")
                description: page.device.online ? i18nc("@info device status", "Connected") : (page.device.statusText ?? "")
            }
            FormCard.FormDelegateSeparator {
                visible: lastSeenDelegate.visible
            }
            FormCard.FormTextDelegate {
                id: lastSeenDelegate
                visible: (page.device.lastSeenText ?? "") !== ""
                text: i18nc("@label time of the last answer", "Last answer")
                description: page.device.lastSeenText ?? ""
            }
            FormCard.FormDelegateSeparator {}
            FormCard.FormTextDelegate {
                text: i18nc("@label", "Receiver")
                description: page.device.receiverText ?? ""
            }
            FormCard.FormDelegateSeparator {
                visible: protocolDelegate.visible
            }
            FormCard.FormTextDelegate {
                id: protocolDelegate
                visible: (page.device.protocol ?? "") !== ""
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
        }
        FormCard.FormCard {
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
