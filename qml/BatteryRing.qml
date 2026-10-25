// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Shapes
import org.kde.kirigami as Kirigami

// The charge of a device as a ring around its icon, like the app icon. The
// ring fills and changes colour smoothly when the value changes.
Item {
    id: ring

    // 0 to 1; below 0 when the device gave no reading.
    property real fraction: -1
    // "charging", "critical", "low", "normal" or "unknown", from the model.
    property string tone: "unknown"
    property string iconName
    property string label

    readonly property real thickness: Math.max(3, Math.round(width * 0.07))
    readonly property real radius: (Math.min(width, height) - thickness) / 2

    implicitWidth: Kirigami.Units.gridUnit * 6
    implicitHeight: implicitWidth

    Accessible.role: Accessible.ProgressBar
    Accessible.name: label

    property real shownFraction: Math.max(0, fraction)
    Behavior on shownFraction {
        NumberAnimation {
            duration: Kirigami.Units.veryLongDuration
            easing.type: Easing.OutCubic
        }
    }

    property color arcColor: Tones.color(tone === "unknown" ? "normal" : tone)
    Behavior on arcColor {
        ColorAnimation {
            duration: Kirigami.Units.longDuration
        }
    }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: "transparent"
            strokeColor: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.12)
            strokeWidth: ring.thickness

            PathAngleArc {
                centerX: ring.width / 2
                centerY: ring.height / 2
                radiusX: ring.radius
                radiusY: ring.radius
                startAngle: -90
                sweepAngle: 360
            }
        }

        ShapePath {
            fillColor: "transparent"
            strokeColor: ring.arcColor
            strokeWidth: ring.thickness
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: ring.width / 2
                centerY: ring.height / 2
                radiusX: ring.radius
                radiusY: ring.radius
                startAngle: -90
                sweepAngle: 360 * ring.shownFraction
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 0

        Kirigami.Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: ring.iconName !== ""
            source: ring.iconName
            width: Math.round(ring.width * 0.38)
            height: width
        }

        QQC2.Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: ring.label
            font.pixelSize: Math.max(Kirigami.Theme.smallFont.pixelSize, Math.round(ring.width * (ring.iconName !== "" ? 0.13 : 0.2)))
            font.weight: Font.DemiBold
            color: ring.fraction < 0 ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.textColor
        }
    }
}
