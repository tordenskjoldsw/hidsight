// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "channel.h"

#include <memory>

class QSocketNotifier;

namespace hidpp
{

// Owns a file descriptor and closes it.
class FileDescriptor
{
public:
    FileDescriptor() = default;
    explicit FileDescriptor(int fd);
    ~FileDescriptor();

    FileDescriptor(FileDescriptor &&other) noexcept;
    FileDescriptor &operator=(FileDescriptor &&other) noexcept;
    FileDescriptor(const FileDescriptor &) = delete;
    FileDescriptor &operator=(const FileDescriptor &) = delete;

    int get() const
    {
        return m_fd;
    }

    bool isValid() const
    {
        return m_fd >= 0;
    }

    void reset();

private:
    int m_fd = -1;
};

// A hidraw node, opened read-write and non-blocking, never exclusively, so
// Solaar or logiops can use the same node at the same time.
class HidrawChannel : public Channel
{
    Q_OBJECT

public:
    static Result<std::unique_ptr<HidrawChannel>> open(const QString &path);
    ~HidrawChannel() override;

    std::optional<Error> send(const Report &report) override;
    void drain() override;

private:
    explicit HidrawChannel(FileDescriptor fd);

    void readAvailable();
    void close();

    FileDescriptor m_fd;
    std::unique_ptr<QSocketNotifier> m_notifier;
};

} // namespace hidpp
