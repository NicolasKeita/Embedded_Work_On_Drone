/*
Filename: Src/Embedded/Hil/Transport/Serial/SerialTransport-Port.cpp
Description: POSIX open/configure path of the non-blocking serial transport.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <cerrno>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

module SerialTransport;

import std;

import Transport;

namespace sim::hil {

namespace {

constexpr std::int32_t kOpenFlags = O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC;

/* Applies the raw 460800 8N1 non-blocking line discipline to an open descriptor. */
[[nodiscard]] bool apply_raw_line_discipline(termios& settings) noexcept
{
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
    return true;
}

/* Configures an open descriptor in raw, non-blocking mode. */
[[nodiscard]] bool configure_serial_port(std::int32_t descriptor) noexcept
{
    termios settings{};

    if (tcgetattr(descriptor, &settings) != 0) {
        return false;
    }
    if (!apply_raw_line_discipline(settings)) {
        return false;
    }
    return tcsetattr(descriptor, TCSANOW, &settings) == 0;
}

/* Opens a serial device path, returning -1 with errno set on failure. */
[[nodiscard]] std::int32_t open_serial_device(std::string_view device_path, std::string& storage)
{
    storage.assign(device_path.data(), device_path.size());

    return open(storage.c_str(), kOpenFlags);
}

}

/* Opens and configures a Linux serial device in raw, non-blocking mode. */
SerialTransport::OpenResult SerialTransport::openPort(std::string_view device_path) noexcept
{
    std::string      path{};
    const std::int32_t descriptor = open_serial_device(device_path, path);

    if (descriptor < 0) {
        return std::unexpected(static_cast<std::errc>(errno));
    }
    if (!configure_serial_port(descriptor)) {
        const std::errc error = static_cast<std::errc>(errno);
        close(descriptor);
        return std::unexpected(error);
    }
    tcflush(descriptor, TCIOFLUSH);
    return std::unique_ptr<SerialTransport>{new SerialTransport{descriptor}};
}

SerialTransport::SerialTransport(std::int32_t descriptor) noexcept : descriptor_{descriptor} {}

SerialTransport::~SerialTransport()
{
    if (descriptor_ >= 0) {
        close(descriptor_);
    }
}

}
