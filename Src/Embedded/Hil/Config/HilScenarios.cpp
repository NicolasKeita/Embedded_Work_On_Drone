/*
Filename: Src/Embedded/Hil/Config/HilScenarios.cpp
Description: HIL execution bindings of the canonical scenarios defined by the
shared FunctionalScenarios registry. Contains no scenario definition of its
own: each HilConfig is derived from the registry's mission profile (target,
duration, wind disturbance and sensor/report overrides); only the HIL fault
activation timing is defined here.

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

HilConfig hil_base_config()
{
    return HilConfig{};
}

namespace {

/*
HIL fault activation timing: property of the real-time bench, not of the scenario
identity. FC1_UNAVAILABLE activates at 40 s (permanent, duration <= 0);
INVALID_SENSOR_DATA activates at 5 s for 20 s.
*/
sim::sil::FaultScenario hil_fault_timing(const sim::test::FunctionalScenario& shared)
{
    sim::sil::FaultScenario fault{
        .start_time = shared.id == "FAULT_INJECTOR-001" ? 40.0 : 5.0,
        .failure_mode = shared.failure_mode,
        .parameters = shared.parameters,
    };

    if (shared.failure_mode == sim::sil::FailureMode::INVALID_SENSOR_DATA) {
        fault.duration = 20.0;
    }
    return fault;
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
    const std::span<const sim::test::FunctionalScenario> shared = sim::test::functional_scenarios();

    for (std::size_t index = 0; index < records.size(); ++index) {
        records[index] = make_hil_record(shared[index]);
    }
    return records;
}

const std::array<HilScenarioRecord, sim::test::functional_scenario_count> kScenarios = build_hil_scenarios();

}

std::span<const HilScenarioRecord> HilScenarioCatalog::all() noexcept
{
    return kScenarios;
}

const HilScenarioRecord* HilScenarioCatalog::find(std::string_view id) noexcept
{
    for (const HilScenarioRecord& scenario : kScenarios) {
        if (scenario.id == id) {
            return &scenario;
        }
    }
    return nullptr;
}

}
