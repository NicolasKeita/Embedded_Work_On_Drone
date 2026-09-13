/*
Filename: Src/Embedded/InterFc/InterFcLink-Parser.cpp
Description: Validation and byte-by-byte parsing of FC-to-FC heartbeat frames.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module InterFcLink;

import std;

namespace FlightCore::InterFc {

namespace {

/* Reads the little-endian 16-bit field starting at the given frame offset. */
[[nodiscard]] std::uint16_t read_u16(const FrameCodec::Frame& frame, std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(frame[offset])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(frame[offset + 1u]) << 8u);
}

/* Reads the little-endian 32-bit field starting at the given frame offset. */
[[nodiscard]] std::uint32_t read_u32(const FrameCodec::Frame& frame, std::size_t offset) noexcept
{
    return static_cast<std::uint32_t>(frame[offset]) | static_cast<std::uint32_t>(frame[offset + 1u]) << 8u
        | static_cast<std::uint32_t>(frame[offset + 2u]) << 16u
        | static_cast<std::uint32_t>(frame[offset + 3u]) << 24u;
}

/* Checks the fixed synchronization, version and checksum fields of a frame. */
[[nodiscard]] bool valid_header(const FrameCodec::Frame& frame) noexcept
{
    if (frame[0] != kSyncFirst || frame[1] != kSyncSecond || frame[2] != kProtocolVersion) {
        return false;
    }
    return frame_crc(std::span<const std::uint8_t>{frame.data(), 20}) == read_u16(frame, 20);
}

/* Checks the enumerated kind, state and detection fields of a frame. */
[[nodiscard]] bool valid_enums(const FrameCodec::Frame& frame) noexcept
{
    const MessageKind   kind = static_cast<MessageKind>(frame[3]);
    const NodeState     state = static_cast<NodeState>(frame[6]);
    const DetectionCode detection = static_cast<DetectionCode>(frame[7]);
    const bool          kind_ok = kind == MessageKind::Heartbeat || kind == MessageKind::Acknowledgement
        || kind == MessageKind::Status || kind == MessageKind::MonitoringSample
        || kind == MessageKind::ResetSupervision;
    const bool state_ok = state == NodeState::Unknown || state == NodeState::Healthy
        || state == NodeState::Degraded || state == NodeState::Safe;
    const bool detection_ok = detection == DetectionCode::None || detection == DetectionCode::Fc1HeartbeatTimeout
        || detection == DetectionCode::CommunicationTimeout
        || detection == DetectionCode::SensorValidationFailed || detection == DetectionCode::ActuatorMismatch;

    return kind_ok && state_ok && detection_ok;
}

/* Decodes a fully buffered frame after checking its fixed fields and checksum. */
[[nodiscard]] std::optional<Message> decode_frame(const FrameCodec::Frame& frame) noexcept
{
    if (!valid_header(frame) || !valid_enums(frame)) {
        return std::nullopt;
    }
    return Message{
        .kind = static_cast<MessageKind>(frame[3]),
        .sequence = read_u16(frame, 4),
        .state = static_cast<NodeState>(frame[6]),
        .detection = static_cast<DetectionCode>(frame[7]),
        .altitude_m = std::bit_cast<std::float32_t>(read_u32(frame, 8)),
        .actual_rpm = std::bit_cast<std::float32_t>(read_u32(frame, 12)),
        .commanded_rpm = std::bit_cast<std::float32_t>(read_u32(frame, 16)),
    };
}

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
