/*
Filename: Src/SIL/Core/FunctionalScenarios.cpp
Description: Shared SIL/HIL functional scenario registry. This file is the single
canonical definition of every scenario executable by both the SIL and HIL
runners: identity (ID, description, family), fault identity (failure mode and
parameters) and mission profile (target, duration, tolerance, wind, sensor and
report overrides).

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

/*
This is the canonical scenario registry shared by the SIL and HIL runners.
Target-specific catalogs may add execution timing, but must not redefine a
functional scenario's identity, fault mode, parameters, mission profile or
expected outcome.
*/
constexpr std::array<FunctionalScenario, functional_scenario_count> kScenarios{{

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
