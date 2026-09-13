/*
Filename: Src/Embedded/InterFc/InterFcLink-Codec.cpp
Description: Encoding of one logical FC-to-FC message into the shared UART framing.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module InterFcLink;

import std;

namespace FlightCore::InterFc {

/* Encodes one logical message into the UART framing shared by both controllers. */
FrameCodec::Frame FrameCodec::encode(const Message& message) noexcept
{
    const std::uint32_t altitude_bits = std::bit_cast<std::uint32_t>(message.altitude_m);
    const std::uint32_t actual_rpm_bits = std::bit_cast<std::uint32_t>(message.actual_rpm);
    const std::uint32_t commanded_rpm_bits = std::bit_cast<std::uint32_t>(message.commanded_rpm);
    Frame               frame{
        kSyncFirst,
        kSyncSecond,
        kProtocolVersion,
        static_cast<std::uint8_t>(message.kind),
        static_cast<std::uint8_t>(message.sequence & 0xFFu),
        static_cast<std::uint8_t>((message.sequence >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(message.state),
        static_cast<std::uint8_t>(message.detection),
        static_cast<std::uint8_t>(altitude_bits & 0xFFu),
        static_cast<std::uint8_t>((altitude_bits >> 8u) & 0xFFu),
        static_cast<std::uint8_t>((altitude_bits >> 16u) & 0xFFu),
        static_cast<std::uint8_t>((altitude_bits >> 24u) & 0xFFu),
        static_cast<std::uint8_t>(actual_rpm_bits & 0xFFu),
        static_cast<std::uint8_t>((actual_rpm_bits >> 8u) & 0xFFu),
        static_cast<std::uint8_t>((actual_rpm_bits >> 16u) & 0xFFu),
        static_cast<std::uint8_t>((actual_rpm_bits >> 24u) & 0xFFu),
        static_cast<std::uint8_t>(commanded_rpm_bits & 0xFFu),
        static_cast<std::uint8_t>((commanded_rpm_bits >> 8u) & 0xFFu),
        static_cast<std::uint8_t>((commanded_rpm_bits >> 16u) & 0xFFu),
        static_cast<std::uint8_t>((commanded_rpm_bits >> 24u) & 0xFFu),
        0,
        0, };
    const std::uint16_t crc = frame_crc(std::span<const std::uint8_t>{frame.data(), 20});

    frame[20] = static_cast<std::uint8_t>(crc & 0xFFu);
    frame[21] = static_cast<std::uint8_t>((crc >> 8u) & 0xFFu);
    return frame;
}

}