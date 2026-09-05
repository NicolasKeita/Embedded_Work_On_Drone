/*
Filename: Src/embedded/transport/parser/hil_protocol_parser-sync.cpp
Description: Sync-hunt and header accumulation steps of the HIL-Proto v1.0
HilFrameParser receive FSM (flight.transport.protocol.parser).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module flight.transport.protocol.parser;

import std;

namespace FlightCore::Transport
{

void HilFrameParser::handleSync1(std::uint8_t byte) noexcept
{
    if (byte == kSync1) {
        state_ = State::WaitSync2;
    }
}

void HilFrameParser::handleSync2(std::uint8_t byte) noexcept
{
    if (byte == kSync2) {
        header_bytes_[0] = kSync1;
        header_bytes_[1] = kSync2;
        index_ = 2;
        state_ = State::ReadHeader;
    }
    else if (byte == kSync1) {
        state_ = State::WaitSync2;
    }
    else {
        state_ = State::WaitSync1;
    }
}

bool HilFrameParser::handleHeaderByte(std::uint8_t byte) noexcept
{
    header_bytes_[index_] = byte;
    ++index_;
    if (index_ != kHeaderSize) {
        return false;
    }

    std::memcpy(&header_, header_bytes_.data(), kHeaderSize);
    if (header_.payload_len > kMaxPayload) {
        ++rejected_;
        reset();
        return false;
    }
    index_ = 0;
    state_ = (header_.payload_len == 0u) ? State::ReadCrc : State::ReadPayload;
    return false;
}

}
