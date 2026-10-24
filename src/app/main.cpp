// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include <KAboutData>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

#include "hidsight-version.h"

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    // QApplication rather than QGuiApplication: the org.kde.desktop style
    // draws the controls through QStyle, which needs the widgets module.
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("hidsight");

    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(u"org.kde.desktop"_s);
    }

    KAboutData about(u"hidsight"_s,
                     i18nc("@title", "Hidsight"),
                     QStringLiteral(HIDSIGHT_VERSION_STRING),
                     i18n("Battery and state of Logitech keyboards and mice"),
                     KAboutLicense::MIT,
                     i18n("© 2026 Tobias Kaminski"));
    about.addAuthor(i18n("Tobias Kaminski"), i18n("Maintainer"));
    about.setOrganizationDomain("tordenskjold.de");
    // Also the Wayland app id, which lets Plasma match the window to the
    // desktop file and its icon.
    about.setDesktopFileName(u"de.tordenskjold.hidsight"_s);
    KAboutData::setApplicationData(about);
    QApplication::setWindowIcon(QIcon::fromTheme(u"de.tordenskjold.hidsight"_s));

    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.loadFromModule("de.tordenskjold.hidsight", "Main");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    return app.exec();
}
