// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "channel.h"

#include <QByteArray>
#include <QPointer>

#include <array>
#include <memory>
#include <optional>
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
// The software id of the HID++ 2.0 requests in a capture, for a dispatcher
// that replays it; the default when the capture has none.
std::uint8_t captureSoftwareId(const std::vector<CaptureEntry> &entries);

// Replaces the identifiers of a device in replies by zeros: the unit id in
// the DeviceFwVersion entity count and four bytes of unknown meaning in each
// firmware entry, which differ between devices of the desk. It learns the
// feature index of DeviceFwVersion from the Root and FeatureSet replies that
// pass by, so it needs every request and reply in order.
class IdentifierRedactor
{
public:
    void observeRequest(const Report &request);
    Report redact(const Report &report);

private:
    std::optional<Report> m_request;
    std::array<std::optional<std::uint8_t>, 256> m_featureSetIndex;
    std::array<std::optional<std::uint8_t>, 256> m_firmwareIndex;
};

// Passes everything to another channel and writes each report to output,
// with the identifiers zeroed (IdentifierRedactor).
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
    IdentifierRedactor m_redactor;
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
    // The software id the capture was recorded with.
    std::uint8_t softwareId() const;

private:
    std::vector<Report> takeReceived();

    std::vector<CaptureEntry> m_entries;
    std::size_t m_position = 0;
    std::optional<Report> m_mismatch;
};

} // namespace hidpp
