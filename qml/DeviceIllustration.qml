// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import org.kde.kirigami as Kirigami

// Hidsight's drawing of a keyboard or a mouse: graphite on a light colour
// scheme, pale grey on a dark one, so it always stands out. A kind without
// a drawing shows its icon.
Item {
    id: root

    // "keyboard", "mouse" or empty, from the model.
    property string illustration
    property string iconName

    readonly property bool darkScheme: Kirigami.ColorUtils.brightnessForColor(Kirigami.Theme.backgroundColor) === Kirigami.ColorUtils.Dark

    Image {
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        visible: root.illustration !== ""
        source: root.illustration !== "" ? Qt.resolvedUrl("illustrations/%1-%2.svg".arg(root.illustration).arg(root.darkScheme ? "light" : "dark")) : ""
        sourceSize.width: width * Screen.devicePixelRatio
        sourceSize.height: height * Screen.devicePixelRatio
        fillMode: Image.PreserveAspectFit
        smooth: true
        asynchronous: true
    }

    Kirigami.Icon {
        anchors.centerIn: parent
        visible: root.illustration === ""
        source: root.iconName
        width: Math.min(parent.width, parent.height) * 0.7
        height: width
    }
}
