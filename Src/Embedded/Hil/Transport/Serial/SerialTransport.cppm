/*
Filename: Src/Embedded/Hil/Transport/Serial/SerialTransport.cppm
Description: Non-blocking Linux serial byte transport for a physical HIL target.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SerialTransport;

import std;

import Transport;

export namespace sim::hil {

class SerialTransport final : public FlightCore::Transport::ITransport {
public:
    ~SerialTransport() override;

    SerialTransport(const SerialTransport&) = delete;
    SerialTransport& operator=(const SerialTransport&) = delete;

    using OpenResult = std::expected<std::unique_ptr<SerialTransport>, std::errc>;

    /* Opens and configures a Linux serial device in raw, non-blocking mode. */
    [[nodiscard]] static OpenResult openPort(std::string_view device_path) noexcept;

    [[nodiscard]] bool sendBytes(std::span<const std::uint8_t> data) noexcept override;
    [[nodiscard]] std::size_t receiveBytes(std::span<std::uint8_t> buffer) noexcept override;
    [[nodiscard]] std::size_t bytesAvailable() const noexcept override;
    void flush() noexcept override;

private:
    explicit SerialTransport(std::int32_t descriptor) noexcept;

    std::int32_t descriptor_ = -1;
};

}
