/*
Filename: Src/Embedded/Hil/Telemetry/TwinWebSocketPublisher-Sockets.cpp
Description: POSIX socket lifecycle and WebSocket frame emission for Digital Twin telemetry.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

module TwinWebSocketPublisher;

import std;

namespace sim::hil {

TwinWebSocketPublisher::TwinWebSocketPublisher(std::uint16_t port) noexcept
{
    server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        return;
    }
    const int reuse = 1;
    static_cast<void>(::setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)));
    static_cast<void>(::fcntl(server_fd_, F_SETFL, O_NONBLOCK));
    const sockaddr_in address{.sin_family = AF_INET,
                              .sin_port = htons(port),
                              .sin_addr = {.s_addr = htonl(INADDR_LOOPBACK)},
                              .sin_zero = {}};
    if (::bind(server_fd_, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0
        || ::listen(server_fd_, 1) < 0) {
        ::close(server_fd_);
        server_fd_ = -1;
    }
}

TwinWebSocketPublisher::~TwinWebSocketPublisher()
{
    close_client();
    if (server_fd_ >= 0) {
        ::close(server_fd_);
    }
}

void TwinWebSocketPublisher::accept_client() noexcept
{
    if (server_fd_ < 0 || client_fd_ >= 0) {
        return;
    }
    client_fd_ = ::accept(server_fd_, nullptr, nullptr);
    if (client_fd_ >= 0) {
        static_cast<void>(::fcntl(client_fd_, F_SETFL, O_NONBLOCK));
        request_size_ = 0;
        handshake_complete_ = false;
    }
}

void TwinWebSocketPublisher::close_client() noexcept
{
    if (client_fd_ >= 0) {
        ::close(client_fd_);
    }
    client_fd_ = -1;
    handshake_complete_ = false;
    request_size_ = 0;
}

void TwinWebSocketPublisher::publish(std::string_view snapshot) noexcept
{
    accept_client();
    complete_handshake();
    if (!handshake_complete_ || snapshot.size() > frame_.size() - 10U) {
        return;
    }
    std::size_t header_size = 2U;
    frame_[0] = 0x81U;
    if (snapshot.size() <= 125U) {
        frame_[1] = static_cast<std::uint8_t>(snapshot.size());
    }
    else if (snapshot.size() <= 65535U) {
        frame_[1] = 126U;
        frame_[2] = static_cast<std::uint8_t>(snapshot.size() >> 8U);
        frame_[3] = static_cast<std::uint8_t>(snapshot.size());
        header_size = 4U;
    }
    else {
        return;
    }
    std::copy(snapshot.begin(), snapshot.end(), frame_.begin() + static_cast<std::ptrdiff_t>(header_size));
    const ssize_t sent = ::send(client_fd_, frame_.data(), header_size + snapshot.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
    if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        close_client();
    }
}

}
