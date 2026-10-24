// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "hidrawchannel.h"
#include "hidpp_logging.h"
#include "raw_logging.h"

#include <QFile>
#include <QSocketNotifier>

#include <array>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <utility>

namespace hidpp
{

namespace
{

Error errorFromErrno(int code)
{
    switch (code) {
    case EACCES:
    case EPERM:
        return {ErrorKind::PermissionDenied, code};
    case ENOENT:
        return {ErrorKind::NotFound, code};
    case ENODEV:
    case ENXIO:
    case EIO:
        return {ErrorKind::Disconnected, code};
    default:
        return {ErrorKind::Transport, code};
    }
}

// Bounds the reports handled per wake-up so a chatty device cannot starve
// the event loop; the notifier fires again for the rest.
constexpr int MaxReportsPerRead = 64;

} // namespace

FileDescriptor::FileDescriptor(int fd)
    : m_fd(fd)
{
}

FileDescriptor::~FileDescriptor()
{
    reset();
}

FileDescriptor::FileDescriptor(FileDescriptor &&other) noexcept
    : m_fd(std::exchange(other.m_fd, -1))
{
}

FileDescriptor &FileDescriptor::operator=(FileDescriptor &&other) noexcept
{
    if (this != &other) {
        reset();
        m_fd = std::exchange(other.m_fd, -1);
    }
    return *this;
}

void FileDescriptor::reset()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

Result<std::unique_ptr<HidrawChannel>> HidrawChannel::open(const QString &path)
{
    FileDescriptor fd(::open(QFile::encodeName(path).constData(), O_RDWR | O_NONBLOCK | O_CLOEXEC));
    if (!fd.isValid()) {
        const int code = errno;
        qCDebug(HIDSIGHT_HIDPP) << "cannot open" << path << "errno" << code;
        return errorFromErrno(code);
    }
    return std::unique_ptr<HidrawChannel>(new HidrawChannel(std::move(fd)));
}

HidrawChannel::HidrawChannel(FileDescriptor fd)
    : m_fd(std::move(fd))
    , m_notifier(std::make_unique<QSocketNotifier>(m_fd.get(), QSocketNotifier::Read))
{
    connect(m_notifier.get(), &QSocketNotifier::activated, this, &HidrawChannel::readAvailable);
}

HidrawChannel::~HidrawChannel() = default;

std::optional<Error> HidrawChannel::send(const Report &report)
{
    if (!m_fd.isValid()) {
        return Error{ErrorKind::Disconnected, 0};
    }
    qCDebug(HIDSIGHT_RAW) << "send" << report;
    const auto bytes = report.bytes();
    ssize_t written = -1;
    do {
        written = ::write(m_fd.get(), bytes.data(), bytes.size());
    } while (written < 0 && errno == EINTR);
    if (written < 0) {
        const Error error = errorFromErrno(errno);
        if (error.kind == ErrorKind::Disconnected) {
            close();
        }
        return error;
    }
    if (static_cast<std::size_t>(written) != bytes.size()) {
        return Error{ErrorKind::Transport, EIO};
    }
    return std::nullopt;
}

void HidrawChannel::drain()
{
    readAvailable();
}

void HidrawChannel::readAvailable()
{
    // hidraw delivers one report per read; a report larger than the buffer
    // arrives cut off and is rejected as not HID++.
    std::array<std::uint8_t, 64> buffer{};
    for (int count = 0; count < MaxReportsPerRead && m_fd.isValid(); ++count) {
        const ssize_t received = ::read(m_fd.get(), buffer.data(), buffer.size());
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno != EAGAIN) {
                qCDebug(HIDSIGHT_HIDPP) << "read failed, errno" << errno;
                close();
            }
            return;
        }
        if (received == 0) {
            return;
        }
        const auto report = Report::fromBytes(std::span<const std::uint8_t>(buffer).first(static_cast<std::size_t>(received)));
        if (!report) {
            qCDebug(HIDSIGHT_HIDPP) << "ignored a non-HID++ report of" << received << "bytes";
            continue;
        }
        qCDebug(HIDSIGHT_RAW) << "received" << *report;
        Q_EMIT reportReceived(*report);
    }
}

void HidrawChannel::close()
{
    if (!m_fd.isValid()) {
        return;
    }
    m_notifier->setEnabled(false);
    m_fd.reset();
    Q_EMIT disconnected();
}

} // namespace hidpp
