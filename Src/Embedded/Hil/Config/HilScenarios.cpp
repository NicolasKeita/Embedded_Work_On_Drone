/*
Filename: Src/Embedded/Hil/Config/HilScenarios.cpp
Description: Registry of nominal and fault-injection HIL flight scenarios.

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

/* Creates the configuration shared by one nominal flight scenario. */
HilConfig flight_config(std::string_view                 id,
                        const sim::control::TargetState& target,
                        std::float64_t                   duration_s)
{
    HilConfig config = hil_base_config();

    config.scenario_id = id;
    config.target = target;
    config.duration_s = duration_s;
    return config;
}

/* Creates the stratosphere configuration with a suitable sensor range. */
HilConfig stratosphere_config()
{
    HilConfig config = flight_config("NOMINAL-017", {.z = 20000.0}, 32430.0);

    config.sensor_limits.max_altitude_m = 25000.0;
    config.report_period_s = 300.0;
    return config;
}

/* Builds a HIL-timed fault from the shared functional scenario definition. */
sim::sil::FaultScenario hil_fault_for(std::string_view id)
{
    using sim::sil::FailureMode;
    const sim::test::FunctionalScenario* scenario = sim::test::find_functional_scenario(id);
    sim::sil::FaultScenario fault{
        .start_time = 5.0,
        .failure_mode = scenario->failure_mode,
        .parameters = scenario->parameters,
    };

    if (scenario->failure_mode == FailureMode::INVALID_SENSOR_DATA) {
        fault.duration = 20.0;
    }
    return fault;
}

const std::array<HilScenarioRecord, 11> kScenarios{{
    {"NOMINAL-001", "Nominal station-keeping mission (no fault)",
     flight_config("NOMINAL-001", {.z = 10.0}, 30.0),
     sim::sil::FaultScenario{}},
    {"FAULT_INJECTOR-001", "FC1 failure during station keeping", hil_base_config(),
     hil_fault_for("FAULT_INJECTOR-001")},
    {"FAULT_INJECTOR-002", "Communication loss during station keeping", hil_base_config(),
     hil_fault_for("FAULT_INJECTOR-002")},
    {"FAULT_INJECTOR-003", "Altitude sensor fault during station keeping", hil_base_config(),
     hil_fault_for("FAULT_INJECTOR-003")},
    {"FAULT_INJECTOR-004", "Main-rotor actuator degradation during station keeping", hil_base_config(),
     hil_fault_for("FAULT_INJECTOR-004")},
    {"NOMINAL-012", "Low vertical takeoff (30 s, z = 5 m)",
     flight_config("NOMINAL-012", {.z = 5.0}, 30.0),
     sim::sil::FaultScenario{}},
    {"NOMINAL-013", "Low forward takeoff (30 s, x = 4 m, z = 6 m)",
     flight_config("NOMINAL-013", {.x = 4.0, .z = 6.0}, 30.0),
     sim::sil::FaultScenario{}},
    {"NOMINAL-014", "Low lateral takeoff (30 s, y = -4 m, z = 7 m)",
     flight_config("NOMINAL-014", {.y = -4.0, .z = 7.0}, 30.0),
     sim::sil::FaultScenario{}},
    {"NOMINAL-015", "Low diagonal takeoff (30 s, x = 3 m, y = 3 m, z = 8 m)",
     flight_config("NOMINAL-015", {.x = 3.0, .y = 3.0, .z = 8.0}, 30.0),
     sim::sil::FaultScenario{}},
    {"NOMINAL-016", "Low offset takeoff (30 s, x = -3 m, y = 2 m, z = 9 m)",
     flight_config("NOMINAL-016", {.x = -3.0, .y = 2.0, .z = 9.0}, 30.0),
     sim::sil::FaultScenario{}},
    {"NOMINAL-017", "Stratosphere climb (approximately 9 h, z = 20 km)", stratosphere_config(),
     sim::sil::FaultScenario{}},
}};

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
