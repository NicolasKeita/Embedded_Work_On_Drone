/*
Filename: Src/embedded/transport/parser/hil_protocol_parser-frame.cpp
Description: Payload/CRC accumulation, frame validation and top-level dispatch of
the HIL-Proto v1.0 HilFrameParser receive FSM (flight.transport.protocol.parser).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module flight.transport.protocol.parser;

import std;

namespace FlightCore::Transport
{

void HilFrameParser::handlePayloadByte(std::uint8_t byte) noexcept
{
    payload_buffer_[index_] = byte;
    ++index_;
    if (index_ == header_.payload_len) {
        index_ = 0;
        state_ = State::ReadCrc;
    }
}

bool HilFrameParser::handleCrcByte(std::uint8_t            byte,
                                   HilHeader&              out_header,
                                   std::span<std::uint8_t> out_payload)
{
    crc_bytes_[index_] = byte;
    ++index_;
    if (index_ == kCrcSize) {
        const bool ok = finalizeFrame(out_header, out_payload);
        reset();
        return ok;
    }
    return false;
}

bool HilFrameParser::finalizeFrame(HilHeader& out_header, std::span<std::uint8_t> out_payload) noexcept
{
    const std::uint16_t received = static_cast<std::uint16_t>(crc_bytes_[0])
                                 | (static_cast<std::uint16_t>(crc_bytes_[1]) << 8);
    std::uint16_t computed = HilCrc::compute(std::span<const std::uint8_t>(header_bytes_.data(), kHeaderSize));

    computed = HilCrc::update(computed, std::span<const std::uint8_t>(payload_buffer_.data(), header_.payload_len));

    if (computed != received || out_payload.size() < header_.payload_len) {
        ++rejected_;
        return false;
    }

    std::copy(payload_buffer_.data(), payload_buffer_.data() + header_.payload_len, out_payload.data());
    out_header = header_;
    return true;
}

void HilFrameParser::reset() noexcept
{
    state_ = State::WaitSync1;
    index_ = 0;
}

bool HilFrameParser::processByte(std::uint8_t            byte,
                                 HilHeader&              out_header,
                                 std::span<std::uint8_t> out_payload)
{
    switch (state_) {
    case State::WaitSync1:
        handleSync1(byte);
        return false;

    case State::WaitSync2:
        handleSync2(byte);
        return false;

    case State::ReadHeader:
        return handleHeaderByte(byte);

    case State::ReadPayload:
        handlePayloadByte(byte);
        return false;

    case State::ReadCrc:
        return handleCrcByte(byte, out_header, out_payload);
    }

    reset();
    return false;
}

}
