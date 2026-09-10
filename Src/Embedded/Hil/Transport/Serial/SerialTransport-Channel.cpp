/*
Filename: Src/Embedded/Hil/Transport/Serial/SerialTransport-Channel.cpp
Description: Non-blocking byte-channel path of the serial transport.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <cerrno>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

module SerialTransport;

import std;

import Transport;

namespace sim::hil {

namespace {

constexpr std::int32_t kWritePollTimeoutMs = 2;
constexpr std::uint32_t kMaximumWritePolls = 5;

/* Waits until a descriptor accepts one more write within the poll budget. */
[[nodiscard]] bool wait_serial_writable(std::int32_t descriptor)
{
    pollfd event{.fd = descriptor, .events = POLLOUT, .revents = 0};
    const std::int32_t ready = poll(&event, 1, kWritePollTimeoutMs);

    return ready >= 0 || errno == EINTR;
}

/* Reports whether a failed write may be retried after a poll wait. */
[[nodiscard]] bool retry_write(ssize_t written, std::uint32_t polls) noexcept
{
    if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        return false;
    }
    return polls < kMaximumWritePolls;
}

}

bool SerialTransport::sendBytes(std::span<const std::uint8_t> data) noexcept
{
    std::size_t offset = 0;
    std::uint32_t polls = 0;

    while (offset < data.size()) {
        const ssize_t written = write(descriptor_, data.data() + offset, data.size() - offset);
        if (written > 0) {
            offset += static_cast<std::size_t>(written);
            continue;
        }
        if (!retry_write(written, polls)) {
            return false;
        }
        if (!wait_serial_writable(descriptor_)) {
            return false;
        }
        ++polls;
    }
    return true;
}

std::size_t SerialTransport::receiveBytes(std::span<std::uint8_t> buffer) noexcept
{
    const ssize_t received = read(descriptor_, buffer.data(), buffer.size());

    if (received <= 0) {
        return 0;
    }
    return static_cast<std::size_t>(received);
}

std::size_t SerialTransport::bytesAvailable() const noexcept
{
    std::int32_t count = 0;

    if (ioctl(descriptor_, FIONREAD, &count) != 0 || count <= 0) {
        return 0;
    }
    return static_cast<std::size_t>(count);
}

void SerialTransport::flush() noexcept
{
    tcflush(descriptor_, TCIFLUSH);
}

}