/*
Filename: Src/Embedded/Hil/Transport/HilTransport-Accept.cpp
Description: Frame validation of the HIL transport : message id, sequence, payload and
sim-timestamp echo checks with the wall-clock round-trip computation.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTransport;

import std;

import HalTypes;
import HilProtocol;
import HilProtocolCodec;
import Transport;

namespace sim::hil {

namespace {
    /* Converts the decoded actuator payload into the diagnostics view. */
    FlightCore::Transport::ActuatorDiagnostics to_diag(const FlightCore::Transport::HilActuatorPayload& p) noexcept
    {
        return FlightCore::Transport::ActuatorDiagnostics{
            .cpu_usage_pct_x100 = p.cpu_usage_pct_x100,
            .stack_watermark_words = p.stack_watermark_words,
            .deadline_miss_count = p.deadline_miss_count,
            .fc_health_status = p.fc_health_status,
        };
    }
}

/* Validates one complete actuator frame and fills the outputs. */
std::expected<ReceiveResult, FrameAcceptanceError> HilTransport::accept_frame(
    const FlightCore::Transport::HilHeader& header,
    std::uint16_t expected_sequence,
    std::uint64_t expected_echo_sim_us,
    std::uint64_t sensor_send_wall_us,
    std::uint64_t receive_wall,
    FlightCore::HAL::ActuatorCommands& out_cmds,
    FlightCore::Transport::ActuatorDiagnostics& out_diag,
    std::int64_t& out_rtt_us)
{
    if (header.msg_id != FlightCore::Transport::kMsgIdActuator) {
        stats_.record_dropped();
        return std::unexpected(FrameAcceptanceError::NotActuatorFrame);
    }
    if (header.sequence_num != expected_sequence) {
        stats_.record_sequence_error();
        return ReceiveResult::SequenceError;
    }
    if (header.payload_len < FlightCore::Transport::kActuatorPayloadSize) {
        stats_.record_dropped();
        return ReceiveResult::InvalidPayload;
    }
    FlightCore::Transport::HilActuatorPayload payload{};
    const std::span<const std::uint8_t> span(payload_buffer_.data(), FlightCore::Transport::kActuatorPayloadSize);
    if (!FlightCore::Transport::decodeActuatorPayload(span, payload)) {
        stats_.record_dropped();
        return ReceiveResult::InvalidPayload;
    }
    if (payload.echo_sim_timestamp_us != expected_echo_sim_us) {
        stats_.record_stale();
        return ReceiveResult::EchoMismatch;
    }
    out_cmds = FlightCore::Transport::toActuatorCommands(payload);
    out_diag = to_diag(payload);
    out_rtt_us = static_cast<std::int64_t>(receive_wall) - static_cast<std::int64_t>(sensor_send_wall_us);
    stats_.record_received(out_rtt_us);
    return ReceiveResult::Ok;
}

}
