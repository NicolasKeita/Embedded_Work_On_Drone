/*
Filename: Src/embedded/transport/hil_protocol.cppm
Description: HIL-Proto v1.0 binary contract (docs/hil/hil_protocol.md): packed
HilHeader, HilSensorPayload (msg 0x01), HilActuatorPayload (msg 0x02),
CRC-16-CCITT (poly 0x1021, init 0xFFFF), a heap-free byte-by-byte HilFrameParser
FSM, and the HAL<->wire converters/encoders used by the PC mock and the STM32
transport layer.

Wire-size note: the protocol prose quotes a 68-octet SensorPacket payload
(78-octet frame). The packed struct reproduced verbatim is 80 octets
(8-byte sim_timestamp_us + 17 float fields + 4-byte sensor_valid_flags),
i.e. a 90-octet frame; 68 == 17x4 is the float-fields-only sum. payload_len is
read from the header at runtime so the parser stays length-agnostic.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module flight.transport.protocol;

import std;

import flight.transport;
import flight.hal.types;

export namespace FlightCore::Transport
{

inline constexpr std::uint8_t kSync1         = 0x48;
inline constexpr std::uint8_t kSync2         = 0x49;
inline constexpr std::uint8_t kMsgIdSensor   = 0x01;
inline constexpr std::uint8_t kMsgIdActuator = 0x02;
inline constexpr std::uint8_t kProtocolVer   = 0x10;

inline constexpr std::size_t kHeaderSize          = 8;
inline constexpr std::size_t kCrcSize            = 2;
inline constexpr std::size_t kMaxPayload         = 128;
inline constexpr std::size_t kSensorPayloadSize    = 80;
inline constexpr std::size_t kActuatorPayloadSize = 44;
inline constexpr std::size_t kSensorFrameSize  = kHeaderSize + kSensorPayloadSize + kCrcSize;
inline constexpr std::size_t kActuatorFrameSize = kHeaderSize + kActuatorPayloadSize + kCrcSize;

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
    std::uint64_t sim_timestamp_us;
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
    std::uint64_t fc_timestamp_us;
    std::uint64_t echo_sim_timestamp_us;
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

class HilCrc
{
public:
    HilCrc() = delete;

    /* CRC-16-CCITT (poly 0x1021, init 0xFFFF) over a single buffer. */
    [[nodiscard]] static constexpr std::uint16_t compute(std::span<const std::uint8_t> data) noexcept
    {
        return update(0xFFFFu, data);
    }

    /* Continues an in-progress CRC with additional bytes (Header then Payload). */
    [[nodiscard]] static constexpr std::uint16_t update(std::uint16_t crc, std::span<const std::uint8_t> data) noexcept
    {
        std::uint16_t local = crc;
        for (std::uint8_t value : data)
        {
            local ^= static_cast<std::uint16_t>(value) << 8;
            for (std::uint8_t bit = 0; bit < 8; ++bit)
            {
                const std::uint16_t mask = (local & 0x8000u) != 0u ? 0x1021u : 0x0000u;
                local = static_cast<std::uint16_t>((local << 1) ^ mask);
            }
        }
        return local;
    }
};

class HilFrameParser
{
public:
    enum class State : std::uint8_t
    {
        WaitSync1,
        WaitSync2,
        ReadHeader,
        ReadPayload,
        ReadCrc
    };

    HilFrameParser() = default;

    [[nodiscard]] State state() const noexcept { return state_; }

    [[nodiscard]] std::uint64_t rejectedFrames() const noexcept { return rejected_; }

    /* Returns the parser to the sync-hunt state without dropping the rejection counter. */
    void reset() noexcept;

    /*
        Feeds one byte; returns true when a complete, CRC-valid frame has been
        assembled, copying its header into out_header and its payload bytes into
        out_payload (which must hold at least header.payload_len bytes).
    */
    [[nodiscard]] bool processByte(std::uint8_t byte, HilHeader& out_header, std::span<std::uint8_t> out_payload) noexcept;

private:
    [[nodiscard]] bool finalizeFrame(HilHeader& out_header, std::span<std::uint8_t> out_payload) noexcept;

    State state_{State::WaitSync1};
    HilHeader header_{};
    std::array<std::uint8_t, kHeaderSize> header_bytes_{};
    std::array<std::uint8_t, kMaxPayload> payload_buffer_{};
    std::array<std::uint8_t, kCrcSize> crc_bytes_{};
    std::size_t index_{0};
    std::uint64_t rejected_{0};
};

inline void HilFrameParser::reset() noexcept
{
    state_ = State::WaitSync1;
    index_ = 0;
}

inline bool HilFrameParser::finalizeFrame(HilHeader& out_header, std::span<std::uint8_t> out_payload) noexcept
{
    const std::uint16_t received = static_cast<std::uint16_t>(crc_bytes_[0])
                                | (static_cast<std::uint16_t>(crc_bytes_[1]) << 8);

    std::uint16_t computed = HilCrc::compute(std::span<const std::uint8_t>(header_bytes_.data(), kHeaderSize));
    computed = HilCrc::update(computed, std::span<const std::uint8_t>(payload_buffer_.data(), header_.payload_len));

    if (computed != received || out_payload.size() < header_.payload_len)
    {
        ++rejected_;
        return false;
    }

    std::copy(payload_buffer_.data(), payload_buffer_.data() + header_.payload_len, out_payload.data());
    out_header = header_;
    return true;
}

inline bool HilFrameParser::processByte(std::uint8_t byte, HilHeader& out_header, std::span<std::uint8_t> out_payload) noexcept
{
    switch (state_)
    {
    case State::WaitSync1:
        if (byte == kSync1) state_ = State::WaitSync2;
        return false;

    case State::WaitSync2:
        if (byte == kSync2)
        {
            header_bytes_[0] = kSync1;
            header_bytes_[1] = kSync2;
            index_ = 2;
            state_ = State::ReadHeader;
        }
        else if (byte == kSync1)
        {
            state_ = State::WaitSync2;
        }
        else
        {
            state_ = State::WaitSync1;
        }
        return false;

    case State::ReadHeader:
        header_bytes_[index_] = byte;
        ++index_;
        if (index_ == kHeaderSize)
        {
            std::memcpy(&header_, header_bytes_.data(), kHeaderSize);
            if (header_.payload_len > kMaxPayload)
            {
                ++rejected_;
                reset();
                return false;
            }
            index_ = 0;
            state_ = (header_.payload_len == 0u) ? State::ReadCrc : State::ReadPayload;
        }
        return false;

    case State::ReadPayload:
        payload_buffer_[index_] = byte;
        ++index_;
        if (index_ == header_.payload_len)
        {
            index_ = 0;
            state_ = State::ReadCrc;
        }
        return false;

    case State::ReadCrc:
        crc_bytes_[index_] = byte;
        ++index_;
        if (index_ == kCrcSize)
        {
            const bool ok = finalizeFrame(out_header, out_payload);
            reset();
            return ok;
        }
        return false;
    }

    reset();
    return false;
}

/* Builds a HilSensorPayload from the HAL sensor snapshot. */
[[nodiscard]] inline HilSensorPayload makeSensorPayload(const FlightCore::HAL::SensorData& sensor) noexcept
{
    HilSensorPayload payload{};
    payload.sim_timestamp_us    = sensor.timestamp_us;
    payload.position_x_m        = sensor.position_x_m;
    payload.position_y_m        = sensor.position_y_m;
    payload.position_z_m        = sensor.position_z_m;
    payload.velocity_x_ms       = sensor.velocity_x_ms;
    payload.velocity_y_ms       = sensor.velocity_y_ms;
    payload.velocity_z_ms       = sensor.velocity_z_ms;
    payload.gyro_p_rad_s        = sensor.gyro_roll_rad_s;
    payload.gyro_q_rad_s        = sensor.gyro_pitch_rad_s;
    payload.gyro_r_rad_s        = sensor.gyro_yaw_rad_s;
    payload.accel_x_m_s2       = sensor.accel_x_m_s2;
    payload.accel_y_m_s2       = sensor.accel_y_m_s2;
    payload.accel_z_m_s2       = sensor.accel_z_m_s2;
    payload.roll_rad           = sensor.roll_rad;
    payload.pitch_rad          = sensor.pitch_rad;
    payload.yaw_rad             = sensor.yaw_rad;
    payload.altitude_baro_m    = sensor.altitude_baro_m;
    payload.wing_rpm_meas      = sensor.wing_rpm_meas;
    payload.sensor_valid_flags = sensor.sensor_valid_flags;
    return payload;
}

/* Decodes raw payload bytes into a HilSensorPayload; false on short input. */
[[nodiscard]] inline bool decodeSensorPayload(std::span<const std::uint8_t> bytes, HilSensorPayload& out) noexcept
{
    if (bytes.size() < sizeof(HilSensorPayload)) return false;
    std::memcpy(&out, bytes.data(), sizeof(HilSensorPayload));
    return true;
}

[[nodiscard]] inline FlightCore::HAL::SensorData toSensorData(const HilSensorPayload& payload) noexcept
{
    FlightCore::HAL::SensorData data{};
    data.timestamp_us        = payload.sim_timestamp_us;
    data.position_x_m         = payload.position_x_m;
    data.position_y_m         = payload.position_y_m;
    data.position_z_m         = payload.position_z_m;
    data.velocity_x_ms        = payload.velocity_x_ms;
    data.velocity_y_ms        = payload.velocity_y_ms;
    data.velocity_z_ms        = payload.velocity_z_ms;
    data.gyro_roll_rad_s      = payload.gyro_p_rad_s;
    data.gyro_pitch_rad_s     = payload.gyro_q_rad_s;
    data.gyro_yaw_rad_s       = payload.gyro_r_rad_s;
    data.accel_x_m_s2         = payload.accel_x_m_s2;
    data.accel_y_m_s2         = payload.accel_y_m_s2;
    data.accel_z_m_s2         = payload.accel_z_m_s2;
    data.roll_rad            = payload.roll_rad;
    data.pitch_rad           = payload.pitch_rad;
    data.yaw_rad             = payload.yaw_rad;
    data.altitude_baro_m     = payload.altitude_baro_m;
    data.wing_rpm_meas       = payload.wing_rpm_meas;
    data.sensor_valid_flags  = payload.sensor_valid_flags;
    return data;
}

struct ActuatorDiagnostics
{
    std::uint32_t cpu_usage_pct_x100   = 0;
    std::uint16_t stack_watermark_words = 0;
    std::uint16_t deadline_miss_count   = 0;
    std::uint8_t  fc_health_status      = 0;
};

/*
    Builds a HilActuatorPayload from the HAL command set. cmds.timestamp_us is
    used as the FC local clock (fc_timestamp_us); echo_sim_timestamp_us must be
    the sim_timestamp_us that was received in the triggering SensorPacket (RTT).
*/
[[nodiscard]] inline HilActuatorPayload makeActuatorPayload(
    const FlightCore::HAL::ActuatorCommands& cmds,
    std::uint64_t echo_sim_timestamp_us,
    const ActuatorDiagnostics& diagnostics) noexcept
{
    HilActuatorPayload payload{};
    payload.fc_timestamp_us         = cmds.timestamp_us;
    payload.echo_sim_timestamp_us  = echo_sim_timestamp_us;
    payload.wing_rpm_cmd           = cmds.wing_rpm_cmd;
    payload.left_servo_cmd_rad     = cmds.left_servo_rad;
    payload.right_servo_cmd_rad    = cmds.right_servo_rad;
    payload.aux_actuator_cmd       = cmds.aux_actuator_cmd;
    payload.cpu_usage_pct_x100     = diagnostics.cpu_usage_pct_x100;
    payload.stack_watermark_words  = diagnostics.stack_watermark_words;
    payload.deadline_miss_count     = diagnostics.deadline_miss_count;
    payload.fc_mode                = cmds.mode_flags;
    payload.fc_health_status       = diagnostics.fc_health_status;
    payload.reserved               = 0;
    return payload;
}

[[nodiscard]] inline bool decodeActuatorPayload(std::span<const std::uint8_t> bytes, HilActuatorPayload& out) noexcept
{
    if (bytes.size() < sizeof(HilActuatorPayload)) return false;
    std::memcpy(&out, bytes.data(), sizeof(HilActuatorPayload));
    return true;
}

[[nodiscard]] inline FlightCore::HAL::ActuatorCommands toActuatorCommands(const HilActuatorPayload& payload) noexcept
{
    FlightCore::HAL::ActuatorCommands cmds{};
    cmds.timestamp_us     = payload.fc_timestamp_us;
    cmds.wing_rpm_cmd     = payload.wing_rpm_cmd;
    cmds.left_servo_rad   = payload.left_servo_cmd_rad;
    cmds.right_servo_rad  = payload.right_servo_cmd_rad;
    cmds.aux_actuator_cmd = payload.aux_actuator_cmd;
    cmds.mode_flags       = payload.fc_mode;
    return cmds;
}

/*
    Serializes a SensorPacket frame (Header + Payload + CRC-16, little-endian)
    ready for ITransport::sendBytes. sequence_num wraps at 65535 per HilHeader.
*/
[[nodiscard]] inline std::array<std::uint8_t, kSensorFrameSize> encodeSensorFrame(const HilSensorPayload& payload, std::uint16_t seq) noexcept
{
    HilHeader header{};
    header.sync_byte_1  = kSync1;
    header.sync_byte_2  = kSync2;
    header.msg_id       = kMsgIdSensor;
    header.protocol_ver = kProtocolVer;
    header.sequence_num = seq;
    header.payload_len  = static_cast<std::uint16_t>(kSensorPayloadSize);

    std::array<std::uint8_t, kSensorFrameSize> frame{};
    std::memcpy(frame.data(), &header, kHeaderSize);
    std::memcpy(frame.data() + kHeaderSize, &payload, kSensorPayloadSize);

    const std::uint16_t crc = HilCrc::compute(std::span<const std::uint8_t>(frame.data(), kHeaderSize + kSensorPayloadSize));
    frame[kSensorFrameSize - 2] = static_cast<std::uint8_t>(crc & 0x00FFu);
    frame[kSensorFrameSize - 1] = static_cast<std::uint8_t>((crc >> 8) & 0x00FFu);
    return frame;
}

[[nodiscard]] inline std::array<std::uint8_t, kActuatorFrameSize> encodeActuatorFrame(const HilActuatorPayload& payload, std::uint16_t seq) noexcept
{
    HilHeader header{};
    header.sync_byte_1  = kSync1;
    header.sync_byte_2  = kSync2;
    header.msg_id       = kMsgIdActuator;
    header.protocol_ver = kProtocolVer;
    header.sequence_num = seq;
    header.payload_len  = static_cast<std::uint16_t>(kActuatorPayloadSize);

    std::array<std::uint8_t, kActuatorFrameSize> frame{};
    std::memcpy(frame.data(), &header, kHeaderSize);
    std::memcpy(frame.data() + kHeaderSize, &payload, kActuatorPayloadSize);

    const std::uint16_t crc = HilCrc::compute(std::span<const std::uint8_t>(frame.data(), kHeaderSize + kActuatorPayloadSize));
    frame[kActuatorFrameSize - 2] = static_cast<std::uint8_t>(crc & 0x00FFu);
    frame[kActuatorFrameSize - 1] = static_cast<std::uint8_t>((crc >> 8) & 0x00FFu);
    return frame;
}

}
