/*
Filename: Tests/Scenarios/FlightScenarios-Mission.cpp
Description: Implementation of the autonomous full-mission scenario (NOMINAL-010).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightScenarios;

import std;

import Aircraft;
import FlightController;
import MissionRunner;
import TestHarness;

namespace sim::test::flight_scenarios {

using sim::control::ControllerConfig;
using sim::control::FlightController;
using sim::control::MissionState;

/*
NOMINAL-010: full mission from (20, -15, 0) to (0, 0, 100).
Phase 1: station keeping at point (20, -15, 100), phase 2: station keeping on the
final target. Validates the state sequence and the overall convergence inside the zone.
*/
void autonomous_mission(TestHarness& runner, std::float64_t hover_rpm)
{
    runner.begin_scenario("NOMINAL-010", "Full mission (20, -15, 0) -> (0, 0, 100)");
    runner.log_header();

    ControllerConfig config{.hover_rpm = hover_rpm};
    FlightController controller{config};
    Aircraft aircraft;

    std::cout << "-- Phase 1: station keeping at point (20, -15, 100) --" << std::endl;
    const MissionRunTrace approach = run_mission(controller, aircraft,
                    {.target = {.x = 20.0, .y = -15.0, .z = 100.0}, .duration = 240.0,
                     .axis = TrackingAxis::z_axis, .tolerance = 1.0, .stop_on_zone = true});

    std::cout << "-- Phase 2: station keeping on target (0, 0, 100) --" << std::endl;
    const MissionRunTrace trace = run_mission(controller, aircraft,
                    {.target = {.z = 100.0}, .duration = 180.0, .axis = TrackingAxis::z_axis, .tolerance = 0.5});

    print_metrics_report("altitude", trace.metrics);
    const AircraftState& finalState = aircraft.state();

    std::array<MissionState, kMaxVisitedStates> visited{};
    std::size_t visitedCount = 0;

    for (std::size_t i = 0; i < approach.visited_count && visitedCount < visited.size(); ++i) {
        visited[visitedCount] = approach.visited_states[i];
        ++visitedCount;
    }
    for (std::size_t i = 0; i < trace.visited_count && visitedCount < visited.size(); ++i) {
        visited[visitedCount] = trace.visited_states[i];
        ++visitedCount;
    }

    runner.check(contains_mission_sequence(std::span<const MissionState>{visited.data(), visitedCount}),
                 "sequence TAKEOFF -> CLIMB -> STATION_KEEPING -> COMPLETE");
    runner.check(std::abs(finalState.x) <= 1.0 && std::abs(finalState.y) <= 1.0,
                 "horizontal position within target zone (+/- 1 m)");
    runner.check(std::abs(finalState.z - 100.0) <= 1.0, "altitude held around 100 m (+/- 1 m)");
}

}
