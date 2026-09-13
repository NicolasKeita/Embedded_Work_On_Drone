/*
Filename: Src/SIL/Core/FunctionalScenarios.cpp
Description: Shared SIL/HIL functional scenario registry containing the nominal
station-keeping scenario and the two supported fault-injection scenarios.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FunctionalScenarios;

import std;

import SilFaultScenario;
import Telemetry;

namespace sim::test {

namespace {

/*
This is the canonical scenario registry shared by the SIL and HIL runners.
Target-specific catalogs may add execution timing, but must not redefine a
functional scenario's identity, fault mode, parameters or expected outcome.
*/
constexpr std::array<FunctionalScenario, 3> kScenarios{{
    {
        "NOMINAL-001",
        "Nominal station-keeping mission with no injected fault",
        FunctionalFamily::Nominal,
        sim::sil::FailureMode::NONE,
        sim::sil::FaultParameters{},
        false,
    },
    {
        "FAULT_INJECTOR-001",
        "FC1 (primary flight controller) failure",
        FunctionalFamily::FaultInjection,
        sim::sil::FailureMode::FC1_UNAVAILABLE,
        sim::sil::FaultParameters{},
        true,
    },
    {
        "FAULT_INJECTOR-003",
        "Altitude sensor fault (out-of-range measurement)",
        FunctionalFamily::FaultInjection,
        sim::sil::FailureMode::INVALID_SENSOR_DATA,
        sim::sil::FaultParameters{
            .corruption = sim::sil::SensorCorruptionMode::AltitudeOutOfRange,
            .corrupted_altitude_m = 99999.0,
        },
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
