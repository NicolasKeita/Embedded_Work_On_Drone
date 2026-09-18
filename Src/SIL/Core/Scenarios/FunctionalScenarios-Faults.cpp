/*
Filename: Src/SIL/Core/Scenarios/FunctionalScenarios-Faults.cpp
Description: Fault-injection catalog of the shared functional scenario registry: the FC1
unavailability and the out-of-range altitude sensor fault.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FunctionalScenarios;

import std;

import FlightControllerTypes;
import SilFaultScenario;
import Telemetry;

namespace sim::test {

namespace {
    constexpr std::array<FunctionalScenario, fault_scenario_count> kFaultScenarios{{
        {
            .id = "FAULT_INJECTOR-001",
            .description = "FC1 (primary flight controller) failure",
            .family = FunctionalFamily::FaultInjection,
            .failure_mode = sim::sil::FailureMode::FC1_UNAVAILABLE,
            .parameters = sim::sil::FaultParameters{},
            .fault_expected = true,
            .target = {.z = 10.0},
            .duration_s = 100.0,
            .tracking_tolerance = 1.0,
            .station_hold_seconds = 120.0,
        },
        {
            .id = "FAULT_INJECTOR-003",
            .description = "Altitude sensor fault (out-of-range measurement)",
            .family = FunctionalFamily::FaultInjection,
            .failure_mode = sim::sil::FailureMode::INVALID_SENSOR_DATA,
            .parameters = sim::sil::FaultParameters{
                .corruption = sim::sil::SensorCorruptionMode::AltitudeOutOfRange,
                .corrupted_altitude_m = 99999.0,
            },
            .fault_expected = true,
            .target = {.z = 10.0},
            .duration_s = 30.0,
            .tracking_tolerance = 1.0,
        },
    }};
}

const std::array<FunctionalScenario, fault_scenario_count>& fault_scenarios() noexcept
{
    return kFaultScenarios;
}

}