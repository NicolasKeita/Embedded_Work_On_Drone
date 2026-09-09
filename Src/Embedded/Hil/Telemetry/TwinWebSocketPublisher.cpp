/*
Filename: Src/Embedded/Hil/Telemetry/TwinWebSocketPublisher.cpp
Description: POSIX socket and WebSocket framing implementation for Digital Twin telemetry.

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

namespace {
    /* Rotates one SHA-1 working word left by the requested bit count. */
    constexpr std::uint32_t rotate_left(std::uint32_t value, std::uint32_t bits) noexcept
    {
        return (value << bits) | (value >> (32U - bits));
    }

    /* Computes the SHA-1 digest required by the WebSocket opening handshake. */
    std::array<std::uint8_t, 20> sha1(std::string_view input) noexcept
    {
        std::array<std::uint8_t, 128> message{};
        const std::size_t message_size = input.size();
        const std::size_t padded_size = ((message_size + 9U + 63U) / 64U) * 64U;
        std::copy(input.begin(), input.end(), message.begin());
        message[message_size] = 0x80U;
        const std::uint64_t bit_length = static_cast<std::uint64_t>(message_size) * 8U;
        for (std::size_t index = 0; index < 8U; ++index) {
            message[padded_size - 1U - index] = static_cast<std::uint8_t>(bit_length >> (index * 8U));
        }

        std::uint32_t h0 = 0x67452301U;
        std::uint32_t h1 = 0xEFCDAB89U;
        std::uint32_t h2 = 0x98BADCFEU;
        std::uint32_t h3 = 0x10325476U;
        std::uint32_t h4 = 0xC3D2E1F0U;

        for (std::size_t offset = 0; offset < padded_size; offset += 64U) {
            std::array<std::uint32_t, 80> words{};
            for (std::size_t index = 0; index < 16U; ++index) {
                const std::size_t base = offset + index * 4U;
                words[index] = (static_cast<std::uint32_t>(message[base]) << 24U)
                               | (static_cast<std::uint32_t>(message[base + 1U]) << 16U)
                               | (static_cast<std::uint32_t>(message[base + 2U]) << 8U)
                               | static_cast<std::uint32_t>(message[base + 3U]);
            }
            for (std::size_t index = 16U; index < words.size(); ++index) {
                words[index] = rotate_left(words[index - 3U] ^ words[index - 8U]
                                               ^ words[index - 14U] ^ words[index - 16U],
                                           1U);
            }

            std::uint32_t a = h0;
            std::uint32_t b = h1;
            std::uint32_t c = h2;
            std::uint32_t d = h3;
            std::uint32_t e = h4;
            for (std::size_t index = 0; index < words.size(); ++index) {
                std::uint32_t function = 0;
                std::uint32_t constant = 0;
                if (index < 20U) {
                    function = (b & c) | ((~b) & d);
                    constant = 0x5A827999U;
                }
                else if (index < 40U) {
                    function = b ^ c ^ d;
                    constant = 0x6ED9EBA1U;
                }
                else if (index < 60U) {
                    function = (b & c) | (b & d) | (c & d);
                    constant = 0x8F1BBCDCU;
                }
                else {
                    function = b ^ c ^ d;
                    constant = 0xCA62C1D6U;
                }
                const std::uint32_t temporary = rotate_left(a, 5U) + function + e + constant + words[index];
                e = d;
                d = c;
                c = rotate_left(b, 30U);
                b = a;
                a = temporary;
            }
            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }

        const std::array<std::uint32_t, 5> words{h0, h1, h2, h3, h4};
        std::array<std::uint8_t, 20> digest{};
        for (std::size_t index = 0; index < words.size(); ++index) {
            digest[index * 4U] = static_cast<std::uint8_t>(words[index] >> 24U);
            digest[index * 4U + 1U] = static_cast<std::uint8_t>(words[index] >> 16U);
            digest[index * 4U + 2U] = static_cast<std::uint8_t>(words[index] >> 8U);
            digest[index * 4U + 3U] = static_cast<std::uint8_t>(words[index]);
        }
        return digest;
    }

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
        const std::size_t begin = request.find(label);
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
}

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
    if (key.empty()) {
        close_client();
        return;
    }
    const std::string material = std::string{key} + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    const std::array<std::uint8_t, 20> digest = sha1(material);
    const std::string response = std::format(
        "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: {}\r\n\r\n",
        base64(digest));
    const ssize_t sent = ::send(client_fd_, response.data(), response.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
    if (sent != static_cast<ssize_t>(response.size())) {
        close_client();
        return;
    }
    handshake_complete_ = true;
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
