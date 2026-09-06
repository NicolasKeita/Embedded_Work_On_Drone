/*
Filename: Src/Embedded/Transport/HilProtocol.cppm
Description: HIL-Proto v1.0 binary contract (docs/hil/hil_protocol.md): wire
constants, packed HilHeader, HilSensorPayload (msg 0x01), HilActuatorPayload
(msg 0x02) and the FC-side ActuatorDiagnostics echoed in the ActuatorPacket.
The receive-side primitives (CRC-16-CCITT, HilFrameParser FSM) live in
HilProtocolParser and the HAL<->wire converters/encoders live in
HilProtocolCodec.

Wire-size note: the protocol prose quotes a 68-octet SensorPacket payload
(78-octet frame). The packed struct reproduced verbatim is 80 octets
(8-byte sim_timestamp_us + 17 float fields + 4-byte sensor_valid_flags),
i.e. a 90-octet frame; 68 == 17x4 is the float-fields-only sum. payload_len is
read from the header at runtime so the parser stays length-agnostic.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilProtocol;

import std;

export namespace FlightCore::Transport
{

inline constexpr std::uint8_t kSync1         = 0x48;
inline constexpr std::uint8_t kSync2         = 0x49;
inline constexpr std::uint8_t kMsgIdSensor   = 0x01;
inline constexpr std::uint8_t kMsgIdActuator = 0x02;
inline constexpr std::uint8_t kProtocolVer   = 0x10;

inline constexpr std::size_t kHeaderSize           = 8;
inline constexpr std::size_t kCrcSize              = 2;
inline constexpr std::size_t kMaxPayload           = 128;
inline constexpr std::size_t kSensorPayloadSize    = 80;
inline constexpr std::size_t kActuatorPayloadSize  = 44;
inline constexpr std::size_t kSensorFrameSize    = kHeaderSize + kSensorPayloadSize + kCrcSize;
inline constexpr std::size_t kActuatorFrameSize  = kHeaderSize + kActuatorPayloadSize + kCrcSize;

#pragma pack(push, 1)
struct HilHeader
{
    std::uint8_t  sync_byte_1;
    std::uint8_t  sync_byte_2;
    std::uint8_t  msg_id;
    std::uint8_t  protocol_ver;
    std::uint16_t sequence_num;
    std::uint16_t payload_len;
};

struct HilSensorPayload
{
    std::uint64_t  sim_timestamp_us;
    std::float32_t position_x_m;
    std::float32_t position_y_m;
    std::float32_t position_z_m;
    std::float32_t velocity_x_ms;
    std::float32_t velocity_y_ms;
    std::float32_t velocity_z_ms;
    std::float32_t gyro_p_rad_s;
    std::float32_t gyro_q_rad_s;
    std::float32_t gyro_r_rad_s;
    std::float32_t accel_x_m_s2;
    std::float32_t accel_y_m_s2;
    std::float32_t accel_z_m_s2;
    std::float32_t roll_rad;
    std::float32_t pitch_rad;
    std::float32_t yaw_rad;
    std::float32_t altitude_baro_m;
    std::float32_t wing_rpm_meas;
    std::uint32_t  sensor_valid_flags;
};

struct HilActuatorPayload
{
    std::uint64_t  fc_timestamp_us;
    std::uint64_t  echo_sim_timestamp_us;
    std::float32_t wing_rpm_cmd;
    std::float32_t left_servo_cmd_rad;
    std::float32_t right_servo_cmd_rad;
    std::float32_t aux_actuator_cmd;
    std::uint32_t  cpu_usage_pct_x100;
    std::uint16_t  stack_watermark_words;
    std::uint16_t  deadline_miss_count;
    std::uint8_t   fc_mode;
    std::uint8_t   fc_health_status;
    std::uint16_t  reserved;
};
#pragma pack(pop)

static_assert(sizeof(HilHeader) == 8);
static_assert(sizeof(HilSensorPayload) == 80);
static_assert(sizeof(HilActuatorPayload) == 44);
static_assert(kMaxPayload >= kSensorPayloadSize);
static_assert(kMaxPayload >= kActuatorPayloadSize);

/*
    FC-side runtime diagnostics echoed inside the ActuatorPacket payload so the
    simulator can monitor the embedded target health over the HIL link.
*/
struct ActuatorDiagnostics
{
    std::uint32_t cpu_usage_pct_x100 = 0;
    std::uint16_t stack_watermark_words = 0;
    std::uint16_t deadline_miss_count = 0;
    std::uint8_t  fc_health_status = 0;
};

}
