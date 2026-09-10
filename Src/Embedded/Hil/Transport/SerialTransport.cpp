/*
Filename: Src/Embedded/Hil/Transport/SerialTransport.cpp
Description: POSIX implementation of the non-blocking physical-target serial transport.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

module SerialTransport;

import std;

namespace sim::hil {

namespace {
    constexpr std::int32_t kWritePollTimeoutMs = 2;
    constexpr std::uint32_t kMaximumWritePolls = 5;

    [[nodiscard]] bool configure_port(std::int32_t descriptor) noexcept
    {
        termios settings{};

        if (tcgetattr(descriptor, &settings) != 0) {
            return false;
        }
        cfmakeraw(&settings);
        if (cfsetispeed(&settings, B460800) != 0 || cfsetospeed(&settings, B460800) != 0) {
            return false;
        }
        settings.c_cflag |= static_cast<tcflag_t>(CLOCAL | CREAD);
        settings.c_cflag &= static_cast<tcflag_t>(~CSTOPB);
        settings.c_cflag &= static_cast<tcflag_t>(~CRTSCTS);
        settings.c_cflag &= static_cast<tcflag_t>(~CSIZE);
        settings.c_cflag |= CS8;
        settings.c_cc[VMIN] = 0;
        settings.c_cc[VTIME] = 0;
        return tcsetattr(descriptor, TCSANOW, &settings) == 0;
    }
}

SerialTransport::SerialTransport(std::int32_t descriptor) noexcept : descriptor_{descriptor} {}

SerialTransport::~SerialTransport()
{
    if (descriptor_ >= 0) {
        close(descriptor_);
    }
}

/* Opens and configures a Linux serial device in raw, non-blocking mode. */
std::expected<std::unique_ptr<SerialTransport>, std::errc>
SerialTransport::openPort(std::string_view device_path) noexcept
{
    const std::string path{device_path};
    const std::int32_t descriptor = open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);

    if (descriptor < 0) {
        return std::unexpected(static_cast<std::errc>(errno));
    }
    if (!configure_port(descriptor)) {
        const std::errc error = static_cast<std::errc>(errno);
        close(descriptor);
        return std::unexpected(error);
    }
    tcflush(descriptor, TCIOFLUSH);
    return std::unique_ptr<SerialTransport>{new SerialTransport{descriptor}};
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
        if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            return false;
        }
        if (polls >= kMaximumWritePolls) {
            return false;
        }
        pollfd event{.fd = descriptor_, .events = POLLOUT, .revents = 0};
        const std::int32_t ready = poll(&event, 1, kWritePollTimeoutMs);
        if (ready < 0 && errno != EINTR) {
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
