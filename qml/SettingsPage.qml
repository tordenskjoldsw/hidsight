// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import de.tordenskjold.hidsight

FormCard.FormCardPage {
    id: page

    title: i18nc("@title", "Settings")

    FormCard.FormHeader {
        title: i18nc("@title:group", "Notifications")
    }
    FormCard.FormCard {
        FormCard.FormSwitchDelegate {
            text: i18nc("@option:check", "Battery low")
            description: i18nc("@info", "When a keyboard or mouse reports a low or critical battery.")
            checked: Settings.notifyLowBattery
            onToggled: Settings.notifyLowBattery = checked
        }
        FormCard.FormDelegateSeparator {}
        FormCard.FormSwitchDelegate {
            text: i18nc("@option:check", "Charge complete")
            description: i18nc("@info", "When a keyboard or mouse on its cable is fully charged.")
            checked: Settings.notifyChargeComplete
            onToggled: Settings.notifyChargeComplete = checked
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Start")
    }
    FormCard.FormCard {
        FormCard.FormSwitchDelegate {
            text: i18nc("@option:check", "Start automatically at login")
            description: i18nc("@info", "Only the tray item appears; the window opens with a click on it.")
            checked: Autostart.enabled
            enabled: !Autostart.busy
            onToggled: Autostart.enabled = checked
        }
        FormCard.FormDelegateSeparator {}
        FormCard.FormSwitchDelegate {
            text: i18nc("@option:check", "Start without the window")
            description: i18nc("@info", "Also when started from the launcher, only the tray item appears.")
            checked: Settings.startHidden
            onToggled: Settings.startHidden = checked
        }
    }

    FormCard.FormCard {
        visible: Autostart.problem !== ""
        Layout.topMargin: Kirigami.Units.largeSpacing

        FormCard.FormTextDelegate {
            icon.name: "dialog-warning"
            text: Autostart.problem
        }
    }
}
