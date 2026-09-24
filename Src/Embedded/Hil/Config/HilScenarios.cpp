/*
Filename: Src/Embedded/Hil/Config/HilScenarios.cpp
Description: HIL execution bindings of the canonical scenarios defined by the
shared FunctionalScenarios registry. Contains no scenario definition of its
own: each HilConfig is derived from the registry's mission profile (target,
duration, wind disturbance, sensor/report overrides and HIL fault activation timing).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilScenarios;

import std;

import FlightControllerTypes;
import FunctionalScenarios;
import HilConfig;
import SilFaultScenario;
import Telemetry;

namespace sim::hil {

/* Returns the host runner defaults used before scenario-specific overrides. */
HilConfig hil_base_config()
{
    return HilConfig{};
}

namespace {

/*
Derives the HIL fault window from the configured target-specific profile.
*/
sim::sil::FaultScenario hil_fault_timing(const sim::test::FunctionalScenario& shared)
{
    return {
        .start_time = shared.hil_fault_start_s,
        .duration = shared.hil_fault_duration_s,
        .failure_mode = shared.failure_mode,
        .parameters = shared.parameters,
    };
}

/* Derives one HIL execution binding from a canonical scenario of the registry. */
HilScenarioRecord make_hil_record(const sim::test::FunctionalScenario& shared)
{
    HilConfig config = hil_base_config();

    config.scenario_id = shared.id;
    config.target = shared.target;
    config.duration_s = shared.duration_s;
    config.wind_x_mps = shared.wind_x_mps;
    config.wind_y_mps = shared.wind_y_mps;
    config.wind_start_s = shared.wind_start_s;
    config.wind_end_s = shared.wind_end_s;
    config.wind_gust_period_s = shared.wind_gust_period_s;

    if (shared.station_hold_seconds > 0.0) {
        config.controller.station_hold_seconds = shared.station_hold_seconds;
    }
    if (shared.sensor_max_altitude_m > 0.0) {
        config.sensor_limits.max_altitude_m = shared.sensor_max_altitude_m;
    }
    if (shared.report_period_s > 0.0) {
        config.report_period_s = shared.report_period_s;
    }

    return {shared.id, shared.description, config,
            shared.fault_expected ? hil_fault_timing(shared) : sim::sil::FaultScenario{}};
}

/* Builds the HIL catalog in the canonical registry order. */
std::array<HilScenarioRecord, sim::test::functional_scenario_count> build_hil_scenarios()
{
    std::array<HilScenarioRecord, sim::test::functional_scenario_count> records{};
    const std::span<const sim::test::FunctionalScenario>                shared = sim::test::functional_scenarios();

    for (std::size_t index = 0; index < records.size(); ++index) {
        records[index] = make_hil_record(shared[index]);
    }
    return records;
}

/* Keeps the derived catalog stable after startup without early static construction. */
std::array<HilScenarioRecord, sim::test::functional_scenario_count>& catalog_storage()
{
    static std::array<HilScenarioRecord, sim::test::functional_scenario_count> scenarios = build_hil_scenarios();

    return scenarios;
}

}

/* Refreshes the HIL profiles after the shared startup configuration has been applied. */
void HilScenarioCatalog::refresh()
{
    catalog_storage() = build_hil_scenarios();
}

/* Returns the catalog prepared before mission execution. */
std::span<const HilScenarioRecord> HilScenarioCatalog::all() noexcept
{
    return catalog_storage();
}

/* Resolves a canonical scenario in the prepared HIL catalog. */
const HilScenarioRecord* HilScenarioCatalog::find(std::string_view id) noexcept
{
    for (const HilScenarioRecord& scenario : catalog_storage()) {
        if (scenario.id == id) {
            return &scenario;
        }
    }
    return nullptr;
}

}
