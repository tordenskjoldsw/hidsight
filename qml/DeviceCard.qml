// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// One device in the overview, as a row across the width: its charge as a
// ring, its name, kind and state, and on the right the facts at a glance.
// When the row is narrow, the facts give way; the device page has them.
Kirigami.AbstractCard {
    id: card

    required property string name
    required property string kindText
    required property string iconName
    required property string statusText
    required property string lastSeenText
    required property string firmwareVersion
    required property real batteryFraction
    required property string batteryTone
    required property int batteryPercentage
    required property int dpi
    required property var statusChips
    required property bool online

    readonly property bool wide: width > Kirigami.Units.gridUnit * 28

    showClickFeedback: true
    activeFocusOnTab: true
    Accessible.name: name
    Accessible.description: i18nc("@info device kind, status", "%1, %2", kindText, statusText)
    Accessible.onPressAction: clicked()
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()

    contentItem: RowLayout {
        spacing: Kirigami.Units.gridUnit

        BatteryRing {
            fraction: card.batteryFraction
            tone: card.batteryTone
            iconName: card.iconName
            label: card.batteryPercentage >= 0 ? i18nc("@info battery charge", "%1 %", card.batteryPercentage) : "–"
            opacity: card.online ? 1 : 0.5
            Layout.preferredWidth: Kirigami.Units.gridUnit * 5
            Layout.preferredHeight: Kirigami.Units.gridUnit * 5

            Behavior on opacity {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
                }
            }
        }

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            ColumnLayout {
                spacing: 0
                Layout.fillWidth: true

                Kirigami.Heading {
                    text: card.name
                    level: 2
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    text: card.kindText
                    color: Kirigami.Theme.disabledTextColor
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            StatusChips {
                chips: card.statusChips
                visible: card.statusChips.length > 0
            }
        }

        ColumnLayout {
            visible: card.wide
            spacing: Kirigami.Units.smallSpacing
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight

            QQC2.Label {
                visible: card.lastSeenText !== ""
                text: i18nc("@info time of the last answer", "Last answer: %1", card.lastSeenText)
                color: Kirigami.Theme.disabledTextColor
                Layout.alignment: Qt.AlignRight
            }
            QQC2.Label {
                visible: text !== ""
                text: card.dpi > 0 ? i18nc("@info dots per inch", "%1 dpi", card.dpi) : card.firmwareVersion !== "" ? i18nc("@info firmware version", "Firmware %1", card.firmwareVersion) : ""
                color: Kirigami.Theme.disabledTextColor
                Layout.alignment: Qt.AlignRight
            }
        }
    }
}
