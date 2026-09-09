/*
Filename: Src/Embedded/Hil/Transport/HilTransport.cppm
Description: Frame-level HIL-Proto exchange and communication statistics over the ITransport byte channel.
Exports: ActuatorDiagnostics (re-export), ReceiveResult, HilCommStats, HilTransport,
send_sensor_frame(), send_actuator_frame(), receive_frame().

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilTransport;

import std;

import HalTypes;
import HilClock;
import HilProtocol;
import HilProtocolCodec;
import HilProtocolParser;
import Transport;

export namespace sim::hil {

enum class FrameAcceptanceError : std::uint8_t { NotActuatorFrame };

using FlightCore::Transport::ActuatorDiagnostics;

/* Reception outcome of one actuator frame on the HIL bench transport (never an aircraft failure mode). */
enum class ReceiveResult {
    Ok,
    Timeout,
    SequenceError,
    MessageTypeError,
    EchoMismatch,
    InvalidPayload,
    SendFailed
};

struct HilCommStats {
    std::uint64_t  messages_sent = 0;
    std::uint64_t  messages_received = 0;
    std::uint64_t  messages_dropped = 0;
    std::uint64_t  sequence_errors = 0;
    std::uint64_t  timeouts = 0;
    std::uint64_t  stale_packets = 0;
    std::int64_t   latency_min_us = -1;
    std::int64_t   latency_max_us = -1;
    std::float64_t latency_mean_us = -1.0;

    void record_received(std::int64_t rtt_us) noexcept;
    void record_dropped() noexcept;
    void record_timeout() noexcept;
    void record_sequence_error() noexcept;
    void record_stale() noexcept;
};

/* Runner-side frame exchange over one shared byte channel (lockstep: one SensorPacket in flight). */
class HilTransport {
public:
    explicit HilTransport(FlightCore::Transport::ITransport* channel) noexcept;

    [[nodiscard]] bool sendSensor(const FlightCore::HAL::SensorData& sensor, std::uint16_t sequence);

    /* Records an actuator packet dropped without consuming a wire receive (comm loss path). */
    void noteDroppedFrame() noexcept;

    /* Records an actuator response timeout without blocking (FC1 silent path). */
    void noteTimeoutFrame() noexcept;

    /* Waits for the ActuatorPacket answering sequence, no later than deadline_wall_us. */
    [[nodiscard]] ReceiveResult receiveActuator(MonotonicClock& clock,
                                                  std::uint16_t expected_sequence,
                                                  std::uint64_t expected_echo_sim_us,
                                                  std::uint64_t sensor_send_wall_us,
                                                  std::uint64_t deadline_wall_us,
                                                  FlightCore::HAL::ActuatorCommands& out_cmds,
                                                  FlightCore::Transport::ActuatorDiagnostics& out_diag,
                                                  std::int64_t& out_rtt_us);

    [[nodiscard]] const HilCommStats& stats() const noexcept;

private:
    /* Validates one complete actuator frame and fills the outputs. */
    [[nodiscard]] std::expected<ReceiveResult, FrameAcceptanceError> accept_frame(
        const FlightCore::Transport::HilHeader& header,
        std::uint16_t expected_sequence,
        std::uint64_t expected_echo_sim_us,
        std::uint64_t sensor_send_wall_us,
        std::uint64_t receive_wall,
        FlightCore::HAL::ActuatorCommands& out_cmds,
        FlightCore::Transport::ActuatorDiagnostics& out_diag,
        std::int64_t& out_rtt_us);

    FlightCore::Transport::ITransport*                           channel_;
    FlightCore::Transport::HilFrameParser                        parser_{};
    std::array<std::uint8_t, FlightCore::Transport::kMaxPayload> payload_buffer_{};
    HilCommStats                                                 stats_{};
};

/* Encodes and sends one SensorPacket frame on the channel (host SDK side helper). */
[[nodiscard]] bool send_sensor_frame(FlightCore::Transport::ITransport& channel,
                                     const FlightCore::HAL::SensorData& sensor, std::uint16_t sequence) noexcept;

/* Encodes and sends one ActuatorPacket frame on the channel (host FC target helper). */
[[nodiscard]] bool send_actuator_frame(FlightCore::Transport::ITransport& channel,
                                       const FlightCore::HAL::ActuatorCommands& cmds,
                                       std::uint64_t echo_sim_ts_us,
                                       const FlightCore::Transport::ActuatorDiagnostics& diagnostics,
                                       std::uint16_t sequence) noexcept;

/* Drains the channel through the parser until a complete, CRC-valid frame is assembled. */
[[nodiscard]] bool receive_frame(FlightCore::Transport::ITransport& channel,
                                 FlightCore::Transport::HilFrameParser& parser,
                                 FlightCore::Transport::HilHeader& out_header,
                                 std::span<std::uint8_t> out_payload);

}
