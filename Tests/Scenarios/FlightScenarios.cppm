/*
Filename: Tests/Scenarios/FlightScenarios.cppm
Description: Autonomous flight scenarios: altitude hold, cascaded X/Y position loops and full mission.
Exports:
    autonomous_altitude_hold(),
    autonomous_altitude(),
    autonomous_position_x(),
    autonomous_position_y(),
    autonomous_mission()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FlightScenarios;

import std;

import PhysicsDispersion;
import TestHarness;

export namespace sim::test::flight_scenarios {

// NOMINAL-001: no-fault reference run, climb to 10 m and hold altitude for the 30-second mission.
void autonomous_altitude_hold(TestHarness& runner, std::float64_t hover_rpm, const sim::PhysicsDispersion& dispersion = {});

// NOMINAL-011: autonomous altitude loop, convergence towards z = 100 m with metrics.
void autonomous_altitude(TestHarness& runner, std::float64_t hover_rpm, const sim::PhysicsDispersion& dispersion = {});

// NOMINAL-008: cascaded X position -> pitch -> servos, return from x = 20 m to x = 0.
void autonomous_position_x(TestHarness& runner, std::float64_t hover_rpm, const sim::PhysicsDispersion& dispersion = {});

// NOMINAL-009: cascaded Y position -> roll -> servos, return from y = -15 m to y = 0.
void autonomous_position_y(TestHarness& runner, std::float64_t hover_rpm, const sim::PhysicsDispersion& dispersion = {});

// NOMINAL-010: full mission TAKEOFF -> CLIMB -> STATION_KEEPING -> COMPLETE.
void autonomous_mission(TestHarness& runner, std::float64_t hover_rpm, const sim::PhysicsDispersion& dispersion = {});

}
