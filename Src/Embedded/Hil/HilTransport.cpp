/*
Filename: Src/Embedded/Hil/HilTransport.cpp
Description: Frame exchange, deadline-bounded actuator receive and communication
statistics of the HIL transport.

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

void HilCommStats::record_received(std::int64_t rtt_us) noexcept
{
    ++messages_received;
    latency_min_us = (latency_min_us < 0) ? rtt_us : std::min(latency_min_us, rtt_us);
    latency_max_us = (latency_max_us < 0) ? rtt_us : std::max(latency_max_us, rtt_us);
    if (latency_mean_us < 0.0) {
        latency_mean_us = static_cast<std::float64_t>(rtt_us);
    }
    else {
        latency_mean_us += (static_cast<std::float64_t>(rtt_us) - latency_mean_us)
                         / static_cast<std::float64_t>(messages_received);
    }
}

void HilCommStats::record_dropped() noexcept { ++messages_dropped; }
void HilCommStats::record_timeout() noexcept { ++timeouts; }
void HilCommStats::record_sequence_error() noexcept { ++sequence_errors; }
void HilCommStats::record_stale() noexcept { ++stale_packets; }

HilTransport::HilTransport(FlightCore::Transport::ITransport* channel) noexcept : channel_{channel} {}

bool HilTransport::sendSensor(const FlightCore::HAL::SensorData& sensor, std::uint16_t sequence)
{
    if (!send_sensor_frame(*channel_, sensor, sequence)) {
        ++stats_.messages_dropped;
        return false;
    }
    ++stats_.messages_sent;
    return true;
}

const HilCommStats& HilTransport::stats() const noexcept { return stats_; }

void HilTransport::noteDroppedFrame() noexcept { stats_.record_dropped(); }
void HilTransport::noteTimeoutFrame() noexcept { stats_.record_timeout(); }

/*
Drains the channel through the parser until the ActuatorPacket answering the
expected sequence and echoed sim timestamp arrives, or the wall-clock deadline is
reached. Validates message id, sequence number and sim-timestamp echo so sequence,
duplicate and stale packets are caught; computes the wall-clock round trip.
*/
ReceiveResult HilTransport::receiveActuator(IWallClock& clock,
                                             std::uint16_t expected_sequence,
                                             std::uint64_t expected_echo_sim_us,
                                             std::uint64_t sensor_send_wall_us,
                                             std::uint64_t deadline_wall_us,
                                             FlightCore::HAL::ActuatorCommands& out_cmds,
                                             FlightCore::Transport::ActuatorDiagnostics& out_diag,
                                             std::int64_t& out_rtt_us)
{
    std::array<std::uint8_t, 128> rx{};

    for (;;) {
        while (channel_->bytesAvailable() > 0) {
            const std::size_t n = channel_->receiveBytes(rx);
            for (std::size_t i = 0; i < n; ++i) {
                FlightCore::Transport::HilHeader header{};
                const std::uint64_t rejected_before = parser_.rejectedFrames();
                if (parser_.processByte(rx[i], header, payload_buffer_)) {
                    const std::uint64_t receive_wall = clock.nowUs();
                    if (header.msg_id != FlightCore::Transport::kMsgIdActuator) {
                        stats_.record_dropped();
                        continue;
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
                    const std::span<const std::uint8_t> span(payload_buffer_.data(),
                                                             FlightCore::Transport::kActuatorPayloadSize);
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
                    out_rtt_us = static_cast<std::int64_t>(receive_wall)
                              - static_cast<std::int64_t>(sensor_send_wall_us);
                    stats_.record_received(out_rtt_us);
                    return ReceiveResult::Ok;
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

bool send_sensor_frame(FlightCore::Transport::ITransport& channel,
                       const FlightCore::HAL::SensorData& sensor, std::uint16_t sequence) noexcept
{
    const auto payload = FlightCore::Transport::makeSensorPayload(sensor);
    const auto frame = FlightCore::Transport::encodeSensorFrame(payload, sequence);
    return channel.sendBytes(frame);
}

bool send_actuator_frame(FlightCore::Transport::ITransport& channel,
                         const FlightCore::HAL::ActuatorCommands& cmds,
                         std::uint64_t echo_sim_ts_us,
                         const FlightCore::Transport::ActuatorDiagnostics& diagnostics,
                         std::uint16_t sequence) noexcept
{
    const auto payload = FlightCore::Transport::makeActuatorPayload(cmds, echo_sim_ts_us, diagnostics);
    const auto frame = FlightCore::Transport::encodeActuatorFrame(payload, sequence);
    return channel.sendBytes(frame);
}

bool receive_frame(FlightCore::Transport::ITransport& channel,
                   FlightCore::Transport::HilFrameParser& parser,
                   FlightCore::Transport::HilHeader& out_header,
                   std::span<std::uint8_t> out_payload)
{
    std::array<std::uint8_t, 128> rx{};

    while (channel.bytesAvailable() > 0) {
        const std::size_t n = channel.receiveBytes(rx);
        for (std::size_t i = 0; i < n; ++i) {
            if (parser.processByte(rx[i], out_header, out_payload)) {
                return true;
            }
        }
    }
    return false;
}

}
