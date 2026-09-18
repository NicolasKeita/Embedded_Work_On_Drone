/*
Filename: Src/SIL/Core/Scenarios/FunctionalScenarios-Wind.cpp
Description: Wind-disturbance catalog of the shared functional scenario registry: the
steady crosswind and the diagonal gust profile.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FunctionalScenarios;

import std;

import FlightControllerTypes;
import SilFaultScenario;

namespace sim::test {

namespace {
    constexpr std::array<FunctionalScenario, wind_scenario_count> kWindScenarios{{
        {
            .id = "WIND-001",
            .description = "Steady crosswind (4 m/s, 8-24 s, altitude 10 m)",
            .family = FunctionalFamily::Nominal,
            .failure_mode = sim::sil::FailureMode::NONE,
            .parameters = sim::sil::FaultParameters{},
            .fault_expected = false,
            .target = {.z = 10.0},
            .duration_s = 45.0,
            .tracking_tolerance = 1.0,
            .wind_y_mps = 4.0,
        },
        {
            .id = "WIND-002",
            .description = "Diagonal gusts (peak 5 m/s, period 4 s, 8-24 s)",
            .family = FunctionalFamily::Nominal,
            .failure_mode = sim::sil::FailureMode::NONE,
            .parameters = sim::sil::FaultParameters{},
            .fault_expected = false,
            .target = {.z = 10.0},
            .duration_s = 45.0,
            .tracking_tolerance = 1.0,
            .wind_x_mps = 3.0,
            .wind_y_mps = 4.0,
            .wind_gust_period_s = 4.0,
        },
    }};
}

const std::array<FunctionalScenario, wind_scenario_count>& wind_scenarios() noexcept
{
    return kWindScenarios;
}

}