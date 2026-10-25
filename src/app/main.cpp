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
#include <QTimer>

#include <optional>

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

#ifdef HIDSIGHT_DEVELOPER
// For screenshots without a desktop (QT_QPA_PLATFORM=offscreen): waits for
// the first reads, opens a device page if asked, lets the animations end and
// saves the window.
void grabWindow(QQuickWindow *window, const QString &path, const QString &openRow, const QString &size)
{
    const QStringList parts = size.split(u'x');
    if (parts.size() == 2) {
        window->resize(parts[0].toInt(), parts[1].toInt());
    }
    window->show();
    QTimer::singleShot(std::chrono::milliseconds(4000), window, [window, path, openRow] {
        // The virtual pointer of the offscreen platform rests at 0,0, over
        // the menu button, whose tooltip would end up in the picture.
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(window, &leave);
        if (!openRow.isEmpty()) {
            if (!QMetaObject::invokeMethod(window, "showDevice", Q_ARG(int, openRow.toInt()))) {
                qWarning() << "cannot open the device in row" << openRow;
            }
        }
        QTimer::singleShot(std::chrono::milliseconds(1500), window, [window, path] {
            const bool saved = window->grabWindow().save(path);
            qInfo().noquote() << (saved ? "saved" : "could not save") << path;
            QCoreApplication::exit(saved ? 0 : 1);
        });
    });
}
#endif

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
    const QCommandLineOption grabOption(u"grab"_s,
                                        i18n("Save a picture of the window to <file> once the devices are read, then quit (developer builds)."),
                                        i18nc("@info command line value", "file"));
    const QCommandLineOption openOption(u"open"_s,
                                        i18n("With --grab, open the device page of the card in <row> first."),
                                        i18nc("@info command line value", "row"));
    const QCommandLineOption sizeOption(u"size"_s,
                                        i18n("With --grab, the size of the window, for example 1100x720."),
                                        i18nc("@info command line value", "size"));
    parser.addOptions({grabOption, openOption, sizeOption});
#endif
    parser.process(app);
    about.processCommandLine(&parser);

    // One Hidsight per session: a second start shows the window of the first.
    // Developer runs stay apart, so they neither end in nor disturb the
    // Hidsight that is already running.
    bool developerRun = false;
#ifdef HIDSIGHT_DEVELOPER
    developerRun = parser.isSet(replayOption) || parser.isSet(grabOption);
#endif
    std::optional<KDBusService> service;
    if (!developerRun) {
        service.emplace(KDBusService::Unique);
    }

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
        // Apart from the installed Hidsight on the same receiver (0x0A) and
        // from hidsight-cli (0x0B), so neither takes the other's replies.
        if (developerRun) {
            devices.setSoftwareId(0x0C);
        }
        devices.start();
    }
#else
    devices.start();
#endif

    BatteryTray tray(devices.devices());
    // A screenshot run must not send notifications to the desktop it runs on.
    bool grabbing = false;
#ifdef HIDSIGHT_DEVELOPER
    grabbing = parser.isSet(grabOption);
#endif
    std::optional<Notifier> notifier;
    if (!grabbing) {
        notifier.emplace(&devices, &settings);
    }

    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.loadFromModule("de.tordenskjold.hidsight", "Main");
    auto *window = engine.rootObjects().isEmpty() ? nullptr : qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!window) {
        return 1;
    }
    tray.setWindow(window);
    if (service) {
        QObject::connect(&*service, &KDBusService::activateRequested, window, [window] {
            showWindow(window);
        });
    }
#ifdef HIDSIGHT_DEVELOPER
    if (parser.isSet(grabOption)) {
        grabWindow(window, parser.value(grabOption), parser.value(openOption), parser.value(sizeOption));
        return app.exec();
    }
#endif
    if (!parser.isSet(backgroundOption) && !settings.startHidden()) {
        showWindow(window);
    }

    return app.exec();
}
