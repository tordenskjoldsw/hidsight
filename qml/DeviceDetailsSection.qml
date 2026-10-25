// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import de.tordenskjold.hidsight

// The details of a device: battery, connection, firmware and, with the
// technical details switched on, protocol and features.
QQC2.ScrollView {
    id: section

    // The device's roles by name, from DeviceDetails.
    property var device: ({})

    contentWidth: availableWidth

    ColumnLayout {
        width: section.availableWidth
        spacing: 0

        FormCard.FormHeader {
            title: i18nc("@title:group", "Battery")
        }
        FormCard.FormCard {
            FormCard.FormTextDelegate {
                id: chargeDelegate
                // The ring shows a percentage; this row only for a device
                // that reports a voltage or a level alone.
                visible: (section.device.batteryPercentage ?? -1) < 0 && (section.device.batteryText ?? "") !== ""
                text: i18nc("@label", "Charge")
                description: section.device.batteryText ?? ""
            }
            FormCard.FormDelegateSeparator {
                visible: chargeDelegate.visible && levelDelegate.visible
            }
            FormCard.FormTextDelegate {
                id: levelDelegate
                visible: (section.device.batteryLevelText ?? "") !== ""
                text: i18nc("@label battery level reported by the device", "Level")
                description: section.device.batteryLevelText ?? ""
            }
            FormCard.FormDelegateSeparator {
                visible: (chargeDelegate.visible || levelDelegate.visible) && lastSeenDelegate.visible
            }
            FormCard.FormTextDelegate {
                id: lastSeenDelegate
                visible: (section.device.lastSeenText ?? "") !== ""
                text: i18nc("@label time of the last answer", "Last answer")
                description: section.device.lastSeenText ?? ""
            }
        }

        FormCard.FormHeader {
            title: i18nc("@title:group", "Connection")
        }
        FormCard.FormCard {
            FormCard.FormTextDelegate {
                text: i18nc("@label", "Receiver")
                description: section.device.receiverText ?? ""
            }
            FormCard.FormDelegateSeparator {
                visible: protocolDelegate.visible
            }
            FormCard.FormTextDelegate {
                id: protocolDelegate
                visible: Settings.showTechnicalDetails && (section.device.protocol ?? "") !== ""
                text: i18nc("@label", "Protocol")
                description: i18nc("@info protocol version", "HID++ %1", section.device.protocol ?? "")
            }
        }

        FormCard.FormHeader {
            title: i18nc("@title:group", "Device")
            visible: deviceCard.visible
        }
        FormCard.FormCard {
            id: deviceCard
            visible: (section.device.firmware ?? []).length > 0 || (section.device.dpi ?? 0) > 0 || (section.device.reportRate ?? 0) > 0

            Repeater {
                model: section.device.firmware ?? []

                delegate: FormCard.FormTextDelegate {
                    required property var modelData
                    text: modelData.label
                    description: modelData.version
                }
            }
            FormCard.FormTextDelegate {
                visible: (section.device.dpi ?? 0) > 0
                text: i18nc("@label pointer resolution", "Resolution")
                description: i18nc("@info dots per inch", "%1 dpi", section.device.dpi ?? 0)
            }
            FormCard.FormTextDelegate {
                visible: (section.device.reportRate ?? 0) > 0
                text: i18nc("@label", "Report rate")
                description: i18nc("@info report interval", "Every %1 ms", section.device.reportRate ?? 0)
            }
        }

        FormCard.FormHeader {
            title: i18nc("@title:group", "Features")
            visible: Settings.showTechnicalDetails
        }
        FormCard.FormCard {
            visible: Settings.showTechnicalDetails

            FormCard.FormTextDelegate {
                visible: (section.device.features ?? []).length === 0
                text: section.device.online ? i18nc("@info", "Reading…") : i18nc("@info", "Read when the device answers")
            }
            Repeater {
                model: section.device.features ?? []

                delegate: FormCard.FormTextDelegate {
                    required property var modelData
                    text: modelData.title
                    description: modelData.detail
                }
            }
        }

        Item {
            Layout.preferredHeight: Kirigami.Units.gridUnit
        }
    }
}
