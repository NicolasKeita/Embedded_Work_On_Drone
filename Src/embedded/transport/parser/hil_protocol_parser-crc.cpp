/*
Filename: Src/embedded/transport/parser/hil_protocol_parser-crc.cpp
Description: CRC-16-CCITT (poly 0x1021, init 0xFFFF) bitwise implementation used
by the HIL-Proto v1.0 frame parser and frame encoders.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module flight.transport.protocol.parser;

import std;

namespace FlightCore::Transport
{

std::uint16_t HilCrc::update(std::uint16_t crc, std::span<const std::uint8_t> data) noexcept
{
    std::uint16_t local = crc;

    for (std::uint8_t value : data) {
        local ^= static_cast<std::uint16_t>(value) << 8;
        for (std::uint8_t bit = 0; bit < 8; ++bit) {
            const std::uint16_t mask = (local & 0x8000u) != 0u ? 0x1021u : 0x0000u;
            local = static_cast<std::uint16_t>((local << 1) ^ mask);
        }
    }
    return local;
}

}
