// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

pragma Singleton

import QtQuick
import org.kde.kirigami as Kirigami

// The colour of a battery tone from the model ("charging", "critical",
// "low", "normal", "unknown"), the same for the ring and the card.
QtObject {
    function color(tone: string): color {
        switch (tone) {
        case "charging":
            return Kirigami.Theme.positiveTextColor;
        case "critical":
            return Kirigami.Theme.negativeTextColor;
        case "low":
            return Kirigami.Theme.neutralTextColor;
        case "unknown":
            return Kirigami.Theme.disabledTextColor;
        default:
            return Kirigami.Theme.highlightColor;
        }
    }
}
