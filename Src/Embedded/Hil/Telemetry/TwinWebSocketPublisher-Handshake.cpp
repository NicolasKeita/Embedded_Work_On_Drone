/*
Filename: Src/Embedded/Hil/Telemetry/TwinWebSocketPublisher-Handshake.cpp
Description: WebSocket opening handshake of the Digital Twin publisher : key extraction,
SHA-1/Base64 accept answer and request draining.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>

module TwinWebSocketPublisher;

import std;

namespace sim::hil {

namespace {
    /* Encodes the WebSocket handshake digest as Base64. */
    std::string base64(std::span<const std::uint8_t> input)
    {
        constexpr std::string_view alphabet =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string output;

        output.reserve(((input.size() + 2U) / 3U) * 4U);
        for (std::size_t index = 0; index < input.size(); index += 3U) {
            const std::uint32_t first = input[index];
            const std::uint32_t second = index + 1U < input.size() ? input[index + 1U] : 0U;
            const std::uint32_t third = index + 2U < input.size() ? input[index + 2U] : 0U;
            const std::uint32_t packed = (first << 16U) | (second << 8U) | third;
            output.push_back(alphabet[(packed >> 18U) & 0x3FU]);
            output.push_back(alphabet[(packed >> 12U) & 0x3FU]);
            output.push_back(index + 1U < input.size() ? alphabet[(packed >> 6U) & 0x3FU] : '=');
            output.push_back(index + 2U < input.size() ? alphabet[packed & 0x3FU] : '=');
        }
        return output;
    }

    /* Extracts the browser key from a complete HTTP WebSocket upgrade request. */
    std::string_view websocket_key(std::string_view request) noexcept
    {
        constexpr std::string_view label = "Sec-WebSocket-Key:";
        const std::size_t          begin = request.find(label);

        if (begin == std::string_view::npos) {
            return {};
        }
        const std::size_t value_begin = request.find_first_not_of(" \t", begin + label.size());
        const std::size_t value_end = request.find("\r\n", value_begin);
        if (value_begin == std::string_view::npos || value_end == std::string_view::npos) {
            return {};
        }
        return request.substr(value_begin, value_end - value_begin);
    }

    /* Sends the 101 Switching Protocols answer; false when the client must be closed. */
    bool send_handshake_response(int client_fd, std::string_view key) noexcept
    {
        const std::string                  material = std::string{key} + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        const std::array<std::uint8_t, 20> digest = sha1(material);
        const std::string response = std::format( "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\n"
            "Connection: Upgrade\r\nSec-WebSocket-Accept: {}\r\n\r\n", base64(digest));
        const ssize_t sent = ::send(client_fd, response.data(), response.size(), MSG_DONTWAIT | MSG_NOSIGNAL);

        return sent == static_cast<ssize_t>(response.size());
    }
}

void TwinWebSocketPublisher::complete_handshake() noexcept
{
    if (client_fd_ < 0 || handshake_complete_) {
        return;
    }
    const std::size_t capacity = request_.size() - request_size_;
    const ssize_t received = ::recv(client_fd_, request_.data() + request_size_, capacity, MSG_DONTWAIT);
    if (received == 0) {
        close_client();
        return;
    }
    if (received < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            close_client();
        }
        return;
    }
    request_size_ += static_cast<std::size_t>(received);
    const std::string_view request{request_.data(), request_size_};
    if (!request.contains("\r\n\r\n")) {
        if (request_size_ == request_.size()) {
            close_client();
        }
        return;
    }
    const std::string_view key = websocket_key(request);
    if (key.empty() || !send_handshake_response(client_fd_, key)) {
        close_client();
        return;
    }
    handshake_complete_ = true;
}

}
