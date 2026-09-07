/*
Filename: Src/Embedded/Hil/Transport/HilTransport-Receive.cpp
Description: Deadline-bounded actuator receive of the HIL transport : frame validation
(message id, sequence, payload, sim-timestamp echo) and wall-clock round trip.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTransport;

import std;

import HalTypes;
import HilClock;
import HilProtocol;
import HilProtocolCodec;
import HilProtocolParser;
import Transport;

namespace sim::hil {

namespace {
    constexpr std::uint64_t kPollSleepUs = 100;

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

/*
Drains the channel through the parser until the ActuatorPacket answering the
expected sequence and echoed sim timestamp arrives, or the wall-clock deadline is
reached. Validates message id, sequence number and sim-timestamp echo so sequence,
duplicate and stale packets are caught; computes the wall-clock round trip.
*/
ReceiveResult HilTransport::receiveActuator(IWallClock&                                 clock,
                                            std::uint16_t                               expected_sequence,
                                            std::uint64_t                               expected_echo_sim_us,
                                            std::uint64_t                               sensor_send_wall_us,
                                            std::uint64_t                               deadline_wall_us,
                                            FlightCore::HAL::ActuatorCommands&          out_cmds,
                                            FlightCore::Transport::ActuatorDiagnostics& out_diag,
                                            std::int64_t&                               out_rtt_us)
{
    std::array<std::uint8_t, 128> rx{};

    for (;;) {
        while (channel_->bytesAvailable() > 0) {
            const std::size_t n = channel_->receiveBytes(rx);
            for (std::size_t i = 0; i < n; ++i) {
                FlightCore::Transport::HilHeader header{};
                const std::uint64_t rejected_before = parser_.rejectedFrames();
                if (parser_.processByte(rx[i], header, payload_buffer_)) {
                    const std::expected<ReceiveResult, FrameAcceptanceError> accepted =
                        accept_frame(header, expected_sequence, expected_echo_sim_us, sensor_send_wall_us,
                                     clock.nowUs(), out_cmds, out_diag,
                                     out_rtt_us);
                    if (accepted.has_value()) {
                        return accepted.value();
                    }
                }
                else if (parser_.rejectedFrames() > rejected_before) {
                    stats_.record_dropped();
                }
            }
        }
        if (clock.nowUs() >= deadline_wall_us) {
            stats_.record_timeout();
            return ReceiveResult::Timeout;
        }
        clock.sleepUntilUs(clock.nowUs() + kPollSleepUs);
    }
}

}
