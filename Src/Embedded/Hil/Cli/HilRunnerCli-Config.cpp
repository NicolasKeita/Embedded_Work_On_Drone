/*
Filename: Src/Embedded/Hil/Cli/HilRunnerCli-Config.cpp
Description: HilConfig assembly of hil_runner : selected scenario record plus the
command-line overrides (duration, cadence, seed, noise and deadline policy).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerCli;

import std;

import Aircraft;
import FunctionalScenarios;
import HilConfig;
import HilHardwareConfig;
import HilRuntimeConfig;
import HilScenarios;
import ScenarioConfig;
import SilFaultScenario;
import SimulationConfig;
import Stm32Discovery;

namespace sim::hil {

/* Applies the numeric command-line overrides to a configuration. */
void apply_numeric_overrides(HilConfig& config, const HilCliOptions& options)
{
    if (options.duration) {
        config.duration_s = *options.duration;
    }
    if (options.telemetry_period) {
        config.report_period_s = *options.telemetry_period;
    }
    if (options.seed) {
        config.seed = *options.seed;
    }
    if (options.noise) {
        config.sensor_noise_stddev = *options.noise;
    }
}

/* Applies the deadline-policy override selected by name. */
void apply_deadline_override(HilConfig& config, std::string_view deadline_name)
{
    if (deadline_name == "Fail") {
        config.deadline_policy = DeadlinePolicy::Fail;
    }
    else if (deadline_name == "Abort") {
        config.deadline_policy = DeadlinePolicy::Abort;
    }
    else if (deadline_name == "Warn") {
        config.deadline_policy = DeadlinePolicy::Warn;
    }
}

/* Loads configured ST-LINK identities before resolving the selected hardware interface. */
std::expected<HilConfig, std::string> hil_config_from_options(const HilCliOptions& options)
{
    const auto simulation = sim::host::load_simulation_config(options.simulation_config_path);
    if (!simulation) {
        return std::unexpected(simulation.error());
    }
    const auto scenarios = sim::host::load_scenario_configs(options.scenarios_directory);
    if (!scenarios) {
        return std::unexpected(scenarios.error());
    }
    HilScenarioCatalog::refresh();
    auto loaded = sim::host::load_hil_runtime_config(options.config_path);
    if (!loaded) {
        return std::unexpected(loaded.error());
    }
    HilConfig config = std::move(*loaded);
    config.scenario_id = options.scenario_id;
    const auto* profile = sim::test::find_functional_scenario(options.scenario_id);
    if (profile == nullptr) {
        return std::unexpected(std::string{"unknown HIL scenario: "} + options.scenario_id);
    }
    config.target = profile->target;
    config.duration_s = profile->duration_s;
    config.wind_x_mps = profile->wind_x_mps;
    config.wind_y_mps = profile->wind_y_mps;
    config.wind_start_s = profile->wind_start_s;
    config.wind_end_s = profile->wind_end_s;
    config.wind_gust_period_s = profile->wind_gust_period_s;
    if (profile->station_hold_seconds > 0.0) {
        config.controller.station_hold_seconds = profile->station_hold_seconds;
    }
    if (profile->sensor_max_altitude_m > 0.0) {
        config.sensor_limits.max_altitude_m = profile->sensor_max_altitude_m;
    }
    if (profile->report_period_s > 0.0) {
        config.report_period_s = profile->report_period_s;
    }
    if (!options.deadline_name.empty() && options.deadline_name != "Warn"
        && options.deadline_name != "Fail" && options.deadline_name != "Abort") {
        return std::unexpected(std::string{"--deadline must be Warn, Fail or Abort"});
    }
    if (options.interface_name != "loopback" || !options.hardware_config_path.empty()) {
        const std::filesystem::path path = options.hardware_config_path.empty()
            ? std::filesystem::path{"config/hil_hardware.conf"}
            : std::filesystem::path{options.hardware_config_path};
        auto hardware = sim::hil::load_hil_hardware_config(path);
        if (!hardware.has_value()) {
            return std::unexpected(hardware.error());
        }
        config.fc1_stlink_serial = std::move(hardware->fc1_stlink_serial);
        config.fc2_stlink_serial = std::move(hardware->fc2_stlink_serial);
    }
    const Stm32DiscoveryResult discovered = discover_stm32_port(config.fc1_stlink_serial);
    resolve_hil_interface(config, options.interface_name, discovered);
    if (config.interface_name != "loopback") {
        config.controller.hover_rpm = kNominalAircraftHoverRpm;
    }
    apply_numeric_overrides(config, options);
    apply_deadline_override(config, options.deadline_name);
    const auto validated = sim::host::validate_hil_runtime_config(config);
    if (!validated) {
        return std::unexpected(validated.error());
    }
    return config;
}

sim::sil::FaultScenario hil_fault_from_options(const HilCliOptions& options)
{
    const HilScenarioRecord* record = HilScenarioCatalog::find(options.scenario_id);

    return record ? record->fault : sim::sil::FaultScenario{};
}

}
