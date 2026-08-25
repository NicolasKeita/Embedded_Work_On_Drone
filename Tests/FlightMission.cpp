/*
Filename: Tests/FlightMission.cpp
Description: Implementation of the autonomous full-mission scenario J.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightMission;

import std;

import Aircraft;
import FlightController;
import FlightScenarios;
import MissionRunner;
import MissionSupport;
import TestHarness;

namespace sim::test::flight_scenarios {

using sim::control::ControllerConfig;
using sim::control::FlightController;
using sim::control::MissionState;

/*
Test 4 : mission complete depuis (20, -15, 0) vers (0, 0, 100). Phase 1 : mise
en station au point (20, -15, 100), phase 2 : station keeping sur la cible
finale. Valide la sequence d'etats et la convergence globale dans la zone.
*/
void autonomous_mission(TestHarness& runner, double hover_rpm)
{
    std::cout << "\n=== Scenario J : mission complete (20, -15, 0) -> (0, 0, 100) ==="
              << std::endl;
    runner.log_header();

    ControllerConfig config{.hover_rpm = hover_rpm};
    FlightController controller{config};
    Aircraft aircraft;

    std::cout << "-- Phase 1 : mise en station au point (20, -15, 100) --" << std::endl;
    const MissionRunTrace approach =
        run_mission(controller, aircraft,
                    {.target = {.x = 20.0, .y = -15.0, .z = 100.0}, .duration = 240.0,
                     .axis = TrackingAxis::z_axis, .tolerance = 1.0, .stop_on_zone = true});

    std::cout << "-- Phase 2 : station keeping sur la cible (0, 0, 100) --" << std::endl;
    const MissionRunTrace trace =
        run_mission(controller, aircraft,
                    {.target = {.z = 100.0}, .duration = 180.0,
                     .axis = TrackingAxis::z_axis, .tolerance = 0.5});

    print_metrics_report("altitude", trace.metrics);
    const AircraftState& finalState = aircraft.state();

    std::vector<MissionState> visited = approach.visited_states;
    visited.insert(visited.end(), trace.visited_states.begin(), trace.visited_states.end());

    runner.check(contains_mission_sequence(visited),
                 "J1 : sequence TAKEOFF -> CLIMB -> STATION_KEEPING -> COMPLETE");
    runner.check(std::abs(finalState.x) <= 1.0 && std::abs(finalState.y) <= 1.0,
                 "J2 : position horizontale dans la zone cible (+/- 1 m)");
    runner.check(std::abs(finalState.z - 100.0) <= 1.0,
                 "J3 : altitude tenue autour de 100 m (+/- 1 m)");
}

}
