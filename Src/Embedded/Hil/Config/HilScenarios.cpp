/*
Filename: Src/Embedded/Hil/Config/HilScenarios.cpp
Description: HIL scenario registry entries bound to the shared functional scenario
IDs (NOMINAL-001 nominal, FAULT_INJECTOR-001..004 fault injection, MONTE_CARLO_FAULT_INJECTOR-001 fault during climb).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilScenarios;

import std;

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
    HilConfig named(std::string_view id)
    {
        HilConfig config = hil_base_config();

        config.scenario_id = id;
        return config;
    }

    /*
    Builds the target-specific FaultScenario for the HIL execution of one shared
    functional scenario: the fault identity (type and parameters) comes from the
    target-agnostic registry, the activation timing is the HIL policy (injection
    at t = 5.0 s, permanent for FC1/comms/actuator and temporary for the sensor
    fault over a 20 s window).
    */
    sim::sil::FaultScenario hil_fault_for(const sim::test::FunctionalScenario& scenario)
    {
        using sim::sil::FaultType;
        sim::sil::FaultScenario fault{
            .start_time = 5.0, .fault_type = scenario.fault_type, .parameters = scenario.parameters};
        if (scenario.fault_type == FaultType::SensorFault) {
            fault.duration = 20.0;
        }
        return fault;
    }

    const std::array<HilScenarioRecord, 6> kScenarios{{
    {
            "NOMINAL-001",
            "Nominal station-keeping mission (no fault)",
            named("NOMINAL-001"),
            sim::sil::FaultScenario{},
        },
    {
            "FAULT_INJECTOR-001",
            "FC1 failure during station keeping",
            named("FAULT_INJECTOR-001"),
            hil_fault_for(*sim::test::find_functional_scenario("FAULT_INJECTOR-001")),
        },
    {
            "FAULT_INJECTOR-002",
            "Communication loss during station keeping",
            named("FAULT_INJECTOR-002"),
            hil_fault_for(*sim::test::find_functional_scenario("FAULT_INJECTOR-002")),
        },
    {
            "FAULT_INJECTOR-003",
            "Altitude sensor fault (out of range) during station keeping",
            named("FAULT_INJECTOR-003"),
            hil_fault_for(*sim::test::find_functional_scenario("FAULT_INJECTOR-003")),
        },
    {
            "FAULT_INJECTOR-004",
            "Main-rotor actuator degradation (efficiency 0.6) during station keeping",
            named("FAULT_INJECTOR-004"),
            hil_fault_for(*sim::test::find_functional_scenario("FAULT_INJECTOR-004")),
        },
    {
            "MONTE_CARLO_FAULT_INJECTOR-001",
            "FC1 failure injected during the climb mode-change transition",
            named("MONTE_CARLO_FAULT_INJECTOR-001"),
            sim::sil::FaultScenario{.start_time = 2.0,
                                    .fault_type = sim::sil::FaultType::FC1Failure},
        },
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
