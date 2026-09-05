/*
Filename: Src/embedded/hal/hal_types.cppm
Description: Flight Controller HAL data structures exchanged between the control
core and platform-specific sensor/actuator/clock implementations (SensorData,
ActuatorCommands, sensor-validity flag bits). Fields mirror the HIL-Proto v1.0
SensorPacket/ActuatorPacket payload so the PC mock and the STM32 HIL transport
share a single wire contract (see flight.transport.protocol).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module flight.hal.types;

import std;

export namespace FlightCore::HAL
{

/* Bitmask positions for SensorData::sensor_valid_flags (HIL-Proto v1.0, sec. 4.1). */
inline constexpr std::uint32_t kFlagImu1Ok       = 1u << 0;
inline constexpr std::uint32_t kFlagImu2Ok       = 1u << 1;
inline constexpr std::uint32_t kFlagBaroOk       = 1u << 2;
inline constexpr std::uint32_t kFlagGpsFixOk     = 1u << 3;
inline constexpr std::uint32_t kFlagTachometerOk = 1u << 4;

/*
    Six-DOF sensor snapshot consumed by the Flight Controller. Each field maps
    one-to-one onto the HIL-Proto v1.0 SensorPacket payload, so bridging the wire
    format needs no rescaling. Units follow docs/hil/hil_protocol.md (m, m/s, rad).
*/
struct SensorData
{
    std::uint64_t timestamp_us;

    std::float32_t position_x_m;
    std::float32_t position_y_m;
    std::float32_t position_z_m;

    std::float32_t velocity_x_ms;
    std::float32_t velocity_y_ms;
    std::float32_t velocity_z_ms;

    std::float32_t gyro_roll_rad_s;
    std::float32_t gyro_pitch_rad_s;
    std::float32_t gyro_yaw_rad_s;

    std::float32_t accel_x_m_s2;
    std::float32_t accel_y_m_s2;
    std::float32_t accel_z_m_s2;

    std::float32_t roll_rad;
    std::float32_t pitch_rad;
    std::float32_t yaw_rad;

    std::float32_t altitude_baro_m;
    std::float32_t wing_rpm_meas;

    std::uint32_t sensor_valid_flags;
};

/*
    Actuator command set produced by the Flight Controller. mode_flags encodes the
    active flight mode and is echoed into HilActuatorPayload::fc_mode.
*/
struct ActuatorCommands
{
    std::uint64_t timestamp_us;

    std::float32_t wing_rpm_cmd;
    std::float32_t left_servo_rad;
    std::float32_t right_servo_rad;
    std::float32_t aux_actuator_cmd;
    std::uint8_t  mode_flags;
};

}
