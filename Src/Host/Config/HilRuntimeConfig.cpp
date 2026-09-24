/*
Filename: Src/Host/Config/HilRuntimeConfig.cpp
Description: Startup loading and validation of HIL host settings and run snapshots.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRuntimeConfig;

import std;

import ConfigFile;
import FlightControllerTypes;

namespace sim::host {
namespace {

/* Reads settings of the emulated controller without changing firmware parameters. */
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

/* Compares the emulated gains and limits with those compiled into the current firmware. */
[[nodiscard]] bool firmware_controller_matches(const sim::control::ControllerConfig& config)
{
    const sim::control::ControllerConfig firmware{};
    return config.kp_altitude == firmware.kp_altitude && config.ki_altitude == firmware.ki_altitude
        && config.kd_altitude == firmware.kd_altitude && config.max_integral_rpm == firmware.max_integral_rpm
        && config.kp_position == firmware.kp_position && config.kd_position == firmware.kd_position
        && config.kp_attitude == firmware.kp_attitude && config.max_tilt_deg == firmware.max_tilt_deg
        && config.min_rpm == firmware.min_rpm && config.max_rpm == firmware.max_rpm
        && config.altitude_tolerance_m == firmware.altitude_tolerance_m
        && config.position_tolerance_m == firmware.position_tolerance_m
        && config.spin_up_seconds == firmware.spin_up_seconds
        && config.takeoff_transition_seconds == firmware.takeoff_transition_seconds
        && config.climb_speed_mps == firmware.climb_speed_mps
        && config.vertical_speed_gain == firmware.vertical_speed_gain;
}

}

/* Loads all host HIL numeric settings, preserving existing defaults for omitted keys. */
std::expected<sim::hil::HilConfig, std::string>
load_hil_runtime_config(const std::filesystem::path& path)
{
    auto loaded = sim::config::load_config_file(path);
    if (!loaded) {
        return std::unexpected(loaded.error());
    }
    auto& reader = *loaded;
    sim::hil::HilConfig config{};
    std::string deadline = "Warn";
    reader.number("dt_s", config.dt_s, 0.000001, 0.1);
    reader.unsigned_integer("seed", config.seed, 0, std::numeric_limits<std::uint64_t>::max());
    reader.number("telemetry_rate_hz", config.telemetry_rate_hz, 0.001, 10000.0);
    reader.number("report_period_s", config.report_period_s, 0.000001, 86400.0);
    reader.number("sensor_noise_stddev_m", config.sensor_noise_stddev, 0.0, 10000.0);
    reader.number("heartbeat_timeout_s", config.heartbeat_timeout_s, 0.000001, 60.0);
    reader.number("actuator_mismatch_rpm", config.actuator_mismatch_rpm, 0.0, 100000.0);
    reader.number("actuator_mismatch_hold_s", config.actuator_mismatch_hold_s, 0.0, 60.0);
    reader.number("thrust_compensation_margin", config.thrust_compensation_margin, 1.0, 10.0);
    reader.number("sensor_max_altitude_m", config.sensor_limits.max_altitude_m, 0.000001, 100000.0);
    reader.number("sensor_max_position_m", config.sensor_limits.max_position_m, 0.000001, 1000000.0);
    reader.unsigned_integer("transport_timeout_us", config.transport_timeout_us, 1, 10000000);
    reader.text("deadline_policy", deadline);
    read_controller(reader, config.controller);
    if (deadline == "Warn") {
        config.deadline_policy = sim::hil::DeadlinePolicy::Warn;
    } else if (deadline == "Fail") {
        config.deadline_policy = sim::hil::DeadlinePolicy::Fail;
    } else if (deadline == "Abort") {
        config.deadline_policy = sim::hil::DeadlinePolicy::Abort;
    } else {
        return std::unexpected(path.string() + ": deadline_policy must be Warn, Fail or Abort");
    }
    const auto finished = reader.finish();
    if (!finished) {
        return std::unexpected(finished.error());
    }
    return config;
}

/* Validates the final scenario and CLI configuration before any control loop starts. */
std::expected<void, std::string> validate_hil_runtime_config(const sim::hil::HilConfig& config)
{
    if (!std::isfinite(config.dt_s) || config.dt_s < 0.000001 || config.dt_s > 0.1
        || !std::isfinite(config.telemetry_rate_hz) || config.telemetry_rate_hz <= 0.0
        || !std::isfinite(config.heartbeat_timeout_s) || config.heartbeat_timeout_s <= 0.0
        || config.transport_timeout_us == 0) {
        return std::unexpected(std::string{"invalid HIL timestep, telemetry rate or timeout"});
    }
    if (!std::isfinite(config.duration_s) || config.duration_s < config.dt_s
        || config.duration_s > 86400.0 || !std::isfinite(config.report_period_s)
        || config.report_period_s < config.dt_s || !std::isfinite(config.sensor_noise_stddev)
        || config.sensor_noise_stddev < 0.0 || config.sensor_noise_stddev > 10000.0) {
        return std::unexpected(std::string{"invalid HIL duration, report period or sensor noise"});
    }
    if (config.telemetry_rate_hz * config.dt_s > 1.0 + 1.0e-12
        || config.heartbeat_timeout_s < config.dt_s
        || static_cast<std::float64_t>(config.transport_timeout_us) > config.dt_s * 1000000.0 + 1.0e-6
        || config.duration_s / config.dt_s > 10000000.0) {
        return std::unexpected(std::string{"HIL telemetry, timeout or step count is inconsistent with dt_s"});
    }
    if (config.controller.min_rpm >= config.controller.max_rpm
        || config.controller.hover_rpm < config.controller.min_rpm
        || config.controller.hover_rpm > config.controller.max_rpm
        || config.controller.takeoff_transition_seconds < config.controller.spin_up_seconds) {
        return std::unexpected(std::string{"inconsistent HIL controller limits or phase timing"});
    }
    if (config.interface_name != "loopback"
        && (std::abs(config.dt_s - 0.01) > 1.0e-12 || !firmware_controller_matches(config.controller))) {
        return std::unexpected(std::string{
            "physical HIL requires dt_s=0.01 and the compiled firmware controller gains; custom gains are loopback-only"});
    }
    return {};
}

/* Serializes effective scenario, hardware and CLI overrides alongside the source snapshots. */
std::string hil_configuration_text(const sim::hil::HilConfig& config,
                                   const sim::sil::FaultScenario& fault)
{
    std::ostringstream output{};
    output << std::setprecision(17)
        << "runner = HIL\nscenario = " << config.scenario_id
        << "\ninterface = " << config.interface_name
        << "\nfc1_stlink_serial = " << config.fc1_stlink_serial
        << "\nfc2_stlink_serial = " << config.fc2_stlink_serial
        << "\ndt_s = " << config.dt_s << "\nduration_s = " << config.duration_s
        << "\nseed = " << config.seed << "\nreport_period_s = " << config.report_period_s
        << "\ntelemetry_rate_hz = " << config.telemetry_rate_hz
        << "\nsensor_noise_stddev_m = " << config.sensor_noise_stddev
        << "\ntransport_timeout_us = " << config.transport_timeout_us
        << "\ndeadline_policy = " << sim::hil::deadline_policy_name(config.deadline_policy)
        << "\ntarget_x_m = " << config.target.x << "\ntarget_y_m = " << config.target.y
        << "\ntarget_z_m = " << config.target.z
        << "\ncontroller.hover_rpm = " << config.controller.hover_rpm
        << "\ncontroller.station_hold_seconds = " << config.controller.station_hold_seconds
        << "\nsensor_max_altitude_m = " << config.sensor_limits.max_altitude_m
        << "\nwind_x_mps = " << config.wind_x_mps << "\nwind_y_mps = " << config.wind_y_mps
        << "\nwind_start_s = " << config.wind_start_s << "\nwind_end_s = " << config.wind_end_s
        << "\nwind_gust_period_s = " << config.wind_gust_period_s
        << "\nfault_start_s = " << fault.start_time << "\nfault_duration_s = " << fault.duration << '\n';
    return output.str();
}

}
