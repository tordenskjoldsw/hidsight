// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import org.kde.kirigami as Kirigami

// The short states of a device ("Charging", "On cable", "Not connected") as
// chips that fade in; the model gives at most two.
Row {
    id: flow

    // [{text, iconName}] from the model.
    property var chips: []

    spacing: Kirigami.Units.smallSpacing

    add: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: Kirigami.Units.longDuration
        }
    }
    move: Transition {
        NumberAnimation {
            properties: "x,y"
            duration: Kirigami.Units.longDuration
            easing.type: Easing.OutCubic
        }
    }

    Repeater {
        model: flow.chips

        delegate: Kirigami.Chip {
            required property var modelData

            text: modelData.text
            icon.name: modelData.iconName
            closable: false
            checkable: false
            interactive: false
        }
    }
}
