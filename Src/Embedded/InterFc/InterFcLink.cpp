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
    const std::uint16_t received_crc = static_cast<std::uint16_t>(frame[20])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(frame[21]) << 8u);
    if (frame_crc(std::span<const std::uint8_t>{frame.data(), 20}) != received_crc) {
        return std::nullopt;
    }
    const MessageKind kind = static_cast<MessageKind>(frame[3]);
    if (kind != MessageKind::Heartbeat && kind != MessageKind::Acknowledgement
        && kind != MessageKind::Status && kind != MessageKind::MonitoringSample
        && kind != MessageKind::ResetSupervision) {
        return std::nullopt;
    }
    const std::uint16_t sequence = static_cast<std::uint16_t>(frame[4])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(frame[5]) << 8u);
    const NodeState state = static_cast<NodeState>(frame[6]);
    if (state != NodeState::Unknown && state != NodeState::Healthy
        && state != NodeState::Degraded && state != NodeState::Safe) {
        return std::nullopt;
    }
    const DetectionCode detection = static_cast<DetectionCode>(frame[7]);
    if (detection != DetectionCode::None && detection != DetectionCode::Fc1HeartbeatTimeout
        && detection != DetectionCode::CommunicationTimeout
        && detection != DetectionCode::SensorValidationFailed
        && detection != DetectionCode::ActuatorMismatch) {
        return std::nullopt;
    }
    const std::uint32_t altitude_bits = static_cast<std::uint32_t>(frame[8])
        | static_cast<std::uint32_t>(frame[9]) << 8u
        | static_cast<std::uint32_t>(frame[10]) << 16u
        | static_cast<std::uint32_t>(frame[11]) << 24u;
    const std::uint32_t actual_rpm_bits = static_cast<std::uint32_t>(frame[12])
        | static_cast<std::uint32_t>(frame[13]) << 8u
        | static_cast<std::uint32_t>(frame[14]) << 16u
        | static_cast<std::uint32_t>(frame[15]) << 24u;
    const std::uint32_t commanded_rpm_bits = static_cast<std::uint32_t>(frame[16])
        | static_cast<std::uint32_t>(frame[17]) << 8u
        | static_cast<std::uint32_t>(frame[18]) << 16u
        | static_cast<std::uint32_t>(frame[19]) << 24u;
    return Message{
        .kind = kind,
        .sequence = sequence,
        .state = state,
        .detection = detection,
        .altitude_m = std::bit_cast<std::float32_t>(altitude_bits),
        .actual_rpm = std::bit_cast<std::float32_t>(actual_rpm_bits),
        .commanded_rpm = std::bit_cast<std::float32_t>(commanded_rpm_bits),
    };
}

}

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
