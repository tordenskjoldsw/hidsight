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

RecordingChannel::RecordingChannel(std::unique_ptr<Channel> inner, QIODevice *output, QObject *parent)
    : Channel(parent)
    , m_inner(std::move(inner))
    , m_output(output)
{
    connect(m_inner.get(), &Channel::reportReceived, this, [this](const Report &report) {
        write(CaptureEntry::Direction::Received, report);
        Q_EMIT reportReceived(report);
    });
    connect(m_inner.get(), &Channel::disconnected, this, &Channel::disconnected);
}

RecordingChannel::~RecordingChannel() = default;

std::optional<Error> RecordingChannel::send(const Report &report)
{
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
