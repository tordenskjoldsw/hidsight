// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "listing.h"

#include "hidpp/capture.h"
#include "hidpp/hidrawchannel.h"

#include "hidsight-version.h"

#include <KAboutData>
#include <KLocalizedString>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QTextStream>

#include <memory>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace
{

// Far larger than a capture of six devices; bounds what is read into memory.
constexpr qint64 MaxCaptureSize = 1024 * 1024;

// Replayed replies arrive at once, so a request the capture left unanswered
// need not wait the full timeout.
constexpr std::chrono::milliseconds ReplayTimeout = 50ms;

QTextStream &err()
{
    static QTextStream stream(stderr);
    return stream;
}

hidpp::Result<std::unique_ptr<hidpp::Channel>> openReplay(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return hidpp::Error{file.exists() ? hidpp::ErrorKind::PermissionDenied : hidpp::ErrorKind::NotFound, 0};
    }
    const QByteArray text = file.read(MaxCaptureSize + 1);
    if (text.size() > MaxCaptureSize) {
        return hidpp::Error{hidpp::ErrorKind::Malformed, 0};
    }
    auto entries = hidpp::parseCapture(text);
    if (!entries) {
        return entries.error();
    }
    return std::unique_ptr<hidpp::Channel>(std::make_unique<hidpp::ReplayChannel>(std::move(entries).value()));
}

hidpp::Result<std::unique_ptr<hidpp::Channel>> openNode(const QString &path)
{
    auto channel = hidpp::HidrawChannel::open(path);
    if (!channel) {
        return channel.error();
    }
    return std::unique_ptr<hidpp::Channel>(std::move(channel).value());
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("hidsight");

    KAboutData about(u"hidsight-cli"_s,
                     i18nc("@title", "Hidsight CLI"),
                     QStringLiteral(HIDSIGHT_VERSION_STRING),
                     i18n("Lists the Logitech devices on a receiver"),
                     KAboutLicense::MIT,
                     i18n("© 2026 Tobias Kaminski"));
    about.setOrganizationDomain("tordenskjold.de");
    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    about.setupCommandLine(&parser);
    parser.addPositionalArgument(u"node"_s, i18n("The HID++ hidraw node of a receiver, for example /dev/hidraw3."));
    const QCommandLineOption jsonOption(u"json"_s, i18n("Print JSON instead of text."));
    const QCommandLineOption recordOption(u"record"_s,
                                          i18n("Write every report sent and received to <file>. The file holds raw reports from the devices."),
                                          i18nc("@info command line value", "file"));
    const QCommandLineOption replayOption(u"replay"_s,
                                          i18n("Read the reports from a capture <file> instead of a node."),
                                          i18nc("@info command line value", "file"));
    parser.addOptions({jsonOption, recordOption, replayOption});
    parser.process(app);
    about.processCommandLine(&parser);

    const bool replay = parser.isSet(replayOption);
    const QStringList positional = parser.positionalArguments();
    if (replay == (positional.size() == 1) || positional.size() > 1) {
        err() << i18n("Give either one hidraw node or --replay with a capture file.") << Qt::endl;
        return 1;
    }
    const QString source = replay ? parser.value(replayOption) : positional.first();

    auto opened = replay ? openReplay(source) : openNode(source);
    if (!opened) {
        if (replay && opened.error().kind == hidpp::ErrorKind::Malformed) {
            err() << i18n("%1 is not a valid capture (line %2).", source, opened.error().code) << Qt::endl;
        } else {
            err() << i18nc("@info node, reason", "Cannot open %1: %2", source, errorText(opened.error())) << Qt::endl;
        }
        return 1;
    }
    std::unique_ptr<hidpp::Channel> channel = std::move(opened).value();
    auto *replayChannel = qobject_cast<hidpp::ReplayChannel *>(channel.get());

    QFile recording;
    if (parser.isSet(recordOption)) {
        recording.setFileName(parser.value(recordOption));
        if (!recording.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            err() << i18n("Cannot write %1: %2", recording.fileName(), recording.errorString()) << Qt::endl;
            return 1;
        }
        recording.write(
            u"# Recorded by hidsight-cli %1 from %2 on %3\n"_s.arg(QStringLiteral(HIDSIGHT_VERSION_STRING), source, QDate::currentDate().toString(Qt::ISODate))
                .toUtf8());
        channel = std::make_unique<hidpp::RecordingChannel>(std::move(channel), &recording);
    }

    hidpp::Dispatcher dispatcher(channel.get());
    if (replay) {
        dispatcher.setTimeout(ReplayTimeout);
    }

    int exitCode = 0;
    listReceiver(dispatcher, source, [&](Listing listing) {
        QTextStream out(stdout);
        if (parser.isSet(jsonOption)) {
            out << QJsonDocument(formatJson(listing)).toJson(QJsonDocument::Indented);
        } else {
            out << formatText(listing);
        }
        if (!listing.connectedDevices || !listing.connectedDevices->hasValue()) {
            exitCode = 1;
        }
        if (replayChannel && (replayChannel->mismatch() || !replayChannel->isAtEnd())) {
            err() << i18n("The requests differ from the capture; the output above is incomplete.") << Qt::endl;
            exitCode = 1;
        }
        QCoreApplication::exit(exitCode);
    });
    return app.exec();
}
