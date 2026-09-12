/*
Filename: Src/Embedded/InterFc/ZephyrUartInterFcTransport.cpp
Description: Zephyr polled USART implementation of the FC-to-FC transport interface.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

module ZephyrUartInterFcTransport;

import std;

import InterFcLink;

namespace FlightCore::InterFc {

/* Creates an FC-to-FC UART transport bound to a Zephyr device. */
ZephyrUartInterFcTransport::ZephyrUartInterFcTransport(const device* uart) noexcept
    : uart_{uart}
{
    if (uart_ != nullptr && device_is_ready(uart_)
        && uart_irq_callback_user_data_set(uart_, receive_uart_bytes, this) == 0) {
        uart_irq_rx_enable(uart_);
        initialized_ = true;
    }
}

/* Reports whether the configured Zephyr UART is available for communication. */
bool ZephyrUartInterFcTransport::ready() const noexcept
{
    return initialized_;
}

/* Moves hardware FIFO bytes into the instance's fixed receive buffer. */
void ZephyrUartInterFcTransport::receive_uart_bytes(const device* uart, void* user_data) noexcept
{
    auto*                        transport = static_cast<ZephyrUartInterFcTransport*>(user_data);
    std::array<std::uint8_t, 16> bytes{};

    while (uart_irq_update(uart) != 0 && uart_irq_is_pending(uart) != 0) {
        if (uart_irq_rx_ready(uart) == 0) {
            continue;
        }
        const std::int32_t received = uart_fifo_read(uart, bytes.data(), bytes.size());
        for (std::int32_t index = 0; index < received; ++index) {
            transport->store_received_byte(bytes[static_cast<std::size_t>(index)]);
        }
    }
}

/* Appends one interrupt-received byte unless the fixed buffer is full. */
void ZephyrUartInterFcTransport::store_received_byte(std::uint8_t byte) noexcept
{
    const std::uint32_t next_head = (receive_head_ + 1u) % receive_capacity;

    if (next_head == receive_tail_) {
        return;
    }
    receive_buffer_[receive_head_] = byte;
    receive_head_ = next_head;
}

/* Sends one framed message with deterministic polled UART writes. */
std::expected<void, TransportError> ZephyrUartInterFcTransport::send(const Message& message) noexcept
{
    if (!ready()) {
        return std::unexpected(TransportError::DeviceUnavailable);
    }
    const FrameCodec::Frame frame = FrameCodec::encode(message);
    for (const std::uint8_t byte : frame) {
        uart_poll_out(uart_, byte);
    }
    return {};
}

/* Polls UART bytes until one valid protocol message is available. */
std::expected<std::optional<Message>, TransportError> ZephyrUartInterFcTransport::poll() noexcept
{
    if (!ready()) {
        return std::unexpected(TransportError::DeviceUnavailable);
    }
    while (receive_tail_ != receive_head_) {
        const std::uint8_t byte = receive_buffer_[receive_tail_];
        receive_tail_ = (receive_tail_ + 1u) % receive_capacity;
        ++received_byte_count_;
        const std::optional<Message> message = parser_.process(byte);
        if (message.has_value()) {
            ++valid_frame_count_;
            return message;
        }
    }
    return std::optional<Message>{};
}

/* Returns the number of raw UART bytes consumed since construction. */
std::uint32_t ZephyrUartInterFcTransport::received_byte_count() const noexcept
{
    return received_byte_count_;
}

/* Returns the number of CRC-valid protocol frames received since construction. */
std::uint32_t ZephyrUartInterFcTransport::valid_frame_count() const noexcept
{
    return valid_frame_count_;
}

}
