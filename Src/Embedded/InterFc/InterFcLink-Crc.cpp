/*
Filename: Src/Embedded/InterFc/InterFcLink-Crc.cpp
Description: CRC-16/CCITT-FALSE checksum shared by the FC-to-FC frame codec.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module InterFcLink;

import std;

namespace FlightCore::InterFc {

/* Computes the CRC-16/CCITT-FALSE checksum used by an FC-to-FC frame. */
std::uint16_t frame_crc(std::span<const std::uint8_t> bytes) noexcept
{
    std::uint16_t crc = 0xFFFF;

    for (const std::uint8_t byte : bytes) {
        crc ^= static_cast<std::uint16_t>(byte) << 8u;
        for (std::uint8_t bit = 0; bit < 8; ++bit) {
            const bool high_bit_set = (crc & 0x8000u) != 0;
            crc = static_cast<std::uint16_t>(crc << 1u);
            if (high_bit_set) {
                crc ^= 0x1021u;
            }
        }
    }
    return crc;
}

}
