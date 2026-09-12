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
            .fc_detection_code = p.fc_detection_code,
        };
    }
}

/* Validates one complete actuator frame and fills the outputs. */
std::expected<ReceiveResult, FrameAcceptanceError> HilTransport::accept_frame(
    const FlightCore::Transport::HilHeader& header,
    const ActuatorExpectations& expectations,
    std::uint64_t receive_wall,
    ActuatorReceiveOutputs outputs)
{
    if (header.msg_id != FlightCore::Transport::kMsgIdActuator) {
        stats_.record_dropped();
        return std::unexpected(FrameAcceptanceError::NotActuatorFrame);
    }
    if (header.sequence_num != expectations.expected_sequence) {
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
    if (payload.echo_sim_timestamp_us != expectations.expected_echo_sim_us) {
        stats_.record_stale();
        return ReceiveResult::EchoMismatch;
    }
    outputs.commands = FlightCore::Transport::toActuatorCommands(payload);
    outputs.diagnostics = to_diag(payload);
    outputs.round_trip_us = static_cast<std::int64_t>(receive_wall)
                            - static_cast<std::int64_t>(expectations.sensor_send_wall_us);
    stats_.record_received(outputs.round_trip_us);
    return ReceiveResult::Ok;
}

}
