// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// A quiet outlined label, such as the kind of a device or "Connected",
// with an optional coloured dot in front.
Rectangle {
    id: pill

    property string text
    // Transparent for no dot.
    property color dotColor: "transparent"

    implicitWidth: row.implicitWidth + Kirigami.Units.largeSpacing * 2
    implicitHeight: row.implicitHeight + Kirigami.Units.smallSpacing
    radius: height / 2
    color: "transparent"
    border.width: 1
    border.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.25)

    RowLayout {
        id: row

        anchors.centerIn: parent
        spacing: Kirigami.Units.smallSpacing

        Rectangle {
            visible: pill.dotColor.a > 0
            implicitWidth: Kirigami.Units.smallSpacing * 1.5
            implicitHeight: implicitWidth
            radius: width / 2
            color: pill.dotColor
        }

        QQC2.Label {
            text: pill.text
            font: Kirigami.Theme.smallFont
            color: Kirigami.Theme.disabledTextColor
        }
    }
}
