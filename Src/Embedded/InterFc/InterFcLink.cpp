/*
Filename: Src/Embedded/InterFc/InterFcLink.cpp
Description: Encoding, validation, and parsing for the FC-to-FC heartbeat protocol.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module InterFcLink;

import std;

namespace FlightCore::InterFc {

namespace {

constexpr std::uint8_t kSyncFirst = 0xA5;
constexpr std::uint8_t kSyncSecond = 0x5A;
constexpr std::uint8_t kProtocolVersion = 1;

/* Computes the CRC-16/CCITT-FALSE checksum used by an FC-to-FC frame. */
[[nodiscard]] std::uint16_t frame_crc(std::span<const std::uint8_t> bytes) noexcept
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

/* Decodes a fully buffered frame after checking its fixed fields and checksum. */
[[nodiscard]] std::optional<Message> decode_frame(const FrameCodec::Frame& frame) noexcept
{
    if (frame[0] != kSyncFirst || frame[1] != kSyncSecond || frame[2] != kProtocolVersion) {
        return std::nullopt;
    }
    const std::uint16_t received_crc = static_cast<std::uint16_t>(frame[6])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(frame[7]) << 8u);
    if (frame_crc(std::span<const std::uint8_t>{frame.data(), 6}) != received_crc) {
        return std::nullopt;
    }
    const MessageKind kind = static_cast<MessageKind>(frame[3]);
    if (kind != MessageKind::Heartbeat && kind != MessageKind::Acknowledgement) {
        return std::nullopt;
    }
    const std::uint16_t sequence = static_cast<std::uint16_t>(frame[4])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(frame[5]) << 8u);
    return Message{.kind = kind, .sequence = sequence};
}

}

/* Encodes one logical message into the UART framing shared by both controllers. */
FrameCodec::Frame FrameCodec::encode(const Message& message) noexcept
{
    Frame               frame{
        kSyncFirst,
        kSyncSecond,
        kProtocolVersion,
        static_cast<std::uint8_t>(message.kind),
        static_cast<std::uint8_t>(message.sequence & 0xFFu),
        static_cast<std::uint8_t>((message.sequence >> 8u) & 0xFFu),
        0, 0, };
    const std::uint16_t crc = frame_crc(std::span<const std::uint8_t>{frame.data(), 6});

    frame[6] = static_cast<std::uint8_t>(crc & 0xFFu);
    frame[7] = static_cast<std::uint8_t>((crc >> 8u) & 0xFFu);
    return frame;
}

/* Consumes one byte and returns a message when a valid frame is complete. */
std::optional<Message> FrameParser::process(std::uint8_t byte) noexcept
{
    if (size_ == 0 && byte != kSyncFirst) {
        return std::nullopt;
    }
    if (size_ == 1 && byte != kSyncSecond) {
        size_ = byte == kSyncFirst ? 1 : 0;
        return std::nullopt;
    }
    frame_[size_] = byte;
    ++size_;
    if (size_ < frame_.size()) {
        return std::nullopt;
    }
    size_ = 0;
    return decode_frame(frame_);
}

}
