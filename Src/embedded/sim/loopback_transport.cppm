/*
Filename: Src/embedded/sim/loopback_transport.cppm
Description: Fixed-size ring-buffer loopback transport for host-side unit tests.
It interconnects the simulator and the HIL-Proto parser locally with zero heap
allocation and deterministic FIFO ordering, exercising the framing layer without a
real UART/USB CDC port.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module flight.sim.loopback;

import std;

import flight.transport;

export namespace FlightCore::Sim
{

class LoopbackTransport final : public FlightCore::Transport::ITransport
{
public:
    static constexpr std::size_t kCapacity = 512;

    LoopbackTransport() = default;

    [[nodiscard]] bool sendBytes(std::span<const std::uint8_t> data) noexcept override;

    [[nodiscard]] std::size_t receiveBytes(std::span<std::uint8_t> buffer) noexcept override;

    [[nodiscard]] std::size_t bytesAvailable() const noexcept override;

    void flush() noexcept override;

private:
    std::array<std::uint8_t, kCapacity> buffer_{};
    std::size_t head_{0};
    std::size_t tail_{0};
    std::size_t count_{0};
};

/* Appends the whole frame or rejects it atomically (no partial writes). */
inline bool LoopbackTransport::sendBytes(std::span<const std::uint8_t> data) noexcept
{
    if (data.size() > kCapacity - count_) return false;
    for (std::uint8_t value : data)
    {
        buffer_[head_] = value;
        head_ = (head_ + 1u) % kCapacity;
        ++count_;
    }
    return true;
}

inline std::size_t LoopbackTransport::receiveBytes(std::span<std::uint8_t> buffer) noexcept
{
    const std::size_t n = (buffer.size() < count_) ? buffer.size() : count_;
    for (std::size_t i = 0; i < n; ++i)
    {
        buffer[i] = buffer_[tail_];
        tail_ = (tail_ + 1u) % kCapacity;
        --count_;
    }
    return n;
}

inline std::size_t LoopbackTransport::bytesAvailable() const noexcept
{
    return count_;
}

inline void LoopbackTransport::flush() noexcept
{
    head_ = 0;
    tail_ = 0;
    count_ = 0;
}

}
