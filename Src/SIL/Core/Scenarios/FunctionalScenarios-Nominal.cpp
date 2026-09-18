/*
Filename: Src/SIL/Core/Scenarios/FunctionalScenarios-Nominal.cpp
Description: Nominal-mission catalog of the shared functional scenario registry: nominal
station keeping, the low vertical/forward/lateral/diagonal/offset takeoffs and the
stratosphere climb.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FunctionalScenarios;

import std;

import FlightControllerTypes;
import SilFaultScenario;

namespace sim::test {

namespace {
    constexpr std::array<FunctionalScenario, nominal_scenario_count> kNominalScenarios{{
        {
            .id = "NOMINAL-001",
            .description = "Nominal station-keeping mission (no fault)",
            .family = FunctionalFamily::Nominal,
            .failure_mode = sim::sil::FailureMode::NONE,
            .parameters = sim::sil::FaultParameters{},
            .fault_expected = false,
            .target = {.z = 10.0},
            .duration_s = 30.0,
            .tracking_tolerance = 1.0,
        },
        {
            .id = "NOMINAL-012",
            .description = "Low vertical takeoff (30 s, z = 5 m)",
            .family = FunctionalFamily::Nominal,
            .failure_mode = sim::sil::FailureMode::NONE,
            .parameters = sim::sil::FaultParameters{},
            .fault_expected = false,
            .target = {.z = 5.0},
            .duration_s = 30.0,
            .tracking_tolerance = 0.75,
        },
        {
            .id = "NOMINAL-013",
            .description = "Low forward takeoff (30 s, x = 4 m, z = 6 m)",
            .family = FunctionalFamily::Nominal,
            .failure_mode = sim::sil::FailureMode::NONE,
            .parameters = sim::sil::FaultParameters{},
            .fault_expected = false,
            .target = {.x = 4.0, .z = 6.0},
            .duration_s = 30.0,
            .tracking_tolerance = 0.75,
        },
        {
            .id = "NOMINAL-014",
            .description = "Low lateral takeoff (30 s, y = -4 m, z = 7 m)",
            .family = FunctionalFamily::Nominal,
            .failure_mode = sim::sil::FailureMode::NONE,
            .parameters = sim::sil::FaultParameters{},
            .fault_expected = false,
            .target = {.y = -4.0, .z = 7.0},
            .duration_s = 30.0,
            .tracking_tolerance = 0.75,
        },
        {
            .id = "NOMINAL-015",
            .description = "Low diagonal takeoff (30 s, x = 3 m, y = 3 m, z = 8 m)",
            .family = FunctionalFamily::Nominal,
            .failure_mode = sim::sil::FailureMode::NONE,
            .parameters = sim::sil::FaultParameters{},
            .fault_expected = false,
            .target = {.x = 3.0, .y = 3.0, .z = 8.0},
            .duration_s = 30.0,
            .tracking_tolerance = 0.75,
        },
        {
            .id = "NOMINAL-016",
            .description = "Low offset takeoff (30 s, x = -3 m, y = 2 m, z = 9 m)",
            .family = FunctionalFamily::Nominal,
            .failure_mode = sim::sil::FailureMode::NONE,
            .parameters = sim::sil::FaultParameters{},
            .fault_expected = false,
            .target = {.x = -3.0, .y = 2.0, .z = 9.0},
            .duration_s = 30.0,
            .tracking_tolerance = 0.75,
        },
        {
            .id = "NOMINAL-017",
            .description = "Stratosphere climb (approximately 9 h, z = 20 km)",
            .family = FunctionalFamily::Nominal,
            .failure_mode = sim::sil::FailureMode::NONE,
            .parameters = sim::sil::FaultParameters{},
            .fault_expected = false,
            .target = {.z = 20000.0},
            .duration_s = 32430.0,
            .tracking_tolerance = 2.0,
            .sensor_max_altitude_m = 25000.0,
            .report_period_s = 300.0,
        },
    }};
}

const std::array<FunctionalScenario, nominal_scenario_count>& nominal_scenarios() noexcept
{
    return kNominalScenarios;
}

}