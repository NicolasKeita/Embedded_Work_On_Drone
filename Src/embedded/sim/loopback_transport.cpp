/*
Filename: Src/embedded/sim/loopback_transport.cpp
Description: Implementation of the fixed-size ring-buffer loopback transport
(flight.sim.loopback) exercising the HIL-Proto framing layer without a real
UART/USB CDC port.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module flight.sim.loopback;

import std;

namespace FlightCore::Sim
{

bool LoopbackTransport::sendBytes(std::span<const std::uint8_t> data) noexcept
{
    if (data.size() > kCapacity - count_) {
        return false;
    }
    for (std::uint8_t value : data) {
        buffer_[head_] = value;
        head_ = (head_ + 1u) % kCapacity;
        ++count_;
    }
    return true;
}

std::size_t LoopbackTransport::receiveBytes(std::span<std::uint8_t> buffer) noexcept
{
    const std::size_t n = (buffer.size() < count_) ? buffer.size() : count_;

    for (std::size_t i = 0; i < n; ++i) {
        buffer[i] = buffer_[tail_];
        tail_ = (tail_ + 1u) % kCapacity;
        --count_;
    }
    return n;
}

void LoopbackTransport::flush() noexcept
{
    head_ = 0;
    tail_ = 0;
    count_ = 0;
}

}
