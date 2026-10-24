// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "channel.h"

#include <QByteArray>
#include <QPointer>

#include <memory>
#include <vector>

class QIODevice;

namespace hidpp
{

// A capture is text, one report per line: "> " and the bytes in hex for a
// report Hidsight sent, "< " for one it received. "#" starts a comment.
// Captures hold raw reports, so they are recorded only on request.
struct CaptureEntry {
    enum class Direction : std::uint8_t {
        Sent,
        Received,
    };

    Direction direction;
    Report report;
};

// Fails with ErrorKind::Malformed and the line number as code.
Result<std::vector<CaptureEntry>> parseCapture(QByteArrayView text);
QByteArray formatCaptureLine(const CaptureEntry &entry);

// Passes everything to another channel and writes each report to output.
class RecordingChannel : public Channel
{
    Q_OBJECT

public:
    RecordingChannel(std::unique_ptr<Channel> inner, QIODevice *output, QObject *parent = nullptr);
    ~RecordingChannel() override;

    std::optional<Error> send(const Report &report) override;
    void drain() override;

private:
    void write(CaptureEntry::Direction direction, const Report &report);

    std::unique_ptr<Channel> m_inner;
    QPointer<QIODevice> m_output;
};

// Plays a capture back: each request must be the next one sent in the
// capture, and the reports received after it in the capture are delivered.
// A request without a recorded answer times out, as it did on the desk.
class ReplayChannel : public Channel
{
    Q_OBJECT

public:
    explicit ReplayChannel(std::vector<CaptureEntry> entries, QObject *parent = nullptr);

    std::optional<Error> send(const Report &report) override;
    void drain() override;

    // Whether every report in the capture was sent or delivered.
    bool isAtEnd() const;
    // The first request that differed from the capture.
    std::optional<Report> mismatch() const;

private:
    std::vector<Report> takeReceived();

    std::vector<CaptureEntry> m_entries;
    std::size_t m_position = 0;
    std::optional<Report> m_mismatch;
};

} // namespace hidpp
