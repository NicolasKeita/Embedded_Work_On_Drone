/*
Filename: Src/SIL/Core/FunctionalScenarios.cpp
Description: Definition of the target-agnostic functional scenario registry
(NOM-001 nominal station keeping and FINJ-001..004 fault injection families).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FunctionalScenarios;

import std;

import SilFaultScenario;
import Telemetry;

namespace sim::test {

namespace {

// constexpr so the registry is constant-initialized: it can be safely read
// during the dynamic initialization of other scenario catalogs (no static
// initialization order fiasco).
constexpr std::array<FunctionalScenario, 5> kScenarios{{
    {
        "NOM-001_StationKeeping",
        "Nominal station-keeping mission with no injected fault",
        FunctionalFamily::Nominal,
        sim::sil::FaultType::None,
        sim::sil::FaultParameters{},
        false,
    },
    {
        "FINJ-001_Fc1Failure",
        "FC1 (primary flight controller) failure",
        FunctionalFamily::FaultInjection,
        sim::sil::FaultType::FC1Failure,
        sim::sil::FaultParameters{},
        true,
    },
    {
        "FINJ-002_CommLoss",
        "FC1-FC2 communication loss",
        FunctionalFamily::FaultInjection,
        sim::sil::FaultType::CommunicationLoss,
        sim::sil::FaultParameters{},
        true,
    },
    {
        "FINJ-003_SensorFault",
        "Altitude sensor fault (out-of-range measurement)",
        FunctionalFamily::FaultInjection,
        sim::sil::FaultType::SensorFault,
        sim::sil::FaultParameters{
            .corruption = sim::sil::SensorCorruptionMode::AltitudeOutOfRange,
            .corrupted_altitude_m = 99999.0,
        },
        true,
    },
    {
        "FINJ-004_ActuatorDegradation",
        "Main-rotor actuator degradation (efficiency 0.6)",
        FunctionalFamily::FaultInjection,
        sim::sil::FaultType::ActuatorDegradation,
        sim::sil::FaultParameters{.efficiency = 0.6},
        true,
    },
}};

}

std::span<const FunctionalScenario> functional_scenarios() noexcept
{
    return kScenarios;
}

const FunctionalScenario* find_functional_scenario(std::string_view id) noexcept
{
    for (const FunctionalScenario& scenario : kScenarios) {
        if (scenario.id == id) {
            return &scenario;
        }
    }
    return nullptr;
}

}
