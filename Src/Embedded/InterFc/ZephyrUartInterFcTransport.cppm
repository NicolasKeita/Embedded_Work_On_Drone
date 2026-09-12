/*
Filename: Src/Embedded/InterFc/ZephyrUartInterFcTransport.cppm
Description: Zephyr USART adapter for the transport-independent FC-to-FC protocol.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/device.h>

export module ZephyrUartInterFcTransport;

import std;

import InterFcLink;

export namespace FlightCore::InterFc {

class ZephyrUartInterFcTransport final : public IInterFcTransport {
public:
    static constexpr std::uint32_t receive_capacity = 64;

    explicit ZephyrUartInterFcTransport(const device* uart) noexcept;

    /* Reports whether the configured Zephyr UART is available for communication. */
    [[nodiscard]] bool ready() const noexcept;

    /* Sends one framed message with deterministic polled UART writes. */
    [[nodiscard]] std::expected<void, TransportError> send(const Message& message) noexcept override;

    /* Polls UART bytes until one valid protocol message is available. */
    [[nodiscard]] std::expected<std::optional<Message>, TransportError> poll() noexcept override;

    /* Returns the number of raw UART bytes consumed since construction. */
    [[nodiscard]] std::uint32_t received_byte_count() const noexcept;

    /* Returns the number of CRC-valid protocol frames received since construction. */
    [[nodiscard]] std::uint32_t valid_frame_count() const noexcept;

private:
    /* Moves hardware FIFO bytes into the instance's fixed receive buffer. */
    static void receive_uart_bytes(const device* uart, void* user_data) noexcept;

    /* Appends one interrupt-received byte unless the fixed buffer is full. */
    void store_received_byte(std::uint8_t byte) noexcept;

    const device* uart_ = nullptr;
    FrameParser parser_{};
    std::array<std::uint8_t, receive_capacity> receive_buffer_{};
    volatile std::uint32_t receive_head_ = 0;
    volatile std::uint32_t receive_tail_ = 0;
    std::uint32_t received_byte_count_ = 0;
    std::uint32_t valid_frame_count_ = 0;
    bool initialized_ = false;
};

}
