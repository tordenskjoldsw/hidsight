// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// One device in the overview: where it is paired and its battery at the
// top, a drawing of the device in the middle, its name, kind and state at
// the bottom. A device that does not answer is shown quieter.
Kirigami.AbstractCard {
    id: card

    required property string name
    required property string kindText
    required property string iconName
    required property string illustration
    required property string slotText
    required property string statusText
    required property string batteryIconName
    required property string batteryTone
    required property int batteryPercentage
    required property var statusChips
    required property bool online

    showClickFeedback: true
    activeFocusOnTab: true
    Accessible.name: name
    Accessible.description: i18nc("@info device kind, status", "%1, %2", kindText, statusText)
    Accessible.onPressAction: clicked()
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        RowLayout {
            spacing: Kirigami.Units.smallSpacing
            Layout.fillWidth: true

            Kirigami.Icon {
                source: "network-wireless-symbolic"
                color: Kirigami.Theme.disabledTextColor
                isMask: true
                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                Layout.preferredHeight: Kirigami.Units.iconSizes.small
            }
            QQC2.Label {
                text: card.slotText
                color: Kirigami.Theme.disabledTextColor
                font: Kirigami.Theme.smallFont
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Kirigami.Icon {
                visible: card.online
                source: card.batteryIconName + "-symbolic"
                color: Tones.color(card.batteryTone)
                isMask: true
                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium

                Behavior on color {
                    ColorAnimation {
                        duration: Kirigami.Units.longDuration
                    }
                }
            }
            QQC2.Label {
                text: !card.online ? i18nc("@info:status", "Not connected") : card.batteryPercentage >= 0 ? i18nc("@info battery charge", "%1 %", card.batteryPercentage) : ""
                color: card.online ? Tones.color(card.batteryTone) : Kirigami.Theme.disabledTextColor
                font.weight: Font.DemiBold
            }
        }

        DeviceIllustration {
            illustration: card.illustration
            iconName: card.iconName
            opacity: card.online ? 1 : 0.45
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 7
            Layout.topMargin: Kirigami.Units.smallSpacing
            Layout.bottomMargin: Kirigami.Units.smallSpacing

            Behavior on opacity {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
                }
            }
        }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing
            Layout.fillWidth: true

            Kirigami.Heading {
                text: card.name
                level: 3
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                // As wide as the name; shorter only when the card is.
                Layout.preferredWidth: implicitWidth
                Layout.minimumWidth: Kirigami.Units.gridUnit * 3
            }

            // The kind as a quiet outlined label next to the name.
            Rectangle {
                implicitWidth: kindLabel.implicitWidth + Kirigami.Units.largeSpacing * 2
                implicitHeight: kindLabel.implicitHeight + Kirigami.Units.smallSpacing
                radius: height / 2
                color: "transparent"
                border.width: 1
                border.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.25)

                QQC2.Label {
                    id: kindLabel
                    anchors.centerIn: parent
                    text: card.kindText
                    font: Kirigami.Theme.smallFont
                    color: Kirigami.Theme.disabledTextColor
                }
            }

            Item {
                Layout.fillWidth: true
            }

            StatusChips {
                chips: card.statusChips
            }
        }
    }
}
