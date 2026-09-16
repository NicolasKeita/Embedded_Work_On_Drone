/*
Filename: Src/Embedded/Transport/Codec/HilProtocolCodec-Sensor.cpp
Description: SensorPacket side of the HIL-Proto v1.1 conversion layer: builds
and decodes the packed HilSensorPayload and serializes the full SensorPacket
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

/* Combines sensor measurements with the runner setpoint configuration. */
HilSensorPayload makeSensorPayload(const FlightCore::HAL::SensorData& sensor,
                                    const HilControlSetpoint& setpoint) noexcept
{
    return HilSensorPayload{
        .sim_timestamp_us   = sensor.timestamp_us,
        .position_x_m       = sensor.position_x_m,
        .position_y_m       = sensor.position_y_m,
        .position_z_m       = sensor.position_z_m,
        .velocity_x_ms      = sensor.velocity_x_ms,
        .velocity_y_ms      = sensor.velocity_y_ms,
        .velocity_z_ms      = sensor.velocity_z_ms,
        .gyro_p_rad_s       = sensor.gyro_roll_rad_s,
        .gyro_q_rad_s       = sensor.gyro_pitch_rad_s,
        .gyro_r_rad_s       = sensor.gyro_yaw_rad_s,
        .accel_x_m_s2       = sensor.accel_x_m_s2,
        .accel_y_m_s2       = sensor.accel_y_m_s2,
        .accel_z_m_s2       = sensor.accel_z_m_s2,
        .roll_rad           = sensor.roll_rad,
        .pitch_rad          = sensor.pitch_rad,
        .yaw_rad            = sensor.yaw_rad,
        .altitude_baro_m    = sensor.altitude_baro_m,
        .wing_rpm_meas      = sensor.wing_rpm_meas,
        .sensor_valid_flags = sensor.sensor_valid_flags,
        .setpoint = setpoint,
    };
}

/* Rejects malformed payloads and non-finite or negative setpoint altitude/hold values. */
bool decodeSensorPayload(std::span<const std::uint8_t> bytes, HilSensorPayload& out) noexcept
{
    if (bytes.size() != sizeof(HilSensorPayload)) {
        return false;
    }
    std::memcpy(&out, bytes.data(), sizeof(HilSensorPayload));
    return std::isfinite(out.setpoint.target_x_m)
        && std::isfinite(out.setpoint.target_y_m)
        && std::isfinite(out.setpoint.target_z_m)
        && out.setpoint.target_z_m >= 0.0f
        && std::isfinite(out.setpoint.station_hold_seconds)
        && out.setpoint.station_hold_seconds >= 0.0f;
}

FlightCore::HAL::SensorData toSensorData(const HilSensorPayload& payload) noexcept
{
    return FlightCore::HAL::SensorData{
        .timestamp_us       = payload.sim_timestamp_us,
        .position_x_m       = payload.position_x_m,
        .position_y_m       = payload.position_y_m,
        .position_z_m       = payload.position_z_m,
        .velocity_x_ms      = payload.velocity_x_ms,
        .velocity_y_ms      = payload.velocity_y_ms,
        .velocity_z_ms      = payload.velocity_z_ms,
        .gyro_roll_rad_s    = payload.gyro_p_rad_s,
        .gyro_pitch_rad_s   = payload.gyro_q_rad_s,
        .gyro_yaw_rad_s     = payload.gyro_r_rad_s,
        .accel_x_m_s2       = payload.accel_x_m_s2,
        .accel_y_m_s2       = payload.accel_y_m_s2,
        .accel_z_m_s2       = payload.accel_z_m_s2,
        .roll_rad           = payload.roll_rad,
        .pitch_rad          = payload.pitch_rad,
        .yaw_rad            = payload.yaw_rad,
        .altitude_baro_m    = payload.altitude_baro_m,
        .wing_rpm_meas      = payload.wing_rpm_meas,
        .sensor_valid_flags = payload.sensor_valid_flags,
    };
}

std::array<std::uint8_t, kSensorFrameSize> encodeSensorFrame(const HilSensorPayload& payload,
                                                             std::uint16_t seq) noexcept
{
    HilHeader                                  header{
        .sync_byte_1  = kSync1,
        .sync_byte_2  = kSync2,
        .msg_id       = kMsgIdSensor,
        .protocol_ver = kProtocolVer,
        .sequence_num = seq,
        .payload_len  = static_cast<std::uint16_t>(kSensorPayloadSize),
    };
    std::array<std::uint8_t, kSensorFrameSize> frame{};

    std::memcpy(frame.data(), &header, kHeaderSize);
    std::memcpy(frame.data() + kHeaderSize, &payload, kSensorPayloadSize);

    const std::span<const std::uint8_t> crc_input(frame.data(), kHeaderSize + kSensorPayloadSize);
    const std::uint16_t crc = HilCrc::compute(crc_input);
    frame[kSensorFrameSize - 2] = static_cast<std::uint8_t>(crc & 0x00FFu);
    frame[kSensorFrameSize - 1] = static_cast<std::uint8_t>((crc >> 8) & 0x00FFu);
    return frame;
}

}
