/*
Filename: Src/Embedded/Transport/Transport.cppm
Description: Transport abstractions sitting below the HIL-Proto v1.0 framer.
ITransport is the span-based byte channel used by both the PC loopback mock and
the future STM32 UART driver; ITransportStream mirrors the raw stream view named
in the protocol specification, modernised to std::span (no raw pointers).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Transport;

import std;

export namespace FlightCore::Transport
{

/*
    Byte-wide full-duplex transport. sendBytes returns false when the channel
    cannot accept the whole frame; receiveBytes returns the number of bytes
    actually available and copied (never blocks).
*/
class ITransport
{
public:
    ITransport() = default;
    virtual ~ITransport() = default;

    ITransport(const ITransport&)            = delete;
    ITransport& operator=(const ITransport&) = delete;

    [[nodiscard]] virtual bool sendBytes(std::span<const std::uint8_t> data) noexcept = 0;

    [[nodiscard]] virtual std::size_t receiveBytes(std::span<std::uint8_t> buffer) noexcept = 0;

    [[nodiscard]] virtual std::size_t bytesAvailable() const noexcept = 0;

    virtual void flush() noexcept {}
};

/*
    Streaming view referenced by the HIL-Proto v1.0 parser section, expressed with
    std::span for ownership safety. Reserved for the physical STM32 UART driver.
*/
class ITransportStream
{
public:
    ITransportStream() = default;
    virtual ~ITransportStream() = default;

    ITransportStream(const ITransportStream&)            = delete;
    ITransportStream& operator=(const ITransportStream&) = delete;

    [[nodiscard]] virtual bool write(std::span<const std::uint8_t> data) noexcept = 0;

    [[nodiscard]] virtual std::size_t read(std::span<std::uint8_t> buffer) noexcept = 0;

    [[nodiscard]] virtual std::size_t bytesAvailable() const noexcept = 0;
};

}
