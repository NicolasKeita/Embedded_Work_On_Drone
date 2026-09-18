/*
Filename: Src/Embedded/Hil/Transport/HilTransport-Stats.cpp
Description: Communication statistics accumulation of the HIL transport and the
HilTransport frame-send/bookkeeping entry points.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTransport;

import std;

import HalTypes;
import HilProtocol;
import Transport;

namespace sim::hil {

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

/* Sends one configured sensor frame and updates transport statistics. */
bool HilTransport::sendSensor(const FlightCore::HAL::SensorData&               sensor,
                              std::uint16_t                                    sequence,
                              const FlightCore::Transport::HilControlSetpoint& setpoint)
{
    if (!send_sensor_frame(*channel_, sensor, sequence, setpoint)) {
        ++stats_.messages_dropped;
        return false;
    }
    ++stats_.messages_sent;
    return true;
}

const HilCommStats& HilTransport::stats() const noexcept { return stats_; }

void HilTransport::noteDroppedFrame() noexcept { stats_.record_dropped(); }
void HilTransport::noteTimeoutFrame() noexcept { stats_.record_timeout(); }

}
