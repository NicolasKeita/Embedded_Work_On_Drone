/*
Filename: Src/Embedded/Hil/HilScenarios.cpp
Description: HIL scenario registry entries bound to the shared functional scenario
IDs (NOM-001 nominal, FINJ-001..004 fault injection, MC-FINJ-001 fault during climb).

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
    HilConfig config{};
    config.scenario_id = "NOM-001_StationKeeping";
    return config;
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
        sim::sil::FaultScenario fault{};
        fault.fault_type = scenario.fault_type;
        fault.parameters = scenario.parameters;
        fault.start_time = 5.0;
        if (scenario.fault_type == FaultType::SensorFault) {
            fault.duration = 20.0;
        }
        return fault;
    }

    const std::array<HilScenarioRecord, 6> kScenarios{{
        {
            "NOM-001_StationKeeping",
            "Nominal station-keeping mission (no fault)",
            named("NOM-001_StationKeeping"),
            sim::sil::FaultScenario{},
        },
        {
            "FINJ-001_Fc1Failure",
            "FC1 failure during station keeping",
            named("FINJ-001_Fc1Failure"),
            hil_fault_for(*sim::test::find_functional_scenario("FINJ-001_Fc1Failure")),
        },
        {
            "FINJ-002_CommLoss",
            "Communication loss during station keeping",
            named("FINJ-002_CommLoss"),
            hil_fault_for(*sim::test::find_functional_scenario("FINJ-002_CommLoss")),
        },
        {
            "FINJ-003_SensorFault",
            "Altitude sensor fault (out of range) during station keeping",
            named("FINJ-003_SensorFault"),
            hil_fault_for(*sim::test::find_functional_scenario("FINJ-003_SensorFault")),
        },
        {
            "FINJ-004_ActuatorDegradation",
            "Main-rotor actuator degradation (efficiency 0.6) during station keeping",
            named("FINJ-004_ActuatorDegradation"),
            hil_fault_for(*sim::test::find_functional_scenario("FINJ-004_ActuatorDegradation")),
        },
        {
            "MC-FINJ-001_Fc1FailureDuringClimb",
            "FC1 failure injected during the climb mode-change transition",
            named("MC-FINJ-001_Fc1FailureDuringClimb"),
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
