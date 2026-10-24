// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>

#include "appsettings.h"
#include "autostart.h"
#include "batterytray.h"
#include "devicemanager.h"
#include "hidsight-version.h"
#include "notifier.h"
#include "qmltypes.h"

using namespace Qt::StringLiterals;

namespace
{

void showWindow(QQuickWindow *window)
{
    window->show();
    window->raise();
    window->requestActivate();
}

} // namespace

int main(int argc, char *argv[])
{
    // QApplication rather than QGuiApplication: the org.kde.desktop style
    // draws the controls through QStyle, which needs the widgets module.
    QApplication app(argc, argv);
    // Closing the window leaves the tray item; Quit ends Hidsight.
    QApplication::setQuitOnLastWindowClosed(false);
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
    about.setHomepage(u"https://github.com/tordenskjoldsw/hidsight"_s);
    about.setBugAddress("https://github.com/tordenskjoldsw/hidsight/issues");
    // Also the Wayland app id, which lets Plasma match the window to the
    // desktop file and its icon.
    about.setDesktopFileName(u"de.tordenskjold.hidsight"_s);
    KAboutData::setApplicationData(about);
    QApplication::setWindowIcon(QIcon::fromTheme(u"de.tordenskjold.hidsight"_s));

    QCommandLineParser parser;
    about.setupCommandLine(&parser);
    const QCommandLineOption backgroundOption(u"background"_s, i18n("Start with only the tray item, without the window."));
    parser.addOption(backgroundOption);
#ifdef HIDSIGHT_DEVELOPER
    const QCommandLineOption replayOption(u"replay"_s,
                                          i18n("Show the devices of a capture <file> instead of the receivers (developer builds)."),
                                          i18nc("@info command line value", "file"));
    parser.addOption(replayOption);
#endif
    parser.process(app);
    about.processCommandLine(&parser);

    // One Hidsight per session: a second start shows the window of the first.
    KDBusService service(KDBusService::Unique);

    AppSettings settings;
    Autostart autostart;
    DeviceManager devices;
    AppSettingsForeign::instance = &settings;
    AutostartForeign::instance = &autostart;
    DeviceManagerForeign::instance = &devices;
#ifdef HIDSIGHT_DEVELOPER
    if (parser.isSet(replayOption)) {
        if (!devices.startReplay(parser.value(replayOption))) {
            return 1;
        }
    } else {
        devices.start();
    }
#else
    devices.start();
#endif

    BatteryTray tray(devices.devices());
    Notifier notifier(&devices, &settings);

    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.loadFromModule("de.tordenskjold.hidsight", "Main");
    auto *window = engine.rootObjects().isEmpty() ? nullptr : qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!window) {
        return 1;
    }
    tray.setWindow(window);
    QObject::connect(&service, &KDBusService::activateRequested, window, [window] {
        showWindow(window);
    });
    if (!parser.isSet(backgroundOption) && !settings.startHidden()) {
        showWindow(window);
    }

    return app.exec();
}
