// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "capture.h"
#include "hidpp_logging.h"

#include <QIODevice>
#include <QTimer>

namespace hidpp
{

namespace
{

// A long report in hex with separators and the direction mark fits easily.
constexpr qsizetype MaxLineLength = 128;

std::optional<std::uint8_t> hexDigit(char c)
{
    if (c >= '0' && c <= '9') {
        return static_cast<std::uint8_t>(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
        return static_cast<std::uint8_t>(c - 'a' + 10);
    }
    if (c >= 'A' && c <= 'F') {
        return static_cast<std::uint8_t>(c - 'A' + 10);
    }
    return std::nullopt;
}

std::optional<Report> parseReport(QLatin1StringView hex)
{
    std::array<std::uint8_t, LongReportSize> bytes{};
    std::size_t count = 0;
    for (const QLatin1StringView token : hex.tokenize(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        if (token.size() != 2 || count == bytes.size()) {
            return std::nullopt;
        }
        const auto high = hexDigit(token[0].toLatin1());
        const auto low = hexDigit(token[1].toLatin1());
        if (!high || !low) {
            return std::nullopt;
        }
        bytes[count++] = static_cast<std::uint8_t>((*high << 4) | *low);
    }
    return Report::fromBytes(std::span<const std::uint8_t>(bytes).first(count));
}

} // namespace

Result<std::vector<CaptureEntry>> parseCapture(QByteArrayView text)
{
    std::vector<CaptureEntry> entries;
    int lineNumber = 0;
    for (const QLatin1StringView rawLine : QLatin1StringView(text).tokenize(QLatin1Char('\n'))) {
        ++lineNumber;
        const QLatin1StringView line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        if (line.size() > MaxLineLength || line.size() < 2 || line[1] != QLatin1Char(' ')) {
            return Error{ErrorKind::Malformed, lineNumber};
        }
        CaptureEntry::Direction direction;
        if (line[0] == QLatin1Char('>')) {
            direction = CaptureEntry::Direction::Sent;
        } else if (line[0] == QLatin1Char('<')) {
            direction = CaptureEntry::Direction::Received;
        } else {
            return Error{ErrorKind::Malformed, lineNumber};
        }
        const auto report = parseReport(line.sliced(2));
        if (!report) {
            return Error{ErrorKind::Malformed, lineNumber};
        }
        entries.push_back(CaptureEntry{direction, *report});
    }
    return entries;
}

QByteArray formatCaptureLine(const CaptureEntry &entry)
{
    const auto bytes = entry.report.bytes();
    QByteArray line = entry.direction == CaptureEntry::Direction::Sent ? QByteArrayLiteral("> ") : QByteArrayLiteral("< ");
    line += QByteArrayView(bytes.data(), qsizetype(bytes.size())).toByteArray().toHex(' ');
    line += '\n';
    return line;
}

namespace
{

constexpr std::uint8_t RootIndex = 0x00;
constexpr std::uint8_t GetFeature = 0;
constexpr std::uint8_t GetFeatureId = 1;
constexpr std::uint8_t GetEntityCount = 0;
constexpr std::uint8_t GetFwInfo = 1;
constexpr std::uint16_t FeatureSetId = 0x0001;
constexpr std::uint16_t DeviceFwVersionId = 0x0003;

std::uint16_t featureIdAt(std::span<const std::uint8_t> params)
{
    return static_cast<std::uint16_t>((params[0] << 8) | params[1]);
}

} // namespace

void CaptureFilter::observeRequest(const Report &request)
{
    m_request = request;
}

std::optional<Report> CaptureFilter::filter(const Report &report)
{
    const ReplyMatch match = m_request ? matchReply(*m_request, report) : ReplyMatch::None;
    if (match == ReplyMatch::None) {
        const bool isEvent =
            report.deviceIndex() != ReceiverIndex && report.subId() != Error10SubId && report.subId() != Error20FeatureIndex && (report.address() & 0x0F) == 0;
        return isEvent ? std::optional<Report>(report) : std::nullopt;
    }
    if (match != ReplyMatch::Reply) {
        return report;
    }
    const Report &request = *m_request;
    const std::uint8_t device = request.deviceIndex();
    const std::uint8_t function = request.address() >> 4;
    const auto params = report.params();

    if (request.subId() == RootIndex && function == GetFeature) {
        const std::uint16_t id = featureIdAt(request.params());
        if (id == FeatureSetId) {
            m_featureSetIndex[device] = params[0];
        } else if (id == DeviceFwVersionId) {
            m_firmwareIndex[device] = params[0];
        }
        return report;
    }
    if (m_featureSetIndex[device] == request.subId() && function == GetFeatureId) {
        if (featureIdAt(params) == DeviceFwVersionId) {
            m_firmwareIndex[device] = request.params()[0];
        }
        return report;
    }
    if (m_firmwareIndex[device] == request.subId()) {
        if (function == GetEntityCount) {
            return report.withParamsZeroed(1, 4);
        }
        if (function == GetFwInfo) {
            return report.withParamsZeroed(11, 4);
        }
    }
    return report;
}

std::uint8_t captureSoftwareId(const std::vector<CaptureEntry> &entries)
{
    for (const CaptureEntry &entry : entries) {
        if (entry.direction == CaptureEntry::Direction::Sent && entry.report.deviceIndex() != ReceiverIndex) {
            return entry.report.address() & 0x0F;
        }
    }
    return DefaultSoftwareId;
}

RecordingChannel::RecordingChannel(std::unique_ptr<Channel> inner, QIODevice *output, QObject *parent)
    : Channel(parent)
    , m_inner(std::move(inner))
    , m_output(output)
{
    connect(m_inner.get(), &Channel::reportReceived, this, [this](const Report &report) {
        if (const auto kept = m_filter.filter(report)) {
            write(CaptureEntry::Direction::Received, *kept);
        }
        Q_EMIT reportReceived(report);
    });
    connect(m_inner.get(), &Channel::disconnected, this, &Channel::disconnected);
}

RecordingChannel::~RecordingChannel() = default;

std::optional<Error> RecordingChannel::send(const Report &report)
{
    m_filter.observeRequest(report);
    write(CaptureEntry::Direction::Sent, report);
    return m_inner->send(report);
}

void RecordingChannel::drain()
{
    m_inner->drain();
}

void RecordingChannel::write(CaptureEntry::Direction direction, const Report &report)
{
    if (!m_output) {
        return;
    }
    m_output->write(formatCaptureLine(CaptureEntry{direction, report}));
}

ReplayChannel::ReplayChannel(std::vector<CaptureEntry> entries, QObject *parent)
    : Channel(parent)
    , m_entries(std::move(entries))
{
}

std::optional<Error> ReplayChannel::send(const Report &report)
{
    if (m_position >= m_entries.size() || m_entries[m_position].direction != CaptureEntry::Direction::Sent || !(m_entries[m_position].report == report)) {
        if (!m_mismatch) {
            m_mismatch = report;
        }
        qCDebug(HIDSIGHT_HIDPP) << "replay: request differs from the capture at entry" << m_position;
        return Error{ErrorKind::Transport, 0};
    }
    ++m_position;
    // Delivered from the event loop, as replies from a node would be.
    QTimer::singleShot(0, this, [this, received = takeReceived()] {
        for (const Report &reply : received) {
            Q_EMIT reportReceived(reply);
        }
        if (isAtEnd()) {
            Q_EMIT finished();
        }
    });
    return std::nullopt;
}

void ReplayChannel::drain()
{
    for (const Report &report : takeReceived()) {
        Q_EMIT reportReceived(report);
    }
}

bool ReplayChannel::isAtEnd() const
{
    return m_position == m_entries.size();
}

std::uint8_t ReplayChannel::softwareId() const
{
    return captureSoftwareId(m_entries);
}

std::optional<Report> ReplayChannel::mismatch() const
{
    return m_mismatch;
}

std::vector<Report> ReplayChannel::takeReceived()
{
    std::vector<Report> received;
    while (m_position < m_entries.size() && m_entries[m_position].direction == CaptureEntry::Direction::Received) {
        received.push_back(m_entries[m_position].report);
        ++m_position;
    }
    return received;
}

} // namespace hidpp
