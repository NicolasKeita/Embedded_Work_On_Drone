/*
Filename: Src/embedded/transport/parser/hil_protocol_parser.cppm
Description: HIL-Proto v1.0 receive-side primitives: CRC-16-CCITT (poly 0x1021,
init 0xFFFF) and the heap-free byte-by-byte HilFrameParser FSM assembling
Header + Payload + CRC frames. Implementations live in the
hil_protocol_parser-*.cpp translation units.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module flight.transport.protocol.parser;

import std;

import flight.transport.protocol;

export namespace FlightCore::Transport
{

class HilCrc
{
public:
    HilCrc() = delete;

    /* CRC-16-CCITT (poly 0x1021, init 0xFFFF) over a single buffer. */
    [[nodiscard]] static constexpr std::uint16_t compute(std::span<const std::uint8_t> data) noexcept
    {
        return update(0xFFFFu, data);
    }

    /* Continues an in-progress CRC with additional bytes (Header then Payload). */
    [[nodiscard]] static std::uint16_t update(std::uint16_t crc, std::span<const std::uint8_t> data) noexcept;
};

class HilFrameParser
{
public:
    enum class State : std::uint8_t
    {
        WaitSync1,
        WaitSync2,
        ReadHeader,
        ReadPayload,
        ReadCrc
    };

    HilFrameParser() = default;

    [[nodiscard]] State state() const noexcept { return state_; }

    [[nodiscard]] std::uint64_t rejectedFrames() const noexcept { return rejected_; }

    /* Returns the parser to the sync-hunt state without dropping the rejection counter. */
    void reset() noexcept;

    /*
        Feeds one byte; returns true when a complete, CRC-valid frame has been
        assembled, copying its header into out_header and its payload bytes into
        out_payload (which must hold at least header.payload_len bytes).
    */
    [[nodiscard]] bool processByte(std::uint8_t byte, HilHeader& out_header, std::span<std::uint8_t> out_payload);

private:
    /* Sync-hunt and header accumulation steps of the receive FSM. */
    void handleSync1(std::uint8_t byte) noexcept;
    void handleSync2(std::uint8_t byte) noexcept;
    [[nodiscard]] bool handleHeaderByte(std::uint8_t byte) noexcept;

    /* Payload/CRC accumulation and final frame validation steps of the FSM. */
    void handlePayloadByte(std::uint8_t byte) noexcept;
    [[nodiscard]] bool handleCrcByte(std::uint8_t byte, HilHeader& out_header,
                                     std::span<std::uint8_t> out_payload);
    [[nodiscard]] bool finalizeFrame(HilHeader& out_header, std::span<std::uint8_t> out_payload) noexcept;

    State                                 state_{State::WaitSync1};
    HilHeader                             header_{};
    std::array<std::uint8_t, kHeaderSize> header_bytes_{};
    std::array<std::uint8_t, kMaxPayload> payload_buffer_{};
    std::array<std::uint8_t, kCrcSize>    crc_bytes_{};
    std::size_t                           index_{0};
    std::uint64_t                         rejected_{0};
};

}
