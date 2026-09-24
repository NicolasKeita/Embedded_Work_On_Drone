/*
Filename: Src/Host/Config/ScenarioConfig.cpp
Description: Validated startup configuration of canonical mission and fault profiles.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module ScenarioConfig;

import std;

import ConfigFile;
import FunctionalScenarios;
import SilFaultScenario;

namespace sim::host {

namespace {

/* Reads mission and disturbance parameters while retaining fixed scenario identity. */
void read_mission_profile(sim::config::Reader& reader, sim::test::FunctionalScenario& scenario)
{
    reader.number("target_x_m", scenario.target.x, -100000.0, 100000.0);
    reader.number("target_y_m", scenario.target.y, -100000.0, 100000.0);
    reader.number("target_z_m", scenario.target.z, 0.0, 50000.0);
    reader.number("duration_s", scenario.duration_s, 0.01, 86400.0);
    reader.number("tracking_tolerance_m", scenario.tracking_tolerance, 0.000001, 10000.0);
    reader.number("station_hold_s", scenario.station_hold_seconds, 0.0, 86400.0);
    reader.number("sensor_max_altitude_m", scenario.sensor_max_altitude_m, 0.0, 1000000.0);
    reader.number("report_period_s", scenario.report_period_s, 0.0, 86400.0);
    reader.number("wind_x_mps", scenario.wind_x_mps, -100.0, 100.0);
    reader.number("wind_y_mps", scenario.wind_y_mps, -100.0, 100.0);
    reader.number("wind_start_s", scenario.wind_start_s, 0.0, 86400.0);
    reader.number("wind_end_s", scenario.wind_end_s, 0.0, 86400.0);
    reader.number("wind_gust_period_s", scenario.wind_gust_period_s, 0.0, 86400.0);
}

/* Reads only fault timing and numeric values for fixed fault-bearing scenarios. */
void read_fault_profile(sim::config::Reader& reader, sim::test::FunctionalScenario& scenario)
{
    if (!scenario.fault_expected) {
        return;
    }
    reader.number("sil_fault_start_s", scenario.sil_fault_start_s, 0.0, 86400.0);
    reader.number("sil_fault_duration_s", scenario.sil_fault_duration_s, 0.0, 86400.0);
    reader.number("hil_fault_start_s", scenario.hil_fault_start_s, 0.0, 86400.0);
    reader.number("hil_fault_duration_s", scenario.hil_fault_duration_s, 0.0, 86400.0);
    if (scenario.failure_mode == sim::sil::FailureMode::INVALID_SENSOR_DATA) {
        reader.number("fault_corrupted_altitude_m", scenario.parameters.corrupted_altitude_m, -1000000.0, 1000000.0);
    }
}

/* Checks that an enabled injection starts and, when temporary, ends during the mission. */
bool valid_fault_window(std::float64_t start_s, std::float64_t duration_s, std::float64_t mission_s)
{
    return start_s < mission_s && (duration_s == 0.0 || start_s + duration_s <= mission_s);
}

/* Rejects inconsistent startup profiles without altering the independent safety verdicts. */
std::expected<void, std::string> validate_profile(const sim::test::FunctionalScenario& scenario)
{
    if (scenario.wind_end_s <= scenario.wind_start_s) {
        return std::unexpected("wind_end_s must be greater than wind_start_s");
    }
    if ((scenario.wind_x_mps != 0.0 || scenario.wind_y_mps != 0.0)
        && scenario.wind_end_s > scenario.duration_s) {
        return std::unexpected("The active wind window must fit within duration_s");
    }
    if (scenario.sensor_max_altitude_m > 0.0 && scenario.target.z > scenario.sensor_max_altitude_m) {
        return std::unexpected("target_z_m exceeds sensor_max_altitude_m");
    }
    if (scenario.report_period_s > scenario.duration_s) {
        return std::unexpected("report_period_s must not exceed duration_s");
    }
    if (scenario.fault_expected
        && !valid_fault_window(scenario.sil_fault_start_s, scenario.sil_fault_duration_s, scenario.duration_s)) {
        return std::unexpected("SIL fault window must start and finish within duration_s (zero duration is permanent)");
    }
    if (scenario.fault_expected
        && !valid_fault_window(scenario.hil_fault_start_s, scenario.hil_fault_duration_s, scenario.duration_s)) {
        return std::unexpected("HIL fault window must start and finish within duration_s (zero duration is permanent)");
    }
    return {};
}

/* Loads and validates one required file before allowing the reader to record its snapshot. */
std::expected<void, std::string> load_profile(const std::filesystem::path& path,
                                            sim::test::FunctionalScenario& scenario)
{
    std::expected<sim::config::Reader, std::string> reader = sim::config::load_config_file(path);

    if (!reader.has_value()) {
        return std::unexpected(reader.error());
    }
    read_mission_profile(reader.value(), scenario);
    read_fault_profile(reader.value(), scenario);
    const std::expected<void, std::string> valid = validate_profile(scenario);

    if (!valid.has_value()) {
        return std::unexpected(path.string() + ": " + valid.error());
    }
    return reader->finish();
}

}

/* Applies all required scenario files as one startup update with stable catalog storage. */
std::expected<void, std::string> load_scenario_configs(const std::filesystem::path& directory)
{
    std::array<sim::test::FunctionalScenario, sim::test::functional_scenario_count> configured =
        sim::test::functional_scenario_defaults();

    for (sim::test::FunctionalScenario& scenario : configured) {
        const std::filesystem::path path = directory / (std::string(scenario.id) + ".conf");
        const std::expected<void, std::string> loaded = load_profile(path, scenario);

        if (!loaded.has_value()) {
            return std::unexpected(loaded.error());
        }
    }
    return sim::test::apply_functional_scenarios(configured);
}

}
