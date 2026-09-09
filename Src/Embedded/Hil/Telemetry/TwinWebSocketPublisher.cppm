/*
Filename: Src/Embedded/Hil/Telemetry/TwinWebSocketPublisher.cppm
Description: Non-blocking localhost WebSocket publisher for Digital Twin snapshots.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module TwinWebSocketPublisher;

import std;

export namespace sim::hil {

class TwinWebSocketPublisher {
public:
    explicit TwinWebSocketPublisher(std::uint16_t port = 8765) noexcept;
    ~TwinWebSocketPublisher();

    TwinWebSocketPublisher(const TwinWebSocketPublisher&) = delete;
    TwinWebSocketPublisher& operator=(const TwinWebSocketPublisher&) = delete;

    /* Publishes one UTF-8 snapshot without waiting for a browser or socket capacity. */
    void publish(std::string_view snapshot) noexcept;

private:
    void accept_client() noexcept;
    void complete_handshake() noexcept;
    void close_client() noexcept;

    int                           server_fd_ = -1;
    int                           client_fd_ = -1;
    bool                          handshake_complete_ = false;
    std::array<char, 4096>        request_{};
    std::size_t                   request_size_ = 0;
    std::array<std::uint8_t, 65536> frame_{};
};

}
