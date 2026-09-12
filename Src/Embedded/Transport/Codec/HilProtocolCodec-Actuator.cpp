/*
Filename: Src/Embedded/Transport/Codec/HilProtocolCodec-Actuator.cpp
Description: ActuatorPacket side of the HIL-Proto v1.0 conversion layer: builds
and decodes the packed HilActuatorPayload and serializes the full ActuatorPacket
frame (Header + Payload + CRC-16).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilProtocolCodec;

import std;

import HalTypes;
import HilProtocol;
import HilProtocolParser;

namespace FlightCore::Transport
{

HilActuatorPayload makeActuatorPayload(const FlightCore::HAL::ActuatorCommands& cmds,
                                       std::uint64_t                            echo_sim_timestamp_us,
                                       const ActuatorDiagnostics&               diagnostics)
{
    return HilActuatorPayload{
        .fc_timestamp_us        = cmds.timestamp_us,
        .echo_sim_timestamp_us  = echo_sim_timestamp_us,
        .wing_rpm_cmd           = cmds.wing_rpm_cmd,
        .left_servo_cmd_rad     = cmds.left_servo_rad,
        .right_servo_cmd_rad    = cmds.right_servo_rad,
        .aux_actuator_cmd       = cmds.aux_actuator_cmd,
        .cpu_usage_pct_x100     = diagnostics.cpu_usage_pct_x100,
        .stack_watermark_words  = diagnostics.stack_watermark_words,
        .deadline_miss_count    = diagnostics.deadline_miss_count,
        .fc_mode                = cmds.mode_flags,
        .fc_health_status       = diagnostics.fc_health_status,
        .fc_detection_code      = diagnostics.fc_detection_code,
        .reserved               = 0,
    };
}

bool decodeActuatorPayload(std::span<const std::uint8_t> bytes, HilActuatorPayload& out) noexcept
{
    if (bytes.size() < sizeof(HilActuatorPayload)) {
        return false;
    }
    std::memcpy(&out, bytes.data(), sizeof(HilActuatorPayload));
    return true;
}

FlightCore::HAL::ActuatorCommands toActuatorCommands(const HilActuatorPayload& payload) noexcept
{
    return FlightCore::HAL::ActuatorCommands{
        .timestamp_us     = payload.fc_timestamp_us,
        .wing_rpm_cmd     = payload.wing_rpm_cmd,
        .left_servo_rad   = payload.left_servo_cmd_rad,
        .right_servo_rad  = payload.right_servo_cmd_rad,
        .aux_actuator_cmd = payload.aux_actuator_cmd,
        .mode_flags       = payload.fc_mode,
    };
}

std::array<std::uint8_t, kActuatorFrameSize> encodeActuatorFrame(const HilActuatorPayload& payload,
                                                                 std::uint16_t seq) noexcept
{
    HilHeader                                    header{
        .sync_byte_1  = kSync1,
        .sync_byte_2  = kSync2,
        .msg_id       = kMsgIdActuator,
        .protocol_ver = kProtocolVer,
        .sequence_num = seq,
        .payload_len  = static_cast<std::uint16_t>(kActuatorPayloadSize),
    };
    std::array<std::uint8_t, kActuatorFrameSize> frame{};

    std::memcpy(frame.data(), &header, kHeaderSize);
    std::memcpy(frame.data() + kHeaderSize, &payload, kActuatorPayloadSize);

    const std::span<const std::uint8_t> crc_input(frame.data(), kHeaderSize + kActuatorPayloadSize);
    const std::uint16_t crc = HilCrc::compute(crc_input);
    frame[kActuatorFrameSize - 2] = static_cast<std::uint8_t>(crc & 0x00FFu);
    frame[kActuatorFrameSize - 1] = static_cast<std::uint8_t>((crc >> 8) & 0x00FFu);
    return frame;
}

}
