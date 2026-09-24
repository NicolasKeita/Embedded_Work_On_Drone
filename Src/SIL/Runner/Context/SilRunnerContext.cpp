/*
Filename: Src/SIL/Runner/Context/SilRunnerContext.cpp
Description: Construction of the SIL run context (subsystem and recorder wiring).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunnerContext;

import std;

import CommsBus;
import FlightController;
import HealthMonitor;
import SafetyManager;
import SilEvents;
import SilTelemetry;

namespace sim::sil {

using sim::safety::HealthMonitorConfig;
using sim::safety::SafetyManagerConfig;

/* Uses the same raised-cosine gust and half-open activation interval as HIL. */
std::float64_t wind_factor(std::float64_t start_s, std::float64_t end_s,
                           std::float64_t gust_period_s, std::float64_t time_s) noexcept
{
    if (time_s < start_s || time_s >= end_s) {
        return 0.0;
    }
    if (gust_period_s <= 0.0) {
        return 1.0;
    }
    const std::float64_t phase = (time_s - start_s) / gust_period_s;
    return 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * phase);
}

/* Resolves the configured wind envelope without allocation or filesystem access. */
std::float64_t wind_factor(const SilConfig& config, std::float64_t time_s) noexcept
{
    return wind_factor(config.wind_start_s, config.wind_end_s, config.wind_gust_period_s, time_s);
}


/*
Assembles the run context: flight controller, comms bus, monitoring stack and
trace are built from the configuration; telemetry recorders reserve their
fixed-rate sample capacity up front.
*/
RunContext::RunContext(const SilConfig& cfg)
    : config{cfg},
      fc1{cfg.controller},
      comms{cfg.seed},
      health{HealthMonitorConfig{.heartbeat_timeout_s = cfg.heartbeat_timeout_s,
                                 .actuator_mismatch_rpm = cfg.actuator_mismatch_rpm,
                                 .sensor_limits = cfg.sensor_limits}},
      safety{SafetyManagerConfig{.degraded_thrust_margin = cfg.thrust_compensation_margin}},
      trace{SilTraceConfig{.level = cfg.trace_level}}
{
    telemetry_recorder.interval_s = cfg.telemetry_rate_hz > 0.0 ? 1.0 / cfg.telemetry_rate_hz : 0.0;
    comms.set_transport_latency(cfg.transport_latency_s);
    const std::size_t expected_samples =
        static_cast<std::size_t>(cfg.duration_s * std::max(cfg.telemetry_rate_hz, std::float64_t{0.0})) + 2;
    telemetry_recorder.samples.reserve(expected_samples);
    telemetry_recorder.truth_samples.reserve(expected_samples);
}

}
