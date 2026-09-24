/*
Filename: Src/Host/Config/SilRuntimeConfig.cpp
Description: Host-only SIL configuration loading, validation and profile precedence.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRuntimeConfig;

import std;

import ConfigFile;
import FunctionalScenarios;
import SilEvents;

namespace sim::host {
namespace {

SilRuntimeOptions active_options{};

/* Reads tunable controller values; firmware controller configuration is unaffected. */
void read_controller(sim::config::Reader& reader, sim::control::ControllerConfig& config)
{
    reader.number("controller.kp_altitude", config.kp_altitude, 0.0, 1000.0);
    reader.number("controller.ki_altitude", config.ki_altitude, 0.0, 1000.0);
    reader.number("controller.kd_altitude", config.kd_altitude, 0.0, 1000.0);
    reader.number("controller.max_integral_rpm", config.max_integral_rpm, 0.0, 12000.0);
    reader.number("controller.kp_position", config.kp_position, 0.0, 1000.0);
    reader.number("controller.kd_position", config.kd_position, 0.0, 1000.0);
    reader.number("controller.kp_attitude", config.kp_attitude, 0.0, 1000.0);
    reader.number("controller.max_tilt_deg", config.max_tilt_deg, 0.01, 89.0);
    reader.number("controller.min_rpm", config.min_rpm, 0.0, 100000.0);
    reader.number("controller.max_rpm", config.max_rpm, 1.0, 100000.0);
    reader.number("controller.altitude_tolerance_m", config.altitude_tolerance_m, 0.000001, 1000.0);
    reader.number("controller.position_tolerance_m", config.position_tolerance_m, 0.000001, 1000.0);
    reader.number("controller.station_hold_seconds", config.station_hold_seconds, 0.0, 86400.0);
    reader.number("controller.spin_up_seconds", config.spin_up_seconds, 0.000001, 3600.0);
    reader.number("controller.takeoff_transition_seconds", config.takeoff_transition_seconds, 0.000001, 3600.0);
    reader.number("controller.climb_speed_mps", config.climb_speed_mps, 0.000001, 100.0);
    reader.number("controller.vertical_speed_gain", config.vertical_speed_gain, 0.0, 1000.0);
}
}

/* Loads finite, bounded runtime settings and rejects inconsistent combinations. */
std::expected<SilRuntimeOptions, std::string> load_sil_runtime_config(std::string_view path)
{
    auto loaded = sim::config::load_config_file(path);
    if (!loaded) {
        return std::unexpected(loaded.error());
    }
    auto& reader = *loaded;
    SilRuntimeOptions options{};
    auto& config = options.runner;
    std::string trace_level = "info";
    reader.number("dt_s", config.dt, 0.000001, 0.1);
    reader.unsigned_integer("seed", config.seed, 0, std::numeric_limits<std::uint64_t>::max());
    reader.number("transport_latency_s", config.transport_latency_s, 0.0, 10.0);
    reader.number("telemetry_rate_hz", config.telemetry_rate_hz, 0.001, 10000.0);
    reader.number("heartbeat_timeout_s", config.heartbeat_timeout_s, 0.000001, 60.0);
    reader.number("actuator_mismatch_rpm", config.actuator_mismatch_rpm, 0.0, 100000.0);
    reader.number("thrust_compensation_margin", config.thrust_compensation_margin, 1.0, 10.0);
    reader.number("sensor_max_altitude_m", config.sensor_limits.max_altitude_m, 0.000001, 100000.0);
    reader.number("sensor_max_position_m", config.sensor_limits.max_position_m, 0.000001, 1000000.0);
    reader.number("report_period_s", options.report_period_s, 0.000001, 86400.0);
    reader.number("steady_window_s", options.steady_window_s, 0.000001, 86400.0);
    reader.boolean("viewer_enabled", options.viewer_enabled);
    reader.number("viewer_period_s", options.viewer_period_s, 0.000001, 60.0);
    reader.number("viewer_replay_max_delay_s", options.viewer_replay_max_delay_s, 0.0, 60.0);
    reader.unsigned_integer("viewer_max_samples", options.viewer_max_samples, 1, 1000000);
    reader.text("trace_level", trace_level);
    read_controller(reader, config.controller);
    const auto finished = reader.finish();
    if (!finished) {
        return std::unexpected(finished.error());
    }
    if (trace_level == "info") {
        config.trace_level = sim::sil::SilLogLevel::Info;
    } else if (trace_level == "debug") {
        config.trace_level = sim::sil::SilLogLevel::Debug;
    } else if (trace_level == "trace") {
        config.trace_level = sim::sil::SilLogLevel::Trace;
    } else {
        return std::unexpected(std::string{path} + ": trace_level must be info, debug or trace");
    }
    if (config.controller.min_rpm >= config.controller.max_rpm
        || config.controller.hover_rpm < config.controller.min_rpm
        || config.controller.hover_rpm > config.controller.max_rpm
        || config.controller.takeoff_transition_seconds < config.controller.spin_up_seconds) {
        return std::unexpected(std::string{path} + ": inconsistent controller RPM limits or phase timing");
    }
    if (config.telemetry_rate_hz * config.dt > 1.0 + 1.0e-12
        || config.heartbeat_timeout_s < config.dt
        || options.report_period_s < config.dt) {
        return std::unexpected(std::string{path} + ": telemetry rate, heartbeat timeout and report period must respect dt_s");
    }
    return options;
}

/* Validates the final merged settings while the process is still in its startup phase. */
std::expected<void, std::string> validate_sil_runtime_profiles(
    const SilRuntimeOptions& options, std::string_view scenario_id)
{
    const auto& config = options.runner;
    if (!std::isfinite(config.dt) || config.dt < 0.000001 || config.dt > 0.1
        || !std::isfinite(options.report_period_s) || options.report_period_s < config.dt
        || options.report_period_s > 86400.0
        || !std::isfinite(config.telemetry_rate_hz) || config.telemetry_rate_hz <= 0.0
        || config.telemetry_rate_hz * config.dt > 1.0 + 1.0e-12
        || config.heartbeat_timeout_s < config.dt) {
        return std::unexpected("SIL configuration: cadence, heartbeat and telemetry must respect dt_s");
    }
    if (!std::isfinite(config.controller.hover_rpm)
        || config.controller.hover_rpm < config.controller.min_rpm
        || config.controller.hover_rpm > config.controller.max_rpm
        || config.controller.min_rpm >= config.controller.max_rpm) {
        return std::unexpected("SIL configuration: model hover RPM must lie within controller RPM limits");
    }
    bool matched = false;
    for (const auto& profile : sim::test::functional_scenarios()) {
        if (!scenario_id.empty() && profile.id != scenario_id) {
            continue;
        }
        matched = true;
        const std::float64_t report = !options.report_period_cli && profile.report_period_s > 0.0
            ? profile.report_period_s : options.report_period_s;
        if (!std::isfinite(profile.duration_s) || profile.duration_s < config.dt
            || profile.duration_s > 86400.0 || profile.duration_s / config.dt + 1.0 > 10000000.0
            || !std::isfinite(report) || report < config.dt || report > 86400.0) {
            return std::unexpected(std::string{profile.id}
                + ": duration/report must respect dt_s and at most 10000000 execution steps");
        }
        if (profile.fault_expected
            && (!std::isfinite(profile.sil_fault_start_s) || profile.sil_fault_start_s < 0.0
                || profile.sil_fault_start_s >= profile.duration_s
                || !std::isfinite(profile.sil_fault_duration_s) || profile.sil_fault_duration_s < 0.0
                || profile.sil_fault_duration_s > profile.duration_s - profile.sil_fault_start_s)) {
            return std::unexpected(std::string{profile.id} + ": SIL fault window exceeds the mission duration");
        }
        if (!std::isfinite(profile.wind_x_mps) || !std::isfinite(profile.wind_y_mps)
            || !std::isfinite(profile.wind_gust_period_s) || profile.wind_gust_period_s < 0.0
            || ((profile.wind_x_mps != 0.0 || profile.wind_y_mps != 0.0)
                && (!std::isfinite(profile.wind_start_s) || !std::isfinite(profile.wind_end_s)
                    || profile.wind_start_s < 0.0 || profile.wind_start_s >= profile.wind_end_s
                    || profile.wind_end_s > profile.duration_s))) {
            return std::unexpected(std::string{profile.id} + ": invalid wind activation window");
        }
    }
    if (!matched && config.duration_s / config.dt + 1.0 > 10000000.0) {
        return std::unexpected("SIL configuration: default mission exceeds 10000000 execution steps");
    }
    return {};
}

/* Installs the fully resolved immutable host baseline before execution. */
void set_sil_runtime_options(const SilRuntimeOptions& options)
{
    active_options = options;
}

/* Returns process settings without accessing the filesystem. */
const SilRuntimeOptions& sil_runtime_options() noexcept
{
    return active_options;
}

/* Applies only explicit canonical overrides over the configured controller baseline. */
sim::control::ControllerConfig sil_controller_config(std::float64_t hover_rpm,
                                                     std::string_view scenario_id)
{
    auto config = active_options.runner.controller;
    config.hover_rpm = hover_rpm;
    const auto* scenario = sim::test::find_functional_scenario(scenario_id);
    if (scenario != nullptr && scenario->station_hold_seconds > 0.0) {
        config.station_hold_seconds = scenario->station_hold_seconds;
    }
    return config;
}

/* Overlays canonical target, duration and explicit health/controller overrides. */
sim::sil::SilConfig sil_config_for_scenario(std::string_view id)
{
    auto config = active_options.runner;
    const auto* scenario = sim::test::find_functional_scenario(id);
    if (scenario != nullptr) {
        config.target = scenario->target;
        config.duration_s = scenario->duration_s;
        config.wind_x_mps = scenario->wind_x_mps;
        config.wind_y_mps = scenario->wind_y_mps;
        config.wind_start_s = scenario->wind_start_s;
        config.wind_end_s = scenario->wind_end_s;
        config.wind_gust_period_s = scenario->wind_gust_period_s;
        config.controller = sil_controller_config(config.controller.hover_rpm, id);
        if (scenario->sensor_max_altitude_m > 0.0) {
            config.sensor_limits.max_altitude_m = scenario->sensor_max_altitude_m;
        }
    }
    return config;
}

/* Applies profile cadence unless the command line explicitly overrides it. */
std::float64_t sil_report_period(std::string_view id) noexcept
{
    const auto* scenario = sim::test::find_functional_scenario(id);
    if (!active_options.report_period_cli && scenario != nullptr && scenario->report_period_s > 0.0) {
        return scenario->report_period_s;
    }
    return active_options.report_period_s;
}

}
